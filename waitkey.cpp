/**
 * waitkey.cpp
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

#include "waitkey.h"

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

#include <iostream>

int waitkey(int timeout_ms) {

  class raii {
  private:
    termios _original_termios;
    termios _new_termios;
    int _flags=0;
  public:
    raii() {
       // Save the original terminal attributes
      tcgetattr(STDIN_FILENO,&_original_termios);

      // Set the terminal to non-canonical mode
      _new_termios = _original_termios;
      _new_termios.c_lflag &= ~(ICANON | ECHO);
      tcsetattr(STDIN_FILENO, TCSANOW, &_new_termios);

      // Set standard input to non-blocking mode
      _flags = fcntl(STDIN_FILENO, F_GETFL, 0);
      fcntl(STDIN_FILENO, F_SETFL, _flags | O_NONBLOCK);

      //std::cout << "Reconfiguring terminal" << std::endl;
    }
    
    ~raii() {
      // Restore the original terminal attributes and file descriptor flags
      tcsetattr(STDIN_FILENO, TCSANOW, &_original_termios);
      fcntl(STDIN_FILENO, F_SETFL, _flags);

      std::cout << "Restoring terminal" << std::endl;

    }
  };

  // Set terminal and restores at the end of program
  static raii init_terminal;

  // Wait for input using select with a timeout
  fd_set read_fds;
  FD_ZERO(&read_fds);
  FD_SET(STDIN_FILENO, &read_fds);
  struct timeval timeout;
  timeout.tv_sec = timeout_ms / 1000;
  timeout.tv_usec = (timeout_ms % 1000) * 1000;
  int ret = select(STDIN_FILENO + 1, &read_fds, nullptr, nullptr, &timeout);
  
  int c = -1;
  if (ret > 0) {
    char ch;
    if (read(STDIN_FILENO, &ch, 1) == 1) {
      c = ch;
    }
  }
  
  return c;
}
