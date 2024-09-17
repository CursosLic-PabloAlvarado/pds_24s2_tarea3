/**
 * jack_client.cpp
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

#include "jack_client.h"

#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <stdexcept>

#include <mutex>
#include <iostream>

std::ostream& operator<<(std::ostream& os,const JackStatus& s) {
  if (s & JackFailure) {
    os << "Failure ";
  }
  if (s & JackInvalidOption) {
    os << "InvalidOption ";
  }
  if (s & JackNameNotUnique) {
    os << "NameNotUnique";
  }
  if (s & JackServerStarted) {
    os << "ServerStarted";
  }
  if (s & JackServerFailed) {
    os << "ServerFailed";
  }
  if (s & JackServerError) {
    os << "ServerError";
  }
  if (s & JackNoSuchClient) {
    os << "NoSuchClient";
  }
  if (s & JackLoadFailure) {
    os << "LoadFailure";
  }
  if (s & JackInitFailure) {
    os << "InitFailure";
  }
  if (s & JackShmFailure) {
    os << "ShmFailure";
  }
  if (s & JackVersionError) {
    os << "VersionError";
  }

  return os;
}


namespace jack {

  // Static member of class client
  jack_client_t* client::_client_ptr  = 0;
  client_state   client::_state       = client_state::Idle;

  jack_nframes_t client::_buffer_size = 0;
  jack_nframes_t client::_sample_rate = 0;

  sndfile_thread client::_file_thread;
  
  jack_port_t*   client::_input_port  = nullptr;
  jack_port_t*   client::_output_port = nullptr;

  
  /*
   * C level callback function.  
   *
   * There must be an instance of a class inherited from jack::client
   * with a method "process", that will be called from here.  This
   * method is the one that jack's C API defines.
   */
  static int process(jack_nframes_t nframes, void *arg) {
    client* ptr=static_cast<client*>(arg);
    
    typedef jack_default_audio_sample_t sample_t;

    jack_port_t *const ip = ptr->input_port();
    jack_port_t *const op = ptr->output_port();
    
    const sample_t* in
      = static_cast<const sample_t*>(jack_port_get_buffer(ip,nframes));
    
    sample_t *const out
      = static_cast<sample_t*>(jack_port_get_buffer(op,nframes));

    // Check if we have to replace the input by audio files' input
    sndfile_thread::file_block* file_block_ptr =
      ptr->next_file_block();
    
    if (file_block_ptr != nullptr) {
      in = &(file_block_ptr->front());
    }

    bool ok = ptr->process(nframes,in,out);

    if (file_block_ptr != nullptr) {
      file_block_ptr->status = sndfile_thread::Status::Garbage;
    }
    
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
  }

  // C level callback function, follows jack's C API.
  static void shutdown(void *arg) {
    client* ptr=static_cast<client*>(arg);
    ptr->shutdown();
  }

  // Callback used to update used sample rate
  static int sample_rate_changed(jack_nframes_t nframes, void *arg) {
    client* ptr=static_cast<client*>(arg);
    ptr->set_sample_rate(nframes);
    return EXIT_SUCCESS;
  }

  // Callback used to update used buffer size
  static int buffer_size_changed(jack_nframes_t nframes, void *arg) {
    client* ptr=static_cast<client*>(arg);
    ptr->set_buffer_size(nframes);
    return EXIT_SUCCESS;
  }
  

  client::client() {
  }

  client::~client() {
    if (_state != client_state::Idle) {
      std::cout << "I> Deactivating and closing JACK client" << std::endl;
      jack_deactivate(_client_ptr);
      jack_client_close(_client_ptr);
      _client_ptr=nullptr;
      _state = client_state::Stopped;
    }
  }
  
  
  client_state client::init() {
    
    static std::mutex state_lock;

    {
      std::lock_guard<std::mutex> lk(state_lock);
    
      if (_state != client_state::Idle) {
        // Jack should only be initialized once.  If it is not Idle, someone
        // already initialized this.  Just report the current state.
        return _state;
      }

      _state = client_state::Initializing;
    }

    std::cerr << "I> Initializing JACK" << std::endl;

    static const char* client_name = "dsp1";
    static const char* server_name = nullptr;

    jack_status_t jack_status;
    jack_options_t options = JackNullOption;
    
    // open a client connection to the JACK server
    _client_ptr = jack_client_open(client_name,
                                   options,
                                   &jack_status,
                                   server_name);

    if (_client_ptr == nullptr) {
      std::cerr << "E> jack_client_open() failed, " << jack_status << std::endl;

      if (jack_status & JackServerFailed) {
        std::cerr << "E> Unable to connect to JACK server" << std::endl;
      }
      return (_state = client_state::Error);
    }
    
    if (jack_status & JackServerStarted) {
      std::cerr << "I> JACK server started" << std::endl;
    }
    
    if (jack_status & JackNameNotUnique) {
      client_name = jack_get_client_name(_client_ptr);
      std::cerr << "I> unique name '" << client_name
                << "' assigned" << std::endl;
    }

    // tell the JACK server to call `process()' whenever there is work
    // to be done.
    if (jack_set_process_callback(_client_ptr,
                                  jack::process,
                                  this) != 0) {
      std::cerr << "E> Unable to set process callback" << std::endl;
      return (_state = client_state::Error);
    }

    // call `jack::shutdown()' if jack ever shuts down, either
	  // entirely, or if it just decides to stop calling us.
    jack_on_shutdown(_client_ptr,
                     jack::shutdown,
                     this);

    // Callbacks to update buffer size and sample rate if necessary
    if (jack_set_buffer_size_callback(_client_ptr,
                                      jack::buffer_size_changed,
                                      this)!=0) {
      std::cerr << "E> Unable to set buffer size callback" << std::endl;
    }
    
    if (jack_set_sample_rate_callback(_client_ptr,
                                      jack::sample_rate_changed,
                                      this)!=0) {
      std::cerr << "E> Unable to set sample rate callback" << std::endl;
    }

    // Get sample rate and buffer size
    _sample_rate = jack_get_sample_rate(_client_ptr);
    _buffer_size = jack_get_buffer_size(_client_ptr);
     
    // display the current sample rate.
    std::cerr << "I> Jack current sample rate: " << _sample_rate << std::endl;
    std::cerr << "I> Jack current buffer size: " << _buffer_size << std::endl;

    // create two ports
    _input_port = jack_port_register(_client_ptr, "input",
                                     JACK_DEFAULT_AUDIO_TYPE,
                                     JackPortIsInput, 0);
    _output_port = jack_port_register(_client_ptr, "output",
                                      JACK_DEFAULT_AUDIO_TYPE,
                                      JackPortIsOutput, 0);

    if ((_input_port == nullptr) || (_output_port == nullptr)) {
      std::cerr << "E> no more JACK ports available" << std::endl;
      return (_state = client_state::Error);
    }

    // Tell the JACK server that we are ready to roll.  Our process()
    // callback will start running now.
    if (jack_activate (_client_ptr)) {
      std::cerr << "E> cannot activate client" << std::endl;
      return (_state = client_state::Error);
    }

    _state = client_state::Running;
    
    const char **ports = nullptr;
    
    // Connect the ports.  You can't do this before the client is
    // activated, because we can't make connections to clients that
    // aren't running.  Note the confusing (but necessary) orientation
    // of the driver backend ports: playback ports are "input" to the
    // backend, and capture ports are "output" from it.
    ports = jack_get_ports(_client_ptr, nullptr, nullptr,
                           JackPortIsPhysical|JackPortIsOutput);
    
    if (ports == nullptr) {
      stop();
      std::cerr << "E> no physical capture ports" << std::endl;
      return (_state = client_state::Error);
    }
    
    if (jack_connect(_client_ptr, ports[0], jack_port_name(_input_port))) {
      fprintf (stderr, "cannot connect input ports\n");
      _state = client_state::Error;
    }
    
    free(ports);
    ports=nullptr;
    
    ports = jack_get_ports (_client_ptr, nullptr, nullptr,
                            JackPortIsPhysical|JackPortIsInput);
    if (ports == nullptr) {
      std::cerr << "E> no physical playback ports" << std::endl;
      return (_state = client_state::Error);
    }

    // Left speaker
    if (jack_connect (_client_ptr, jack_port_name(_output_port), ports[0])) {
      std::cerr << "E> Cannot connect output ports" << std::endl;
      _state = client_state::Error;
    }

    // Right speaker
    if (jack_connect (_client_ptr, jack_port_name(_output_port), ports[1])) {
      std::cerr << "E> Cannot connect output ports" << std::endl;
      _state = client_state::Error;
    }

    
    free(ports);
    ports=nullptr;

    // Initialize and start the audio file reading thread
    _file_thread.init(_buffer_size,_sample_rate);
    _file_thread.spawn();
    
    return (_state);
  }
  
  /*
   * JACK calls this shutdown_callback if the server ever shuts down or
   * decides to disconnect the client.
   */
  void client::shutdown() {
    _state = client_state::ShuttingDown;
    std::cerr << "I> Shutdown called" << std::endl;
  }

  void client::stop() {
    jack_deactivate(_client_ptr);
    _state = client_state::Stopped;
  }

  void client::set_sample_rate(const jack_nframes_t sample_rate) {
    
    std::cout << "I> Sample rate changed from " << _sample_rate
              << " to " << sample_rate << std::endl;
    
    _sample_rate = sample_rate;
  }
  
  void client::set_buffer_size(const jack_nframes_t buffer_size) {
    
    std::cout << "I> buffer size changed from " << _buffer_size
              << " to " << buffer_size << std::endl;

    _buffer_size = buffer_size; 
  }

  jack_port_t* client::input_port() const {
    return _input_port;
  }
  
  jack_port_t* client::output_port() const {
    return _output_port;
  }

  bool client::add_file(const std::filesystem::path& f) {
    return _file_thread.append_file(f);
  }

  bool client::stop_files() {
    return _file_thread.stop_files();
  }

  
  sndfile_thread::file_block* client::next_file_block() {
    return _file_thread.next_block();
  }
  
}
