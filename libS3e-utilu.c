#include <sys/socket.h>
#include <fcntl.h>
#include <libS3-gwu.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int rcv_data(int sockfd, int remain, void *p)
{
  DEBUG("rcv_data: %d\n", remain);
  uint8_t *pos = (uint8_t *)p;
  ssize_t ret;
  char *hostaddrp;       /* dotted decimal host addr string */
  struct hostent *hostp; /* client host info */
  struct sockaddr_in servaddr, clientaddr;
  int clientlen = sizeof(clientaddr);
  // memset(&servaddr, 0, sizeof(servaddr));

  while (remain > 0)
  {
    // char udp_buffer[1024+1];
    // memset(udp_buffer, 0, 1024);
    ret = recv(sockfd, pos, remain, MSG_NOSIGNAL);
    // ret = recvfrom(sockfd, pos, remain, 0, (SA *)&clientaddr, &clientlen);

    // hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
    //                       sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    // if (hostp == NULL)
    //   FATAL("ERROR on gethostbyaddr");
    // hostaddrp = inet_ntoa(clientaddr.sin_addr);
    // if (hostaddrp == NULL)
    //   FATAL("ERROR on inet_ntoa\n");
    // INFO("rcv_data received datagram from %s (%s)\n",
    //      hostp->h_name, hostaddrp);
    // // // INFO("server received %d/%d bytes: %s\n", strlen(buf), n, buf);

    // getsockname(sockfd, (SA *)&clientaddr, &clientlen);

    // INFO("[%s:%u] > ", inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
    // /* client address                  client port      */

    if (ret <= 0)
    {
      if (ret == 0 && errno == 0)
      {
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

int snd_data(int sockfd, int remain, void const *p)
{
  DEBUG("snd: %d\n", remain);
  uint8_t *pos = (uint8_t *)p;
  ssize_t ret;
  while (remain > 0)
  {
    ret = send(sockfd, pos, remain, MSG_NOSIGNAL);
    // ret = sendto(sockfd, pos, remain, 0,(SA*) &cli, sizeof(cli));
    if (ret <= 0)
    {
      INFO("write error: %s\n", strerror(errno));
      return -1;
    }
    remain -= ret;
    pos += ret;
  }
  return 0;
}

int rcv_data_from_client(int sockfd, int remain, void *p)
{
  // DEBUG("rcv: %d\n", remain);
  uint8_t *pos = (uint8_t *)p;
  ssize_t ret;
  struct sockaddr_in clientaddr;
  clientaddr.sin_family = AF_INET;
  clientaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  // clientaddr.sin_port = htons(CLI_PORT);
  while (remain > 0)
  {
    ret = recvfrom(sockfd, pos, remain, 0, (SA *)&clientaddr, sizeof(clientaddr));
    if (ret <= 0)
    {
      if (ret == 0 && errno == 0)
      {
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

int rcv_data_from_server(int sockfd, int remain, void *p)
{
  // DEBUG("rcv: %d\n", remain);
  uint8_t *pos = (uint8_t *)p;
  ssize_t ret;
  struct sockaddr_in servaddr;
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  servaddr.sin_port = htons(SERV_PORT);
  while (remain > 0)
  {
    ret = recvfrom(sockfd, pos, remain, 0, (SA *)&servaddr, sizeof(servaddr));
    if (ret <= 0)
    {
      if (ret == 0 && errno == 0)
      {
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

int snd_data_to_client(int sockfd, int remain, void const *p)
{
  DEBUG("snd_data_to_client: %d\n", remain);
  uint8_t *pos = (uint8_t *)p;
  ssize_t ret;

  struct sockaddr_in clientaddr;
  clientaddr.sin_family = AF_INET;
  clientaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  clientaddr.sin_port = htons(CLI_PORT);

  while (remain > 0)
  {
    // ret = send(sockfd, pos, remain, MSG_NOSIGNAL);
    ret = sendto(sockfd, pos, remain, 0, (SA *)&clientaddr, sizeof(clientaddr));

    if (ret <= 0)
    {
      INFO("snd_data_to_client error: %s\n", strerror(errno));
      return -1;
    }
    remain -= ret;
    pos += ret;
  }
  return 0;
}

int snd_data_to_server(int sockfd, int remain, void const *p)
{
  DEBUG("snd_data_to_server: %d\n", remain);
  uint8_t *pos = (uint8_t *)p;
  ssize_t ret;

  struct sockaddr_in servaddr;
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  servaddr.sin_port = htons(SERV_PORT);

  while (remain > 0)
  {
    // ret = send(sockfd, pos, remain, MSG_NOSIGNAL);
    ret = sendto(sockfd, pos, remain, 0, (SA *)&servaddr, sizeof(servaddr));

    if (ret <= 0)
    {
      INFO("snd_data_to_server error: %s\n", strerror(errno));
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
  if (opts >= 0)
  {

    opts = (opts | O_NONBLOCK);
    if (fcntl(sockfd, F_SETFL, opts) >= 0)
    {
      return 0;
    }
  }

  return -1;
}