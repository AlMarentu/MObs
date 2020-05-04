// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2020 Matthias Lautner
//
// This is part of MObs https://github.com/AlMarentu/MObs.git
//
// MObs is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


#ifndef MOBS_BASE64_H
#define MOBS_BASE64_H

#include <string>
#include <iostream>
#include <vector>
#include <algorithm>


/** \file base64.h
 \brief Hilfsklassen für base64 */


namespace mobs {

/// Rückgabe base64-Wert des Zeichens oder 99 bei whitespace oder -1 bei ungültig
int from_base64(wchar_t c);
/// Rückgabe des zum base64-Wert gehörigen Zeichens
wchar_t to_base64(int i);


template<class InputIt, class OutputIt>
/// transformer, der Container umkopiert und dabei im Ziel die untersten 8-Bit der Quelle bas64 Kodiert
/// @param first Quell-Itertor Start
/// @param last Quell-Iterator Ende
/// @param d_first Ziel-Insert-Iterator
OutputIt copy_base64(InputIt first, InputIt last, OutputIt d_first)
{
  int i = 0;
  int a = 0;
  while (first != last) {
    a = (a << 8) + (*first & 0xff);
    if (++i == 3) {
      *d_first++ = char(to_base64(a >> 18));
      *d_first++ = char(to_base64((a >> 12) & 0x3f));
      *d_first++ = char(to_base64((a >> 6) & 0x3f));
      *d_first++ = char(to_base64(a & 0x3f));
      i = a = 0;
    }
    first++;
  }
  if (i == 2) {
    *d_first++ = char(to_base64(a >> 10));
    *d_first++ = char(to_base64((a >> 4) & 0x3f));
    *d_first++ = char(to_base64((a & 0x0f) << 2));
    *d_first++ = ('=');
  }
  if (i == 1) {
    *d_first++ = char(to_base64(a >> 2));
    *d_first++ = char(to_base64((a & 0x03) << 4));
    *d_first++ = ('=');
    *d_first++ = ('=');
  }
  return d_first;
};

/// Rückgabe des zum base64-Wert gehörigen Zeichens
template<typename T>
std::string to_string_base64(const T &t) { std::string u; copy_base64(t.cbegin(), t.cend(), std::back_inserter(u)); return u; }
template<typename T>
std::wstring to_wstring_base64(const T &t) { std::wstring u; copy_base64(t.cbegin(), t.cend(), std::back_inserter(u)); return u; }
template<typename T>
std::wostream &to_wostream_base64(std::wostream &str, const T &t) { copy_base64(t.cbegin(), t.cend(), std::ostreambuf_iterator<wchar_t>(str)); return str; }


//template std::wostream &to_wostream_base64<std::vector<u_char>>(std::wostream &, const std::vector<u_char> &);




}

#endif
