#ifndef LIBS3_GW_H
#define LIBS3_GW_H

#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <libs3.h>
#include <linux/limits.h>

#define MAX_DATA_SIZE 1024*1024*100
#define PORT 2020
#define SA struct sockaddr
#define WARNING(...) printf("LIBS3R WARNING %s:%d ", __FILE__, __LINE__); printf(__VA_ARGS__);
#define INFO(...) printf("LIBS3R INFO %s:%d ", __FILE__, __LINE__); printf(__VA_ARGS__);

#ifdef USE_DEBUG
#define DEBUG(...) printf("LIBS3R DEBUG %s:%d ", __FILE__, __LINE__); printf(__VA_ARGS__);
#else
#define DEBUG(...) ;
#endif


#define FATAL(...) do{ printf(__VA_ARGS__); exit(0); } while(0);

typedef enum{
  REQ_CREATE_BUCKET,
  REQ_DELETE_BUCKET,
  REQ_LIST_SERVICE,
  REQ_LIST_BUCKET,
  REQ_TEST_BUCKET,
  REQ_PUT_OBJECT,
  REQ_GET_OBJECT,
  REQ_HEAD_OBJECT,
  REQ_DELETE_OBJECT,
  REQ_UNDEFINED
} op_req_e;

#define HEADER_LENGTH (sizeof(uint32_t)+ sizeof(op_req_e))

typedef struct{
  uint32_t length;
  op_req_e op;
} req_t;

typedef struct{
  uint32_t length;
  S3Status status;
} resp_t;

int rcv_data(int sockfd, int remain, void* pos);
int snd_data(int sockfd, int remain, void const* pos);
int set_nonblock_fd(int sockfd);
#endif
