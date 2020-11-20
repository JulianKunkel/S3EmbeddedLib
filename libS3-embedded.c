#include <libs3.h>

const char *S3_get_status_name(S3Status status){
  return "libS3EmbeddedStatus";
}

void S3_deinitialize(){

}

S3Status S3_initialize(const char *userAgentInfo, int flags,
                       const char *defaultS3HostName){
 return S3StatusOK;
}

void S3_list_service(S3Protocol protocol, const char *accessKeyId,
                     const char *secretAccessKey, const char *securityToken,
                     const char *hostName, const char *authRegion,
                     S3RequestContext *requestContext,
                     int timeoutMs,
                     const S3ListServiceHandler *handler, void *callbackData){

}


void S3_create_bucket(S3Protocol protocol, const char *accessKeyId,
                      const char *secretAccessKey, const char *securityToken,
                      const char *hostName, const char *bucketName,
                      const char *authRegion, S3CannedAcl cannedAcl,
                      const char *locationConstraint,
                      S3RequestContext *requestContext,
                      int timeoutMs,
                      const S3ResponseHandler *handler, void *callbackData){
}

void S3_delete_bucket(S3Protocol protocol, S3UriStyle uriStyle,
                      const char *accessKeyId, const char *secretAccessKey,
                      const char *securityToken, const char *hostName,
                      const char *bucketName, const char *authRegion,
                      S3RequestContext *requestContext,
                      int timeoutMs,
                      const S3ResponseHandler *handler, void *callbackData){
}

void S3_list_bucket(const S3BucketContext *bucketContext,
                    const char *prefix, const char *marker,
                    const char *delimiter, int maxkeys,
                    S3RequestContext *requestContext,
                    int timeoutMs,
                    const S3ListBucketHandler *handler, void *callbackData){
}

void S3_test_bucket(S3Protocol protocol, S3UriStyle uriStyle,
                    const char *accessKeyId, const char *secretAccessKey,
                    const char *securityToken, const char *hostName,
                    const char *bucketName, const char *authRegion,
                    int locationConstraintReturnSize,
                    char *locationConstraintReturn,
                    S3RequestContext *requestContext,
                    int timeoutMs,
                    const S3ResponseHandler *handler, void *callbackData){
}

void S3_get_object(const S3BucketContext *bucketContext, const char *key,
                   const S3GetConditions *getConditions,
                   uint64_t startByte, uint64_t byteCount,
                   S3RequestContext *requestContext,
                   int timeoutMs,
                   const S3GetObjectHandler *handler, void *callbackData){
}

void S3_head_object(const S3BucketContext *bucketContext, const char *key,
                    S3RequestContext *requestContext,
                    int timeoutMs,
                    const S3ResponseHandler *handler, void *callbackData){
}


void S3_delete_object(const S3BucketContext *bucketContext, const char *key,
                      S3RequestContext *requestContext,
                      int timeoutMs,
                      const S3ResponseHandler *handler, void *callbackData){
}

void S3_put_object(const S3BucketContext *bucketContext, const char *key,
                   uint64_t contentLength,
                   const S3PutProperties *putProperties,
                   S3RequestContext *requestContext,
                   int timeoutMs,
                   const S3PutObjectHandler *handler, void *callbackData){
}
