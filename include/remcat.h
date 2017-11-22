#ifndef _REMCAT_H_
#define _REMCAT_H_

#if defined(SUNOS_5)
#define OS_SUNOS5
typedef short u_int16_t;
typedef int u_int32_t;
typedef char u_int8_t;
#elif defined(__GNUC__) && (defined(__APPLE_CPP__) || defined(__APPLE_CC__) || \
                            defined(__MACOS_CLASSIC__))
#define OS_MACOSX
#elif defined(__GNUC__) && defined(__linux__)
#define OS_LINUX
#include <linux/types.h>
#else
#error "Unsupported operating system"
#endif

/* tftp's standard port number */
#define TFTP_PORT 69

#define MODE_OCTET "octet"

#define BLOCK_SIZE 512

#define TFTP_TYPE_GET 0

/* TFTP op-codes */
#define OPCODE_RRQ 1
#define OPCODE_DATA 3
#define OPCODE_ACK 4
#define OPCODE_ERR 5

/* Timeout in seconds */
#define TFTP_TIMEOUT 2

/* Timeout in seconds */
#define TFTP_TIMEOUT 2

static char* err_codes[8] = {"Undef",
                             "File not found",
                             "Access violation",
                             "Disk full or allocation exceeded",
                             "Illegal TFTP operation",
                             "Unknown transfer ID",
                             "File already exists",
                             "No such user"};

#define TFTP_RRQ_HDR_LEN sizeof(struct tftp_rrq)
#define TFTP_DATA_HDR_LEN sizeof(struct tftp_data)
#define TFTP_RRQ_LEN(f, m) (sizeof(struct tftp_rrq) + strlen(f) + strlen(m) + 2)

#endif /* _REMCAT_H_ */