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


#ifndef MOBS_OBJTYPES_H
#define MOBS_OBJTYPES_H

#include <string>
#include <iostream>
#include <sstream>
#include <codecvt>

/** \file objtypes.h
 \brief Definitionen von Konvertierungsroutinen von und nach std::string */

namespace mobs {

/// String-Konvertierung
/// @param val Wert in utf8
/// @return Wert als u32string
inline std::u32string to_u32string(std::string val) { std::wstring_convert<std::codecvt_utf8<char32_t>,char32_t> c;
return c.from_bytes(val); };

/// String-Konvertierung
/// @param val Wert in utf8
/// @return Wert als wstring
inline std::wstring to_wstring(std::string val) { std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> c; return c.from_bytes(val); };

//template <typename T>
//inline std::string to_string(T t) { return std::to_string(t); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(std::string t) { return t; };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
std::string to_string(std::u32string t);
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
std::string to_string(std::u16string t);
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
std::string to_string(std::wstring t);
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(char t) { return std::string() + t; };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(signed char t) { return to_string(char(t)); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(unsigned char t) { return to_string(char(t)); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(char16_t t) { return to_string(std::u16string() + t); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(char32_t t) { return to_string(std::u32string() + t); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(wchar_t t) { return to_string(std::wstring() + t); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(int t) { return std::to_string(t); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(short int t) { return std::to_string(t); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(long int t) { return std::to_string(t); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(long long int t) { return std::to_string(t); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(unsigned int t) { return std::to_string(t); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(unsigned short int t) { return std::to_string(t); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(unsigned long int t) { return std::to_string(t); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(unsigned long long int t) { return std::to_string(t); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(bool t) { return t ? u8"true":u8"false"; };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(const char *t) { return to_string(std::string(t)); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(const wchar_t *t) { return to_string(std::wstring(t)); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(const char16_t *t) { return to_string(std::u16string(t)); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(const char32_t *t) { return to_string(std::u32string(t)); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
std::string to_string(float t);
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
std::string to_string(double t);
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
std::string to_string(long double t);

template <typename T>
/// \brief Konvertierung von std::string
/// @param str Konvertierter Wert
/// @param t Wert
/// @return true, wenn fehlerfrei
inline bool string2x(const std::string &str, T &t) {std::stringstream s; s.str(str); s >> t; return s.eof() and not s.bad() and not s.fail(); };
/// \private
template <> inline bool string2x(const std::string &str, std::string &t) { t = str; return true; };
/// \private
template <> bool string2x(const std::string &str, std::wstring &t);
/// \private
template <> bool string2x(const std::string &str, std::u32string &t);
/// \private
template <> bool string2x(const std::string &str, std::u16string &t);
/// \private
template <> bool string2x(const std::string &str, char &t);
/// \private
template <> bool string2x(const std::string &str, signed char &t);
/// \private
template <> bool string2x(const std::string &str, unsigned char &t);
/// \private
template <> bool string2x(const std::string &str, char16_t &t);
/// \private
template <> bool string2x(const std::string &str, char32_t &t);
/// \private
template <> bool string2x(const std::string &str, wchar_t &t);
/// \private
template <> bool string2x(const std::string &str, bool &t);

}

#endif
