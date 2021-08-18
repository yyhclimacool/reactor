#include <glog/logging.h>
#include <vector>
#include <algorithm>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <iostream>

static void InitLogging(const char *procname) {
  //FLAGS_log_dir = "./logs";
  FLAGS_logbufsecs = 0;
  FLAGS_alsologtostderr = true;
  google::InitGoogleLogging(procname);
}

int main(int argc, char **argv) {
  (void) argc;

  signal(SIGPIPE, SIG_IGN);
  signal(SIGCHLD, SIG_IGN);

  InitLogging(argv[0]);

  int listenfd;
  if ((listenfd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP)) < 0) {
    LOG(FATAL) << "create listenfd faild.";
    return -1;
  }

  struct sockaddr_in servaddr;
  memset(&servaddr, 0x00, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(8000);
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

  int on = 1;
  if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
    LOG(FATAL) << "setsockopt for listenfd failed.";
    return -1;
  }
  if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) <0) {
    LOG(FATAL) << "call bind for listenfd failed.";
    return -1;
  }
  if (listen(listenfd, SOMAXCONN) < 0) {
    LOG(FATAL) << "call listen for listenfd failed.";
    return -1;
  } else {
    LOG(INFO) << "listening on localhost:8000";
  }

  std::vector<int> clients;
  int epollfd = epoll_create(EPOLL_CLOEXEC);

  struct epoll_event event;
  event.data.fd = listenfd;
  event.events = EPOLLIN;
  epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &event);

  std::vector<epoll_event> events(16);
  struct sockaddr_in peeraddr;
  socklen_t peerlen;

  int connfd;
  int nready;

  int idlefd = open("/dev/null", O_RDONLY | O_CLOEXEC);

  while(1) {
    // events是输入输出参数
    nready = epoll_wait(epollfd, events.data(), static_cast<int>(events.size()), 10 * 1000 /* ms */);
    if (nready == -1) {
      if (errno == EINTR) {
        continue;
      }
    }
    if (nready == 0) {
      LOG(INFO) << "poll nothing happened, continue to poll ...";
      continue;
    }
    if (nready == static_cast<int>(events.size())) {
      events.resize(events.size() * 2);
    }

    LOG(INFO) << "num fds in epoll[" << clients.size() << "]";
    LOG(INFO) << "epoll returned ready num of fds[" << nready << "]";

    for (int i = 0; i < nready; ++i) {
      if (events[i].data.fd == listenfd) {
        peerlen = sizeof(peeraddr);
        connfd = accept4(listenfd, (struct sockaddr *)&peeraddr, &peerlen, SOCK_NONBLOCK|SOCK_CLOEXEC);
        if (connfd == -1) {
          if (errno == EMFILE) {
            LOG(WARNING) << "out of fds in this process. close connection.";
            close(idlefd);
            idlefd = accept(listenfd, NULL, NULL);
            close(idlefd);
            idlefd = open("/dev/null", O_RDONLY | O_CLOEXEC);
            continue;
          } else {
            LOG(FATAL) << "accept failed.";
            return -1;
          }
        }
        LOG(INFO) << "accepted a new connection from[" << inet_ntoa(peeraddr.sin_addr) << ":" << ntohs(peeraddr.sin_port) << "]";
        clients.push_back(connfd);
        event.data.fd = connfd;
        event.events = EPOLLIN;
        epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &event);
      } else if (events[i].events & EPOLLIN) {
        connfd = events[i].data.fd;
        if (connfd < 0) continue;
        char buf[1024] = {0};
        int ret = read(connfd, buf, 1024);
        if (ret == -1) {
          LOG(FATAL) << "read from fd[" << connfd << "] failed.";
          continue;
        }
        if (ret == 0) {
          LOG(INFO) << "client[" << connfd << "] close connection, del from epoll";
          // remove from epollfd
          close(connfd);
          event = events[i];
          epoll_ctl(epollfd, EPOLL_CTL_DEL, connfd, &event);
          clients.erase(std::remove(clients.begin(), clients.end(), connfd), clients.end());
          continue;
        }

        LOG(INFO) << "message[" << buf << "], fromfd[" << connfd << "].";
        write(connfd, buf, strlen(buf));
      }
    }
  }
  google::ShutdownGoogleLogging();
}
