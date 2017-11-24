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
#include "../include/remcat.h"

#define BUFF_SIZE 1024

// driver
int main(int argc, char const* argv[]) {
  int sock;
  int port = TFTP_PORT;
  struct sockaddr_in server;
  struct hostent* host;
  char buffer[BUFF_SIZE];
  // for sockopt
  int optval;
  // for error checks throught the program
  int status;

  /* Socket: Create */
  if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    fprintf(stderr, "Error creating sock\n");
    exit(1);
  }

  /* It lets us rerun the server immediately after we kill it;
   * otherwise we have to wait about 20 secs.
   * Eliminates "ERROR on binding: Address already in use" error. */
  optval = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof int);

  /* Internet address phase*/
  memset((char*)&server, 0, sizeof server);

  /* Bind the socket with the port */
  if ((status = bind(sock, (struct sockaddr*)&server, sizeof server)) < 0) {
    fprintf(stderr, "Bind error\n");
    exit(1);
  }

  /* Main server loop */
  while (1) {
    /* recvfrom the client */
    // first reset the buffer
    memset((char*)buffer, 0, BUFF_SIZE);
  }

  return 0;
}