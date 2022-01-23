#pragma once

#include <functional>

#include "timestamp.h"

namespace muduo {

class Buffer;
class TcpConnection;
using ConnectionCallback = std::function<void(const std::shared_ptr<TcpConnection> &)>;
// 数据已经被读入了(buf,len)
using MessageCallback = std::function<void(const std::shared_ptr<TcpConnection> &, Buffer *, Timestamp)>;
using WriteCompleteCallback = std::function<void(const std::shared_ptr<TcpConnection> &)>;
using HighWaterMarkCallback = std::function<void(const std::shared_ptr<TcpConnection> &, size_t)>;
using CloseCallback = std::function<void(const std::shared_ptr<TcpConnection> &)>;

void default_connection_callback(const std::shared_ptr<TcpConnection> &);
void default_message_callback(const std::shared_ptr<TcpConnection> &, Buffer *, Timestamp);

} // namespace muduo
