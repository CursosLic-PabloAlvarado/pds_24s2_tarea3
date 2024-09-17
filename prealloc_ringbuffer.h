/**
 * prealloc_ringbuffer.h
 *
 * Copyright (C) 2023-2024  Pablo Alvarado
 * EL5805 Procesamiento Digital de Señales
 * Escuela de Ingeniería Electrónica
 * Tecnológico de Costa Rica
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the authors nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _PREALLOC_RINGBUFFER_H
#define _PREALLOC_RINGBUFFER_H

#include <vector>

/**
 * Simple ring-buffer that preallocates all of its contents and never
 * adds or deletes new ones, unless expressely told to do so.
 *
 * The idea is to encapsulate some pointer control to give the feeling
 * of a ring buffer, but the elements are never allocated or
 * deallocated, unless expressly resizing them
 */
template<class T>
class prealloc_ringbuffer {
public:
  typedef T value_type;
  typedef value_type&& rvalue_type;
  typedef typename std::vector<T>::size_type size_type;

  /// Create an empty ringbuffer
  prealloc_ringbuffer();

  /// Create an empty ringbuffer with the given capacity
  prealloc_ringbuffer(size_type size,const value_type& other);

  /// Discard current data and reinitialize the ringbuffer
  void allocate(size_type size,const value_type& other);
  
  void pop_front();
  void push_back();

  value_type& front();
  const value_type& front() const;

  value_type& back();
  const value_type& back() const;

  value_type& operator[](const std::size_t idx);
  const value_type& operator[](const std::size_t idx) const;

  inline size_type size() const {return _size;}
  inline bool empty() const {return _size==0;}
  inline bool full() const {return _size==_data.size();}
  
protected:
  std::vector<T> _data;
  
  std::size_t _start;
  std::size_t _end;

  std::size_t _size;
  
};

#include "prealloc_ringbuffer.tpp"

#endif
