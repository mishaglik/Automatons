#ifndef CASTING_HPP
#define CASTING_HPP

#include <cassert>
#include <iostream>

namespace mgk {

template <class To, class From>
bool isa(const From& obj) {
  return To::classof(&obj);
}

template <class To, class From>
bool isa(const From* obj) {
  return To::classof(obj);
}

template <class To, class From>
To* cast(From& obj) {
  assert(isa<To>(obj));
  return static_cast<To*>(&obj);
}

template <class To, class From>
To* cast(From* obj) {
  assert((isa<To, From>(obj)));
  return static_cast<To*>(obj);
}

template <class To, class From>
const To* cast(const From* obj) {
  assert((isa<To, From>(obj)));
  return static_cast<const To*>(obj);
}

template <class To, class From>
const To* cast(const From& obj) {
  assert((isa<To, From>(obj)));
  return static_cast<const To*>(&obj);
}

}  // namespace mgk

#endif /* CASTING_HPP */
