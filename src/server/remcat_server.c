/*
 * A Remote cat server using the TFTP protocol.
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

#define BUFF_SIZE 512

struct timeval timeout;

/* A connection handle */
struct tftp_conn {
  FILE* fp;
  int fd;
  int sock;          /* Socket to communicate with server */
  u_int16_t blocknr; /* The current block number */
  char* file_name;   /* The file name of the file we are putting or getting */
  char* mode;        /* TFTP mode */
  struct hostent* hostp;     /* client host info */
  struct sockaddr_in client; /* Remote peer address */
  struct sockaddr_in server; /* server (current machine) address */
  socklen_t addrlen_c;       /* The remote address length */
  socklen_t addrlen_s;       /* The remote address length */
  char msgbuffer[BUFF_SIZE]; /* Buffer for messages being sent or received */
};

ssize_t send_data(struct tftp_conn* tc) {
  ssize_t count;

  *((short*)tc->msgbuffer) = htons(OPCODE_DATA);
  *(short*)(tc->msgbuffer + 2) = htons(tc->blocknr);

  if ((count = sendto(tc->sock, tc->msgbuffer, sizeof(tc->msgbuffer), 0,
                      (struct sockaddr*)&tc->client, tc->addrlen_c)) < 0) {
    fprintf(stderr, "Error in sending\n");
  }
  return count;
}

/* Receive message */
int recv_message(struct tftp_conn* tc) {
  int count;
  if ((count = recvfrom(tc->sock, tc->msgbuffer, sizeof(tc->msgbuffer), 0,
                        (struct sockaddr*)&tc->client,
                        (socklen_t*)&tc->addrlen_c)) < 0) {
    fprintf(stderr, "Error receiving data\n");
  }
  return count;
}

/* Send error */
int send_error(struct tftp_conn* tc) {
  *((short*)tc->msgbuffer) = htons(OPCODE_ERR);
  if ((sendto(tc->sock, tc->msgbuffer, sizeof(tc->msgbuffer), 0,
              (struct sockaddr*)&tc->client, tc->addrlen_c)) < 0) {
    fprintf(stderr, "Error in sending\n");
    return -1;
  }
  return 1;
}

int process_rrq(struct tftp_conn* tc) {
  // pointer to the file we want to read from.
  char fname[100];
  int last_pack = 0;
  ssize_t data_len = 508;
  tc->blocknr = 0;
  int blocknr;
  int countdown;
  int count;

  /* parse client request */
  strcpy(fname, tc->msgbuffer + 2);
  tc->file_name = fname;

  // check if file exists
  if ((tc->fp = fopen(tc->file_name, "r")) == NULL) {
    printf("Error, could not open file\n");
    strcpy(tc->msgbuffer + 4, "Error opening file\0");
    if (send_error(tc) < 0) {
      printf("Error sending\n");
      exit(1);
    }
    exit(1);
  }

  // while the packet to send isn't the last packet
  while (!last_pack) {
    memset(tc->msgbuffer + 4, 0, 508);
    data_len = fread(tc->msgbuffer + 4, sizeof(char), data_len, tc->fp);
    tc->blocknr++;
    // if this is the last packet, we need to close the connection
    if (data_len < 508)
      last_pack = 1;

    // perform a countdown to resend the data packet
    // in case of a timeout
    for (countdown = TFTP_TIMEOUT; countdown; countdown--) {
      // send the packets 512 bytes at a time
      if ((count = send_data(tc)) < 0) {
        fprintf(stderr, "transfer killed\n");
        exit(1);
      }

      // receive the ack
      count = recv_message(tc);

      if (count >= 0 && count < 4) {
        fprintf(stderr, "message with invalid size received\n");
        exit(1);
      }

      if (count >= 4)
        break;
    }

    if (!countdown) {
      printf("%s.%u: transfer timed out\n", inet_ntoa(tc->client.sin_addr),
             ntohs(tc->client.sin_port));
      exit(1);
    }

    if (ntohs(*((short*)tc->msgbuffer)) == OPCODE_ERR) {
      printf("%s.%u: error message received: \n",
             inet_ntoa(tc->client.sin_addr), ntohs(tc->client.sin_port));
      exit(1);
    }

    if (ntohs(*((short*)tc->msgbuffer)) != OPCODE_ACK) {
      printf("%s.%u: invalid message during transfer received\n",
             inet_ntoa(tc->client.sin_addr), ntohs(tc->client.sin_port));
      // send_error(s, 0, "invalid message during transfer", tc->client,
      //           addrlen_c);
      exit(1);
    }

    if ((blocknr = ntohs(*((short*)(tc->msgbuffer + 2)))) !=
        tc->blocknr) {  // the ack number is too high
      printf("%s.%u: invalid ack number received\n",
             inet_ntoa(tc->client.sin_addr), ntohs(tc->client.sin_port));
      // send_error(tc->sock, 0, "invalid ack number", tc->client, addrlen_c);
      exit(1);
    }
  }
  return data_len + 4;  // + 4 for the OPCODE and BLOCK
}

