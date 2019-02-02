#include "net/server.h"

int main(int argc,char* argv[]){
  // 注册忽略信号hanlder
  init_signal_handlers();
  start_server();
}
