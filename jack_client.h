/**
 * jack_client.h
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

#ifndef _JACK_CLIENT_H
#define _JACK_CLIENT_H

#include <jack/jack.h>
#include <ostream>

#include "sndfile_thread.h"


namespace jack {

  enum class client_state {
    Idle,
    Initializing,
    Running,
    ShuttingDown,
    Stopped,
    Error
  };
  
  /**
   * Jack client class
   *
   * This class wraps some basic jack functionality.
   *
   * It follows the monostate pattern to keep just one state
   * encompassing jack's functionality
   */
  class client {
  private:
    
    /// Part of monostate 
    static jack_client_t* _client_ptr;
    static client_state   _state;

    static jack_nframes_t _buffer_size;
    static jack_nframes_t _sample_rate;

    static sndfile_thread _file_thread;
    
  protected:
    
    static jack_port_t*   _input_port;
    static jack_port_t*   _output_port;
    
  public:
    typedef jack_default_audio_sample_t sample_t;
    
    /**
     * Creates a client in Idle state.  You still have to call init()
     * when ready to start processing.
     */
    client();
    client(const client&) = delete; // not copyable
    virtual ~client();

    /**
     * Process nframes from the input in array writing the output on
     * out.
     *
     * Return true if successful or false otherwise
     */
    virtual bool process(jack_nframes_t nframes,
                         const sample_t *const in,
                         sample_t *const out) = 0;
    
    virtual void shutdown();

    client& operator=(const client&) = delete; // not copyable

    /**
     * Initialize all necessary Jack callbacks, ports, etc. and
     * start processing.
     */
    virtual client_state init();

    /**
     * Stop processing.  After calling this method, the application must
     * end, as no Jack client will be available anymore.
     */
    void stop();
    
    void set_sample_rate(const jack_nframes_t sample_rate);
    void set_buffer_size(const jack_nframes_t buffer_size);

    inline jack_nframes_t buffer_size() const {return _buffer_size;}
    inline jack_nframes_t sample_rate() const {return _sample_rate;}

    /**
     * Get input port
     */
    jack_port_t* input_port() const;

    /**
     * Get output port
     */
    jack_port_t* output_port() const;


    /**
     * Add file to playlist
     */
    bool add_file(const std::filesystem::path& file);

    /**
     * Stop playing files
     */
    bool stop_files();
    
    /**
     * Get the next block from the current file
     */
    sndfile_thread::file_block* next_file_block();
    
  };
  
} // namespace jack

std::ostream& operator<<(std::ostream& os,const JackStatus& s);

#endif
