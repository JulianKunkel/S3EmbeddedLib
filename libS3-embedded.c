#include <libs3.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <linux/limits.h>
#include <dirent.h>


#define WARNING(...) printf("LIBS3E WARNING %s:%d ", __FILE__, __LINE__); printf(__VA_ARGS__);
#define FATAL(...) do{ printf(__VA_ARGS__); exit(0); } while(0);

typedef struct {
  char * dirname;
} options_t;

static options_t opt;

#define SET_BUCKET_NAME(buf, hostname, name) sprintf(buf, "%s/%s", hostname ? hostname : opt.dirname, name);

const char *S3_get_status_name(S3Status status){
  return "libS3EmbeddedStatus";
}

void S3_deinitialize(){

}

S3Status S3_initialize(const char *userAgentInfo, int flags,
                       const char *defaultS3HostName)
{
  memset(& opt, 0, sizeof(opt));
  if(defaultS3HostName != NULL){
   opt.dirname = strdup(defaultS3HostName);
  }else{
   opt.dirname = "s3embedded";
  }
  WARNING("Using the S3embedded library with storage: %s\n", opt.dirname);

  struct stat sbuf;
  int ret = stat(opt.dirname, & sbuf);
  if(ret != 0){
   ret = mkdir(opt.dirname, S_IRWXU);
   if(ret != 0){
     FATAL("Couldn't create directory: %s\n", opt.dirname);
   }
  }
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
  S3Status status = S3StatusOK;
  char path[PATH_MAX];
  SET_BUCKET_NAME(path, hostName, bucketName);
  int ret = mkdir(path, S_IRWXU);
  if (ret != 0){
    WARNING("mkdir: %s %s\n", path, strerror(errno));
    status = S3StatusErrorBucketAlreadyExists;
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
  S3Status status = S3StatusOK;
  char path[PATH_MAX];
  SET_BUCKET_NAME(path, hostName, bucketName);
  int ret = rmdir(path);
  if (ret != 0){
    WARNING("rmdir: %s %s\n", path, strerror(errno));
    status = S3StatusErrorAccessDenied;
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
  S3Status status = S3StatusOK;
  struct dirent * d;
  char username[100];
  getlogin_r(username, 100);
  DIR * dir = opendir(hostName ? hostName : opt.dirname);
  if(dir){
    while ((d = readdir(dir)) != NULL){
      // skip dot files:
      if(d->d_name[0] == '.') {
        continue;
      }
      int creationDateSeconds = 0;
      handler->listServiceCallback(username, username, d->d_name, creationDateSeconds, callbackData);
    }
    closedir(dir);
  }else{
    WARNING("opendir: %s %s\n", hostName ? hostName : opt.dirname, strerror(errno));
    status = S3StatusErrorAccessDenied;
  }
  handler->responseHandler.completeCallback(status, & errs, callbackData);
}

void S3_list_bucket(const S3BucketContext *bucketContext,
                    const char *prefix, const char *marker,
                    const char *delimiter, int maxkeys,
                    S3RequestContext *requestContext,
                    int timeoutMs,
                    const S3ListBucketHandler *handler, void *callbackData)
{
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
}

void S3_get_object(const S3BucketContext *bucketContext, const char *key,
                   const S3GetConditions *getConditions,
                   uint64_t startByte, uint64_t byteCount,
                   S3RequestContext *requestContext,
                   int timeoutMs,
                   const S3GetObjectHandler *handler, void *callbackData)
{
}

void S3_head_object(const S3BucketContext *bucketContext, const char *key,
                    S3RequestContext *requestContext,
                    int timeoutMs,
                    const S3ResponseHandler *handler, void *callbackData)
{
}


void S3_delete_object(const S3BucketContext *bucketContext, const char *key,
                      S3RequestContext *requestContext,
                      int timeoutMs,
                      const S3ResponseHandler *handler, void *callbackData)
{
}

void S3_put_object(const S3BucketContext *bucketContext, const char *key,
                   uint64_t contentLength,
                   const S3PutProperties *putProperties,
                   S3RequestContext *requestContext,
                   int timeoutMs,
                   const S3PutObjectHandler *handler, void *callbackData)
{
}
