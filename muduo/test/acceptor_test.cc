#include "muduo/src/acceptor.h"

#include <glog/logging.h>

#include "muduo/src/event_loop.h"
#include "muduo/src/inet_address.h"
#include "muduo/src/sockops.h"

using namespace muduo;

void new_connection(int connfd, const InetAddress &addr) {
  LOG(INFO) << "accepted a new connection from[" << addr.to_ip_port() << "].";
  ::write(connfd, "How Are You?\n", 13);
  sockets::close(connfd);
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  LOG(INFO) << "main pid[" << getpid() << "].";
  InetAddress listenaddr(9981);
  EventLoop   loop;
  Acceptor    acceptor(&loop, listenaddr);
  acceptor.set_new_connection_callback(new_connection);
  acceptor.listen();
  loop.loop();
}
