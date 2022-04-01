#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <assert.h>

#include <libs3.h>
#include <libS3-gw.h>
#include <libS3e.h>

#include <fcntl.h>

typedef struct{
  char * dirname;
  int buffer_size_send;
  int buffer_size_rcv;
} options_t;

static options_t opt;


static int handle_request(int sockfd, req_t * req){
  resp_t rsp = {.length = 0, .status = S3StatusOK};

  DEBUG("Request: %d\n", req->length);
  char * data = malloc(req->length);
  int retval = 0;
  int ret;
  if(rcv_data(sockfd, req->length, data) != 0){
    rsp.status = S3StatusErrorEntityTooLarge;
    snd_data(sockfd, sizeof(rsp), & rsp);
    free(data);
    return -1;
  }

  DEBUG("op: %d path: %s\n", req->op, data);
  // verification: check pathname
  for(char * it = data ; *it != 0; it++){
    char c = *it;
    if(! ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '/' || c == '-' || c == '_')){
      rsp.status = S3StatusInvalidBucketNameCharacterSequence;
      if(snd_data(sockfd, HEADER_LENGTH,  & rsp) != 0){
        retval = -1;
      }
      goto cleanup;
    }
  }

  char path[PATH_MAX];
  SET_NAME(path, data);
  switch(req->op){
    case(REQ_GET_OBJECT):{
      uint64_t * pos = (uint64_t*) (data + strlen(data) + 1);
      uint64_t startByte = *pos;
      pos++;
      uint64_t byteCount = *pos;

      FILE * fd = fopen(path, "r");
      if(fd == NULL){
        rsp.status = S3StatusErrorAccessDenied;
        snd_data(sockfd, HEADER_LENGTH,  & rsp);
        break;
      }
      char * buffer = malloc(byteCount);
      if (startByte) {
        fseeko(fd, startByte, SEEK_SET);
      }
      size_t readb = fread(buffer, 1, byteCount, fd);
      if(readb == 0){
        rsp.status = S3StatusErrorAccessDenied;
        snd_data(sockfd, HEADER_LENGTH,  & rsp);
      }else{
        rsp.length = readb;
        if(snd_data(sockfd, HEADER_LENGTH,  & rsp) != 0){
          retval = -1;
        }else{
          snd_data(sockfd, readb,  buffer);
        }
      }
      fclose(fd);
      free(buffer);
      break;
    }case(REQ_PUT_OBJECT):{
      FILE * fd = fopen(path, "w");
      if(fd != NULL){
        int * pos = (int*) (data + strlen(data) + 1);
        int size = *pos;
        pos++;
        char * buffer = (char*) pos;
        size_t written = 0;
        if (size > 0){
          written = fwrite(buffer, 1, size, fd);
        }
        fclose(fd);
        if(written != size){
          rsp.status = S3StatusErrorAccessDenied;
        }
        snd_data(sockfd, HEADER_LENGTH,  & rsp);
        break;
      }
      rsp.status = S3StatusErrorAccessDenied;
      snd_data(sockfd, HEADER_LENGTH,  & rsp);
      break;
    }case(REQ_DELETE_OBJECT):{
      ret = unlink(path);
      if(ret != 0){
        DEBUG("Couldn't delete object: %s\n", path);
        rsp.status = S3StatusErrorNoSuchBucket;
      }
      if(snd_data(sockfd, HEADER_LENGTH,  & rsp) != 0){
        retval = -1;
        goto cleanup;
      }
      break;
    }case(REQ_HEAD_OBJECT):{
      struct stat sbuf;
      ret = stat(path, & sbuf);
      if(ret != 0){
        DEBUG("Couldn't stat object: %s\n", path);
        rsp.status = S3StatusErrorNoSuchBucket;
        if(snd_data(sockfd, HEADER_LENGTH, & rsp) != 0){
          retval = -1;
          goto cleanup;
        }
      }else{
        rsp.length = sizeof(sbuf.st_size) + sizeof(sbuf.st_mtime);
        assert(rsp.length == 16);
        if(snd_data(sockfd, HEADER_LENGTH, & rsp) != 0){
          retval = -1;
          goto cleanup;
        }
        if(snd_data(sockfd, sizeof(sbuf.st_size), & sbuf.st_size) != 0){
          retval = -1;
          goto cleanup;
        }
        if(snd_data(sockfd, sizeof(sbuf.st_mtime), & sbuf.st_mtime) != 0){
          retval = -1;
          goto cleanup;
        }
      }
      break;
    }case(REQ_TEST_BUCKET):{
      struct stat sbuf;
      ret = stat(path, & sbuf);
      if(ret != 0){
        DEBUG("Couldn't stat directory: %s\n", path);
        rsp.status = S3StatusErrorNoSuchBucket;
      }
      if(snd_data(sockfd, HEADER_LENGTH,  & rsp) != 0){
        retval = -1;
        goto cleanup;
      }
      break;
    }case(REQ_LIST_BUCKET):{
      char * prefix = data + strlen(data) + 1;
      int prefix_len = *(int*) prefix;
      prefix += sizeof(int);
      DIR * dir = opendir(path);
      struct dirent * d;
      if(dir){
        char * buff = malloc(MAX_DATA_SIZE);
        int pos = 0;
        while ((d = readdir(dir)) != NULL){
          // skip dot files:
          if(d->d_name[0] == '.') {
            continue;
          }
          if(prefix_len > 0){
            int cmp = strncmp(prefix, d->d_name, prefix_len);
            if(cmp != 0 || (cmp == 0 && d->d_name[prefix_len] == 0)){            // do not match the prefix itself
              continue;
            }
            DEBUG("%s=%s %d %c\n", d->d_name, prefix, cmp, d->d_name[prefix_len]);
          }
          if(pos + PATH_MAX > MAX_DATA_SIZE){
            FATAL("Too big directory content!");
          }
          pos += sprintf(& buff[pos], "%s", d->d_name) + 1;
        }
        closedir(dir);
        rsp.length = pos;
        if(snd_data(sockfd, HEADER_LENGTH,  & rsp) != 0){
          retval = -1;
          goto cleanup;
        }
        if(snd_data(sockfd, pos, buff) != 0){
          retval = -1;
          goto cleanup;
        }
        free(buff);
      }else{
        rsp.status = S3StatusErrorAccessDenied;
        if(snd_data(sockfd, HEADER_LENGTH,  & rsp) != 0){
          retval = -1;
          goto cleanup;
        }
      }
      break;
    }
    case(REQ_LIST_SERVICE):{
      struct dirent * d;
      DIR * dir = opendir(opt.dirname);
      if(dir){
        char * buff = malloc(MAX_DATA_SIZE);
        int pos = 0;
        while ((d = readdir(dir)) != NULL){
          // skip dot files:
          if(d->d_name[0] == '.') {
            continue;
          }
          if(pos + PATH_MAX > MAX_DATA_SIZE){
            FATAL("Too big directory content!");
          }
          pos += sprintf(& buff[pos], "%s", d->d_name) + 1;
        }
        closedir(dir);
        rsp.length = pos;
        if(snd_data(sockfd, HEADER_LENGTH,  & rsp) != 0){
          retval = -1;
          goto cleanup;
        }
        if(snd_data(sockfd, pos,  buff) != 0){
          retval = -1;
          goto cleanup;
        }
        free(buff);
      }else{
        rsp.status = S3StatusErrorAccessDenied;
        if(snd_data(sockfd, HEADER_LENGTH,  & rsp) != 0){
          retval = -1;
          goto cleanup;
        }
      }
      break;
    }case(REQ_CREATE_BUCKET):
      ret = mkdir(path, S_IRWXU);
      if(ret != 0){
        DEBUG("Couldn't create directory: %s\n", path);
        rsp.status = S3StatusErrorBucketAlreadyExists;
      }
      if(snd_data(sockfd, HEADER_LENGTH,  & rsp) != 0){
        retval = -1;
        goto cleanup;
      }
      break;
    case(REQ_DELETE_BUCKET):
      ret = rmdir(path);
      if(ret != 0){
        DEBUG("Couldn't delete directory: %s\n", path);
        rsp.status = S3StatusErrorAccessDenied;
      }
      if(snd_data(sockfd, HEADER_LENGTH,  & rsp) != 0){
        retval = -1;
        goto cleanup;
      }
      break;
    default:
      DEBUG("Unsupported operation: %d\n", req->op);
      rsp.status = S3StatusBadContentType;

      if(snd_data(sockfd, HEADER_LENGTH,  & rsp) != 0){
        retval = -1;
        goto cleanup;
      }
      break;
  }
