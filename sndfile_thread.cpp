/**
 * sndfile_thread.cpp
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

#include "sndfile_thread.h"

#include <sndfile.h>

#include <cassert>
#include <chrono>
#include <iostream>
#include <algorithm>



/******************************
 * sndfile_thread::file_block
 ******************************/

sndfile_thread::file_block::file_block()
  : status(Status::Garbage)
  , _end(nullptr) {}

sndfile_thread::file_block::~file_block() {
  status = Status::Garbage;
  _data.reset();
  _end=nullptr;
}

sndfile_thread::file_block::file_block(std::size_t size)
  : status(Status::Garbage)
  , _data(new float[size])
  , _end(_data.get()+size) {
  
  if (size==0) {
    _data.reset();
    _end=nullptr;
  }
  
}

sndfile_thread::file_block::file_block(const file_block& other) {
  this->operator=(other);
}

sndfile_thread::file_block&
sndfile_thread::file_block::operator=(const file_block& other) {
  
  if (this == &other) {
    return *this;
  }
  
  status = other.status;
  if (other.empty()) {
    _data.reset();
    _end=nullptr;
  } else {
    _data.reset(new float[other.size()]);
    _end=_data.get()+other.size();
    std::copy(other.begin(),other.end(),this->begin());
  }
  return *this;
}

sndfile_thread::file_block::file_block(file_block&& other) {
  this->operator=(other);
}

sndfile_thread::file_block&
sndfile_thread::file_block::operator=(file_block&& other) {
  if (this == &other) {
    return *this;
  }

  status = other.status;
  other.status = Status::Garbage;
  
  if (other.empty()) {
    _data.reset();
    _end=nullptr;
  } else {
    _data = std::move(other._data);
    _end=other._end;
    other._end=nullptr;
  }

  return *this;
}


void sndfile_thread::file_block::resize(size_t size) {
  status = Status::Garbage;
  _data.reset(new float[size]);
  _end=_data.get()+size;
}

/******************************
 * sndfile_thread
 ******************************/


sndfile_thread::sndfile_thread()
  : _block_size(0u)
  , _ringbuffer_size(0u)
  , _buffer()
  , _sampling_rate(0u)
  , _running(false)
  , _file_handler(nullptr)
  , _playing_file(false) {
}

                               

sndfile_thread::sndfile_thread(const std::size_t block_size,
                               const std::size_t sampling_rate,
                               const std::size_t buffer_size)
  : _block_size(block_size)
  , _ringbuffer_size(buffer_size)
  , _buffer(buffer_size,file_block(block_size))
  , _sampling_rate(sampling_rate)
  , _running(false)
  , _file_handler(nullptr)
  , _playing_file(false) {
}

sndfile_thread::~sndfile_thread() {
  _running=false;
  if (_thread.joinable()) {
    _thread.join();
  }
}

void sndfile_thread::init(const std::size_t block_size,
                          const std::size_t sampling_rate,
                          const std::size_t buffer_size) {
  if (!_playing_file) {
    _block_size = block_size;
    _ringbuffer_size = buffer_size;
    _buffer.allocate(buffer_size,file_block(block_size));
    _sampling_rate = sampling_rate;
    _running = false;
    _file_handler = nullptr;
    _playing_file = false;
  }
}

/**
 * Get pointer to next valid block.
 * This is called from jack's process method, so it must be non-blocking
 * and as fast as possible.  At the end of that process, the block should
 * be marked as Garbage, to signalize it can be reused.
 *
 * Return nullptr if no valid block available
 */  
sndfile_thread::file_block* sndfile_thread::next_block() {
  
  for (std::size_t i=0;i<_buffer.size(); ++i) {
    file_block& block = _buffer[i];
    if (block.status == Status::ReadyToPlay) {
      block.status = Status::Playing;
      return &block;
    }
  }

  return nullptr;
}