// driver
int main(int argc, char const* argv[]) {
  struct tftp_conn* tc;
  // Always a GET request (in our case)
  int port = 5069;
  tc = (struct tftp_conn*)malloc(sizeof(struct tftp_conn));
  // for sockopt
  int optval;
  // for error checks throught the program
  int status;
  // recv value ssize_t
  ssize_t recv;
  tc->addrlen_c = sizeof(struct sockaddr_in);
  tc->addrlen_s = sizeof(struct sockaddr_in);
  /* Socket: Create */
  char* hostaddrp;
  if ((tc->sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    fprintf(stderr, "Error creating sock\n");
    exit(1);
  }

  /* It lets us rerun the server immediately after we kill it;
   * otherwise we have to wait about 20 secs.
   * Eliminates "ERROR on binding: Address already in use" error. */
  optval = 1;
  setsockopt(tc->sock, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval,
             sizeof(int));

  /* Internet address phase*/
  memset((char*)&tc->server, 0, sizeof(struct sockaddr_in));
  bzero((char*)&tc->server, sizeof(struct sockaddr_in));
  tc->server.sin_family = AF_INET;
  tc->server.sin_addr.s_addr = htonl(INADDR_ANY);
  tc->server.sin_port = htons(5069);

  /* Bind the socket with the port */
  if ((status = bind(tc->sock, (struct sockaddr*)&tc->server,
                     sizeof(struct sockaddr_in))) < 0) {
    fprintf(stderr, "Bind error\n");
    exit(1);
  }

  /* Main server loop */
  while (1) {
    /* Listen for a connection */
    timeout.tv_sec = TFTP_TIMEOUT;
    timeout.tv_usec = 0;

    // first reset the buffer
    memset((char*)tc->msgbuffer, 0, BUFF_SIZE);
    if ((recv = recvfrom(tc->sock, (char*)tc->msgbuffer, BUFF_SIZE, 0,
                         (struct sockaddr*)&(tc->client),
                         (socklen_t*)&tc->addrlen_c)) < 0) {
      fprintf(stderr, "Error receiving from client\n");
    }
    /* Determine who sent the request */
    tc->hostp = gethostbyaddr((const char*)&tc->client.sin_addr.s_addr,
                              sizeof(tc->client.sin_addr.s_addr), AF_INET);
    if (tc->hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(tc->client.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");

    switch (ntohs(*(short*)tc->msgbuffer)) {
      // first request packet, process it and send the first data packet
      // should there be no error
      case OPCODE_RRQ:
        if ((status = process_rrq(tc)) < 0) {
          fprintf(stderr, "Error processing RRQ\n");
        }
        break;
      default:
        fprintf(stderr, "invalid request or opcode %hi\n",
                *((short*)tc->msgbuffer));
        break;
    }
  }
  // release resources
  free(tc);
  // get rid of dangling pointers
  tc = NULL;

  return 0;
}
