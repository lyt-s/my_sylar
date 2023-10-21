#include "address.h"

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include "log.h"

#include "endian.hpp"
namespace sylar {
static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

template <class T>
static T CreateMask(uint32_t bits) {
  return (1 << (sizeof(T) * 8 - bits)) - 1;
}

int Address::getFamily() const {
  // todo family
  return getAddr()->sa_family;
}
std::string Address::toString() const {
  std::stringstream ss;
  insert(ss);
  return ss.str();
}

// stl 用来做排序比较 todo
bool Address::operator<(const Address &rhs) const {
  socklen_t minlen = std::min(getAddrLen(), rhs.getAddrLen());
  // 先比较最小长度 todo memcmp
  int result = memcmp(getAddr(), rhs.getAddr(), minlen);
  if (result < 0) {
    return true;
  } else if (result > 0) {
    return false;
  }
  // 相等比长度
  else if (getAddrLen() < rhs.getAddrLen()) {
    return true;
  }
  return false;
}

bool Address::operator==(const Address &rhs) const {
  return getAddrLen() == rhs.getAddrLen() && memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
}

bool Address::operator!=(const Address &rhs) const { return !(*this == rhs); }

// IPv4
IPv4Address::IPv4Address(const sockaddr_in address) { m_addr = address; }
IPv4Address::IPv4Address(uint32_t address, uint32_t port) {
  // todo
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sin_family = AF_INET;
  m_addr.sin_port = byteswapOnLittleEndian(port);  //转成网络字节序
  m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
}

const sockaddr *IPv4Address::getAddr() const { return (sockaddr *)&m_addr; }
socklen_t IPv4Address::getAddrLen() const { return sizeof(m_addr); }

std::ostream &IPv4Address::insert(std::ostream &os) const {
  uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
  // todo
  os << ((addr >> 24) & 0xff) << "." << ((addr >> 16) & 0xff) << "." << ((addr >> 8) & 0xff) << "."
     << (addr & 0xff) << ".";

  os << ": " << byteswapOnLittleEndian(m_addr.sin_port);
  return os;
}

IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) {
  if (prefix_len > 32) {
    return nullptr;
  }

  sockaddr_in baddr(m_addr);
  baddr.sin_addr.s_addr |= byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
  // todo
  return std::make_shared<IPv4Address>(baddr);
}
IPAddress::ptr IPv4Address::networdAddress(uint32_t prefix_len) {
  if (prefix_len > 32) {
    return nullptr;
  }

  sockaddr_in baddr(m_addr);
  baddr.sin_addr.s_addr &= byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
  // todo
  return IPv4Address::ptr(new IPv4Address(baddr));
}

IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len) {
  sockaddr_in subnet;
  memset(&subnet, 0, sizeof(subnet));
  subnet.sin_family = AF_INET;
  subnet.sin_addr.s_addr = ~byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
  return IPv4Address::ptr(new IPv4Address(subnet));
}

uint32_t IPv4Address::getPort() const { return byteswapOnLittleEndian(m_addr.sin_port); }
void IPv4Address::setPort(uint32_t v) { m_addr.sin_port = byteswapOnLittleEndian(v); }

// IPv6
IPv6Address::IPv6Address() {
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sin6_family = AF_INET6;
}

IPv6Address::IPv6Address(const sockaddr_in6 address) { m_addr = address; }

IPv6Address::IPv6Address(const char *address, uint32_t port) {
  // todo
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sin6_family = AF_INET6;
  m_addr.sin6_port = byteswapOnLittleEndian(port);  //转成网络字节序
  memcpy(&m_addr.sin6_addr, address, 16);
}