cleanup:
  free(data);
  return retval;
}

// void setnonblocking(int sockfd) {
//     int flag = fcntl(sockfd, F_GETFL, 0);
//     if (flag < 0) {
// 		INFO("fcntl F_GETFL fail");
//         return;
//     }
//     if (fcntl(sockfd, F_SETFL, flag | O_NONBLOCK) < 0) {
// 		INFO("fcntl F_SETFL fail");
//     }
// }

static void * handle_connection(void * sockP){
  int sockfd = (int)(size_t) sockP;
  req_t data;
  while(1) {
      if(rcv_data(sockfd, HEADER_LENGTH,  & data) != 0) break;
      //INFO("Rcvd: %d\n", data.length);
      if(data.length > MAX_DATA_SIZE){
        INFO("Aborting connecting MSG size limit: %d\n", MAX_DATA_SIZE);
        goto cleanup;
      }
      if(handle_request(sockfd, & data) != 0) break;
  }
cleanup:
  close(sockfd);
  sockfd = -1;
  return NULL;
}

int main(int argc, char ** argv){
    opt.buffer_size_send = 64*1024;
    opt.buffer_size_rcv = 64*1024;
    if(argc != 2){
      printf("Synopsis: %s <DIRNAME>\n", argv[0]);
      exit(1);
    }
    opt.dirname = argv[1];

    struct stat sbuf;
    int ret = stat(opt.dirname, & sbuf);
    if(ret != 0){
     ret = mkdir(opt.dirname, S_IRWXU);
     if(ret != 0){
       FATAL("Couldn't create directory: %s\n", opt.dirname);
     }
    }

    int sockfd;
    int connfd;
    socklen_t len;
    struct sockaddr_in servaddr, cli;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        FATAL("socket creation failed...\n");
    }
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
    servaddr.sin_family = AF_INET;




    // setnonblocking(sockfd);

    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        FATAL("socket bind failed: %s\n", strerror(errno));
    }

    // if ((listen(sockfd, 5)) != 0) {
    //     FATAL("Listen failed...\n");
    // }
    len = sizeof(cli);

    printf("Startup complete\n");
    // Accept the data packet from client and verification
    while(1){
      // connfd = accept(sockfd, (SA*) &cli, &len);
      // if (connfd < 0) {
      //   FATAL("server acccept failed: %s\n", strerror(errno));
      // }
      int optval = 1;
      socklen_t optlen = sizeof(optval);
      if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, optlen) < 0) {
        FATAL("setsockopt SO_REUSEPORT");
      }
      if(setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, & opt.buffer_size_send, optlen) < 0){
        FATAL("setsockopt SO_SNDBUF");
      }
      if(setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, & opt.buffer_size_rcv, optlen) < 0){
        FATAL("setsockopt SO_RCVBUF");
      }

      getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, & optval, & optlen);
      INFO("rcv buffer size = %d\n", optval);

      pthread_t * thread = malloc(sizeof(pthread_t));
      ret = pthread_create(thread, NULL, handle_connection, (void*)(size_t) sockfd);
      if(ret != 0){
        WARNING("Couldn't create thread: %s\n", strerror(errno));
      }
    }

    // After chatting close the socket
    close(sockfd);
    return 0;
}
