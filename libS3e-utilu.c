#include <sys/socket.h>
#include <fcntl.h>
#include <libS3-gw.h>

int rcv_data(int sockfd, int remain, void* p){
  DEBUG("rcv: %d\n", remain);
  uint8_t * pos = (uint8_t*) p;
  ssize_t ret;

  while(remain > 0){
    char udp_buffer[1024+1];
    memset(udp_buffer, 0, 1024);
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
  DEBUG("snd: %d\n", remain);
  uint8_t * pos = (uint8_t*) p;
  ssize_t ret;
  while(remain > 0){
        ret = send(sockfd, pos, remain, MSG_NOSIGNAL);
    // ret = sendto(sockfd, pos, remain, 0,(SA*)&servaddr, sizeof(servaddr));

    // ret = sendto(sockfd, pos, remain, MSG_NOSIGNAL);
        // sendto(sockfd, (const char *)hello, strlen(hello), 
        // MSG_CONFIRM, (const struct sockaddr *) &cliaddr,
        //     len);
    if(ret <= 0){
      INFO("write error: %s\n", strerror(errno));
      return -1;
    }
    remain -= ret;
    pos += ret;
  }
  return 0;
}


int set_nonblock_fd(int sockfd)
{
   int opts;
   
   opts = fcntl(sockfd, F_GETFL);
   if (opts >= 0) {
   
      opts = (opts | O_NONBLOCK);
      if (fcntl(sockfd, F_SETFL,opts) >= 0) {
         return 0;
      }
   }
   
   return -1;
}