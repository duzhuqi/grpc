/*
 *
 * Copyright 2017 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef GRPC_CORE_LIB_GPRPP_INLINED_VECTOR_H
#define GRPC_CORE_LIB_GPRPP_INLINED_VECTOR_H

#include <grpc/support/port_platform.h>

#include <cassert>

#include "src/core/lib/gprpp/memory.h"

namespace grpc_core {

// NOTE: We eventually want to use absl::InlinedVector here.  However,
// there are currently build problems that prevent us from using absl.
// In the interim, we define a custom implementation as a place-holder,
// with the intent to eventually replace this with the absl
// implementation.
//
// This place-holder implementation does not implement the full set of
// functionality from the absl version; it has just the methods that we
// currently happen to need in gRPC.  If additional functionality is
// needed before this gets replaced with the absl version, it can be
// added, with the following proviso:
//
// ANY METHOD ADDED HERE MUST COMPLY WITH THE INTERFACE IN THE absl
// IMPLEMENTATION!
//
// TODO(nnoble, roth): Replace this with absl::InlinedVector once we
// integrate absl into the gRPC build system in a usable way.
template <typename T, size_t N>
class InlinedVector {
 public:
  InlinedVector() { init_data(); }
  ~InlinedVector() { destroy_elements(); }

  // For now, we do not support copying.
  InlinedVector(const InlinedVector&) = delete;
  InlinedVector& operator=(const InlinedVector&) = delete;

  T* data() {
    return dynamic_ != nullptr ? dynamic_ : reinterpret_cast<T*>(inline_);
  }

  const T* data() const {
    return dynamic_ != nullptr ? dynamic_ : reinterpret_cast<const T*>(inline_);
  }

  T& operator[](size_t offset) {
    assert(offset < size_);
    return data()[offset];
  }

  const T& operator[](size_t offset) const {
    assert(offset < size_);
    return data()[offset];
  }

  void reserve(size_t capacity) {
    if (capacity > capacity_) {
      T* new_dynamic = static_cast<T*>(gpr_malloc(sizeof(T) * capacity));
      for (size_t i = 0; i < size_; ++i) {
        new (&new_dynamic[i]) T(std::move(data()[i]));
        data()[i].~T();
      }
      gpr_free(dynamic_);
      dynamic_ = new_dynamic;
      capacity_ = capacity;
    }
  }

  template <typename... Args>
  void emplace_back(Args&&... args) {
    if (size_ == capacity_) {
      reserve(capacity_ * 2);
    }
    new (&(data()[size_])) T(std::forward<Args>(args)...);
    ++size_;
  }

  void push_back(const T& value) { emplace_back(value); }

  void push_back(T&& value) { emplace_back(std::move(value)); }

  size_t size() const { return size_; }
  bool empty() const { return size_ == 0; }

  size_t capacity() const { return capacity_; }

  void clear() {
    destroy_elements();
    init_data();
  }

 private:
  void init_data() {
    dynamic_ = nullptr;
    size_ = 0;
    capacity_ = N;
  }

  void destroy_elements() {
    for (size_t i = 0; i < size_; ++i) {
      T& value = data()[i];
      value.~T();
    }
    gpr_free(dynamic_);
  }

  typename std::aligned_storage<sizeof(T)>::type inline_[N];
  T* dynamic_;
  size_t size_;
  size_t capacity_;
};

}  // namespace grpc_core

#endif /* GRPC_CORE_LIB_GPRPP_INLINED_VECTOR_H */
