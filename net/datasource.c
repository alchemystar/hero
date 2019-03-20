#include "datasource.h"

// must use reference , or it's just a copy
Datasource* golbalDatasource;

// index for choose the reactor work poll
int create_backend_connection(Datasource* datasource,int index){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        printf("create socket failed,sockfd=%d, errno=%d, error:%s\n",sockfd, errno, strerror(errno));
        return FALSE;
    }
    // connect
    if(connect(sockfd,&(datasource->db_addr),sizeof(struct sockaddr)) < 0){
        printf("connect failed,sockfd=%d, errno=%d, error:%s\n",sockfd, errno, strerror(errno));
        goto error_process;
    }
    // init
    connection* conn = init_conn_and_mempool(sockfd,&datasource->db_addr,IS_BACK_CONN);
    conn->datasource = datasource;
    if(conn == NULL){
        close(sockfd);
        return FALSE;
    }
    Reactor* reactor = datasource->reactor;
    printf("connect okay\n");
    // 添加可读事件到reactor反应堆
    if(-1 == poll_add_event(reactor->worker_fd_arrays[index%reactor->worker_count],conn->sockfd,EPOLLIN,conn)){
        // 添加失败，则close掉连接
        release_conn_and_mempool(conn);
        return FALSE;
    }
    return TRUE;
error_process:
    release_conn_and_mempool(conn);
    return FALSE;
}

int init_datasource(Reactor* reactor) {
    mem_pool* mem_pool = mem_pool_create(DEFAULT_MEM_POOL_SIZE);
    Datasource* datasource = (Datasource*)mem_pool_alloc(sizeof(Datasource),mem_pool);
    // init datasource server addr
    datasource->db_addr.sin_family=AF_INET;
    datasource->db_addr.sin_addr.s_addr=inet_addr(BACKEND_INET_ADDR);;
    datasource->db_addr.sin_port=htons(BACKEND_SERVER_PORT);
    bzero(&(datasource->db_addr.sin_zero), 8);
    datasource->reactor = reactor;
    datasource->current_conn_pos = 0;
    datasource->conn_pool_size = DATASOUCE_LENGTH;
    datasource->conn_array = (connection**)mem_pool_alloc_ignore_check(sizeof(connection**) * DATASOUCE_LENGTH,mem_pool);
    golbalDatasource = datasource;
    printf("init pthread mutex\n");
    pthread_mutex_init(&(datasource->mutex),NULL);
    // create backend connection
    printf("init datasource\n");
    for(int i=0 ; i < DATASOUCE_LENGTH ; i++){
        printf("creating one backend connection,index=%d\n",i);
        if(FALSE == create_backend_connection(datasource,i)){
                printf("create backend connection error\n");
                goto error_process;
        }
    }
    return TRUE;
error_process:
    printf("create dataSource error\n");
    return FALSE;
}

int put_conn_to_datasource(connection* conn,Datasource* datasource){
    pthread_mutex_lock(&(datasource->mutex));
    int current_conn_pos = datasource->current_conn_pos;
    printf("current pos = %d\n",current_conn_pos);
    if(current_conn_pos >= datasource->conn_pool_size){
        pthread_mutex_unlock(&datasource->mutex);
        release_conn_and_mempool(conn);
        return TRUE;
    }else{
       datasource->conn_array[current_conn_pos] = conn;
       datasource->current_conn_pos++;
       pthread_mutex_unlock(&(datasource->mutex));
       printf("put one conn to datasource,conn_pos=%d\n",current_conn_pos); 
       printf("next_conn_pos=%d\n",datasource->current_conn_pos); 
    }
    return TRUE;
}

connection* get_conn_from_datasource(Datasource* datasource){
    connection* conn = NULL;
    pthread_mutex_lock(&datasource->mutex);
    printf("conn_pos=%d\n",datasource->current_conn_pos);
    printf("conn_pool_size=%d\n",datasource->conn_pool_size);
    printf("reactor count=%d\n",datasource->reactor->worker_count);
    if(datasource->current_conn_pos <= 0){
        pthread_mutex_unlock(&(datasource->mutex));
    }else{
        // 由于current_conn_pos表示的是待放入的
        datasource->current_conn_pos--;
        conn = datasource->conn_array[datasource->current_conn_pos];
        pthread_mutex_unlock(&(datasource->mutex));
        printf("got one conn from datasource,conn_pos=%d\n",datasource->current_conn_pos); 
    }
    return conn;
}

int write_query_command_to_back(connection* conn,char* sql){
     // 如果写入失败，要将后端conn删除，返回false后，再由前面删除
    if(!write_query_command(conn->write_buffer,sql,COM_QUERY)){
        release_conn_and_mempool(conn);
        return FALSE;
    }
    if(!write_nonblock(conn)){
        release_conn_and_mempool(conn);
        return FALSE;
    }
    return TRUE;
}

int write_query_command(packet_buffer*pb,char* sql,unsigned char* sql_type){
    int packet_size = 1 + strlen(sql);
    printf("write query command,packet_size=%d\n",packet_size);
    int packet_id = 0;
    if(!write_UB3(pb,packet_size)){
        return FALSE;    
    }
    if(!write_byte(pb,packet_id)){
        return FALSE;    
    }
    if(!write_byte(pb,sql_type)){
        return FALSE;    
    }
    if(!write_bytes(pb,sql,strlen(sql))){
        return FALSE;   
    }
    return TRUE; 
}

int default_execute(front_conn* front,char* sql,int is_selecting){
    connection* back_actual_conn = NULL;
    // pass to backend 
    printf("get conn from datasource\n");
    if(NULL == front->back){
        back_actual_conn = get_conn_from_datasource(golbalDatasource);
    }else{
        printf("front->back is not null\n");
        back_actual_conn = front->back->conn;
    }
    // 设置为正在进行select
    back_actual_conn->back->selecting = is_selecting;
    back_actual_conn->front = front;
    front->back = back_actual_conn->back;
    back_actual_conn->back->front = front;
    if(back_actual_conn == NULL){
        printf("get conn NULL\n");
        return FALSE;
    }else{
        printf("got one database conn\n");
    }
    return write_query_command_to_back(back_actual_conn,sql);    
}