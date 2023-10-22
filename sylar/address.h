#ifndef SYLAR_ADDRESS_H_
#define SYLAR_ADDRESS_H_

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <cstdint>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>
extern "C++" {

#include "endian.h"
}
namespace sylar {

class IPAddress;

class Address {
 public:
  typedef std::shared_ptr<Address> ptr;
  static Address::ptr Create(const sockaddr *address, socklen_t addrlen);
  static bool Lookup(std::vector<Address::ptr> &result, const std::string &host,
                     int family = AF_UNSPEC, int type = 0, int protocol = 0);

  static Address::ptr LookupAny(const std::string &host, int family = AF_UNSPEC, int type = 0,
                                int protocol = 0);

  static Address::ptr LookupAnyIPAddress(const std::string &host, int family = AF_UNSPEC,
                                         int type = 0, int protocol = 0);

  static bool GetInterfaceAddress(
      std::multimap<std::string, std::pair<Address::ptr, uint32_t>> &result,
      int family = AF_UNSPEC);

  static bool GetInterfaceAddress(std::vector<std::pair<Address::ptr, uint32_t>> &result,
                                  const std::string &iface, int family = AF_UNSPEC);

  virtual ~Address() {}

  int getFamily() const;

  virtual const sockaddr *getAddr() const = 0;
  virtual sockaddr *getAddr() = 0;
  virtual socklen_t getAddrLen() const = 0;

  virtual std::ostream &insert(std::ostream &os) const = 0;
  std::string toString() const;

  // stl 用来做排序比较 todo
  bool operator<(const Address &rhs) const;
  bool operator==(const Address &rhs) const;
  bool operator!=(const Address &rhs) const;

 private:
};

class IPAddress : public Address {
 public:
  typedef std::shared_ptr<IPAddress> ptr;

  static IPAddress::ptr Create(const char *address, uint16_t port = 0);

  virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;
  virtual IPAddress::ptr networdAddress(uint32_t prefix_len) = 0;
  virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

  virtual uint32_t getPort() const = 0;
  virtual void setPort(uint32_t v) = 0;
};

class IPv4Address : public IPAddress {
 public:
  using ptr = std::shared_ptr<IPv4Address>;

  static IPv4Address::ptr Create(const char *address, uint16_t port = 0);

  IPv4Address(const sockaddr_in address);
  IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

  const sockaddr *getAddr() const override;
  sockaddr *getAddr() override;
  socklen_t getAddrLen() const override;
  std::ostream &insert(std::ostream &os) const override;

  IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
  IPAddress::ptr networdAddress(uint32_t prefix_len) override;
  IPAddress::ptr subnetMask(uint32_t prefix_len) override;
  uint32_t getPort() const override;
  void setPort(uint32_t v) override;

 private:
  sockaddr_in m_addr;
};

class IPv6Address : public IPAddress {
 public:
  using ptr = std::shared_ptr<IPv6Address>;
  static IPv6Address::ptr Create(const char *address, uint16_t port = 0);

  IPv6Address();
  IPv6Address(const sockaddr_in6 address);
  IPv6Address(const uint8_t address[16], uint16_t port = 0);

  const sockaddr *getAddr() const override;
  sockaddr *getAddr() override;
  socklen_t getAddrLen() const override;
  std::ostream &insert(std::ostream &os) const override;

  IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
  IPAddress::ptr networdAddress(uint32_t prefix_len) override;
  IPAddress::ptr subnetMask(uint32_t prefix_len) override;
  uint32_t getPort() const override;
  void setPort(uint32_t v) override;

 private:
  sockaddr_in6 m_addr;
};

class UnixAddress : public Address {
 public:
  UnixAddress();
  using ptr = std::shared_ptr<UnixAddress>;
  UnixAddress(const std::string &path);

  const sockaddr *getAddr() const override;
  sockaddr *getAddr() override;
  socklen_t getAddrLen() const override;
  void setAddrlen(uint32_t v);
  std::ostream &insert(std::ostream &os) const override;

 private:
  sockaddr_un m_addr;
  socklen_t m_length;
};

class UnknownAddress : public Address {
 public:
  using ptr = std::shared_ptr<UnknownAddress>;

  UnknownAddress(int family);
  UnknownAddress(const sockaddr &addr);

  const sockaddr *getAddr() const override;
  sockaddr *getAddr() override;
  socklen_t getAddrLen() const override;
  std::ostream &insert(std::ostream &os) const override;

 private:
  sockaddr m_addr;
};
}  // namespace sylar
#endif  // SYLAR_ADDRESS_H_