const sockaddr *IPv6Address::getAddr() const { return (sockaddr *)&m_addr; }
socklen_t IPv6Address::getAddrLen() const { return sizeof(m_addr); }
std::ostream &IPv6Address::insert(std::ostream &os) const {
  os << "[";
  uint16_t *addr = (uint16_t *)m_addr.sin6_addr.s6_addr;
  bool used_zeros = false;
  for (size_t i = 0; i < 8; ++i) {
    if (addr[i] == 0 && !used_zeros) {
      continue;
    }
    if (i && addr[i - 1] == 0 && !used_zeros) {
      os << ":";
      used_zeros = true;
    }
    if (i) {
      os << ":";
    }
    // ipv6 格式 todo
    os << std::hex << (int)byteswapOnLittleEndian(addr[i]) << std::dec;
  }
  if (used_zeros && addr[7] == 0) {
    os << "::";
  }
  os << "]:" << byteswapOnLittleEndian(m_addr.sin6_port);
  return os;
}

IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) {
  sockaddr_in6 baddr(m_addr);
  baddr.sin6_addr.s6_addr[prefix_len / 8] |= CreateMask<uint8_t>(prefix_len % 8);
  for (int i = prefix_len / 8 + 1; i < 16; ++i) {
    baddr.sin6_addr.s6_addr[i] = 0xff;
  }
  //   todo
  return std::make_shared<IPv6Address>(baddr);
}
IPAddress::ptr IPv6Address::networdAddress(uint32_t prefix_len) {
  sockaddr_in6 baddr(m_addr);
  // todo
  baddr.sin6_addr.s6_addr[prefix_len / 8] &= CreateMask<uint8_t>(prefix_len % 8);
  //   for (int i = prefix_len / 8 + 1; i < 16; ++i) {
  //     baddr.sin6_addr.s6_addr[i] = 0xff;
  //   }

  //   todo
  return std::make_shared<IPv6Address>(baddr);
}
IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len) {
  sockaddr_in6 subnet(m_addr);
  memset(&subnet, 0, sizeof(subnet));
  subnet.sin6_family = AF_INET6;
  subnet.sin6_addr.s6_addr[prefix_len / 8] = ~CreateMask<uint8_t>(prefix_len % 8);

  for (uint32_t i = 0; i < prefix_len / 8; ++i) {
    subnet.sin6_addr.s6_addr[i] = 0xFF;
  }
  return IPv6Address::ptr(new IPv6Address(subnet));
}

uint32_t IPv6Address::getPort() const { return (int)byteswapOnLittleEndian(m_addr.sin6_port); }
void IPv6Address::setPort(uint32_t v) { m_addr.sin6_port = byteswapOnLittleEndian(v); }

// Unix
// -1??
static const size_t MAX_PATH_LEN = sizeof(((sockaddr_un *)0)->sun_path) - 1;

UnixAddress::UnixAddress() {
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sun_family = AF_UNIX;
  m_length = offsetof(sockaddr_un, sun_path) + MAX_PATH_LEN;
}
UnixAddress::UnixAddress(const std::string &path) {
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sun_family = AF_UNIX;
  m_length = path.size() + 1;

  if (!path.empty() && path[0] == '\0') {
    --m_length;
  }
  if (m_length > sizeof(m_addr.sun_path)) {
    throw std::logic_error("path too long");
  }
  memcpy(m_addr.sun_path, path.c_str(), m_length);
  m_length += offsetof(sockaddr_un, sun_path);
}

const sockaddr *UnixAddress::getAddr() const { return (sockaddr *)&m_addr; }
socklen_t UnixAddress::getAddrLen() const { return m_length; }
std::ostream &UnixAddress::insert(std::ostream &os) const {
  if (m_length > offsetof(sockaddr_un, sun_path) && m_addr.sun_path[0] == '\0') {
    return os << "\\0"
              << std::string(m_addr.sun_path + 1, m_length - offsetof(sockaddr_un, sun_path) - 1);
  }
  return os << m_addr.sun_path;
}

// unknown
UnknownAddress::UnknownAddress(int family) {
  memset(&m_addr, 0, sizeof(m_addr));
  m_addr.sa_family = family;
}
const sockaddr *UnknownAddress::getAddr() const { return &m_addr; }
socklen_t UnknownAddress::getAddrLen() const { return sizeof(m_addr); }
std::ostream &UnknownAddress::insert(std::ostream &os) const {
  os << "[unknownAddress faily=]" << m_addr.sa_family << "]";
  return os;
}
}  // namespace sylar