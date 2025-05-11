// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2025 Matthias Lautner
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


#include "logging.h"
#include "objtypes.h"
#include "converter.h"

#ifdef __MINGW32__
#include <windows.h>
#include <minwindef.h>
#else
#include <pwd.h>
#include <sys/utsname.h>
#include <unistd.h>
#endif

#include <sstream>
#include <utility>
#include <vector>
#include <random>
#include <iomanip>
#include <regex>
#include <cwchar>


using namespace std;

namespace mobs {


static const wchar_t inval = L'\u00bf';            //       INVERTED QUESTION MARK
static const wchar_t winval = L'\uFFFD';

wchar_t to_iso_8859_1(wchar_t c) {
  return (c < L'\u0000' or c > L'\u00ff') ? inval : c;
}

wchar_t to_iso_8859_9(wchar_t c) {
  switch (c) {
    case 0x011E: return wchar_t(0xD0); //     LATIN CAPITAL LETTER G WITH BREVE
    case 0x0130: return wchar_t(0xDD); //     LATIN CAPITAL LETTER I WITH DOT ABOVE
    case 0x015E: return wchar_t(0xDE); //     LATIN CAPITAL LETTER S WITH CEDILLA
    case 0x011F: return wchar_t(0xF0); //     LATIN SMALL LETTER G WITH BREVE
    case 0x0131: return wchar_t(0xFD); //     LATIN SMALL LETTER DOTLESS I
    case 0x015F: return wchar_t(0xFE); //     LATIN SMALL LETTER S WITH CEDILLA
    case 0xD0:
    case 0xDD:
    case 0xDE:
    case 0xF0:
    case 0xFD:
    case 0xFE:
      return inval;
    default: return (c < L'\u0000' or c > L'\u00ff') ? inval : c;
  }
}

wchar_t to_iso_8859_15(wchar_t c) {
  switch (c) {
    case 0x20AC: return wchar_t(0xA4); //       EURO SIGN
    case 0x0160: return wchar_t(0xA6); //       LATIN CAPITAL LETTER S WITH CARON
    case 0x0161: return wchar_t(0xA8); //       LATIN SMALL LETTER S WITH CARON
    case 0x017D: return wchar_t(0xB4); //       LATIN CAPITAL LETTER Z WITH CARON
    case 0x017E: return wchar_t(0xB8); //       LATIN SMALL LETTER Z WITH CARON
    case 0x0152: return wchar_t(0xBC); //       LATIN CAPITAL LIGATURE OE
    case 0x0153: return wchar_t(0xBD); //       LATIN SMALL LIGATURE OE
    case 0x0178: return wchar_t(0xBE); //       LATIN CAPITAL LETTER Y WITH DIAERESIS
    case 0xA4:
    case 0xA6:
    case 0xA8:
    case 0xB4:
    case 0xB8:
    case 0xBC:
    case 0xBD:
    case 0xBE:
      return inval;
    default: return (c < L'\u0000' or c > L'\u00ff') ? inval : c;
  }
}

wchar_t to_windows_1252(wchar_t c) {
  switch (c) {
    case 0x20AC: return wchar_t(0x80); //       EURO SIGN
    case 0x0201: return wchar_t(0x82); // Komma
    case 0x0192: return wchar_t(0x83); // f
    case 0x201E: return wchar_t(0x84); // Anführungszeichen unten
    case 0x2026: return wchar_t(0x85); // 3 Punkte
    case 0x2020: return wchar_t(0x86); // Kreuz
    case 0x2021: return wchar_t(0x87); // Doppelkreuz
    case 0x02C6: return wchar_t(0x88); // Dach
    case 0x2030: return wchar_t(0x89); // Promille
    case 0x0160: return wchar_t(0x8A); //       LATIN CAPITAL LETTER S WITH CARON
    case 0x2039: return wchar_t(0x8B); // <
    case 0x0152: return wchar_t(0x8C); //       LATIN CAPITAL LIGATURE OE
    case 0x017D: return wchar_t(0x8E); //       LATIN CAPITAL LETTER Z WITH CARON
    case 0x2018: return wchar_t(0x91); // ' Anfang
    case 0x2019: return wchar_t(0x92); // ' Ende
    case 0x201C: return wchar_t(0x93); // " Anfang
    case 0x201D: return wchar_t(0x94); // " Ende
    case 0x2022: return wchar_t(0x95); // Punkt
    case 0x2013: return wchar_t(0x96); // --
    case 0x2014: return wchar_t(0x97); // ---
    case 0x02DC: return wchar_t(0x98); // Welle
    case 0x2122: return wchar_t(0x99); // TM
    case 0x0161: return wchar_t(0x9A); //       LATIN SMALL LETTER S WITH CARON
    case 0x203A: return wchar_t(0x9B); // >
    case 0x0153: return wchar_t(0x9C); //       LATIN SMALL LIGATURE OE
    case 0x017E: return wchar_t(0x9E); //       LATIN SMALL LETTER Z WITH CARON
    case 0x0178: return wchar_t(0x9F); //       LATIN CAPITAL LETTER Y WITH DIAERESIS
    case 0x80 ... 0x9F:
     return inval;
    default: return (c < L'\u0000' or c > L'\u00ff') ? inval : c;
  }
}

inline wchar_t from_iso_8859_1(wchar_t c) { return c; }

wchar_t from_iso_8859_9(wchar_t c) {
  switch (c) {
    case 0xD0: return wchar_t(0x011E); //     LATIN CAPITAL LETTER G WITH BREVE
    case 0xDD: return wchar_t(0x0130); //     LATIN CAPITAL LETTER I WITH DOT ABOVE
    case 0xDE: return wchar_t(0x015E); //     LATIN CAPITAL LETTER S WITH CEDILLA
    case 0xF0: return wchar_t(0x011F); //     LATIN SMALL LETTER G WITH BREVE
    case 0xFD: return wchar_t(0x0131); //     LATIN SMALL LETTER DOTLESS I
    case 0xFE: return wchar_t(0x015F); //     LATIN SMALL LETTER S WITH CEDILLA
    default: return c;
  }
}

wchar_t from_iso_8859_15(wchar_t c) {
  switch (c) {
    case 0xA4: return wchar_t(0x20AC); //       EURO SIGN
    case 0xA6: return wchar_t(0x0160); //       LATIN CAPITAL LETTER S WITH CARON
    case 0xA8: return wchar_t(0x0161); //       LATIN SMALL LETTER S WITH CARON
    case 0xB4: return wchar_t(0x017D); //       LATIN CAPITAL LETTER Z WITH CARON
    case 0xB8: return wchar_t(0x017E); //       LATIN SMALL LETTER Z WITH CARON
    case 0xBC: return wchar_t(0x0152); //       LATIN CAPITAL LIGATURE OE
    case 0xBD: return wchar_t(0x0153); //       LATIN SMALL LIGATURE OE
    case 0xBE: return wchar_t(0x0178); //       LATIN CAPITAL LETTER Y WITH DIAERESIS
    default: return c;
  }
}

wchar_t from_windows_1252(wchar_t c) {
  switch (c) {
    case 0x80: return wchar_t(0x20AC); //       EURO SIGN
    case 0x82: return wchar_t(0x0201); // Komma
    case 0x83: return wchar_t(0x0192); // f
    case 0x84: return wchar_t(0x201E); // Anführungszeichen unten
    case 0x85: return wchar_t(0x2026); // 3 Punkte
    case 0x86: return wchar_t(0x2020); // Kreuz
    case 0x87: return wchar_t(0x2021); // Doppelkreuz
    case 0x88: return wchar_t(0x02C6); // Dach
    case 0x89: return wchar_t(0x2030); // Promille
    case 0x8A: return wchar_t(0x0160); //       LATIN CAPITAL LETTER S WITH CARON
    case 0x8B: return wchar_t(0x2039); // <
    case 0x8C: return wchar_t(0x0152); //       LATIN CAPITAL LIGATURE OE
    case 0x8E: return wchar_t(0x017D); //       LATIN CAPITAL LETTER Z WITH CARON
    case 0x91: return wchar_t(0x2018); // ' Anfang
    case 0x92: return wchar_t(0x2019); // ' Ende
    case 0x93: return wchar_t(0x201C); // " Anfang
    case 0x94: return wchar_t(0x201D); // " Ende
    case 0x95: return wchar_t(0x2022); // Punkt
    case 0x96: return wchar_t(0x2013); // --
    case 0x97: return wchar_t(0x2014); // ---
    case 0x98: return wchar_t(0x02DC); // Welle
    case 0x99: return wchar_t(0x2122); // TM
    case 0x9A: return wchar_t(0x0161); //       LATIN SMALL LETTER S WITH CARON
    case 0x9B: return wchar_t(0x203A); // >
    case 0x9C: return wchar_t(0x0153); //       LATIN SMALL LIGATURE OE
    case 0x9E: return wchar_t(0x017E); //       LATIN SMALL LETTER Z WITH CARON
    case 0x9F: return wchar_t(0x0178); //       LATIN CAPITAL LETTER Y WITH DIAERESIS
    default: return c;
  }
}

static const char tab_7up[] =
        "         ,          "
        "            ,       "
        "    ,  ,0123456789,,"
        "     ABCDEFGHIJKLMNO"
        "PQRSTUVWXYZ      ABC"
        "DEFGHIJKLMNOPQRSTUVW"
        "XYZ        F      S "
        "O Z           S O ZY"
        "      ,             "
        "            AAAAAAAC"
        "EEEEIIIIDNOOOOO OUUU"
        "UYPSAAAAAAACEEEEIIII"
        "DNOOOOO OUUUUYPYAAAA"
        "AACCCCCCCCDDDDEEEEEE"
        "EEEEGGGGGGGGHHHHIIII"
        "IIIIIIJJJJKKKLLLLLLL"
        "LLLNNNNNNNNNOOOOOOOO"
        "RRRRRRSSSSSSSSTTTTTT"
        "UUUUUUUUUUUUWWYYYYZZ"
        "ZZZ BBBBBBCCCDDDDDEE"
        "EFFGGHIIKKLLMNNOOORR"
        "PPPSSSTTTTTUUOOYYZZZ"
        "ZZ          ZZZLLLNN"
        "NAAIIOOUUUUUUUUUU AA"
        "AAAAGGGGKKOOOOZZZZZZ"
        "GGHPNNAAAAOOAAAAEEEE"
        "IIIIOOOORRRRUUUUSSTT"
        "  HH    ZZAAEEOOOOOO"
        "OOYY      ACC      B";


wchar_t to_7up(wchar_t c) {
  if (sizeof(tab_7up) != 581)
    abort();
  if (c < 0 or c > static_cast<size_t>(sizeof(tab_7up)))
    return L' ';
  return static_cast<size_t>(tab_7up[size_t(c)]);
}

codec_iso8859_1::result codec_iso8859_1::do_out(mbstate_t& state,
                                                const wchar_t* from,
                                                const wchar_t* from_end,
                                                const wchar_t*& from_next,
                                                char* to,
                                                char* to_end,
                                                char*& to_next) const
{
  for (; from != from_end and to != to_end; from++, to++)
  {
    *to = u_char(to_iso_8859_1(*from));
//    cerr << to_string(*from);
  }
  to_next = to;
  from_next = from;
  return ok;
}

codec_iso8859_1::result codec_iso8859_1::do_in (state_type& state,
                                                const char* from,
                                                const char* from_end,
                                                const char*& from_next,
                                                wchar_t* to,
                                                wchar_t* to_end,
                                                wchar_t*& to_next) const
{
  for (; from != from_end and to != to_end; from++, to++)
  {
    *to = from_iso_8859_1(*(unsigned char*)from);
//    cerr << to_string(*to);
  }
  to_next = to;
  from_next = from;
  return ok;
}

bool codec_iso8859_1::do_always_noconv() const noexcept {
  return false;
}

codec_iso8859_9::result codec_iso8859_9::do_out(mbstate_t& state,
                                                const wchar_t* from,
                                                const wchar_t* from_end,
                                                const wchar_t*& from_next,
                                                char* to,
                                                char* to_end,
                                                char*& to_next) const
{
  for (; from != from_end and to != to_end; from++, to++)
  {
    *to = u_char(to_iso_8859_9(*from));
  }
  to_next = to;
  from_next = from;
  return ok;
}

codec_iso8859_9::result codec_iso8859_9::do_in (state_type& state,
                                                const char* from,
                                                const char* from_end,
                                                const char*& from_next,
                                                wchar_t* to,
                                                wchar_t* to_end,
                                                wchar_t*& to_next) const
{
  for (; from != from_end and to != to_end; from++, to++)
  {
    *to = from_iso_8859_9(*(unsigned char*)from);
  }
  to_next = to;
  from_next = from;
  return ok;
}

bool codec_iso8859_9::do_always_noconv() const noexcept {
  return false;
}

codec_iso8859_15::result codec_iso8859_15::do_out(mbstate_t& state,
                                                  const wchar_t* from,
                                                  const wchar_t* from_end,
                                                  const wchar_t*& from_next,
                                                  char* to,
                                                  char* to_end,
                                                  char*& to_next) const
{
  for (; from != from_end and to != to_end; from++, to++)
  {
    *to = u_char(to_iso_8859_15(*from));
  }
  to_next = to;
  from_next = from;
  return ok;
}

codec_iso8859_15::result codec_iso8859_15::do_in (state_type& state,
                                                  const char* from,
                                                  const char* from_end,
                                                  const char*& from_next,
                                                  wchar_t* to,
                                                  wchar_t* to_end,
                                                  wchar_t*& to_next) const
{
  for (; from != from_end and to != to_end; from++, to++)
  {
    *to = from_iso_8859_15(*(unsigned char*)from);
  }
  to_next = to;
  from_next = from;
  return ok;
}

bool codec_iso8859_15::do_always_noconv() const noexcept {
  return false;
}


codec_windows_1252::result codec_windows_1252::do_out(mbstate_t& state,
                                                  const wchar_t* from,
                                                  const wchar_t* from_end,
                                                  const wchar_t*& from_next,
                                                  char* to,
                                                  char* to_end,
                                                  char*& to_next) const
{
  for (; from != from_end and to != to_end; from++, to++)
  {
    *to = u_char(to_windows_1252(*from));
  }
  to_next = to;
  from_next = from;
  return ok;
}

codec_windows_1252::result codec_windows_1252::do_in (state_type& state,
                                                  const char* from,
                                                  const char* from_end,
                                                  const char*& from_next,
                                                  wchar_t* to,
                                                  wchar_t* to_end,
                                                  wchar_t*& to_next) const
{
  for (; from != from_end and to != to_end; from++, to++)
  {
    *to = from_windows_1252(*(unsigned char*)from);
  }
  to_next = to;
  from_next = from;
  return ok;
}

bool codec_windows_1252::do_always_noconv() const noexcept {
  return false;
}


int from_base64(wchar_t c) {
  if (c < 0 or c > 127)
    return -1;
  static const vector<int> b64Chars = {
          -1, -1, -1, -1, -1, -1, -1, -1, -1, 99, 99, -1, 99, 99, -1, -1,
          -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
          99, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
          52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
          -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
          15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
          -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
          41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1 };
  return b64Chars[c];
}

wchar_t to_base64(int i) {
  const wstring base64 = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  if (i < 0 or i > 63)
    return winval;
  return base64[size_t(i)];
}




wchar_t from_html_tag(const std::wstring &tok)
{
  wchar_t c = '\0';
  if (tok == L"lt")
    c = '<';
  else if (tok == L"gt")
    c = '>';
  else if (tok == L"amp")
    c = '&';
  else if (tok == L"quot")
    c = '"';
  else if (tok == L"apos")
    c = '\'';
  else if (tok[0] == L'#') {
    size_t p;
    try {
      int i = std::stoi(mobs::to_string(tok.substr(tok[1] == 'x' ? 2:1)), &p, tok[1] == 'x' ? 16:10);
      if (p == tok.length() - (tok[1] == 'x' ? 2:1) and
          (i == 9 or i == 10 or i == 13 or (i >= 32 and i <= 0xD7FF) or
           (i >= 0xE000 and i <= 0xFFFD) or (i >= 0x10000 and i <= 0x10FFFF)))
        c = i;
    } catch (...) {}
  }
  return c;
}

std::wstring::const_iterator to7Up(std::wstring::const_iterator begin, std::wstring::const_iterator end, std::string &result) {
  char last = ' ';
  for (; begin != end; begin++) {
    char n = ' ';
//    wchar_t x = *begin;
    if (*begin >= 0 and size_t(*begin) < sizeof(tab_7up) -1)
      n = tab_7up[size_t(*begin)];
    switch (n) {
      case '\0' ... 0x1f:
        LOG(LM_ERROR, " EEEE " << to_string(*begin));
      case ',':
        begin++;
        return begin;
      case ' ':
        last = ' ';
        break;
      case 'E':
        if (last == 'A' or last == 'O' or last == 'U') // Umschrift AE OE UE ignorieren
          break;
        __attribute__ ((fallthrough));
      default:
        if (n == last)
          continue;
        result += n;
        last = n;
    }
  }
  return begin;
}

std::wstring toLower(const std::wstring &tx) {
  std::locale loc;
#ifndef __WIN32__
  try {
    const char *cp = getenv("LANG");
    if (cp)
      loc = std::locale(cp);
    else
      loc = std::locale("de_DE.UTF-8");
  } catch (...) {
    loc = std::locale();
  }
  // LOG(LM_DEBUG, "LOCALE = " << loc.name());
#endif
  wstring lo;
  lo.reserve(tx.length());
  std::transform(tx.begin(), tx.end(), std::back_inserter(lo), [loc](const wchar_t c) { return std::tolower(c, loc);} );
  return lo;
}

std::wstring toUpper(const std::wstring &tx) {
  std::locale loc;
#ifndef __WIN32__
  try {
    const char *cp = getenv("LANG");
    if (cp)
      loc = std::locale(cp);
    else
      loc = std::locale("de_DE.UTF-8");
  } catch (...) {
    loc = std::locale();
  }
  //LOG(LM_DEBUG, "LOCALE = " << loc.name());
#endif
  wstring lo;
  lo.reserve(tx.length());
  std::transform(tx.begin(), tx.end(), std::back_inserter(lo), [loc](const wchar_t c) { return std::toupper(c, loc);} );
  return lo;
}

std::string toLower(const string &tx) {
  return mobs::to_string(mobs::toLower(mobs::to_wstring(tx)));
}

std::string toUpper(const string &tx) {
  return mobs::to_string(mobs::toUpper(mobs::to_wstring(tx)));
}

void from_string_base64(const string &base64, vector<u_char> &v) {
  Base64Reader bd(v);
  for (auto c:base64)
    bd.put(u_char(c));
  bd.done();
}


void Base64Reader::start() {
  base64.clear();
  b64Cnt = 0;
  b64Value = 0;
}

void Base64Reader::done() {
  if (b64Cnt > 0 and b64Cnt < 4)
    put('=');
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
void Base64Reader::put(wchar_t c) {
  int v = from_base64(c);
  if (v < 0) {
    if (c == '=') { // padding
      switch (b64Cnt) {
        case 3:
          base64.push_back(b64Value >> 10);
          base64.push_back((b64Value >> 2) & 0xff);
          __attribute__ ((fallthrough));
        case 100:
          b64Cnt = 999; // Wenn noch ein = kommt ⇒ fehler
          break;
        case 2:
          base64.push_back(b64Value >> 4);
          b64Cnt = 100; // es darf noch 1 = kommen
          break;
        case 1: throw std::runtime_error("base64 unexpected end");
        default: throw std::runtime_error("base64 unexpected padding");
      }
    }
    else
      throw std::runtime_error("base64 padding");
  }
  else if (v < 64) {
    if (b64Cnt > 3)
      throw std::runtime_error("base64 invalid");
    b64Value = (b64Value << 6) + v;
    if (++b64Cnt == 4) {
      base64.push_back(b64Value >> 16);
      base64.push_back((b64Value >> 8) & 0xff);
      base64.push_back(b64Value & 0xff);
      b64Cnt = 0;
      b64Value = 0;
    }
  }
}
#pragma clang diagnostic pop



bool StrConv<std::vector<u_char>>::c_string2x(const std::string &str, std::vector<u_char> &t, const ConvFromStrHint &) {
  try {
    Base64Reader r(t);
    for (auto c:str)
      r.put(c);
    r.done();
  }
  catch (exception &e) {
    LOG(LM_INFO, "Error, in base64: " << e.what());
    return false;
  }
  return true;
}
bool StrConv<std::vector<u_char>>::c_wstring2x(const std::wstring &wstr, std::vector<u_char> &t, const ConvFromStrHint &) {
  try {
    Base64Reader r(t);
    for (auto c:wstr)
      r.put(c);
    r.done();
  }
  catch (exception &e) {
    LOG(LM_INFO, "Error, in base64: " << e.what());
    return false;
  }
  return true;
}


std::string StrConv<std::vector<u_char>>::c_to_string(const std::vector<u_char> &t, const ConvToStrHint &cts) {
  std::string u;
  copy_base64(t.cbegin(), t.cend(), std::back_inserter(u), cts.withIndentation() ? "\n  ":"");
  return u;
}
std::wstring StrConv<std::vector<u_char>>::c_to_wstring(const std::vector<u_char> &t, const ConvToStrHint &cts) {
  std::wstring u;
  copy_base64(t.cbegin(), t.cend(), std::back_inserter(u), cts.withIndentation() ? "\n  ":"");
  return u;
}

bool StrConv<std::vector<u_char>>::c_from_blob(const void *p, uint64_t sz, vector<u_char> &t) {
  if (not p)
    return false;
  t = std::move(std::vector<u_char>((u_char *)p, (u_char *)p + sz));
  return true;
}

bool StrConv<std::vector<u_char>>::c_to_blob(const vector<u_char> &t, const void *&p, uint64_t &sz) {
  p = (void *)&t[0]; sz = t.size();
  return true;
}



std::string gen_uuid_v4_p() {
  static std::random_device rd;
  static std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 15);
  std::uniform_int_distribution<> dis2(8, 11);
  std::stringstream ss;
  int i;
  ss << std::hex;
  for (i = 0; i < 8; i++) {
    ss << dis(gen);
  }
  ss << '-';
  for (i = 0; i < 4; i++) {
    ss << dis(gen);
  }
  ss << "-4"; // Version 4
  for (i = 0; i < 3; i++) {
    ss << dis(gen);
  }
  ss << '-';
  ss << dis2(gen); // enthält Variante Bit 1 0 x ...
  for (i = 0; i < 3; i++) {
    ss << dis(gen);
  }
  ss << '-';
  for (i = 0; i < 12; i++) {
    ss << dis(gen);
  };
  return ss.str();
}

std::string timeOffsetToStr(long gmtoff) {
  std::stringstream s;
  if (gmtoff) {
    s << (gmtoff> 0 ? '+' : '-');
    if (gmtoff < 0)
      gmtoff = - gmtoff;
    s << std::setw(2)  << std::setfill('0') << gmtoff / 3600 << ':'
      << std::setw(2)  << std::setfill('0')<< (gmtoff % 3600) / 60;
  }
  else
    s << 'Z';
  return s.str();
}

class StringFormatterData {
public:
  class Rule {
  public:
    Rule(const wstring& re, wstring fo) : regex(re, std::regex_constants::ECMAScript), format(std::move(fo)) {};
     // TODO icase
    wregex regex;
    wstring format;
  };
  vector<Rule> rules{};
};

StringFormatter::StringFormatter() {
  data = std::unique_ptr<StringFormatterData>(new StringFormatterData);
}

StringFormatter::~StringFormatter() = default;

int StringFormatter::insertPattern(const wstring &regex, const wstring &format) {
  data->rules.emplace_back(regex, format);
  return data->rules.size();
}

bool StringFormatter::empty() const {
  return data->rules.empty();
}

int StringFormatter::format(const wstring &input, wstring &result, int ruleBegin) const {
  if (ruleBegin < 1)
    ruleBegin = 1;
  for (size_t i = ruleBegin -1; i < data->rules.size(); i++) {
    StringFormatterData::Rule &rule = data->rules[i];
    wsmatch match;
    if (regex_match(input, match, rule.regex)) {
      result.clear();
      wstring cmd;
      int pos = 0;
      for(auto c:rule.format) {
        if (cmd.empty()) {
          if (c == L'%')
            cmd += '%';
          else
            result += c;
          continue;
        }
        switch (c) {
          case L'%':
            if (cmd == L"%") {
              result += c;
              cmd.clear();
              continue;
            }
            if (pos > 0)
              THROW("unmatches '%' in format");
            try {
              size_t idx;
              pos = stoi(cmd.substr(1), &idx);
              if (idx != cmd.length() -1)
                throw runtime_error("XX");
            }
            catch (exception &e) {
              THROW("invalid position id in format");
            }
            cmd.clear();
          case L'0' ... L'9':
          case L'.':
          case L',':
          case L'-':
          case L'+':
            cmd += c;
            break;
          case L'd':
          case L'x':
          case L'X':
            if (pos == 0)
              THROW("position missing in format");
            if (pos >= match.size())
              THROW("position out of range in format");
            int64_t num;
            try {
              size_t idx;
              num = stoll(match[pos].str(), &idx);
              if (idx != match[pos].str().length())
                throw runtime_error("XX");
            }
            catch (exception &e) {
              THROW("invalid number " << mobs::to_string(match[pos].str()));
            }
            cmd += c;
            wchar_t buf[32];
            swprintf(buf, sizeof(buf), cmd.c_str(), num);
            result += buf;
            pos = 0;
            cmd.clear();
            break;
          case L's':
          case L'S':
          {
            wchar_t fill = L' ';
            if (pos == 0)
              THROW("position missing in format");
            if (pos > match.size())
              THROW("position out of range in format");
            int len = 0;
            if (cmd.length() > 2 and cmd[1] != L'-' and (cmd[1] == L'0' or not isdigit(cmd[1]))) {
              fill = cmd[1];
              cmd.erase(1, 1);
            }
            else if (cmd.length() > 3 and cmd[1] == L'-' and (cmd[2] == L'0' or not isdigit(cmd[2]))) {
              fill = cmd[2];
              cmd.erase(2, 1);
            }
            if (cmd.length() > 1)
              try {
                size_t idx;
                len = stoi(cmd.substr(1), &idx);
                if (idx != cmd.length() - 1)
                  throw runtime_error("XX");
              }
              catch (exception &e) {
                THROW("invalid length id in format");
              }
            if (len and match[pos].str().length() > abs(len))
              THROW("string to long");
            if (len < 0)
              result += wstring(size_t(-len - match[pos].str().length()), fill);
            if (c == L'S')
              result += toUpper(match[pos].str());
            else
              result += match[pos].str();
            if (len > 0)
              result += wstring(size_t(len - match[pos].str().length()), fill);
            pos = 0;
            cmd.clear();
            break;
          }
          default:
            cmd += c;
        }
      }
      return i+1;
    }
  }
  return 0;
}

const string &getLoginName()
{
  static std::string username;
  if (username.empty())
  {
#ifdef __MINGW32__
    char buf[300];
    DWORD bufSize = sizeof(buf);
    if (GetUserNameA(buf, &bufSize))
      username = std::string(buf);
    if (username.empty())
      throw std::runtime_error("can't GetUserName");
#else
    struct passwd *pw = ::getpwuid(geteuid());
    if (pw == nullptr)
      throw std::runtime_error("can't get pwd entry");
    username = pw->pw_name;
#endif
  }
  return username;
}


const string &getNodeName()
{
  static std::string nodename;
  if (nodename.empty())
  {
#ifdef __MINGW32__
    char buf[300];
    DWORD bufSize = sizeof(buf);
    if (GetComputerNameA(buf, &bufSize))
      nodename = std::string(buf);
    if (nodename.empty())
      throw std::runtime_error("GetComputerName failed");
#else
    struct utsname uts;
    if (::uname(&uts) == -1)
      throw std::runtime_error("uname failed");
    nodename = uts.nodename;
    auto pos =  nodename.find('.');
    if (pos != string::npos and pos > 0)
      nodename.resize(pos);
#endif
  }
  return nodename;
}

}
