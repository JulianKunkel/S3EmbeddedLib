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
#include <libS3e.h>

#define BUFF_SIZE 1024 * 1024 * 100
#define WARNING(...)                                   \
  printf("LIBS3E WARNING %s:%d ", __FILE__, __LINE__); \
  printf(__VA_ARGS__);
#define FATAL(...)       \
  do                     \
  {                      \
    printf(__VA_ARGS__); \
    exit(0);             \
  } while (0);

typedef struct
{
  char *dirname;
} options_t;

static options_t opt;

const char *S3_get_status_name(S3Status status)
{
  return "libS3EmbeddedStatus";
}

void S3_deinitialize()
{
}

S3Status S3_initialize(const char *userAgentInfo, int flags,
                       const char *defaultS3HostName)
{
  memset(&opt, 0, sizeof(opt));
  if (defaultS3HostName != NULL)
  {
    opt.dirname = strdup(defaultS3HostName);
  }
  else
  {
    opt.dirname = "s3embedded";
  }
  WARNING("Using the S3embedded library with storage: %s\n", opt.dirname);

  struct stat sbuf;
  int ret = stat(opt.dirname, &sbuf);
  if (ret != 0)
  {
    ret = mkdir(opt.dirname, S_IRWXU | S_IRWXG);
    if (ret != 0 && errno != EEXIST)
    {
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
  SET_BUCKET_HASH_DIR(path, hostName, bucketName);
  int ret = mkdir(path, S_IRWXU | S_IRWXG);
  if (ret != 0 && errno != EEXIST)
  {
    WARNING("mkdir: %s %s\n", path, strerror(errno));
    status = S3StatusErrorBucketAlreadyExists;
  }
  else
  {
#ifdef S3_EXTRA_BUCKET
    // might be that the bucket has just been deleted. Possible race condition between create/delete bucket!
    SET_BUCKET_NAME(path, hostName, bucketName);
    int ret = mkdir(path, S_IRWXU | S_IRWXG);
    if (ret != 0 && errno != EEXIST)
    {
      WARNING("mkdir: %s %s\n", path, strerror(errno));
      status = S3StatusErrorBucketAlreadyExists;
    }
#endif
  }
  handler->completeCallback(status, &errs, callbackData);
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
  if (ret != 0)
  {
    WARNING("rmdir: %s %s\n", path, strerror(errno));
    status = S3StatusErrorAccessDenied;
    errs.message = strerror(errno);
  }
  else
  {
#ifdef S3_EXTRA_BUCKET
    SET_BUCKET_HASH_DIR(path, hostName, bucketName);
    ret = rmdir(path); // might be that the bucket is still in use. Possible race condition between create/delete bucket!
#endif
  }

  handler->completeCallback(status, &errs, callbackData);
}

/* This function does only approximate the buckets as we added a level of indirection! */
void S3_list_service(S3Protocol protocol, const char *accessKeyId,
                     const char *secretAccessKey, const char *securityToken,
                     const char *hostName, const char *authRegion,
                     S3RequestContext *requestContext,
                     int timeoutMs,
                     const S3ListServiceHandler *handler, void *callbackData)
{
  S3ErrorDetails errs = {0};
  S3Status status = S3StatusOK;
  struct dirent *d;
  char username[100];
  getlogin_r(username, 100);
  DIR *dir = opendir(hostName ? hostName : opt.dirname);
  if (dir)
  {
    while ((d = readdir(dir)) != NULL)
    {
      // skip dot files:
      if (d->d_name[0] == '.')
      {
        continue;
      }
      int creationDateSeconds = 0;
      handler->listServiceCallback(username, username, d->d_name, creationDateSeconds, callbackData);
    }
    closedir(dir);
  }
  else
  {
    WARNING("opendir: %s %s\n", hostName ? hostName : opt.dirname, strerror(errno));
    status = S3StatusErrorAccessDenied;
    errs.message = strerror(errno);
  }
  handler->responseHandler.completeCallback(status, &errs, callbackData);
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
  struct dirent *d;
  char path[PATH_MAX];
  SET_BUCKET_NAME(path, bc->hostName, bc->bucketName);
  char username[100];
  getlogin_r(username, 100);
  int prefix_len = prefix != NULL ? strlen(prefix) : 0;
  DIR *dir = opendir(path);
  if (dir)
  {
    S3ListBucketContent c = {0};
    c.ownerDisplayName = username;
    c.ownerId = username;
    while ((d = readdir(dir)) != NULL)
    {
      // skip dot files:
      if (d->d_name[0] == '.')
      {
        continue;
      }
      int cmp = strncmp(prefix, d->d_name, prefix_len);
      if ((prefix_len > 0 && cmp != 0) || (cmp == 0 && d->d_name[prefix_len] == 0))
      {
        // do not match the prefix itself
        continue;
      }
      c.key = d->d_name;
      handler->listBucketCallback(0, NULL, 1, &c, 0, NULL, callbackData);
    }
    closedir(dir);
  }
  else
  {
    WARNING("opendir: %s %s\n", path, strerror(errno));
    if (errno == EACCES)
    {
      struct stat sbuf;
      int ret = stat(path, &sbuf);
      if (ret == 0)
      {
        WARNING("STAT user: %d group: %d mode: %o\n", (int)sbuf.st_uid, (int)sbuf.st_gid, (int)sbuf.st_mode);
      }
    }
    status = S3StatusErrorAccessDenied;
    errs.message = strerror(errno);
  }
  handler->responseHandler.completeCallback(status, &errs, callbackData);
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
  S3Status status = S3StatusOK;
  char path[PATH_MAX];
  SET_BUCKET_NAME(path, hostName, bucketName);
  struct stat sbuf;
  int ret = stat(path, &sbuf);
  if (ret != 0)
  {
    status = S3StatusErrorNoSuchBucket;
  }

  handler->completeCallback(status, &errs, callbackData);
}

void S3_put_object(const S3BucketContext *bc, const char *key,
                   uint64_t contentLength,
                   const S3PutProperties *putProperties,
                   S3RequestContext *requestContext,
                   int timeoutMs,
                   const S3PutObjectHandler *handler, void *callbackData)
{
  S3ErrorDetails errs = {0};
  S3Status status = S3StatusOK;
  char path[PATH_MAX];
  SET_OBJECT_NAME(path, bc->hostName, bc->bucketName, key);
  FILE *fd = fopen(path, "w");
  if (fd != NULL)
  {
    char *buffer = malloc(BUFF_SIZE);
    int size = handler->putObjectDataCallback(BUFF_SIZE, buffer, callbackData);
    size_t written = 0;
    if (size > 0)
    {
      written = fwrite(buffer, 1, size, fd);
    }
    fclose(fd);
    free(buffer);
    if (written != size)
    {
      WARNING("write %d %s\n", size, strerror(errno));
      status = S3StatusErrorAccessDenied;
      errs.message = strerror(errno);
    }
  }
  else
  {
    WARNING("write %s", strerror(errno));
    status = S3StatusErrorAccessDenied;
    errs.message = strerror(errno);
  }
  handler->responseHandler.completeCallback(status, &errs, callbackData);
}

void S3_get_object(const S3BucketContext *bc, const char *key,
                   const S3GetConditions *getConditions,
                   uint64_t startByte, uint64_t byteCount,
                   S3RequestContext *requestContext,
                   int timeoutMs,
                   const S3GetObjectHandler *handler, void *callbackData)
{
  S3ErrorDetails errs = {0};
  S3Status status = S3StatusOK;
  char path[PATH_MAX];
  SET_OBJECT_NAME(path, bc->hostName, bc->bucketName, key);
  FILE *fd = fopen(path, "r");
  if (fd == NULL)
  {
    status = S3StatusErrorAccessDenied;
    errs.message = strerror(errno);
  }
  else
  {
    char *buffer = malloc(byteCount);
    if (startByte)
    {
      fseeko(fd, startByte, SEEK_SET);
    }
    size_t readb = fread(buffer, 1, byteCount, fd);
    if (readb == 0)
    {
      status = S3StatusErrorAccessDenied;
      errs.message = strerror(errno);
    }
    else
    {
      handler->getObjectDataCallback(readb, buffer, callbackData);
    }
    fclose(fd);
    free(buffer);
  }
  handler->responseHandler.completeCallback(status, &errs, callbackData);
}

void S3_head_object(const S3BucketContext *bc, const char *key,
                    S3RequestContext *requestContext,
                    int timeoutMs,
                    const S3ResponseHandler *handler, void *callbackData)
{
  S3ErrorDetails errs = {0};
  S3Status status = S3StatusOK;
  char path[PATH_MAX];
  SET_OBJECT_NAME(path, bc->hostName, bc->bucketName, key);
  struct stat sbuf;
  int ret = stat(path, &sbuf);
  if (ret != 0)
  {
    status = S3StatusErrorAccessDenied;
    errs.message = strerror(errno);
  }
  else
  {
    S3ResponseProperties p;
    p.contentLength = sbuf.st_size;
    p.lastModified = sbuf.st_mtime;
    handler->propertiesCallback(&p, callbackData);
  }

  handler->completeCallback(status, &errs, callbackData);
}

void S3_delete_object(const S3BucketContext *bc, const char *key,
                      S3RequestContext *requestContext,
                      int timeoutMs,
                      const S3ResponseHandler *handler, void *callbackData)
{
  S3ErrorDetails errs = {0};
  S3Status status = S3StatusOK;
  char path[PATH_MAX];
  SET_OBJECT_NAME(path, bc->hostName, bc->bucketName, key);
  int ret = unlink(path);
  if (ret != 0)
  {
    status = S3StatusErrorAccessDenied;
    errs.message = strerror(errno);
  }

  handler->completeCallback(status, &errs, callbackData);
}

// added to achieve compatibilty with libs3

int S3_status_is_retryable(S3Status status)
{
  return 0;
}

void S3_get_server_access_logging(const S3BucketContext *bucketContext,
                                  char *targetBucketReturn,
                                  char *targetPrefixReturn,
                                  int *aclGrantCountReturn,
                                  S3AclGrant *aclGrants,
                                  S3RequestContext *requestContext,
                                  int timeoutMs,
                                  const S3ResponseHandler *handler,
                                  void *callbackData)
{
}

void S3_get_lifecycle(const S3BucketContext *bucketContext,
                      char *lifecycleXmlDocumentReturn, int lifecycleXmlDocumentBufferSize,
                      S3RequestContext *requestContext,
                      int timeoutMs,
                      const S3ResponseHandler *handler, void *callbackData)
{
}

void S3_set_server_access_logging(const S3BucketContext *bucketContext,
                                  const char *targetBucket,
                                  const char *targetPrefix, int aclGrantCount,
                                  const S3AclGrant *aclGrants,
                                  S3RequestContext *requestContext,
                                  int timeoutMs,
                                  const S3ResponseHandler *handler,
                                  void *callbackData)
{
}

void S3_list_multipart_uploads(S3BucketContext *bucketContext,
                               const char *prefix, const char *keymarker,
                               const char *uploadidmarker,
                               const char *encodingtype, const char *delimiter,
                               int maxuploads, S3RequestContext *requestContext,
                               int timeoutMs,
                               const S3ListMultipartUploadsHandler *handler,
                               void *callbackData)
{
}

void S3_get_acl(const S3BucketContext *bucketContext, const char *key,
                char *ownerId, char *ownerDisplayName,
                int *aclGrantCountReturn, S3AclGrant *aclGrants,
                S3RequestContext *requestContext,
                int timeoutMs,
                const S3ResponseHandler *handler, void *callbackData)
{
}

S3Status S3_generate_authenticated_query_string(char *buffer, const S3BucketContext *bucketContext,
                                                const char *key, int expires, const char *resource,
                                                const char *httpMethod)
{
  return S3StatusOK;
}

S3Status S3_validate_bucket_name(const char *bucketName, S3UriStyle uriStyle)
{
  return S3StatusOK;
}

void S3_copy_object(const S3BucketContext *bucketContext, const char *key,
                    const char *destinationBucket, const char *destinationKey,
                    const S3PutProperties *putProperties,
                    int64_t *lastModifiedReturn, int eTagReturnSize,
                    char *eTagReturn, S3RequestContext *requestContext,
                    int timeoutMs,
                    const S3ResponseHandler *handler, void *callbackData)
{
  /* Use the put object with 0 length */
  S3_put_object(bucketContext, key,
                0, // No length => std. copy of < 5GB
                putProperties,
                requestContext,
                timeoutMs,
                handler, callbackData);
}

void S3_copy_object_range(const S3BucketContext *bucketContext,
                          const char *key, const char *destinationBucket,
                          const char *destinationKey,
                          const int partNo, const char *uploadId,
                          const unsigned long startOffset, const unsigned long count,
                          const S3PutProperties *putProperties,
                          int64_t *lastModifiedReturn, int eTagReturnSize,
                          char *eTagReturn, S3RequestContext *requestContext,
                          int timeoutMs,
                          const S3ResponseHandler *handler, void *callbackData)

{
  /* Use the put object with 0 length */
  S3_put_object(bucketContext, key,
                0, // No length => std. copy of < 5GB
                putProperties,
                requestContext,
                timeoutMs,
                handler, callbackData);
}

void S3_abort_multipart_upload(S3BucketContext *bucketContext, const char *key,
                               const char *uploadId,
                               int timeoutMs,
                               S3AbortMultipartUploadHandler *handler)
{
}

void S3_upload_part(S3BucketContext *bucketContext, const char *key,
                    S3PutProperties *putProperties,
                    S3PutObjectHandler *handler, int seq,
                    const char *upload_id, int partContentLength,
                    S3RequestContext *requestContext,
                    int timeoutMs,
                    void *callbackData)
{
}

void S3_set_lifecycle(const S3BucketContext *bucketContext,
                      const char *lifecycleXmlDocument,
                      S3RequestContext *requestContext,
                      int timeoutMs,
                      const S3ResponseHandler *handler, void *callbackData)
{
}

void S3_initiate_multipart(S3BucketContext *bucketContext, const char *key,
                           S3PutProperties *putProperties,
                           S3MultipartInitialHandler *handler,
                           S3RequestContext *requestContext,
                           int timeoutMs,
                           void *callbackData)
{
}

void S3_list_parts(S3BucketContext *bucketContext, const char *key,
                   const char *partnumbermarker, const char *uploadid,
                   const char *encodingtype, int maxparts,
                   S3RequestContext *requestContext,
                   int timeoutMs,
                   const S3ListPartsHandler *handler, void *callbackData)
{
}

void S3_complete_multipart_upload(S3BucketContext *bucketContext,
                                  const char *key,
                                  S3MultipartCommitHandler *handler,
                                  const char *upload_id, int contentLength,
                                  S3RequestContext *requestContext,
                                  int timeoutMs,
                                  void *callbackData)
{
}

void S3_set_acl(const S3BucketContext *bucketContext, const char *key,
                const char *ownerId, const char *ownerDisplayName,
                int aclGrantCount, const S3AclGrant *aclGrants,
                S3RequestContext *requestContext,
                int timeoutMs,
                const S3ResponseHandler *handler, void *callbackData)
{
}