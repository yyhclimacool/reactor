#include <glog/logging.h>

#include "muduo/src/current_thread.h"
#include "muduo/src/event_loop.h"
#include "muduo/src/inet_address.h"
#include "muduo/src/tcpserver.h"

using namespace muduo;

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  LOG(INFO) << "pid[" << getpid() << "], tid[" << tid() << "]";
  EventLoop   loop;
  InetAddress listen_addr(9981);
  TcpServer   server(&loop, listen_addr, "main_tcp_server");
  server.start();
  loop.loop();
}