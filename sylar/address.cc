#include "address.h"
#include <cstring>
#include <sstream>

namespace sylar {

int Address::getFamily() const {
  // todo family
  return getAddr()->sa_family;
}
std::string Address::toString() {
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
IPv4Address::IPv4Address(uint32_t address, uint32_t port) {}

const sockaddr *IPv4Address::getAddr() const {}
socklen_t IPv4Address::getAddrLen() const {}
std::ostream &IPv4Address::insert(std::ostream &os) const {}

IPAddress::ptr IPv4Address::broadcastAddress(uint32_t prefix_len) {}
IPAddress::ptr IPv4Address::networdAddress(uint32_t prefix_len) {}
IPAddress::ptr IPv4Address::subnetMask(uint32_t prefix_len) {}
uint32_t IPv4Address::getPort() const {}
void IPv4Address::setPort(uint32_t v) {}

// IPv6
IPv6Address::IPv6Address(uint32_t address, uint32_t port) {}

const sockaddr *IPv6Address::getAddr() const {}
socklen_t IPv6Address::getAddrLen() const {}
std::ostream &IPv6Address::insert(std::ostream &os) const {}

IPAddress::ptr IPv6Address::broadcastAddress(uint32_t prefix_len) {}
IPAddress::ptr IPv6Address::networdAddress(uint32_t prefix_len) {}
IPAddress::ptr IPv6Address::subnetMask(uint32_t prefix_len) {}
uint32_t IPv6Address::getPort() const {}
void IPv6Address::setPort(uint32_t v) {}

// Unix
UnixAddress::UnixAddress(const std::string &path) {}

const sockaddr *UnixAddress::getAddr() const {}
socklen_t UnixAddress::getAddrLen() const {}
std::ostream &UnixAddress::insert(std::ostream &os) const {}

// unknown
UnknownAddress::UnknownAddress() {}
const sockaddr *UnknownAddress::getAddr() const {}
socklen_t UnknownAddress::getAddrLen() const {}
std::ostream &UnknownAddress::insert(std::ostream &os) const {}
}  // namespace sylar