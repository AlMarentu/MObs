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


#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedGlobalDeclarationInspection"
#ifndef MOBS_CONVERTER_H
#define MOBS_CONVERTER_H

#include <string>
#include <locale>
#include <algorithm>


/** \file converter.h
 \brief Hilfsklassen für codecs und base64 */


namespace mobs {

/// wandelt einen Unicode-zeichen in ein ISO8859-1 Zeichen um; im Fehlerfall wird U+00bf INVERTED QUESTION MARK geliefert
wchar_t to_iso_8859_1(wchar_t c);
/// wandelt einen Unicode-zeichen in ein ISO8859-9 Zeichen um; im Fehlerfall wird U+00bf INVERTED QUESTION MARK geliefert
wchar_t to_iso_8859_9(wchar_t c);
/// wandelt einen Unicode-zeichen in ein ISO8859-15 Zeichen um; im Fehlerfall wird U+00bf INVERTED QUESTION MARK geliefert
wchar_t to_iso_8859_15(wchar_t c);

/// wandelt ein ISO8859-9 Zeichen in Unicode um
wchar_t from_iso_8859_9(wchar_t c);
/// wandelt ein ISO8859-15 Zeichen in Unicode um
wchar_t from_iso_8859_15(wchar_t c);

/// codec für wfstream
class codec_iso8859_1 : virtual public std::codecvt<wchar_t, char, std::mbstate_t> {
public:
/// \private
  result do_out(mbstate_t& state, const wchar_t* from, const wchar_t* from_end, const wchar_t*& from_next,
                        char* to, char* to_end, char*& to_next) const override;
  /// \private
  result do_in (state_type& state, const char* from, const char* from_end, const char*& from_next,
                        wchar_t* to, wchar_t* to_limit, wchar_t*& to_next) const override;
};
/// codec für wfstream
class codec_iso8859_9 : virtual public std::codecvt<wchar_t, char, std::mbstate_t> {
public:
  /// \private
  result do_out(mbstate_t& state, const wchar_t* from, const wchar_t* from_end, const wchar_t*& from_next,
                        char* to, char* to_end, char*& to_next) const override;
  /// \private
  result do_in (state_type& state, const char* from, const char* from_end, const char*& from_next,
                        wchar_t* to, wchar_t* to_limit, wchar_t*& to_next) const override;
};
/// codec für wfstream
class codec_iso8859_15 : virtual public std::codecvt<wchar_t, char, std::mbstate_t> {
public:
  /// \private
  result do_out(mbstate_t& state, const wchar_t* from, const wchar_t* from_end, const wchar_t*& from_next,
                        char* to, char* to_end, char*& to_next) const override;
  /// \private
  result do_in (state_type& state, const char* from, const char* from_end, const char*& from_next,
                        wchar_t* to, wchar_t* to_limit, wchar_t*& to_next) const override;
};


/// Rückgabe base64-Wert des Zeichens oder 99 bei whitespace oder -1 bei ungültig
int from_base64(wchar_t c);
/// Rückgabe des zum base64-Wert gehörigen Zeichens
wchar_t to_base64(int i);

#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
template<class InputIt, class OutputIt>
/// transformer, der Container umkopiert und dabei im Ziel die untersten 8-Bit der Quelle bas64 Kodiert
/// @param first Quell-Itertor Start
/// @param last Quell-Iterator Ende
/// @param d_first Ziel-Insert-Iterator
/// @param linebreak String, der nach 15 Blöcken ausgegeben wird
OutputIt copy_base64(InputIt first, InputIt last, OutputIt d_first, const std::string& linebreak = "")
{
  int i = 0;
  int a = 0;
  int l = 0;
  while (first != last) {
    a = (a << 8) + (*first & 0xff);
    if (++i == 3) {
      *d_first++ = char(to_base64(a >> 18));
      *d_first++ = char(to_base64((a >> 12) & 0x3f));
      *d_first++ = char(to_base64((a >> 6) & 0x3f));
      *d_first++ = char(to_base64(a & 0x3f));
      i = 0;
      a = 0;
      if (l++ > 15) {
        for (auto c:linebreak)
          *d_first++ = c;
        l = 0;
      }
    }
    first++;
  }
  if (i == 2) {
    *d_first++ = char(to_base64(a >> 10));
    *d_first++ = char(to_base64(a >> 4 & 0x3f));
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
}
#pragma clang diagnostic pop

/// Umwandlung eies Containers mit base64-Inhalt nach \c std::string
template<typename T>
std::string to_string_base64(const T &t) { std::string u; copy_base64(t.cbegin(), t.cend(), std::back_inserter(u)); return u; }
/// Umwandlung eies Containers mit base64-Inhalt nach \c std::wstring
template<typename T>
std::wstring to_wstring_base64(const T &t) { std::wstring u; copy_base64(t.cbegin(), t.cend(), std::back_inserter(u)); return u; }
/// Umwandlung eies Containers mit base64-Inhalt nach \c std::wostream
template<typename T>
std::wostream &to_wostream_base64(std::wostream &str, const T &t) { copy_base64(t.cbegin(), t.cend(), std::ostreambuf_iterator<wchar_t>(str)); return str; }


//template std::wostream &to_wostream_base64<std::vector<u_char>>(std::wostream &, const std::vector<u_char> &);

/// wandelt einen HTML-Character-Code in Unicode um; die Angabe erfolg ohne '&' und ';': Z.B.  "amp" oder "#xd"
wchar_t from_html_tag(const std::wstring &tok);

/// Klasse zum Auswerten vin Base64
class Base64Reader {
public:
  /// Konstruktor übergibt Buffer
  explicit Base64Reader(std::vector<u_char> &v) : base64(v) { start(); }
  /// nächstes Zeichen parsen
  void put(wchar_t c);
  /// Ende des zu parsenden Textes
  void done();
  /// Neustart
  void clear() { start(); };

  private:
  void start();
  std::vector<u_char> &base64;
  int b64Value = 0;
  int b64Cnt = 0;
};




}

#endif

#pragma clang diagnostic pop