#ifndef SYLAR_ENDIAN_H_
#define SYLAR_ENDIAN_H_
// todo

#define SYLAR_LITTLE_ENDIAN 1  // 小端
#define SYLAR_BIG_ENDIAN 2     //大端

#include <byteswap.h>
#include <cstdint>
#include <type_traits>

namespace sylar {

template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type byteswap(T value) {
  return (T)bswap_64((uint64_t)value);
}

template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type byteswap(T value) {
  return (T)bswap_32((uint32_t)value);
}

template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type byteswap(T value) {
  return (T)bswap_16((uint16_t)value);
}

#if BYTE_ORDER == BIG_ENDIAN
#define SYLAR_BYTE_ORDER SYLAR_BIG_ENDIAN
#else
#define SYLAR_BYTE_ORDER SYLAR_LITTLE_ENDIAN
#endif

#if SYLAR_BYTE_ORDER == SYLAR_BIG_ENDIAN
template <class T>
T byteswapOnLittleEndian(T t) {
  return t;
}

template <class T>
T byteswapOnBigEndian(T t) {
  // todo
  return byteswap(t);
}

#else

template <class T>
T byteswapOnLittleEndian(T t) {
  return byteswap(t);
}

template <class T>
T byteswapOnBigEndian(T t) {
  return t;
}

#endif
}  // namespace sylar

#endif  // SYLAR_ENDIAN_H_