/*
 * A Remote cat client using the TFTP protocol.
 *
 * MIT License
 *
 * Copyright (c) 2017 Suraj Bennur (Benn)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * dealin the Software without restriction, including without limitation the
 * rightsto use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sellcopies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions: The above copyright
 * notice and this permission notice shall be included in all copies or
 * substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS",
 * WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "remcat.h"

/* Should cover most needs */
#define MSGBUF_SIZE BLOCK_SIZE

/* A connection handle */
struct tftp_conn {
  int type;        /* Are we putting or getting? - In our case, we only get */
  int sock;        /* Socket to communicate with server */
  int blocknr;     /* The current block number */
  char* file_name; /* The file name of the file we are putting or getting */
  char* mode;      /* TFTP mode */
  struct hostent* host;      /* Server host info  */
  struct sockaddr_in server; /* Remote peer address */
  socklen_t addrlen;         /* The remote address length */
  char msgbuf[MSGBUF_SIZE];  /* Buffer for messages being sent or received */
};

/* Connect to a remote TFTP server. */
struct tftp_conn* tftp_connect(int type,
                               char* fname,
                               char* hostname,
                               char* mode) {
  struct addrinfo hints;
  struct tftp_conn* tc = NULL;

  /* error checks */
  if (!fname || !mode || !hostname)
    return NULL;
  tc = (struct tftp_conn*)malloc(sizeof(struct tftp_conn));
  tc->addrlen = sizeof(struct sockaddr_in);
  if (!tc)
    return NULL;

  /* Create a socket.
   * Check return value. */

  if ((tc->sock = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
    perror("socket failed");
    exit(1);
  }

  if (type == TFTP_TYPE_GET)
    tc->file_name = fname;
  else {
    fprintf(stderr, "Invalid TFTP mode, must be get\n");
    return NULL;
  }

  // reset all the fields in sockarr hints
  memset(&hints, 0, sizeof(hints));
  // ai_family is unspecified and socktype is of UDP
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  // copy TFTP port address to port_str
  char port_str[5];
  sprintf(port_str, "%d", 5069);

  /* get address from host name.
   * If error, gracefully clean up.*/
  if ((tc->host = gethostbyname(hostname)) == NULL) {
    fprintf(stderr, "unknown host: %s\n", hostname);
    exit(2);
  }
  // use IPV4 version
  tc->server.sin_family = AF_INET;
  // set hostname and port
  memcpy(&tc->server.sin_addr.s_addr, tc->host->h_addr, tc->host->h_length);
  tc->server.sin_port = htons(5069);

  /* Assign address to the connection handle.
   * You can assume that the first address in the hostent
   * struct is the correct one */
  memcpy(&tc->server, hints.ai_addr, hints.ai_addrlen);

  tc->addrlen = sizeof(struct sockaddr_in);
  tc->type = type;
  tc->mode = mode;
  tc->blocknr = 0;
  // reset msgbuf
  memset(tc->msgbuf, 0, MSGBUF_SIZE);
  // return connection handle
  return tc;
}

/*
  Send a read request to the server.
  1. Format message.
  2. Send the request using the connection handle.
  3. Return the number of bytes sent, or negative on error.
 */
int tftp_send_rrq(struct tftp_conn* tc) {
  /* Create a buffer to hold the file data and a char pointer to point to cells
   * within buffer */
  char* p;
  int count;
  tc->blocknr = 0;
  *(short*)tc->msgbuf = htons(OPCODE_RRQ); /* The op-code  is 2 bytes */
  p = tc->msgbuf + 2;                      /* Point to the next location */
  strcpy(p, tc->file_name);                /* The file name */
  p += strlen(tc->file_name) + 1;          /* Keep the null  */
  strcpy(p, MODE_OCTET);                   /* The Mode      */
  p += strlen(MODE_OCTET) + 1;

  /* Send Read Request to tftp server */
  if ((count = sendto(tc->sock, tc->msgbuf, p - tc->msgbuf, 0,
                      (struct sockaddr*)&tc->server, tc->addrlen)) < 0) {
    fprintf(stderr, "Could not send the RRQ\n");
    exit(1);
  }

  return count;
}

int validate_data(struct tftp_conn* tc) {
  // check for valid block#
  if (ntohs(*(short*)(tc->msgbuf + 2)) != ++tc->blocknr) {
    fprintf(stderr, "Incorrect block number\n");
    return -1;
  }

  return 1;

  //*((short*)(tc->msgbuf + 2)) = htons();
  /* Received data block, send ack */
}
int send_ack(struct tftp_conn* tc) {
  int count;
  /* Send an ack packet. The block number we want to ack is already in
             the buffer so we just need to change the opcode. Note that the ACK
             is sent to the port number which the server just sent the data
             from, NOT to port 69 */
  *(short*)tc->msgbuf = htons(OPCODE_ACK);
  (*(short*)(tc->msgbuf + 2)) = htons(tc->blocknr);
  if ((count = sendto(tc->sock, tc->msgbuf, 4, 0, (struct sockaddr*)&tc->server,
                      tc->addrlen)) < 0) {
    fprintf(stderr, "Error sending ACK\n");
    return -1;
  }
  return 1;
}

int tftp_recv(struct tftp_conn* tc) {
  char* p;
  int fdstdout;
  int count, server_len, data_len;
  int status;
  p = tc->msgbuf + 2;             /* Point to the next location */
  p += strlen(tc->file_name) + 1; /* Keep the null terminator */
  p += strlen(MODE_OCTET) + 1;
  int retval = 0;

  /* Sanity check */
  if (!tc)
    return -1;

  /*
     GET the file, block by block, in a loop.
   */
  do {
    /* Check the message type and take the necessary
     * action. */

    /*ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                    struct sockaddr *src_addr, socklen_t *addrlen);*/
    server_len = sizeof(tc->server);
    data_len = 0;
    if ((count = recvfrom(tc->sock, tc->msgbuf, MSGBUF_SIZE, 0,
                          (struct sockaddr*)&(tc->server),
                          (socklen_t*)&tc->addrlen)) < 0) {
      fprintf(stderr, "Error from recvfrom.\n");
      exit(1);
    }
    // get data count and update count
    char* p = (tc->msgbuf + 4);
    while (*p) {
      data_len++;
      p++;
    }

    // get DATA and send ACK
    switch (ntohs(*(short*)tc->msgbuf)) {
      case OPCODE_DATA:
        if ((status = validate_data(tc)) > 0) {
          if ((fdstdout = write(1, tc->msgbuf + 4, count - 4)) < 0) {
            fprintf(stderr, "Error with fwrite\n");
            continue;
          }
          if (send_ack(tc) < 0) {
            fprintf(stderr, "Error sending ACK\n");
            continue;
          }
        }
        break;
      case OPCODE_ERR:
        /* Handle error... */
        fprintf(stderr, "remcat: %s\n", tc->msgbuf + 4);
        break;
      default:
        fprintf(stderr, "\nUnknown message type\n");
        goto out;
    }

  } while (data_len == 508);

  // printf("\nTotal data bytes sent/received: %d.\n", totlen);
out:
  return retval;
}

/* Close the connection handle, i.e., delete our local state. */
void tftp_close(struct tftp_conn* tc) {
  if (!tc)
    return;
  close(tc->sock);
  free(tc);
}

// driver
int main(int argc, char* argv[]) {
  /* sanity checks */
  if (argc != 3) {
    fprintf(stderr, "usage: %s hostname filename\n", argv[0]);
    exit(1);
  }
  struct tftp_conn* tc;
  // Always a GET request (in our case)
  int type = TFTP_TYPE_GET;
  // the file will be displayed on the STDOUT
  char* fname = argv[2];
  int count = -1;
  int retval = -1;

  /* Connect to the remote server */
  if ((tc = tftp_connect(type, fname, argv[1], MODE_OCTET)) == 0) {
    fprintf(stderr, "Failed to connect!\n");
    return -1;
  }

  /* Send a read request to the server. */
  if ((count = tftp_send_rrq(tc)) < 0) {
    fprintf(stderr, "Failed to send an RRQ!\n");
  }

  /* If RRQ succeeds,
     Transfer the remote file in blocks to the STDOUT */
  if ((retval = tftp_recv(tc)) < 0) {
    fprintf(stderr, "File transfer failed!\n");
  }

  /* We are done. Cleanup our state. */
  tftp_close(tc);

  return 0;
}
