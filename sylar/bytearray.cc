#include "bytearray.h"
#include <bits/types/struct_iovec.h>
#include <string.h>
#include <fstream>
#include <iomanip>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include "endian.h"
#include "log.h"

namespace sylar {

static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

ByteArray::Node::Node(size_t s) : ptr(new char[s]), next(nullptr), size(s) {}
ByteArray::Node::Node() : ptr(nullptr), next(nullptr), size(0) {}
ByteArray::Node::~Node() {
  if (ptr) {
    delete[] ptr;
  }
}

ByteArray::ByteArray(size_t base_size)
    : m_baseSize(base_size),
      m_position(0),
      m_capacity(base_size),
      m_size(0),
      m_endian(SYLAR_BIG_ENDIAN),
      m_root(new Node(base_size)),
      m_cur(m_root) {}

ByteArray::~ByteArray() {
  Node *tmp = m_root;
  while (tmp) {
    m_cur = tmp;
    tmp = tmp->next;
    delete m_cur;
  }
}

bool ByteArray::isLittleEndian() const { return m_endian == SYLAR_LITTLE_ENDIAN; }
void ByteArray::SetIsLittleEndian(bool val) {
  if (val) {
    m_endian = SYLAR_LITTLE_ENDIAN;
  } else {
    m_endian = SYLAR_BIG_ENDIAN;
  }
}

// write
// 固定长度,才有字节序
void ByteArray::writeFint8(int8_t value) { write(&value, sizeof(value)); }

void ByteArray::writeFuint8(uint8_t value) { write(&value, sizeof(value)); }
void ByteArray::writeFint16(int16_t value) {
  if (m_endian != SYLAR_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}

void ByteArray::writeFuint16(uint16_t value) {
  if (m_endian != SYLAR_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}

void ByteArray::writeFint32(int32_t value) {
  if (m_endian != SYLAR_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}

void ByteArray::writeFuint32(uint32_t value) {
  if (m_endian != SYLAR_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}

void ByteArray::writeFint64(int64_t value) {
  if (m_endian != SYLAR_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}

void ByteArray::writeFuint64(uint64_t value) {
  if (m_endian != SYLAR_BYTE_ORDER) {
    value = byteswap(value);
  }
  write(&value, sizeof(value));
}

static uint32_t EncodeZigzag32(const int32_t &v) {
  if (v < 0) {
    // -1 变成1 ，1 就变成 2
    // todo  Zigzag
    return ((uint32_t)(-v)) * 2 - 1;
  } else {
    return v * 2;
  }
}

static uint64_t EncodeZigzag64(const int64_t &v) {
  if (v < 0) {
    // -1 变成1 ，1 就变成 2
    // todo  Zigzag
    return ((uint64_t)(-v)) * 2 - 1;
  } else {
    return v * 2;
  }
}

static int32_t DecodeZigzag32(const uint32_t &v) { return (v >> 1) ^ -(v & 1); }

static int64_t DecodeZigzag64(const uint64_t &v) { return (v >> 1) ^ -(v & 1); }
// 可变长度

void ByteArray::writeInt32(int32_t value) { writeUint32(EncodeZigzag32(value)); }
void ByteArray::writeUint32(uint32_t value) {
  uint8_t tmp[5];
  // todo  谷歌protobuf 压缩方式
  uint8_t i = 0;
  while (value >= 0x80) {
    tmp[i++] = (value & 0x7F) | 0x80;
    value >>= 7;
  }
  tmp[i++] = value;
  write(tmp, i);
}

void ByteArray::writeInt64(int64_t value) { writeUint64(EncodeZigzag64(value)); }

void ByteArray::writeUint64(uint64_t value) {
  uint8_t tmp[10];
  uint8_t i = 0;
  while (value >= 0x80) {
    tmp[i++] = (value & 0x7F) | 0x80;
    value >>= 7;
  }
  tmp[i++] = value;
  write(tmp, i);
}

void ByteArray::writeFloat(float value) {
  uint32_t v;
  memcpy(&v, &value, sizeof(value));
  writeFuint32(v);
}
void ByteArray::writeDouble(double value) {
  uint64_t v;
  memcpy(&v, &value, sizeof(value));
  writeFuint64(v);
}
// length:int16, data
void ByteArray::writeStringF16(const std::string &value) {
  writeFuint16(value.size());
  write(value.c_str(), value.size());
}
// length:int32, data
void ByteArray::writeStringF32(const std::string &value) {
  writeFuint32(value.size());
  write(value.c_str(), value.size());
}
// length:int64, data
void ByteArray::writeStringF64(const std::string &value) {
  writeFuint64(value.size());
  write(value.c_str(), value.size());
}
// length:varint, data
void ByteArray::writeStringVint(const std::string &value) {
  writeUint64(value.size());
  write(value.c_str(), value.size());
}
// data
void ByteArray::writeStringWithoutLength(const std::string &value) {
  write(value.c_str(), value.size());
}

// read
// todo tlv 编码
int8_t ByteArray::readFint8() {
  int8_t v;
  read(&v, sizeof(v));
  return v;
}
uint8_t ByteArray::readFuint8() {
  uint8_t v;
  read(&v, sizeof(v));
  return v;
}

#define XX(type)                      \
  type v;                             \
  read(&v, sizeof(v));                \
  if (m_endian == SYLAR_BYTE_ORDER) { \
    return v;                         \
  } else {                            \
    return byteswap(v);               \
  }

int16_t ByteArray::readFint16() { XX(int16_t); }
uint16_t ByteArray::readFuint16() { XX(uint16_t); }
int32_t ByteArray::readFint32() { XX(int32_t); }
uint32_t ByteArray::readFuint32() { XX(uint32_t); }
int64_t ByteArray::readFint64() { XX(int64_t); }
uint64_t ByteArray::readFuint64() { XX(uint64_t); }

#undef XX

int32_t ByteArray::readInt32() { return DecodeZigzag32(readUint32()); }

uint32_t ByteArray::readUint32() {
  uint32_t result = 0;
  for (int i = 0; i < 32; i += 7) {
    uint8_t b = readFuint8();
    if (b < 0x80) {
      result |= ((uint32_t)b) << i;
      break;
    } else {
      result |= (((uint32_t)(b & 0x7f)) << i);
    }
  }
  return result;
}

int64_t ByteArray::readInt64() { return DecodeZigzag64(readUint64()); }

uint64_t ByteArray::readUint64() {
  uint64_t result = 0;
  for (int i = 0; i < 64; i += 7) {
    uint8_t b = readFuint8();
    if (b < 0x80) {
      result |= ((uint64_t)b) << i;
      // bug
      break;
    } else {
      result |= (((uint64_t)(b & 0x7f)) << i);
    }
  }
  return result;
}

float ByteArray::readFloat() {
  uint32_t v = readFuint32();
  float value;
  memcpy(&value, &v, sizeof(v));
  return value;
}

double ByteArray::readDouble() {
  uint32_t v = readFuint64();
  float value;
  memcpy(&value, &v, sizeof(v));
  return value;
}

// length:int16, data
std::string ByteArray::readStringF16() {
  // 先读 uint整形，再度固定长度的string 放到string内
  uint16_t len = readFuint16();
  std::string buff;
  buff.resize(len);
  read(&buff[0], len);
  return buff;
}

// length:int32, data
std::string ByteArray::readStringF32() {
  uint32_t len = readFuint32();
  std::string buff(len, ' ');
  read(&buff[0], len);
  return buff;
}
// length:int64, data
std::string ByteArray::readStringF64() {
  uint64_t len = readFuint64();
  std::string buff;
  buff.resize(len);
  read(&buff[0], len);
  return buff;
}
// length:varint, data 和protobuf方案一样
std::string ByteArray::readStringVint() {
  uint64_t len = readFuint64();
  std::string buff;
  buff.resize(len);
  read(&buff[0], len);
  return buff;
}

// 内部操作
void ByteArray::clear() {
  m_position = m_size = 0;
  m_capacity = m_baseSize;
  Node *tmp = m_root->next;
  while (tmp) {
    m_cur = tmp;
    tmp = tmp->next;
    delete m_cur;
  }
  m_cur = m_root;
  m_root->next = NULL;
}

// void ByteArray::write(const void *buf, size_t size) {
//   if (size == 0) {
//     return;
//   }
//   addCapacity(size);
//   // 当前位置
//   size_t npos = m_position % m_baseSize;
//   // 当前容量
//   size_t ncap = m_cur->size - npos;
//   size_t bpos = 0;

//   while (size > 0) {
//     // 当前的容量可以容纳要写入的大小
//     if (ncap >= size) {
//       // todo bpos当前写入的偏移量
//       memcpy(m_cur->ptr + npos, (const char *)buf + bpos, size);
//       if (m_cur->size == (npos + size)) {
//         m_cur = m_cur->next;
//       }
//       // 写完后，当前位置往后移size大小
//       m_position += size;
//       // 记录偏移
//       bpos += size;
//       size = 0;
//     } else {
//       // 容不下
//       // 先把当前节点写满
//       memcpy(m_cur->ptr + npos, (const char *)buf + bpos, ncap);
//       m_position += ncap;
//       bpos += ncap;
//       // 还要写入的数量
//       size -= ncap;
//       // 开始下一个节点
//       m_cur = m_cur->next;
//       // 重置
//       ncap = m_cur->size;
//       ncap = 0;
//     }
//   }
//   // 位置比size大,m_size--容量(已经写入的)
//   if (m_position > m_size) {
//     m_size = m_position;
//   }
// }

void ByteArray::write(const void *buf, size_t size) {
  if (size == 0) {
    return;
  }
  addCapacity(size);

  size_t npos = m_position % m_baseSize;
  size_t ncap = m_cur->size - npos;
  size_t bpos = 0;

  while (size > 0) {
    if (ncap >= size) {
      memcpy(m_cur->ptr + npos, (const char *)buf + bpos, size);
      if (m_cur->size == (npos + size)) {
        m_cur = m_cur->next;
      }
      m_position += size;
      bpos += size;
      size = 0;
    } else {
      memcpy(m_cur->ptr + npos, (const char *)buf + bpos, ncap);
      m_position += ncap;
      bpos += ncap;
      size -= ncap;
      m_cur = m_cur->next;
      ncap = m_cur->size;
      npos = 0;
    }
  }

  if (m_position > m_size) {
    m_size = m_position;
  }
}
// todo
// void ByteArray::read(void *buf, size_t size) {
//   // 读取长度超出可以被读取的长度
//   if (size > getReadSize()) {
//     throw std::out_of_range("not enough len");
//   }

//   size_t npos = m_position % m_baseSize;
//   size_t ncap = m_cur->size - npos;
//   size_t bpos = 0;
//   while (size > 0) {
//     if (ncap >= size) {
//       memcpy((char *)buf + bpos, m_cur->ptr + npos, size);
//       if (m_cur->size == (npos + size)) {
//         m_cur = m_cur->next;
//       }
//       m_position += size;
//       bpos += size;
//       size = 0;
//     } else {
//       memcpy((char *)buf + bpos, m_cur->ptr + npos, ncap);
//       m_position += ncap;
//       bpos += ncap;
//       size -= ncap;
//       m_cur = m_cur->next;
//       ncap = m_cur->size;
//       npos = 0;
//     }
//   }
// }

void ByteArray::read(void *buf, size_t size) {
  if (size > getReadSize()) {
    throw std::out_of_range("not enough len");
  }

  size_t npos = m_position % m_baseSize;
  size_t ncap = m_cur->size - npos;
  size_t bpos = 0;
  while (size > 0) {
    if (ncap >= size) {
      memcpy((char *)buf + bpos, m_cur->ptr + npos, size);
      if (m_cur->size == (npos + size)) {
        m_cur = m_cur->next;
      }
      m_position += size;
      bpos += size;
      size = 0;
    } else {
      memcpy((char *)buf + bpos, m_cur->ptr + npos, ncap);
      m_position += ncap;
      bpos += ncap;
      size -= ncap;
      m_cur = m_cur->next;
      ncap = m_cur->size;
      npos = 0;
    }
  }
}

void ByteArray::read(void *buf, size_t size, size_t position) const {
  // 读取长度超出可以被读取的长度
  if (size > getReadSize()) {
    throw std::out_of_range("not enough len");
  }

  size_t npos = position % m_baseSize;
  size_t ncap = m_cur->size - npos;
  size_t bpos = 0;
  Node *cur = m_cur;
  while (size > 0) {
    if (ncap >= size) {
      memcpy((char *)buf + bpos, cur->ptr + npos, size);
      if (cur->size == (npos + size)) {
        cur = cur->next;
      }
      position += size;
      bpos += size;
      size = 0;
    } else {
      memcpy((char *)buf + bpos, cur->ptr + npos, ncap);
      position += ncap;
      bpos += ncap;
      size -= ncap;
      cur = cur->next;
      ncap = m_cur->size;
      npos = 0;
    }
  }
}

void ByteArray::setPosition(size_t v) {
  if (v > m_size) {
    throw std::out_of_range("set_position out of range");
  }
  m_position = v;
  if (m_position > m_size) {
    m_size = m_position;
  }
  m_cur = m_root;
  while (v > m_cur->size) {
    v -= m_cur->size;
    m_cur = m_cur->next;
  }
  if (v == m_cur->size) {
    m_cur = m_cur->next;
  }
}

bool ByteArray::writeToFile(const std::string &name) const {
  std::ofstream ofs;
  ofs.open(name, std::ios::trunc | std::ios::binary);
  // 打开失败
  if (!ofs) {
    SYLAR_LOG_ERROR(g_logger) << "WriteToFile name" << name << "error, error=" << errno
                              << strerror(errno);
    return false;
  }

  int64_t read_size = getReadSize();
  int64_t pos = m_position;
  Node *cur = m_cur;
  while (read_size > 0) {
    int diff = pos % m_baseSize;
    // 首先取出偏移位，计算长度
    // 如果读的长度比每个链表的长度还大，取链表长度，否则取读的长度
    int64_t len = (read_size > (int64_t)m_baseSize ? m_baseSize : read_size) - diff;

    ofs.write(cur->ptr + diff, len);
    cur = cur->next;
    pos += len;
    read_size -= len;
  }

  return true;
}
bool ByteArray::readFromFile(const std::string &name) {
  std::ifstream ifs;
  ifs.open(name, std::ios::binary);
  if (!ifs) {
    SYLAR_LOG_ERROR(g_logger) << "readToFile name" << name << "error, error=" << errno
                              << strerror(errno);
    return false;
  }

  std::shared_ptr<char> buff(new char[m_baseSize], [](char *ptr) { delete[] ptr; });
  while (!ifs.eof()) {
    ifs.read(buff.get(), m_baseSize);
    // ifs.gcount真正读到的长度
    write(buff.get(), ifs.gcount());
  }
  return true;
}

// 确保写入时有足够的size容量
void ByteArray::addCapacity(size_t size) {
  if (size == 0) {
    return;
  }
  // 边界的处理 todo  =

  size_t old_cap = getCapacity();
  if (old_cap >= size) {
    return;
  }
  // 额外增加容量
  size = size - old_cap;
  // 额外增加的是一个节点的多少倍，剩下处理的(不足一个节点的)，比老的大，要再加一个(old已经减去了)？？
  size_t count = (size / m_baseSize) + (((size % m_baseSize) > old_cap) ? 1 : 0);
  Node *tmp = m_root;
  // 找到为空的节点的上一个节点
  while (tmp->next) {
    tmp = tmp->next;
  }
  // m_cur = tmp; // add
  Node *first = NULL;
  for (size_t i = 0; i < count; ++i) {
    tmp->next = new Node(m_baseSize);
    if (first == NULL) {
      first = tmp->next;
    }
    tmp = tmp->next;
    m_capacity += m_baseSize;
  }
  // 如果原来节点容量为已满，那么起始节点就是first
  if (old_cap == 0) {
    m_cur = first;
  }
}

std::string ByteArray::toString() const {
  std::string str;
  str.resize(getReadSize());
  if (str.empty()) {
    return str;
  }
  read(&str[0], str.size(), m_position);
  return str;
}

// 16进制
std::string ByteArray::toHexString() const {
  std::string str = toString();
  std::stringstream ss;

  for (size_t i = 0; i < str.size(); ++i) {
    if (i > 0 && i % 32 == 0) {
      ss << std::endl;
    }
    // todo iomanip
    ss << std::setw(2) << std::setfill('0') << std::hex << (int)(uint8_t)str[i] << " ";
  }

  return ss.str();
}

uint64_t ByteArray::getReadBuffers(std::vector<iovec> &buffers, uint64_t len) const {
  len = len > getReadSize() ? getReadSize() : len;
  if (len == 0) {
    return 0;
  }

  uint64_t size = len;

  size_t npos = m_position % m_baseSize;
  size_t ncap = m_cur->size - npos;
  struct iovec iov;

  Node *cur = m_cur;
  while (len > 0) {
    if (ncap >= len) {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = len;
      len = 0;
    } else {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = ncap;
      len -= ncap;
      cur = cur->next;
      ncap = cur->size;
      npos = 0;
    }
    buffers.push_back(iov);
  }
  return size;
}
uint64_t ByteArray::getReadBuffers(std::vector<iovec> &buffers, uint64_t len,
                                   uint64_t position) const {
  len = len > getReadSize() ? getReadSize() : len;
  if (len == 0) {
    return 0;
  }

  uint64_t size = len;

  size_t npos = position % m_baseSize;

  size_t count = position / m_baseSize;
  Node *cur = m_root;
  while (count > 0) {
    cur = cur->next;
    --count;
  }
  size_t ncap = cur->size - npos;
  struct iovec iov;
  while (len > 0) {
    if (ncap >= len) {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = len;
      len = 0;
    } else {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = ncap;
      len -= ncap;
      cur = cur->next;
      ncap = cur->size;
      npos = 0;
    }
    buffers.push_back(iov);
  }
  return size;  //??
}
uint64_t ByteArray::getWriteBuffers(std::vector<iovec> &buffers, uint64_t len) {
  if (len == 0) {
    return 0;
  }
  addCapacity(len);
  uint64_t size = len;
  size_t npos = m_position % m_baseSize;
  size_t ncap = m_cur->size - npos;
  struct iovec iov;
  Node *cur = m_cur;
  while (len > 0) {
    if (ncap >= len) {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = len;
      len = 0;
    } else {
      iov.iov_base = cur->ptr + npos;
      iov.iov_len = ncap;

      len -= ncap;
      cur = cur->next;
      ncap = cur->size;
      npos = 0;
    }
    buffers.push_back(iov);
  }
  return size;
}

}  // namespace sylar