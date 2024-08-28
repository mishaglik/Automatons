#ifndef CASTING_HPP
#define CASTING_HPP

#include <cassert>
#include <iostream>

namespace mgk {

template <class To, class From>
bool Isa(const From& obj) {
  return To::Classof(&obj);
}

template <class To, class From>
bool Isa(const From* obj) {
  return To::Classof(obj);
}

template <class To, class From>
To* Cast(From& obj) {
  assert(Isa<To>(obj));
  return static_cast<To*>(&obj);
}

template <class To, class From>
To* Cast(From* obj) {
  assert((Isa<To, From>(obj)));
  return static_cast<To*>(obj);
}

template <class To, class From>
const To* Cast(const From* obj) {
  assert((Isa<To, From>(obj)));
  return static_cast<const To*>(obj);
}

template <class To, class From>
const To* Cast(const From& obj) {
  assert((Isa<To, From>(obj)));
  return static_cast<const To*>(&obj);
}

}  // namespace mgk

#endif /* CASTING_HPP */
