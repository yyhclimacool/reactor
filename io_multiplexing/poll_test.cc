#include <glog/logging.h>
#include <vector>
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

  struct pollfd pfd;
  pfd.fd = listenfd;
  pfd.events = POLLIN;
  std::vector<pollfd> pollfds;
  pollfds.push_back(pfd);
  int nready;
  struct sockaddr_in peeraddr;
  socklen_t peerlen;
  int connfd;

  int idlefd = open("/dev/null", O_RDONLY | O_CLOEXEC);

  while(1) {
    nready = poll(pollfds.data(), pollfds.size(), 10 * 1000 /* ms */);
    if (nready == -1) {
      if (errno == EMFILE) {
        LOG(WARNING) << "out of fds in this process. close connection.";
        close(idlefd);
        idlefd = accept(listenfd, NULL, NULL);
        close(idlefd);
        idlefd = open("/dev/null", O_RDONLY | O_CLOEXEC);
        continue;
      } else {
        LOG(FATAL) << "poll failed.";
        return -1;
      }
    }
    if (nready == 0) {
      LOG(INFO) << "poll nothing happened, continue to poll ...";
    }

    // new connection incomming
    if (pollfds[0].revents & POLLIN) {
      peerlen = sizeof(peeraddr);
      connfd = accept4(listenfd, (struct sockaddr *)&peeraddr, &peerlen, SOCK_NONBLOCK|SOCK_CLOEXEC);
      if (connfd == -1) LOG(WARNING) << "accept4 for listenfd failed.";
      pfd.fd = connfd;
      pfd.events = POLLIN;
      pfd.revents = 0;
      pollfds.push_back(pfd);
      --nready;
      LOG(INFO) << "accepted a new connection from[" << inet_ntoa(peeraddr.sin_addr) << ":" << ntohs(peeraddr.sin_port) << "]";
      if (nready == 0) continue;
    }

    LOG(INFO) << "num fds in poll[" << pollfds.size() << "]";
    LOG(INFO) << "poll returned ready num of fds[" << nready << "]";

    // iterator fds left
    for(auto it = pollfds.begin()+1; it != pollfds.end() && nready > 0; ++it) {
      if (it->revents & POLLIN) {
        --nready;
        connfd = it->fd;
        char buf[1024] = {0};
        int ret = read(connfd, buf, 1024);
        if (ret == -1) LOG(FATAL) << "read from connfd[" << connfd << "] failed.";

        // peer close
        if (ret == 0) {
          LOG(INFO) << "client close fd[" << connfd << "]";
          it = pollfds.erase(it);
          --it;
          close(connfd);
          continue;
        }
        LOG(INFO) << "fromfd[" << connfd << "], message[" << buf << "].";
        write(connfd, buf, strlen(buf));
      }
    }
  }
  google::ShutdownGoogleLogging();
}
