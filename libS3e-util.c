#include <sys/socket.h>
#include <libS3-gw.h>

int rcv_data(int sockfd, int remain, void* p){
  //DEBUG("rcv: %d\n", remain);
  uint8_t * pos = (uint8_t*) p;
  ssize_t ret;
  while(remain > 0){
    ret = recv(sockfd, pos, remain, MSG_NOSIGNAL);
    if(ret <= 0){
      if(ret == 0 && errno == 0){
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

int snd_data(int sockfd, int remain, void const* p){
  //DEBUG("snd: %d\n", remain);
  uint8_t * pos = (uint8_t*) p;
  ssize_t ret;
  while(remain > 0){
    ret = send(sockfd, pos, remain, MSG_NOSIGNAL);
    if(ret <= 0){
      INFO("write error: %s\n", strerror(errno));
      return -1;
    }
    remain -= ret;
    pos += ret;
  }
  return 0;
}
