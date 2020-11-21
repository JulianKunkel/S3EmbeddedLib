#ifndef LIBS3_GW_H
#define LIBS3_GW_H

#include <stdint.h>

#define MAX_DATA_SIZE 1024*1024*10

typedef enum{
  REQ_UNDEFINED
} op_req_e;

typedef struct{
  uint32_t length;
  op_req_e op;
  uint8_t data[];
} req_t;

typedef enum{
  RESP_UNDEFINED
} op_resp_e;

typedef struct{
  uint32_t length;
  op_resp_e op;
  uint8_t data[];
} resp_t;

#endif
