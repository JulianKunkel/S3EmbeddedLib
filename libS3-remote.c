#include <libs3.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <libS3-gw.h>

typedef struct {
  char * hostname;
  int socket;
  int buffer_size_send;
  int buffer_size_rcv;
} options_t;

static options_t opt;

#define SET_OBJECT_NAME(buf, name, key) sprintf(buf, "%s/%s", name, key);


const char *S3_get_status_name(S3Status status){
  return "libS3Remote";
}

void S3_deinitialize(){
  close(opt.socket);
}

S3Status S3_initialize(const char *userAgentInfo, int flags,
                       const char *defaultS3HostName)
{
  memset(& opt, 0, sizeof(opt));
  opt.buffer_size_send = 64*1024;
  opt.buffer_size_rcv = 64*1024;
  if(opt.hostname){
    opt.hostname = strdup(defaultS3HostName);
  }else{
    opt.hostname = "localhost:2020";
  }
  WARNING("Using the S3remote library with storage: %s\n", opt.hostname);

  int sockfd;
  struct sockaddr_in servaddr;

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
      FATAL("socket creation failed\n");
  }
  memset(&servaddr, 0, sizeof(servaddr));

  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  servaddr.sin_port = htons(PORT);

  if (connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) != 0) {
      FATAL("connection to server failed: %s\n", strerror(errno));
  }
  opt.socket = sockfd;

  int optval = 1;
  socklen_t optlen = sizeof(optval);
  // BSD:
  //if(setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &optval, optlen)){
  //  FATAL("setsockopt()");
  //}
  if(setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
    FATAL("setsockopt()");
  }
  if(setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, & opt.buffer_size_send, optlen) < 0){
    FATAL("setsockopt SO_SNDBUF");
  }
  if(setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, & opt.buffer_size_rcv, optlen) < 0){
    FATAL("setsockopt SO_RCVBUF");
  }
  getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, & optval, & optlen);
  INFO("send buffer size = %d\n", optval);
  getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, & optval, & optlen);
  INFO("rcv buffer size = %d\n", optval);

  return S3StatusOK;
}

void S3_create_bucket(S3Protocol protocol, const char *accessKeyId,
                      const char *secretAccessKey, const char *securityToken,
                      const char *hostName, const char *bucketName,
                      const char *authRegion, S3CannedAcl cannedAcl,
                      const char *locationConstraint,
                      S3RequestContext *requestContext,
                      int timeoutMs,
                      const S3ResponseHandler *handler, void *callbackData)
{
  S3ErrorDetails errs = {0};
  S3Status status = S3StatusErrorBucketAlreadyExists;

  req_t req = {.length = strlen(bucketName) + 1, .op = REQ_CREATE_BUCKET};
  resp_t resp;

  if(snd_data(opt.socket, HEADER_LENGTH, & req) == 0 &&
    snd_data(opt.socket, req.length, bucketName) == 0 &&
    rcv_data(opt.socket, HEADER_LENGTH, & resp) == 0
    ){
      status = resp.status;
  }

  handler->completeCallback(status, & errs, callbackData);
}

void S3_delete_bucket(S3Protocol protocol, S3UriStyle uriStyle,
                      const char *accessKeyId, const char *secretAccessKey,
                      const char *securityToken, const char *hostName,
                      const char *bucketName, const char *authRegion,
                      S3RequestContext *requestContext,
                      int timeoutMs,
                      const S3ResponseHandler *handler, void *callbackData)
{
  S3ErrorDetails errs = {0};
  S3Status status = S3StatusErrorAccessDenied;

  req_t req = {.length = strlen(bucketName) + 1, .op = REQ_DELETE_BUCKET};
  resp_t resp;


  if(snd_data(opt.socket, HEADER_LENGTH, & req) == 0 &&
    snd_data(opt.socket, req.length, bucketName) == 0 &&
    rcv_data(opt.socket, HEADER_LENGTH, & resp) == 0
    ){
      status = resp.status;
  }

  handler->completeCallback(status, & errs, callbackData);
}

