# remote-cat
An TFTP based client to read and display the contents of a remote file on the local system's STDOUT.

## Introduction
TFTP is a simple file transfer protocol that uses UDP as its underlying protocol and enables a client to fetch a file from a remote server (host where TFTP is installed). It is designed to be small and easy to implement. Therefore, it lacks most of the features of a regular FTP. The only thing it can do is read and write files (or mail) from/to a remote server. Hence, we are going to use this property to our advantage and transfer files from servers and render the data on local STDOUT.

+ A client initiates a transfer by issuing a read request to read a file from a remote server. 
+ The server on accepting the request sends the file in fixed block lengths usually within a single IP packet. 
+ The client on receiving the packet must send an ACK before it can receive another block. 
+ The server sends numbered packets to the client and the client replies with the ACKs. 
+ The final data packet should contain less than full sized block to indicate that it is the final block.

## Architecture

    remcat Client --------------->  TFTP Server (file.txt)
                    RRQ                                           


    remcat Client <---------------  TFTP Server (file.txt)
     [STDOUT]         DAT (TFTP)                                
                       

### TFTP Packets

            2 bytes     string    1 byte     string   1 byte
            ------------------------------------------------
           | Opcode |  Filename  |   0  |    Mode    |   0  |
           --------------------------------------------------
                                RRQ/WRQ packet

                   2 bytes     2 bytes      n bytes
                   ----------------------------------
                  | Opcode |   Block #  |   Data     |
                   ----------------------------------
                                DATA packet

                        2 bytes     2 bytes
                         ---------------------
                        | Opcode |   Block #  |
                         ---------------------
                                ACK packet

                2 bytes     2 bytes      string   1 byte
               -----------------------------------------
              | Opcode |  ErrorCode |   ErrMsg   |   0  |
               -----------------------------------------
                                ERROR packet



## Usage
    user@system:~ remcat hostname file.txt                  
    
## References
    RFC1350 details: https://www.ietf.org/rfc/rfc1350.txt

## NOTE:
The development of remote-cat server (src/remcat_server.c) is still in progress. As a temporary workaround, you can download and install the tftpd server from a debian style package manager to test the client. (The existing client (src/remcat.c) works well with the tftpd server). Read the instructions below to download and install the TFTPD server until the (work in progress) server is fully developed.

### TFTPD Server Install and Setup
        1. Install following packages.

        sudo apt-get install xinetd tftpd tftp

        2. Create /etc/xinetd.d/tftp and put this entry

        service tftp
        {
        protocol        = udp
        port            = 69
        socket_type     = dgram
        wait            = yes
        user            = nobody
        server          = /usr/sbin/in.tftpd
        server_args     = /tftpboot
        disable         = no
        }

        3. Create a folder /tftpboot this should match whatever you gave in server_args. mostly it will be tftpboot

        sudo mkdir /tftpboot
        sudo chmod -R 777 /tftpboot
        sudo chown -R nobody /tftpboot

        4. Restart the xinetd service.

        sudo /etc/init.d/xinetd restart

        5. In addition, you can use the lsof command to verify if the server is up and running.

        sudo lsof -i | grep tftp
