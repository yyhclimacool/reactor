#include "tcpserver.h"

#include <glog/logging.h>

#include "acceptor.h"
#include "event_loop.h"
#include "inet_address.h"
#include "sockops.h"

using namespace muduo;

TcpServer::TcpServer(EventLoop *acceptor_loop, const InetAddress &listen_addr, const std::string &name)
    : _acceptor_loop(acceptor_loop)
    , _ip_port(listen_addr.to_ip_port())
    , _name(name)
    , _acceptor(new Acceptor(acceptor_loop, listen_addr)) {
  _acceptor->set_new_connection_callback(
    std::bind(&TcpServer::new_connection, this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer() {
  _acceptor_loop->assert_in_loop_thread();
  LOG(INFO) << "TcpServer::~TcpServer [" << _name << "] destructing";

  // TODO: 目前不需要做什么，Acceptor会自动析构
}

void TcpServer::start() {
  if (_started.fetch_sub(1) == 1) {
    assert(!_acceptor->listening());
    _acceptor_loop->run_in_loop(std::bind(&Acceptor::listen, _acceptor.get()));
  }
}

void TcpServer::new_connection(int connfd, const InetAddress &perr_addr) {
  _acceptor_loop->assert_in_loop_thread();
  ++_conn_id;
  char buf[ 64 ];
  snprintf(buf, sizeof buf, "-%s#%ld", _ip_port.c_str(), _conn_id);
  std::string conn_name = _name + buf;
  LOG(INFO) << "TcpServer::new_connection server_name[" << _name << "], conn_name[" << conn_name << "], connfd["
            << connfd << "], from[" << perr_addr.to_ip_port() << "]";
  InetAddress local_addr(sockets::get_local_addr(connfd));
  // TODO: create tcpconnectionptr instead of close
  sockets::close(connfd);
}