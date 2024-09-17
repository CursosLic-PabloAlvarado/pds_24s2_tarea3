/**
 * parse_filter.tpp
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

#ifndef _PARSE_FILTER_TPP
#define _PARSE_FILTER_TPP

#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <boost/tokenizer.hpp>
#include <boost/lexical_cast.hpp>

template <typename T>
std::vector< std::vector<T> > parse_filter(const std::string& filename) {
  std::vector< std::vector<T> > sos_matrix;
  std::ifstream file(filename);
  std::string line;

  std::locale::global(std::locale::classic()); // Force "C" locale globally
  
  while (std::getline(file, line)) {
    if (line.empty() || line[0] == '#') {
      continue;  // Skip comments and empty lines
    }
    
    boost::char_separator<char> sep(" \t");
    boost::tokenizer< boost::char_separator<char> > tok(line,sep);
    std::vector<T> row;
    
    for (const auto& token : tok) {
      row.push_back(boost::lexical_cast<T>(token));
    }
        
    sos_matrix.push_back(std::move(row));
  }
    
  return sos_matrix;
}

#endif
