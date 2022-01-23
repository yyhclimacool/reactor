#pragma once

#include <vector>

namespace muduo {
// A Buffer class modeled after org.jboss.netty.buffer.ChannelBuffer and is copyable
// +-------------------+------------------+------------------+
// | prependable bytes |  readable bytes  |  writable bytes  |
// |                   |     (CONTENT)    |                  |
// +-------------------+------------------+------------------+
// |                   |                  |                  |
// 0      <=      readerIndex   <=   writerIndex    <=     size
class Buffer {
public:
  static const size_t kCheapPrepend = 8;
  static const size_t kInitialSize = 1024;

  explicit Buffer(size_t initialize_size = kInitialSize)
      : _buffer(kCheapPrepend + initial_size)
      , _reader_index(kCheapPrepend)
      , _writer_index(kCheapPrepend) {
    assert(readable_bytes() == 0); // 显然初始时刻，readable_bytes() == 0
    assert(writable_bytes() == initial_size);
    assert(prependable_bytes() == kCheapPrepend);
  }

  // NOTE: implicit copy-ctor, move-ctor, assignment, dtor are fine

  size_t readable_bytes() const { return _writer_index - _reader_index; }
  size_t writeable_bytes() const { return _buffer.size() - _writer_index; }
  size_t prependable_bytes() const { return _reader_index; } // 读指针前面的空间都是可以prepend的

  const char *peek() const { return begin() + _reader_index; } // _reader_index默认是从kCheapPrepend开始的

private:
  char       *begin() { return _buffer.data(); }
  const char *begin() const { return _buffer.data(); }

  void make_space(size_t len) {
    if (writable_bytes() + prependable_bytes() < len + kCheapPrepend) {
      // TODO: move readable data
      _buffer.resize(_writer_index + len);
    } else {
      // move readable data to the front, make more space inside
    }
  }

  std::vector<char> _buffer;
  size_t            _reader_index;
  size_t            _writer_index;
  static const char kCRLF[]; // TODO: 这个怎么理解？
};
} // namespace muduo