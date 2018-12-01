#include "server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include "basic.h"
#include "network.h"

// 先用bio搞一搞
// 后端调试成功之后再采用epoll
void start_server(){
    // server fd
    int sockfd_server;
    // accept fd 
    int sockfd;
    int fd_temp;
    struct sockaddr_in sock_addr;
    struct sockaddr_in s_addr_client;

    sockfd_server = socket(AF_INET,SOCK_STREAM,0);
    // 设置重新利用PORT(ADDR),方便调试
    setsockopt(sockfd_server, SOL_SOCKET, SO_REUSEPORT, &(int){ 1 }, sizeof(int));
    memset(&sock_addr,0,sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    sock_addr.sin_port = htons(SERVER_PORT);
    // bind
    fd_temp=bind(sockfd_server,(struct sockaddr*)(&sock_addr),sizeof(sock_addr));
    if(fd_temp == - 1){
        fprintf(stdout,"bind error!\n");
        exit(1);
    }
    // listen
    fd_temp=listen(sockfd_server,MAX_BACK_LOG);
    if(fd_temp == -1){
        fprintf(stdout,"listen error!\n");
        exit(1);
    }
    // accept
    int client_length = sizeof(s_addr_client);
    while(1){
        printf("waiting for new connection...");
        sockfd = accept(sockfd_server,(struct sockaddr_ *)(&s_addr_client),(socklen_t *)&(client_length));
        if(sockfd == -1){
            printf("Accept error!\n");
            continue;
        }
        printf("A new connection occurs\n");
        process_connection(sockfd);
    }
}

void process_connection(int sockfd){
    mem_pool* pool = mem_pool_create(DEFAULT_MEM_POOL_SIZE);
    handle_one_connection(sockfd,pool);
    mem_pool_free(pool);
    // close(sockfd);
}
