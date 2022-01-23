#include "tcpserver.h"

#include <glog/logging.h>

#include "acceptor.h"
#include "event_loop.h"
#include "inet_address.h"
#include "sockops.h"

using namespace muduo;

TcpServer::TcpServer(EventLoop *acceptor_loop, const InetAddress &listen_addr, const std::string &name)
    : _acceptor_loop(acceptor_loop)
    , _acceptor(new Acceptor(acceptor_loop, listen_addr))
    , _ip_port(listen_addr.to_ip_port())
    , _name(name) {
  _acceptor->set_new_connection_callback(
    std::bind(&TcpServer::new_connection, this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer() {
  _acceptor_loop->assert_in_loop_thread();
  LOG(INFO) << "TcpServer::~TcpServer server_name[" << _name << "] destructing";

  for (auto &item : _connections) {
    auto conn = item.second; // 拷贝shared_ptr指针
    item.second.reset();     // 重置shared_ptr指针
    conn->get_loop()->run_in_loop(std::bind(&TcpConnection::connect_destroyed, conn));
  }
}

void TcpServer::start() {
  if (_started.fetch_sub(1) == 1) {
    _thread_pool->start(_thread_init_callback);
    assert(!_acceptor->listening());
    // TODO:为什么要在loop线程中运行呢？
    _acceptor_loop->run_in_loop(std::bind(&Acceptor::listen, _acceptor.get()));
  }
}

void TcpServer::new_connection(int connfd, const InetAddress &peer_addr) {
  // 由于new_connection函数是在Acceptor的loop线程中调用的，Accetptor中的loop对象和这里的this->_acceptor_loop是同一个
  _acceptor_loop->assert_in_loop_thread();
  EventLoop *next_loop = _thread_pool->get_next_loop();
  ++_conn_id;
  char buf[ 64 ];
  snprintf(buf, sizeof buf, "-%s#%ld", _ip_port.c_str(), _conn_id);
  std::string conn_name = _name + buf;
  LOG(INFO) << "TcpServer::new_connection server_name[" << _name << "], conn_name[" << conn_name << "], connfd["
            << connfd << "], from[" << perr_addr.to_ip_port() << "]";
  InetAddress local_addr(sockets::get_local_addr(connfd));
  // TODO: poll with zero timeout to double confirm the new connection
  // TODO: use make_shared if necessary
  auto conn = std::make_shared<TcpConnection>(next_loop, conn_name, connfd, local_addr, peer_addr);
  // 该函数是在loop线程中运行，不涉及多线程操作，避免了线程安全问题
  _connections[ conn_name ] = conn;
  conn->set_connection_callback(_connection_callback);
  conn->set_message_callback(_message_callback);
  conn->set_write_complete_callback(_write_complete_callback);
  conn->set_close_callback(std::bind(&TcpServer::remove_connection, this, std::placeholders::_1)); // FIXME: unsafe
  next_loop->run_in_loop(std::bind(&TcpConnection::connect_established, conn));
}

void TcpServer::remove_connection(const std::shared_ptr<TcpConnection> &conn) {
  // FIXME: unsafe
  _acceptor_loop->run_in_loop(std::bind(&TcpServer::remove_connection_in_loop, this, conn));
}

void TcpServer::remove_connection_in_loop(const std::shread_ptr<TcpConnection> &conn) {
  _acceptor_loop->assert_in_loop_thread();
  LOG(INFO) << "TcpServer::remove_connection_in_loop server_name[" << _name << "], conn_name[" << conn->name();
  // 也是因为在io线程中运行，所以是线程安全的
  size_t n = _connections.erase(conn->name());
  (void)n;
  assert(n == 1);
  auto *ioloop = conn->get_loop();
  ioloop->queue_in_loop(std::bind(&TcpConnection::connect_destroyed, conn));
}