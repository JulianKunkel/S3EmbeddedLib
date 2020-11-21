#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <libS3-gw.h>

#define WARNING(...) printf("LIBS3R WARNING %s:%d ", __FILE__, __LINE__); printf(__VA_ARGS__);
#define INFO(...) printf("LIBS3R INFO %s:%d ", __FILE__, __LINE__); printf(__VA_ARGS__);
#define FATAL(...) do{ printf(__VA_ARGS__); exit(0); } while(0);

#define PORT 2020
#define SA struct sockaddr

typedef struct{
  int buffer_size_send;
  int buffer_size_rcv;
} options_t;

static options_t opt;

int rcv_data(int sockfd, int remain, uint8_t* pos){
  int ret;
  while(remain > 0){
    ret = read(sockfd, pos, remain);
    if(ret == 0){
      if(errno == 0){
        INFO("closed connection\n");
        return -1;
      }
      INFO("read error: %s\n", strerror(errno));
      return -1;
    }
    remain -= ret;
    pos += ret;
  }
  return 0;
}

static void * handle_connection(void * sockP){
  int sockfd = (int)(size_t) sockP;
  req_t * data = malloc(MAX_DATA_SIZE + sizeof(int));
  resp_t * rsp = malloc(MAX_DATA_SIZE + sizeof(int));
  while(1) {
      if(rcv_data(sockfd, sizeof(data->length), (uint8_t*) & data->length) != 0) break;
      INFO("Rcvd: %d\n", data->length);
      if(data->length > MAX_DATA_SIZE){
        INFO("Aborting connecting MSG size limit: %d\n", MAX_DATA_SIZE);
        goto cleanup;
      }
      if(rcv_data(sockfd, data->length, (uint8_t*) & data + sizeof(data->length)) != 0) break;

      // create response
      rsp->length = sizeof(uint32_t);

      // send response
      uint8_t * pos = (uint8_t*) & rsp;
      int remain = rsp->length;
      int ret;
      while(remain > 0){
        ret = write(sockfd, pos, remain);
        if(ret == 0){
          INFO("write error: %s\n", strerror(errno));
          goto cleanup;
        }
        remain -= ret;
        pos += ret;
      }
  }
cleanup:
  close(sockfd);
  free(data);
  free(rsp);
  return NULL;
}

int main(int argc, char ** argv){
    opt.buffer_size_send = 64*1024;
    opt.buffer_size_rcv = 64*1024;

    int sockfd;
    int ret;
    int connfd;
    socklen_t len;
    struct sockaddr_in servaddr, cli;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        FATAL("socket creation failed...\n");
    }
    memset(&servaddr, 0, sizeof(servaddr));

    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
    servaddr.sin_family = AF_INET;

    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) {
        FATAL("socket bind failed: %s\n", strerror(errno));
    }

    if ((listen(sockfd, 5)) != 0) {
        FATAL("Listen failed...\n");
    }
    len = sizeof(cli);

    printf("Startup complete\n");
    // Accept the data packet from client and verification
    while(1){
      connfd = accept(sockfd, (SA*) &cli, &len);
      if (connfd < 0) {
        FATAL("server acccept failed: %s\n", strerror(errno));
      }
      int optval = 1;
      socklen_t optlen = sizeof(optval);
      if(setsockopt(connfd, SOL_SOCKET, SO_KEEPALIVE, &optval, optlen) < 0) {
        FATAL("setsockopt()");
      }

      getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, & optval, & optlen);
      INFO("send buffer size = %d\n", optval);
      getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, & optval, & optlen);
      INFO("rcv buffer size = %d\n", optval);

      if(setsockopt(connfd, SOL_SOCKET, SO_SNDBUF, & opt.buffer_size_send, optlen) < 0){
        FATAL("setsockopt SO_SNDBUF");
      }
      if(setsockopt(connfd, SOL_SOCKET, SO_RCVBUF, & opt.buffer_size_rcv, optlen) < 0){
        FATAL("setsockopt SO_RCVBUF");
      }

      pthread_t * thread = malloc(sizeof(pthread_t));
      ret = pthread_create(thread, NULL, handle_connection, (void*)(size_t) connfd);
      if(ret != 0){
        WARNING("Couldn't create thread: %s\n", strerror(errno));
      }
    }

    // After chatting close the socket
    close(sockfd);
    return 0;
}
