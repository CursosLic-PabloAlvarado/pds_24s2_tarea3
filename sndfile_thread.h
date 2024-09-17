/**
 * sndfile_thread.h
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

#ifndef _SNDFILE_THREAD_H
#define _SNDFILE_THREAD_H

#include <cstddef>
#include <thread>
#include <filesystem>
#include <mutex>
#include <list>
#include <optional>

#include <sndfile.h>


#include "prealloc_ringbuffer.h"

/**
 * This is a worker class in the middle, to read audio files and
 * provide jack with the data without blocking it with file I/O operations.
 *
 * It uses a ringbuffer to cache some blocks as fast as it can,
 * up to a given number of blocks, set in the constructor.  Then,
 * jack's process asks for a new block with "next()".  If there are
 * blocks available, that method returns a pointer to a block holding
 * the data.  If no blocks are yet available, then a nullptr is
 * returned.  The main thread can add as many files as it wants with
 * the "add_file()" method.
 */
class sndfile_thread {
public:
  /// Status of each block in the buffer
  enum class Status {
    ReadyToPlay,
    Playing,
    Garbage
  };

  /**
   * A file_block holds a status indicator and the proper data
   *
   * It manages the data, and exposes them so that the sndfile library
   * can access it directly.  
   */
  class file_block {
  public:
    /// Constructs an empty block, with "Garbage" status
    file_block();
    /// Releases all allocated memory
    ~file_block();

    /// Creates a block, holding a data array with size 
    file_block(std::size_t size);
    /// Construct a new instance with a copy of the other block
    file_block(const file_block& other);
    /// Moves the content of the other instance to this one
    file_block(file_block&& other);
    /// Copies the other content into this instance
    file_block& operator=(const file_block& other);
    /// Move assignment
    file_block& operator=(file_block&& other);
    
    Status status;
    inline float& front() {return *_data.get();}
    inline const float& front() const {return *_data.get();}
    inline size_t size() const {return _end-_data.get();}
    inline float* begin() {return &front();}
    inline const float* begin() const {return &front();}
    inline float* end() {return _end;}
    inline const float* end() const {return _end;}
    inline bool empty() const {return _end==nullptr;}

    void resize(size_t size);
    
  private:
    
    std::unique_ptr<float[]> _data;
    float* _end;
  };

  // Inactive thread creation
  sndfile_thread();
  
  /**
   * Construct the thread object
   * @param block_size size of each data block to be read from file.  
   *                   It should be equal to jack's buffer size, used in
   *                   process
   */
  sndfile_thread(const std::size_t block_size,
                 const std::size_t sampling_rate,
                 const std::size_t buffer_size=10);

  ~sndfile_thread();

  /**
   * Initialize the thread.
   *
   * This must be called once, before the thread is running
   */
  void init(const std::size_t block_size,
            const std::size_t sampling_rate,
            const std::size_t buffer_size=10);

  
  /**
   * Get the next valid block.
   *
   * Return nullptr if no valid block available
   */  
  file_block* next_block();

  /**
   * Add a file to the playlist if it exists.
   *
   * Returns true if the file exists and was added to the playlist or
   * false otherwise.
   */
  bool append_file(const std::filesystem::path& file);

  /**
   * Stop all files from playing
   */
  bool stop_files();
  
  /**
   * Start a thread to manage the audio files
   *
   * If the thread already started, nothing happens.
   */
  void spawn();

  inline std::thread& thread() {return _thread;}

 
private:

  
  std::size_t _block_size = 0u;
  std::size_t _ringbuffer_size = 0u;
  prealloc_ringbuffer<file_block> _buffer;
  std::size_t _sampling_rate = 0u;
  bool _running = false;

  /// Object running run()
  std::thread _thread;

  /// List of remaining files to be played
  std::list<std::filesystem::path> _playlist;
  std::mutex _playlist_mutex;

  /// Handler to file being played
  SNDFILE* _file_handler;
  bool _playing_file;
  std::size_t _current_file_sample_rate;
  std::size_t _current_file_channels;
  
  
  /// The real worker thread
  void run();

  /// Check if there are audio file to be opened
  void check_files();
  
  /// Fill all available spaces with file information
  void read_buffers();

  /**
   * Read a single block and leave it on the given block.
   *
   * If the block could be successfully read, then the block will be
   * in a status of ReadyToPlay
   */
  void read_block(file_block& block);

  /**
   * Size of the file cache.
   *
   * This considers the ratio between file and jack sampling rates and
   * Jack's block size.
   */
  std::size_t _cache_size;

  /**
   * The file might hold a different number of channels and sampling rate than
   * currently used by Jack.  Hence, we need to load the data first in this 
   * cache buffer, to resample and average all channels per sample.
   */
  file_block _file_cache;
 
};


#endif
