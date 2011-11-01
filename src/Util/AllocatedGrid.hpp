/*
 * Copyright (C) 2010 Max Kellermann <max@duempel.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * FOUNDATION OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef XCSOAR_ALLOCATED_GRID_HPP
#define XCSOAR_ALLOCATED_GRID_HPP

#include "AllocatedArray.hpp"

#include <assert.h>

/**
 * A two dimensional array allocated on the heap with a length
 * determined at runtime.
 */
template<class T>
class AllocatedGrid {
protected:
  AllocatedArray<T> array;
  unsigned width, height;

public:
  typedef typename AllocatedArray<T>::iterator iterator;
  typedef typename AllocatedArray<T>::const_iterator const_iterator;

  gcc_constexpr_ctor AllocatedGrid():width(0), height(0) {}
  AllocatedGrid(unsigned _width, unsigned _height)
    :array(_width * _height), width(_width), height(_height) {}

  bool Defined() const {
    return array.size() > 0;
  }

  unsigned GetWidth() const {
    return width;
  }

  unsigned GetHeight() const {
    return height;
  }

  unsigned GetSize() const {
    return width * height;
  }

  const T &Get(unsigned x, unsigned y) const {
    assert(x < width);
    assert(y < height);

    return array[y * width + x];
  }

  T &Get(unsigned x, unsigned y) {
    assert(x < width);
    assert(y < height);

    return array[y * width + x];
  }

  const T &GetLinear(unsigned i) const {
    assert(i < GetSize());

    return array[i];
  }

  T &GetLinear(unsigned i) {
    assert(i < GetSize());

    return array[i];
  }

  iterator begin() {
    return array.begin();
  }

  const_iterator begin() const {
    return array.begin();
  }

  iterator end() {
    return begin() + width * height;
  }

  const_iterator end() const {
    return begin() + width * height;
  }

  const_iterator GetPointerAt(unsigned x, unsigned y) const {
    assert(x < width);
    assert(y < height);

    return begin() + y * width + x;
  }

  void Reset() {
    width = height = 0;
    array.ResizeDiscard(0);
  }

  void GrowDiscard(unsigned _width, unsigned _height) {
    array.GrowDiscard(_width * _height);
    width = _width;
    height = _height;
  }
};

#endif