void S3_list_service(S3Protocol protocol, const char *accessKeyId,
                     const char *secretAccessKey, const char *securityToken,
                     const char *hostName, const char *authRegion,
                     S3RequestContext *requestContext,
                     int timeoutMs,
                     const S3ListServiceHandler *handler, void *callbackData)
{
  S3ErrorDetails errs = {0};
  S3Status status = S3StatusErrorAccessDenied;
  char username[100];
  getlogin_r(username, 100);

  req_t req = {.length = 1, .op = REQ_LIST_SERVICE};
  resp_t resp;
  char data = 0;

  if(snd_data(opt.socket, HEADER_LENGTH, & req) == 0 &&
    snd_data(opt.socket, req.length, & data) == 0 &&
    rcv_data(opt.socket, HEADER_LENGTH, & resp) == 0
    ){
      status = resp.status;
      // receive the actual data
      char * data = malloc(resp.length);
      if(rcv_data(opt.socket, resp.length, data) != 0){
        status = S3StatusErrorAccessDenied;
      }else{
        char * last = data;
        for(int i=0; i < resp.length; i++){
          if(data[i] == '\0'){
            int creationDateSeconds = 0;
            handler->listServiceCallback(username, username, last, creationDateSeconds, callbackData);
            last = & data[i] + 1;
          }
        }
      }
      free(data);
  }
  handler->responseHandler.completeCallback(status, & errs, callbackData);
}

void S3_list_bucket(const S3BucketContext *bc,
                    const char *prefix, const char *marker,
                    const char *delimiter, int maxkeys,
                    S3RequestContext *requestContext,
                    int timeoutMs,
                    const S3ListBucketHandler *handler, void *callbackData)
{
  S3ErrorDetails errs = {0};
  S3Status status = S3StatusOK;
  char username[100];
  getlogin_r(username, 100);

  int prefix_len = prefix == NULL ? 0 : strlen(prefix) + 1;
  int bucket_len = strlen(bc->bucketName) + 1;
  req_t req = {.length = bucket_len + prefix_len + sizeof(prefix_len), .op = REQ_LIST_BUCKET};
  resp_t resp;

  if(snd_data(opt.socket, HEADER_LENGTH, & req) == 0 &&
    snd_data(opt.socket, bucket_len, bc->bucketName) == 0 &&
    snd_data(opt.socket, sizeof(prefix_len), &prefix_len) == 0 &&
    snd_data(opt.socket, prefix_len, prefix) == 0 &&
    rcv_data(opt.socket, HEADER_LENGTH, & resp) == 0
    ){
    S3ListBucketContent c = {0};
    c.ownerDisplayName = username;
    c.ownerId = username;

    status = resp.status;
    // receive the actual data
    char * data = malloc(resp.length);
    if(rcv_data(opt.socket, resp.length, data) != 0){
      status = S3StatusErrorAccessDenied;
    }else{
      char * last = data;
      for(int i=0; i < resp.length; i++){
        if(data[i] == '\0'){
          c.key = last;
          handler->listBucketCallback(0, NULL, 1, &c, 0, NULL, callbackData);
          last = & data[i] + 1;
        }
      }
    }
    free(data);
  }

  handler->responseHandler.completeCallback(status, & errs, callbackData);
}



void S3_test_bucket(S3Protocol protocol, S3UriStyle uriStyle,
                    const char *accessKeyId, const char *secretAccessKey,
                    const char *securityToken, const char *hostName,
                    const char *bucketName, const char *authRegion,
                    int locationConstraintReturnSize,
                    char *locationConstraintReturn,
                    S3RequestContext *requestContext,
                    int timeoutMs,
                    const S3ResponseHandler *handler, void *callbackData)
{
  S3ErrorDetails errs = {0};
  S3Status status = S3StatusErrorNoSuchBucket;
  req_t req = {.length = strlen(bucketName) + 1, .op = REQ_TEST_BUCKET};
  resp_t resp;

  if(snd_data(opt.socket, HEADER_LENGTH, & req) == 0 &&
    snd_data(opt.socket, req.length, bucketName) == 0 &&
    rcv_data(opt.socket, HEADER_LENGTH, & resp) == 0
    ){
      status = resp.status;
  }
  handler->completeCallback(status, & errs, callbackData);
}