bool sndfile_thread::append_file(const std::filesystem::path& file) {
  if (std::filesystem::exists(file)) {

    std::lock_guard<std::mutex> lock(_playlist_mutex);
    _playlist.push_back(file);
    
    return true;
  }
  return false;
    
}

bool sndfile_thread::stop_files() {
  std::lock_guard<std::mutex> lock(_playlist_mutex);
  _playing_file=false;
  _playlist.clear();

  // Clear the ringbuffer
  while (!_buffer.empty()) {
    _buffer.pop_front();
  }
  
  return true;
}



void sndfile_thread::spawn() {
  if (!_running) {
    _thread = std::thread(&sndfile_thread::run,this);
  }
}

void sndfile_thread::check_files() {
  while (!_playing_file) {
    std::unique_lock<std::mutex> lock(_playlist_mutex);
    if (_playlist.empty()) {
      return;
    }
    else {
      std::filesystem::path file=_playlist.front();
      _playlist.pop_front();
      lock.unlock();
      
      // Try to open the file
      SF_INFO info;
      info.format = 0; // this has to be set to zero before calling sf_open
      _file_handler = sf_open(file.c_str(),SFM_READ,&info);
      
      if (_file_handler == 0) { // not zero if error
        std::cout << "Error opening file: '" << file << "'" << std::endl;
        continue;
      }
      
      _current_file_sample_rate = info.samplerate;
      _current_file_channels    = info.channels;

      _cache_size = (_block_size * _current_file_sample_rate +
                     _sampling_rate - 1)/_sampling_rate;
      
      _file_cache.resize(_current_file_channels * _cache_size);
      
      // File seems to work
      _playing_file=true;
    }
  }
}

void sndfile_thread::read_buffers() {
  if (_playing_file) {
    // Garbage collect
    while(!_buffer.empty() && (_buffer.front().status == Status::Garbage)) {
      _buffer.pop_front();
    }

    // Read as many new blocks as possible
    while(_playing_file && !_buffer.full()) {
      _buffer.push_back();
      read_block(_buffer.back());
    }
  }  
}

void sndfile_thread::read_block(file_block& block) {
  assert(_playing_file);

  if (_file_handler != nullptr) {
    float* mem = &_file_cache.front();
    
    // this reads the buffer from the file, and returns the read "frames"
    sf_count_t cnt = sf_readf_float(_file_handler,mem,_cache_size);

    if (std::size_t(cnt)<_cache_size) {
      // EOF reached?
      if (_file_handler != nullptr) {
        sf_close(_file_handler);
        _file_handler=nullptr;
        _playing_file=false;
      }
    }
    
    const float fstep=float(_current_file_sample_rate)/float(_sampling_rate);
    
    // how many of "our" samples does cnt equate to?
    std::size_t jack_samples =
      std::min(block.size(),cnt*_sampling_rate/_current_file_sample_rate);
    auto it = block.begin();
    const auto eit = it+jack_samples;
    
    // now, we want to fill with the data available
    for (float fidx=0.0f;it!=eit;++it,fidx+=fstep) {
      const auto idx=static_cast<std::size_t>(fidx)*_current_file_channels;
      const float* r=mem+idx;
      const float *const re=r+_current_file_channels;
      float acc=*r;
      for (++r;r!=re;++r) {
        acc+=*r; // add up all channels
      }; 
      *it=acc/_current_file_channels;
    }

    for (;it!=block.end();++it) {
      *it=0.0f;
    }
    
  }

  block.status = Status::ReadyToPlay;
}

void sndfile_thread::run() {
  /// Only one thread should be doing this
  if (_running) return;

  std::cout << "sndfile_thread running" << std::endl;
  
  _running = true;

  double us = 1e6*double(_block_size)/double(_sampling_rate);
  auto sleep_time = std::chrono::duration<double,std::micro>(us);
  
  while(_running) {
    check_files();
    read_buffers();

    std::this_thread::sleep_for(sleep_time);
  }

  std::cout << "sndfile_thread stopped" << std::endl;
  
}
