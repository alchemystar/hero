#include "server.h"
#include <sys/socket.h>
#include <netinet/in.h>
// 先用bio搞一搞
// 后端调试成功之后再采用epoll
void start_server(){
    int sockfd_server;
    int sockfd;
    struct sockaddr_in sock_addr;
    char recv_buffer[RECV_MAX_BUFFER_SIZE];
    char send_buffer[SEND_MAX_BUFFER_SIZE];

    sockfd = socket(AF_INET,SOCK_STREAM,0);
}