void S3_put_object(const S3BucketContext *bc, const char *key,
                   uint64_t contentLength,
                   const S3PutProperties *putProperties,
                   S3RequestContext *requestContext,
                   int timeoutMs,
                   const S3PutObjectHandler *handler, void *callbackData)
{
  S3ErrorDetails errs = {0};
  S3Status status = S3StatusErrorNoSuchBucket;
  char path[PATH_MAX];
  SET_OBJECT_NAME(path, bc->bucketName, key);


  char * buffer = malloc(MAX_DATA_SIZE);
  int size = handler->putObjectDataCallback(MAX_DATA_SIZE, buffer, callbackData);
  int len = strlen(path) + 1;
  req_t req = {.length = len + size + sizeof(int), .op = REQ_PUT_OBJECT};
  resp_t resp;

  if(snd_data(opt.socket, HEADER_LENGTH, & req) == 0 &&
    snd_data(opt.socket, len, path) == 0 &&
    snd_data(opt.socket, sizeof(int), & size) == 0 &&
    snd_data(opt.socket, size, buffer) == 0 &&
    rcv_data(opt.socket, HEADER_LENGTH, & resp) == 0
    ){
      status = resp.status;
  }
  free(buffer);

  handler->responseHandler.completeCallback(status, & errs, callbackData);
}


void S3_get_object(const S3BucketContext *bc, const char *key,
                   const S3GetConditions *getConditions,
                   uint64_t startByte, uint64_t byteCount,
                   S3RequestContext *requestContext,
                   int timeoutMs,
                   const S3GetObjectHandler *handler, void *callbackData)
{
  S3ErrorDetails errs = {0};
  S3Status status = S3StatusErrorNoSuchBucket;
  char path[PATH_MAX];
  SET_OBJECT_NAME(path, bc->bucketName, key);

  int len = strlen(path) + 1;
  req_t req = {.length = len + sizeof(uint64_t)*2, .op = REQ_GET_OBJECT};
  resp_t resp;

  if(snd_data(opt.socket, HEADER_LENGTH, & req) == 0 &&
    snd_data(opt.socket, len, path) == 0 &&
    snd_data(opt.socket, sizeof(uint64_t), & startByte) == 0 &&
    snd_data(opt.socket, sizeof(uint64_t), & byteCount) == 0 &&
    rcv_data(opt.socket, HEADER_LENGTH, & resp) == 0
    ){
      status = resp.status;
      if(status == S3StatusOK){
        char * data = malloc(resp.length);
        if(rcv_data(opt.socket, resp.length, data) == 0){
          handler->getObjectDataCallback(resp.length, data, callbackData);
        }else{
          status = S3StatusErrorAccountProblem;
        }
        free(data);
      }
  }

  handler->responseHandler.completeCallback(status, & errs, callbackData);
}

void S3_head_object(const S3BucketContext *bc, const char *key,
                    S3RequestContext *requestContext,
                    int timeoutMs,
                    const S3ResponseHandler *handler, void *callbackData)
{
  S3ErrorDetails errs = {0};
  S3Status status = S3StatusErrorAccessDenied;
  char path[PATH_MAX];
  SET_OBJECT_NAME(path, bc->bucketName, key);

  req_t req = {.length = strlen(path) + 1, .op = REQ_HEAD_OBJECT};
  resp_t resp;
  if(snd_data(opt.socket, HEADER_LENGTH, & req) == 0 &&
    snd_data(opt.socket, req.length, path) == 0 &&
    rcv_data(opt.socket, HEADER_LENGTH, & resp) == 0
    ){
      status = resp.status;
      if(resp.status == S3StatusOK){
        uint64_t size;
        uint64_t mtime;
        rcv_data(opt.socket, sizeof(size), & size);
        rcv_data(opt.socket, sizeof(mtime), & mtime);
        S3ResponseProperties p;
        p.contentLength = size;
        p.lastModified = mtime;
        handler->propertiesCallback(& p, callbackData);
      }
  }

  handler->completeCallback(status, & errs, callbackData);
}


void S3_delete_object(const S3BucketContext *bc, const char *key,
                      S3RequestContext *requestContext,
                      int timeoutMs,
                      const S3ResponseHandler *handler, void *callbackData)
{
  S3ErrorDetails errs = {0};
  S3Status status = S3StatusErrorNoSuchBucket;
  char path[PATH_MAX];
  SET_OBJECT_NAME(path, bc->bucketName, key);

  req_t req = {.length = strlen(path) + 1, .op = REQ_DELETE_OBJECT};
  resp_t resp;
  if(snd_data(opt.socket, HEADER_LENGTH, & req) == 0 &&
    snd_data(opt.socket, req.length, path) == 0 &&
    rcv_data(opt.socket, HEADER_LENGTH, & resp) == 0
    ){
      status = resp.status;
  }
  handler->completeCallback(status, & errs, callbackData);
}
