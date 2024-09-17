/**
 * prealloc_ringbuffer.tpp
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

#ifndef _PREALLOC_RINGBUFFER_TPP
#define _PREALLOC_RINGBUFFER_TPP

#include <utility>

template<class T>
prealloc_ringbuffer<T>::prealloc_ringbuffer()
  : _data()
  , _start(0)
  , _end(0)
  , _size(0) {
}

template<class T>
prealloc_ringbuffer<T>::prealloc_ringbuffer(size_type size,
                                            const value_type& other)
  : _data(size,other)
  , _start(0)
  , _end(0)
  , _size(0) {
}

template<class T>
void prealloc_ringbuffer<T>::allocate(size_type size,
                                      const value_type& other) {
  _data.resize(size,other);
  _start = 0u;
  _end = 0u;
  _size = 0u;
}

template<class T>
void prealloc_ringbuffer<T>::pop_front() {

  if (_size>0u) {
    _start = (_start + 1u) % _data.size();
    --_size;
  }
  
}

template<class T>
void prealloc_ringbuffer<T>::push_back() {

  _end = (_end + 1u) % _data.size();
  
  if (_size == _data.size()) {
    pop_front();
  } else {
    ++_size;
  }
}

template<class T>
typename prealloc_ringbuffer<T>::value_type& prealloc_ringbuffer<T>::front() {
  return const_cast<value_type &>(std::as_const(*this).front());
}

template<class T>
const typename prealloc_ringbuffer<T>::value_type&
prealloc_ringbuffer<T>::front() const {
  return _data[_start];
}

template<class T>
typename prealloc_ringbuffer<T>::value_type& prealloc_ringbuffer<T>::back() {
  return const_cast<value_type &>(std::as_const(*this).back());
}

template<class T>
const typename prealloc_ringbuffer<T>::value_type&
prealloc_ringbuffer<T>::back() const {
  if (_end == 0) 
    return _data.back();
  else
    return _data[_end-1];  
}

template<class T>
typename prealloc_ringbuffer<T>::value_type&
prealloc_ringbuffer<T>::operator[](const std::size_t idx) {
  return const_cast<value_type &>(std::as_const(*this)[idx]);
}

template<class T>
const typename prealloc_ringbuffer<T>::value_type&
prealloc_ringbuffer<T>::operator[](const std::size_t idx) const {
  return _data[(idx+_start)%_data.size()];
}



#endif
