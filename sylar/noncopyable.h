#ifndef SYLAR_NONCOPYABLE_H_
#define SYLAR_NONCOPYABLE_H_

namespace sylar {
class Noncopyable {
 public:
  Noncopyable() = default;
  ~Noncopyable() = default;
  Noncopyable(const Noncopyable &) = delete;
  Noncopyable &operator=(const Noncopyable &) = delete;
};
}  // namespace sylar

#endif  // SYLAR_NONCOPYABLE_H_