// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2024 Matthias Lautner
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

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedGlobalDeclarationInspection"

#include <string>
#include <sstream>
#include <vector>
#include <list>
#include <limits>
#include <functional>
#include <cstdint>

#ifndef INT_MAX
#include <limits.h>
#endif
#ifndef SIZE_T_MAX
#define SIZE_T_MAX std::numeric_limits<size_t>::max() ///< Maximum des Typs size_t
#endif
#ifdef __MINGW32__
typedef unsigned char u_char;
typedef unsigned int u_int;
typedef unsigned long u_long;
#endif


/** \file objtypes.h
 \brief Definitionen von Konvertierungsroutinen von und nach std::string */

/**
 \brief Deklaration eines \c enum

 Im Beispiel wird ein enum direction {Dleft, Dright, Dup, Ddown};
 deklariert. Zusätzlich wird eine Konvertierungsklasse \c directionStrEnumConv sowie eine weitere Hilfsklasse erzeugt;
 ebenso wird eine Funktion enum_to_string definiert, um den Enum in seine Text-Representation zu wandeln
 \code
 MOBS_ENUM_DEF(direction, Dleft, Dright, Dup, Ddown);
 MOBS_ENUM_VAL(direction, "left", "right", "up", "down");
 \endcode
 Danach kann eine Variable
 \code
     MemMobsEnumVar(direction, d);
 \endcode
im Mobs-Objekt verwendet werden, mit Klartext-Namen arbeiten kann
*/
#define MOBS_ENUM_DEF(typ, ... ) \
enum typ { __VA_ARGS__ }; \
class mobsEnum_##typ##_define { \
public: \
static size_t numOf(enum typ e) { \
  static const std::vector<enum typ> v = { __VA_ARGS__ }; \
  size_t pos = 0; \
  for (auto i:v) \
    if (i == e) \
      return pos; \
    else \
      pos++; \
  throw std::runtime_error("enum does not exist"); \
} \
static enum typ toEnum(size_t pos) { \
  static const std::vector<enum typ> v = { __VA_ARGS__ }; \
  if (pos >= v.size()) \
    throw std::runtime_error("enum out of range"); \
  return v[pos]; } \
}

/// Deklaration der Ausgabewerte zu einem \c MOBS_ENUM_DEF
#define MOBS_ENUM_VAL(typ, ... ) \
class typ##StrEnumConv : public mobs::StrConvBaseT<typ> { \
public: \
  static std::string toStr(size_t pos) { \
    static const std::vector<std::string> v = { __VA_ARGS__ }; \
    if (pos >= v.size()) \
      throw std::runtime_error("enum " #typ " out of range"); \
    return v[pos]; } \
  static size_t numOf(const std::string &s) { \
    static const std::vector<std::string> v = { __VA_ARGS__ }; \
    size_t pos = 0; \
    for (auto i:v) \
      if (i == s) \
        return pos; \
      else \
        pos++; \
    throw std::runtime_error("enum " #typ ": name does not exist"); \
  } \
  static enum typ fromStr(const std::string &s) { return mobsEnum_##typ##_define::toEnum(numOf(s)); } \
  static std::string toStr(enum typ e) { return (toStr(mobsEnum_##typ##_define::numOf(e))); } \
  static size_t numOf(enum typ e) { return mobsEnum_##typ##_define::toEnum(e); } \
  static inline bool c_string2x(const std::string &str, typ &t, const mobs::ConvFromStrHint &cfh) { \
    if (cfh.acceptExtended()) { try { t = fromStr(str); return true; } catch (...) { } } \
    if (not cfh.acceptCompact()) return false; \
    int i; if (not mobs::string2x(str, i)) return false; t = typ(i); return true; } \
  static inline bool c_wstring2x(const std::wstring &wstr, typ &t, const mobs::ConvFromStrHint &cfh) { return c_string2x(mobs::to_string(wstr), t, cfh); } \
  static inline std::string c_to_string(const typ &t, const mobs::ConvToStrHint &cth) { return cth.compact() ? mobs::to_string(int(t)) : toStr(t); } \
  static inline std::wstring c_to_wstring(const typ &t, const mobs::ConvToStrHint &cth) { return cth.compact() ? mobs::to_wstring(int(t)) : mobs::to_wstring(toStr(t)); }; \
  static inline bool c_is_chartype(const mobs::ConvToStrHint &cth) { return not cth.compact(); } \
  static inline bool c_is_mobsEnum() { return true; } \
  static inline typ c_empty() { return mobsEnum_##typ##_define::toEnum(0); } \
  static inline bool c_from_uint(uint64_t u, typ &t) { t = typ(u); return true; } \
  static inline uint64_t c_max() { \
     static const std::vector<std::string> v = { __VA_ARGS__ };\
     if (v.empty())  throw std::runtime_error("enum " #typ ": is empty"); \
     return v.size() -1; }       \
  static std::string c_to_str(int i) { return toStr((enum typ)i); }  \
}; \
inline std::string enum_to_string(enum typ e) { return typ##StrEnumConv::toStr(e); };

namespace mobs {

/** \brief String-Konvertierung
 *
 * @param val Wert in utf8
 * @return Wert als u32string
 */
std::u32string to_u32string(std::string val);

/** \brief String in Anführungszeichen (") setzen mit escaping
 *
 * Es werden nur die Zeichen \0, ' und \ escaped
 * @param s in UTF8
 * @return
 */
std::string to_quote(const std::string &s);

/** \brief String in einfache (') Anführungszeichen setzen mit escaping
 *
 * Es werden nur die Zeichen \0, ' und \ escaped
 * @param s in UTF8
 * @return
 */
std::string to_squote(const std::string &s);


/** \brief String in Anführungszeichen (") setzen mit escaping für JSON (nach RFC 8259)
 *
 * es werden alle Zeichen escaped, die in JSON nach RFC 8259 escaped werden müssen
 * Dies beinhalet auch die Zeichen \b, \f, \n, \r, \t, \v  und \uXXXX für X < 0x20
 * @param s in UTF8
 * @return
 */
std::string to_quoteJson(const std::string &s);


//template <typename T>
//std::string to_string(T t) { std::stringstream s; s << t; return s.str(); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(const std::string &t) { return t; }
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
inline std::string to_string(char t) { if (t) return  std::string() + t; return {}; }
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(signed char t) { return to_string(char(t)); }
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(unsigned char t) { return to_string(char(t)); }
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(char16_t t) { if (t) return to_string(std::u16string() + t); return {}; }
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(char32_t t) { if (t) return to_string(std::u32string() + t); return {}; }
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(wchar_t t) { if (t) return to_string(std::wstring() + t); return {}; }
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(int t) { return std::to_string(t); }
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(short int t) { return std::to_string(t); }
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(long int t) { return std::to_string(t); }
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(long long int t) { return std::to_string(t); }
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(unsigned int t) { return std::to_string(t); }
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(unsigned short int t) { return std::to_string(t); }
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(unsigned long int t) { return std::to_string(t); }
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(unsigned long long int t) { return std::to_string(t); }
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(bool t) { return t ? u8"true":u8"false"; }
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(const char *t) { return to_string(std::string(t)); }
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(const wchar_t *t) { return to_string(std::wstring(t)); }
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(const char16_t *t) { return to_string(std::u16string(t)); }
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(const char32_t *t) { return to_string(std::u32string(t)); }
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





/// \brief Konvertierung nach std::wstring
/// @param t Wert
/// @return Wert als std::wstring
std::wstring to_wstring(const std::string &t);
/// \brief Konvertierung nach std::wstring
/// @param t Wert
/// @return Wert als std::wstring
std::wstring to_wstring(const std::u32string &t);
/// \brief Konvertierung nach std::wstring
/// @param t Wert
/// @return Wert als std::wstring
std::wstring to_wstring(const std::u16string &t);
/// \brief Konvertierung nach std::wstring
/// @param t Wert
/// @return Wert als std::wstring
inline std::wstring to_wstring(const std::wstring &t) { return t; }
/// \brief Konvertierung nach std::wstring
/// @param t Wert
/// @return Wert als std::wstring
inline std::wstring to_wstring(wchar_t t) { if (t < 0 or t > 0x10FFFF) return L"\uFFFD"; else if (t) return std::wstring() + t; return {}; }
/// \brief Konvertierung nach std::wstring
/// @param t Wert
/// @return Wert als std::wstring
inline std::wstring to_wstring(char t) { return to_wstring(wchar_t(t)); }
/// \brief Konvertierung nach std::wstring
/// @param t Wert
/// @return Wert als std::wstring
inline std::wstring to_wstring(signed char t) { return to_wstring(wchar_t(t)); }
/// \brief Konvertierung nach std::wstring
/// @param t Wert
/// @return Wert als std::wstring
inline std::wstring to_wstring(unsigned char t) { return to_wstring(wchar_t(t)); }
/// \brief Konvertierung nach std::wstring
/// @param t Wert
/// @return Wert als std::wstring
inline std::wstring to_wstring(char16_t t) { return to_wstring(wchar_t(t)); }
/// \brief Konvertierung nach std::wstring
/// @param t Wert
/// @return Wert als std::wstring
inline std::wstring to_wstring(char32_t t) { return to_wstring(wchar_t(t)); }
/// \brief Konvertierung nach std::wstring
/// @param t Wert
/// @return Wert als std::wstring
inline std::wstring to_wstring(int t) { return std::to_wstring(t); }
/// \brief Konvertierung nach std::wstring
/// @param t Wert
/// @return Wert als std::wstring
inline std::wstring to_wstring(short int t) { return std::to_wstring(t); }
/// \brief Konvertierung nach std::wstring
/// @param t Wert
/// @return Wert als std::wstring
inline std::wstring to_wstring(long int t) { return std::to_wstring(t); }
/// \brief Konvertierung nach std::wstring
/// @param t Wert
/// @return Wert als std::wstring
inline std::wstring to_wstring(long long int t) { return std::to_wstring(t); }
/// \brief Konvertierung nach std::wstring
/// @param t Wert
/// @return Wert als std::wstring
inline std::wstring to_wstring(unsigned int t) { return std::to_wstring(t); }
/// \brief Konvertierung nach std::wstring
/// @param t Wert
/// @return Wert als std::wstring
inline std::wstring to_wstring(unsigned short int t) { return std::to_wstring(t); }
/// \brief Konvertierung nach std::wstring
/// @param t Wert
/// @return Wert als std::wstring
inline std::wstring to_wstring(unsigned long int t) { return std::to_wstring(t); }
/// \brief Konvertierung nach std::wstring
/// @param t Wert
/// @return Wert als std::wstring
inline std::wstring to_wstring(unsigned long long int t) { return std::to_wstring(t); }
/// \brief Konvertierung nach std::wstring
/// @param t Wert
/// @return Wert als std::wstring
inline std::wstring to_wstring(bool t) { return t ? L"true":L"false"; }
/// \brief Konvertierung nach std::wstring
/// @param t Wert
/// @return Wert als std::wstring
inline std::wstring to_wstring(const char *t) { return to_wstring(std::string(t)); }
/// \brief Konvertierung nach std::wstring
/// @param t Wert
/// @return Wert als std::wstring
inline std::wstring to_wstring(const wchar_t *t) { return std::wstring(t); }
/// \brief Konvertierung nach std::wstring
/// @param t Wert
/// @return Wert als std::wstring
inline std::wstring to_wstring(const char16_t *t) { return to_wstring(std::u16string(t)); }
/// \brief Konvertierung nach std::wstring
/// @param t Wert
/// @return Wert als std::wstring
inline std::wstring to_wstring(const char32_t *t) { return to_wstring(std::u32string(t)); }
/// \brief Konvertierung nach std::wstring
/// @param t Wert
/// @return Wert als std::wstring
std::wstring to_wstring(float t);
/// \brief Konvertierung nach std::wstring
/// @param t Wert
/// @return Wert als std::wstring
std::wstring to_wstring(double t);
/// \brief Konvertierung nach std::wstring
/// @param t Wert
/// @return Wert als std::wstring
std::wstring to_wstring(long double t);


//template <typename T>
/// String-Konvertierung
/// @param val Wert in utf8
/// @return Wert als wstring
//inline std::wstring to_wstring(T t) { return to_wstring(to_string(t)); }

/// \private
//template <>




template <typename T>
/// \brief Konvertierung von std::string
/// @param str Konvertierter Wert
/// @param t Wert
/// @return true, wenn fehlerfrei
inline bool string2x(const std::string &str, T &t) {std::stringstream s; s.str(str); s >> t; return s.eof() and not s.bad() and not s.fail(); }
/// \private
template <> inline bool string2x(const std::string &str, std::string &t) { t = str; return true; }
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

template <typename T>
/// \brief Konvertierung von std::wstring
/// @param wstr Konvertierter Wert
/// @param t Wert
/// @return true, wenn fehlerfrei
inline bool wstring2x(const std::wstring &wstr, T &t) {std::stringstream s; s.str(mobs::to_string(wstr)); s >> t; return s.eof() and not s.bad() and not s.fail(); }
/// \private
template <> inline bool wstring2x(const std::wstring &wstr, std::string &t) { t = to_string(wstr); return true; }
/// \private
template <> inline bool wstring2x(const std::wstring &wstr, std::wstring &t) { t = wstr; return true; }
/// \private
template <> bool wstring2x(const std::wstring &wstr, std::u32string &t);
/// \private
template <> bool wstring2x(const std::wstring &wstr, std::u16string &t);

// ist typename ein Character-Typ
template <typename T>
/// \brief prüft, ob der Typ T aus Text besteht - also zB. in JSON in Hochkommata steht
inline bool mobschar(T) { return not std::numeric_limits<T>::is_specialized; }
/// \private
template <> inline bool mobschar(char) { return true; }
/// \private
template <> inline bool mobschar(char16_t) { return true; }
/// \private
template <> inline bool mobschar(char32_t) { return true; }
/// \private
template <> inline bool mobschar(wchar_t) { return true; }
/// \private
template <> inline bool mobschar(unsigned char) { return true; }
/// \private
template <> inline bool mobschar(signed char) { return true; }

template <class T>
/// \private
inline T mobsempty(const T&t) { return t; };
/// \private
template <> inline char mobsempty(const char &) { return ' '; };
/// \private
template <> inline char16_t mobsempty(const char16_t &) { return u' '; };
/// \private
template <> inline char32_t mobsempty(const char32_t &) { return U' '; };
/// \private
template <> inline wchar_t mobsempty(const wchar_t &) { return L' '; };
/// \private
template <> inline unsigned char mobsempty(const unsigned char &) { return ' '; };
/// \private
template <> inline signed char mobsempty(const signed char &) { return ' '; };



/// Hilfsklasse für Konvertierungsklasse
class ConvToStrHint {
public:
  ConvToStrHint() = delete;
  ConvToStrHint(const ConvToStrHint&) = default;
  ConvToStrHint &operator=(const ConvToStrHint&) = default;
  ConvToStrHint(ConvToStrHint &&) = default;
  /// Konstruktor
  /// @param print_compact führt bei einigen Typen zur vereinfachten Ausgabe als ganzzahliger Wert
  /// @param altNames verwende alternative Namen wenn Vorhanden
  /// @param pfix verwende den Prefix vor dem Namen, falls vorhanden
  /// @param lowercase wandelt den Namen in Kleinbuchstaben
  explicit ConvToStrHint(bool print_compact, bool altNames = false, bool pfix = false, bool lowercase = false) : comp(print_compact),
                                                                                                                 altnam(altNames), prefix(pfix), toLower(lowercase) {}
  virtual ~ConvToStrHint() = default;
  /// \private
  virtual bool compact() const { return comp; }
  /// \private
  virtual bool useAltNames() const { return altnam; }
  /// \private
  virtual bool usePrefix() const { return prefix; }
  /// \private
  bool withIndentation() const { return indent; }
  /// \private
  bool toLowercase() const { return toLower; }

protected:
  /// \private
  bool comp = false;
  /// \private
  bool altnam = false;
  /// \private
  bool indent = false;
  /// \private
  bool prefix = false;
  /// \private
  bool toLower = false;
};

/// Hilfsklasse für Konvertierungsklasse - Basisklasse
class ConvFromStrHint {
public:
  ConvFromStrHint() = default;
  ConvFromStrHint(const ConvFromStrHint &) = default;
  ConvFromStrHint &operator=(const ConvFromStrHint &) = default;
  virtual ~ConvFromStrHint() = default;
  /// darf ein nicht-kompakter Wert als Eingabe fungieren
  virtual bool acceptExtended() const = 0;
  /// darf ein kompakter Wert (Zahlenwert bei MobsEnum oder Datum) als Eingabe fungieren
  virtual bool acceptCompact() const = 0;

  /// Standard Konvertierungshinweis, kompakte und erweiterte Eingaben sind erlaubt
  static const ConvFromStrHint &convFromStrHintDflt;
  /// Konvertierungshinweis, der nur erweiterte Eingabe zulässt
  static const ConvFromStrHint &convFromStrHintExplizit;

};
class CryptBufBase;
/// Ausgabeformat für \c to_string() Methode von Objekten
class ConvObjToString : virtual public ConvToStrHint {
public:
  /// Funktionssignatur für die Crypto-Funktion
  using EncrypFun = std::function<mobs::CryptBufBase*()>;
  ConvObjToString() : ConvToStrHint(false) {}
  /// \private
  bool toXml() const { return xml; }
  /// \private
  bool toJson() const { return not xml; }
  /// \private
  bool withQuotes() const { return quotes; }
  /// \private
  bool omitNull() const { return onull; }
  /// \private
  bool modOnly() const { return modified; }
  /// \private
  bool skipVersion() const { return skipVers; }
  /// \private
  EncrypFun getEncFun() const { return encryptor; }
  /// Ausgabe als XML-Datei
  ConvObjToString exportXml() const { ConvObjToString c(*this); c.xml = true; return c; }
  /// Ausgabe als JSON
  ConvObjToString exportJson() const { ConvObjToString c(*this); c.xml = false; c.quotes = true; return c; }
  /// Verwende alternative Namen
  ConvObjToString exportAltNames() const { ConvObjToString c(*this); c.altnam = true; return c; }
  /// Export mit Einrückungen
  ConvObjToString doIndent() const { ConvObjToString c(*this); c.indent = true; return c; }
  /// Export ohne Einrückungen
  ConvObjToString noIndent() const { ConvObjToString c(*this); c.indent = false; return c; }
  /// Elementbezeichner in Kleinbuchstaben
  ConvObjToString exportLowercase() const { ConvObjToString c(*this); c.toLower = true; return c; }
  /// Verwende den Prefix zum Namen
  ConvObjToString exportPrefix() const { ConvObjToString c(*this); c.prefix = true; return c; }
  /// Verwende native Bezeichner von enums und Zeiten
  ConvObjToString exportCompact() const { ConvObjToString c(*this); c.comp = true; return c; }
  /// Ausgabe im Klartext von enums und Uhrzeit
  ConvObjToString exportExtended() const { ConvObjToString c(*this); c.comp = false; return c; }
  /// Ausgabe von null-Werten überspringen
  ConvObjToString exportWoNull() const { ConvObjToString c(*this); c.onull = true; return c; }
  /// Ausgabe von Elementen mit Modified-Flag
  ConvObjToString exportModified() const { ConvObjToString c(*this); c.modified = true; return c; }
  /// Überspringe Versions-Elemente
  ConvObjToString exportSkipVersion() const { ConvObjToString c(*this); c.skipVers = true; return c; }
  /// setze Encrypt-Function
  ConvObjToString setEncryptor(EncrypFun e) const { ConvObjToString c(*this); c.encryptor = e; return c; }
private:
  bool xml = false;
  bool quotes = false;
  bool onull = false;
  bool modified = false;
  bool skipVers = false;
  EncrypFun encryptor = nullptr;
};

/// Konfiguration für \c string2Obj
class ConvObjFromStr : virtual public ConvFromStrHint {
public:
  /// Funktionssignatur für die Crypto-Funktion
  using DecrypFun = std::function<mobs::CryptBufBase*(const std::string &algorithm, const std::string &keyName)>;
  /// Enums für Behandlung NULL-Werte
  enum Nulls { ignore, omit, clear, force, except };
  /// Eingabe ist im XML-Format
  virtual bool acceptXml() const { return xml; }
  /// darf ein kompakter Wert als Eingabe fungieren
  bool acceptCompact() const override { return compact; }
  /// darf ein nicht-kompakter Wert als Eingabe fungieren
  bool acceptExtended() const override { return extended; };
  /// Verwende alternative Namen
  virtual bool acceptAltNames() const { return altNam; };
  /// Verwende original Namen
  virtual bool acceptOriNames() const { return oriNam; };
  /// setze Arraysize auf letztes Element
  virtual bool shrinkArray() const { return shrink; };
  /// werfe eine Exception bei unbekannter Variable
  virtual bool exceptionIfUnknown() const { return exceptUnk; };
  /// Groß-/Kleinschreibung ignorieren
  virtual bool caseInsensitive() const { return ignCase; };
  /// Null-Werte behandeln
  virtual enum Nulls nullHandling() const { return null; };
  /// \private
  DecrypFun getDecFun() const { return decryptor; }
  /// Verwenden den XML-Parser
  ConvObjFromStr useXml() const { ConvObjFromStr c(*this); c.xml = true; return c; }
  /// Werte in Kurzform akzeptieren (zB. Zahl anstatt enum-Text)
  ConvObjFromStr useCompactValues() const { ConvObjFromStr c(*this); c.compact = true; c.extended = false; return c; }
  /// Werte in Langform akzeptieren (zB. enum-Text, Datum)
  ConvObjFromStr useExtendedValues() const { ConvObjFromStr c(*this); c.compact = false; c.extended = true; return c; }
  /// Werte in beliebiger Form akzeptieren
  ConvObjFromStr useAutoValues() const { ConvObjFromStr c(*this); c.compact = true; c.extended = true; return c; }
  /// Nur Original-Namen akzeptieren
  ConvObjFromStr useOriginalNames() const { ConvObjFromStr c(*this); c.oriNam = true; c.altNam = false; return c; }
  /// Nur Alternativ-Namen akzeptieren
  ConvObjFromStr useAlternativeNames() const { ConvObjFromStr c(*this); c.oriNam = false; c.altNam = true; return c; }
  /// Original oder Alternativ-Namen akzeptieren
  ConvObjFromStr useAutoNames() const {  ConvObjFromStr c(*this); c.oriNam = true; c.altNam = true; return c; }
  /// Vektoren beim Schreiben entsprechend verkleinern
  ConvObjFromStr useDontShrink() const {  ConvObjFromStr c(*this); c.shrink = false; return c; }
  /// null-Elemente werden beim Einlesen abhängig von \c nullAllowed  auf null gesetzt im anderen Fall wird eine exception geworfen
  ConvObjFromStr useExceptNull() const {  ConvObjFromStr c(*this); c.null = except; return c; }
  /// null-Elemente werden beim Einlesen generell überlesen
  ConvObjFromStr useOmitNull() const {  ConvObjFromStr c(*this); c.null = omit; return c; }
  /// null-Elemente werden beim Einlesen anhand \c nullAllowed behandelt also ignoriert, wenn nullAllowed = false
  ConvObjFromStr useIgnoreNull() const {  ConvObjFromStr c(*this); c.null = ignore; return c; }
  /// null-Elemente werden beim Einlesen anhand \c nullAllowed behandelt und Leer gesetzt, wenn nullAllowed = false
  ConvObjFromStr useClearNull() const {  ConvObjFromStr c(*this); c.null = clear; return c; }
  /// null-Elemente werden beim Einlesen unabhängig von \c nullAllowed  auf null gesetzt (default)
  ConvObjFromStr useForceNull() const {  ConvObjFromStr c(*this); c.null = force; return c; }
  /// wird versucht eine unbekannte Variable einzulesen, wird eine exception geworfen
  ConvObjFromStr useExceptUnknown() const {  ConvObjFromStr c(*this); c.exceptUnk = true; return c; }
  /// ignoriere beim Einlesen die Groß-/Kleinschreibung der Elementnamen
  ConvObjFromStr useIgnoreCase() const {  ConvObjFromStr c(*this); c.ignCase = true; return c; }
  /// setze Decrypt-Function
  ConvObjFromStr setDecryptor(DecrypFun d) const { ConvObjFromStr c(*this); c.decryptor = d; return c; }
protected:
  /// \private
  bool xml = false;
  /// \private
  bool compact = true;
  /// \private
  bool extended = true;
  /// \private
  bool oriNam = true;
  /// \private
  bool altNam = false;
  /// \private
  bool shrink = true;
  /// \private
  bool exceptUnk = false;
  /// \private
  bool ignCase = false;
  /// \private
  enum Nulls null = force;
  /// \private
  DecrypFun decryptor = nullptr;
};

///Template für Hilfsfunktion zum Konvertieren eines Datentyps in einen unsigned int
/// @param t zu wandelnder Typ
/// @param u Ergebnisrückgabe als Zahl
/// \return true wenn Konvertierung möglich
template<typename T> inline bool to_uint64(T t, uint64_t &u) { return false; }
///Template für Hilfsfunktion zum Konvertieren eines Datentyps in einen signed int
/// @param t zu wandelnder Typ
/// @param i Ergebnisrückgabe als Zahl
/// \return true wenn Konvertierung möglich
template<typename T> inline bool to_int64(T t, int64_t &i) { return false; }
///Template für Hilfsfunktion zum Konvertieren eines eines \c int in einen Typ t
/// \return true wenn Konvertierung möglich
template<typename T> inline bool from_number(int64_t, T &t) { return false; }
///Template für Hilfsfunktion zum Konvertieren eines eines \c uint in einen Typ t
/// \return true wenn Konvertierung möglich
template<typename T> inline bool from_number(uint64_t, T &t) { return false; }
///Template für Hilfsfunktion zum Konvertieren eines eines \c double in einen Typ t
/// \return true wenn Konvertierung möglich
template<typename T> inline bool from_number(double, T &t) { return false; }


template<> bool to_int64(int t, int64_t &i);
template<> bool to_int64(short int t, int64_t &i);
template<> bool to_int64(long int t, int64_t &i);
template<> bool to_int64(long long int t, int64_t &i);
template<> bool to_uint64(unsigned int t, uint64_t &u);
template<> bool to_uint64(unsigned short int t, uint64_t &u);
template<> bool to_uint64(unsigned long int t, uint64_t &u);
template<> bool to_uint64(unsigned long long int t, uint64_t &u);
template<> bool from_number(int64_t, int &t);
template<> bool from_number(int64_t, short int &t);
template<> bool from_number(int64_t, long int &t);
template<> bool from_number(int64_t, long long int &t);
template<> bool from_number(uint64_t, unsigned int &t);
template<> bool from_number(uint64_t, unsigned short int &t);
template<> bool from_number(uint64_t, unsigned long int &t);
template<> bool from_number(uint64_t, unsigned long long int &t);
template<> bool from_number(uint64_t, bool &t);

/** \brief Infos zum aktuellen Wert, wenn dieser als Zahl darstellbar ist.
 *
 * Der Inhalt ist unabhängig vo Zustand \c isNull und hängt nur vom definierten Datentyp ab
 * bei \c bool ist \c isUnsigned gesetzt und max = 1
 *
 * Bei isTime-Typen sind bei Jahren von 1701 bis 1969 Sonderfunktionen unter Windows implementiert wenn entweder
 * GMT verwendet wird oder nur ein Datum. Der maximale Wert beträgt
 * 2262-04-11T23:47:16.854775Z oder 9223372036854775 µs = INT64MAX ns
 */
class MobsMemberInfo {
public:
  bool isUnsigned = false; ///< hat Wert von typ \c signed, i64, min und max sind gesetzt
  bool isSigned = false; ///< wht Wert vom Typ \c unsigned, u64 und max sind gesetzt
  bool isFloat = false; ///< wht Wert vom Typ \c Fließkommazahl, d ist gesetzt
  bool isTime = false; ///< Wert ist Millisekunde seit Unix-Epoche i64 ist gesetzt
  bool isEnum = false; ///<  Wert ist ein enum -> besser als Klartext darstellen \see acceptExtended
  bool is_specialized = false; ///< is_specialized aus std::numeric_limits
  bool isBlob = false; ///< Binär-Daten-Objekt
  bool hasCompact = false; ///< hat member Compact-Modus
  int64_t i64 = 0; ///< Inhalt des Wertes wenn signed (unabhängig von \c isNull)
  uint64_t u64 = 0; ///< Inhalt des Wertes wenn unsigned oder Blob-Size in Bytes (unabhängig von \c isNull)
  int64_t t64 = 0; ///< Inhalt des Wertes wenn isTime in Mikrosekunden seit Epoche (unabhängig von \c isNull)
  int64_t min = 0; ///< Minimalwert des Datentyps
  uint64_t max = 0; ///< Maximalwert des Datentyps
  uint64_t granularity = 0; ///< Körnung des Datentyps nur bei \c isTime; 1 = Mikrosekunden
  const void *blob = nullptr; ///< Inhalt des Wertes wenn isBlob
  double d = 0.0; ///< Inhalt des Wertes wenn isFloat
  unsigned int size = 0; ///< sizeof wenn is_specialized
  std::string (* eToStr)(int) = nullptr;
  /// fülle ein Time-Struct mit local time (nur wenn isTime == true)
  void toLocalTime(struct ::tm &ts) const;
  /// fülle ein Time-Struct mit gmt (nur wenn isTime == true)
  void toGMTime(struct ::tm &ts) const;
  /// setze i64 aus einem Time-Struct mit local time (nur wenn isTime == true)
  void fromLocalTime(struct ::tm &ts);
  /// setze i64 aus einem Time-Struct mit gmt (nur wenn isTime == true)
  void fromGMTime(struct ::tm &ts);
  /// setzte Zeit-Wert in Mikrosekunden
  void setTime(int64_t t);
  /// setze Int-Wert
  /// \throw runtime_error, wenn out of range oder kein int
  void setInt(int64_t t);
  /// setze Int-Wert
  /// \throw runtime_error, wenn out of range oder kein int
  void setUInt(uint64_t t);
  /// setze Bool-Wert
  /// \throw runtime_error, wenn kein bool
  void setBool(bool t);
  /// passt die Info entsprechen den compact-Mode an
  void changeCompact(bool compact);
  /// ist Zahl (signed/unsigned/time/bool/float)
  bool isNumber() const;
};

class MemberBase;

/** \brief Erweiterte \c MobsMemberInfo die den Wert als String enthält, falls nicht anders darstellbar
 *
 */
class MobsMemberInfoDb : public MobsMemberInfo {
public:
  MobsMemberInfoDb() = default;
  /// Konstruktor Text
  explicit MobsMemberInfoDb(const std::string &t) : MobsMemberInfo(), text(t) { }
  /// Konstruktor MemberInfo
  explicit MobsMemberInfoDb(const MobsMemberInfo &m) : MobsMemberInfo(m), text() { }
  /** \brief Ausgabe als Text
   *
   * @param needQuotes Rückgabe, ob Quoting erforderlich, falls nicht \c nullptr
   * @return MemberInfo als Text
   */
  std::string toString(bool *needQuotes = nullptr) const;
  std::string text; ///< Membervariable für Text
};

/// Interne Struktur zur Ablage von Query-Informationen
struct QueryInfo {
  /** Konstruktor
   *
   * @param mem Zeiger aud MobsMenberVariable
   * @param oper Zeiger auf Oerator z.B. "<=" oder "IN"
   */
  QueryInfo(const MemberBase *mem, const char *oper) : mem(mem), op(oper) { }
  const MemberBase *mem;  ///< Zeiger aud MobsMenberVariable
  const char *op;         ///< Zeiger auf Oerator z.B. "<=" oder "IN"
  std::list<MobsMemberInfoDb> content{}; ///< Liste der konstanten Parameter zur Operation
};


/// Basis der Konvertierungs-Klasse für Serialisierung von und nach std::string
class StrConvBase {
public:
  /// Angabe, ob die Ausgabe als Text erfolgt (quoting, escaping nötig)
  static inline bool c_is_chartype(const ConvToStrHint &) { return true; }
  /// zeigt an, ob spezialisierter Typ vorliegt (\c \<limits>)
  static inline bool c_is_specialized() { return false; }
  /// zeigt an, ob des Element ein binäres Datenobjekt ist
  static inline bool c_is_blob() { return false; }
  /// zeigt an, ob des Element ein MOBSENUM ist
  static inline bool c_is_mobsEnum() { return false; }
  /// Körnung des Datentyps wenn es ein Date-Typ ist, \c 0 sonst
  static inline uint64_t c_time_granularity() { return 0; }
  /// \private
  static inline uint64_t c_max() { return 0; }
  /// \private
  static inline int64_t c_min() { return 0; }
};

/// Basis der Konvertierungs-Klasse Typabhängig für Serialisierung von und nach std::string
template <typename T>
class StrConvBaseT : public StrConvBase {
public:
  /// Angabe, ob die Ausgabe als Text erfolgt (quoting, escaping nötig)
  static inline bool c_is_chartype(const ConvToStrHint &) { return true; }
  /// zeigt an, ob spezialisierter Typ vorliegt (\c \<limits>)
  static inline bool c_is_specialized() { return false; }
  /// zeigt an, ob des Element ein binäres Datenobjekt ist
  static inline bool c_is_blob() { return false; }
  /// zeigt an, ob des Element ein MOBSENUM ist
  static inline bool c_is_mobsEnum() { return false; }
  /// Körnung des Datentyps wenn es ein Date-Typ ist, \c 0 sonst
  static inline uint64_t c_time_granularity() { return 0; }
  /// \private
  static inline uint64_t c_max() { return 0; }
  /// \private
  static inline int64_t c_min() { return 0; }
  /// liefert eine \e leere Variable die zum löschen oder Initialisieren einer Membervariablen verwendet wird
  /// \see clear()
  static inline T c_empty() { return mobsempty(T()); }
  /// Einlesen von int_64_t
  static bool c_from_int(int64_t i, T &t) { return false; }
  /// Einlesen von int_64_t
  static bool c_from_uint(uint64_t u, T &t) { return false; }
  /// Einlesen von double
  static bool c_from_double(double d, T &t) { return false; }
  /// Wandeln in double
  static bool c_to_double(T t, double &) { return false; }
  /// Wandeln in uint64
  static bool c_to_uint64(T t, uint64_t &) { return false; }
  /// Wandeln in int64
  static bool c_to_int64(T t, int64_t &) { return false; }
  /// Zeiger auf Byte Array
  static bool c_to_blob(const T &t, const void *&, uint64_t &) { return false; }
  /// Blob einlesen
  static inline bool c_from_blob(const void *p, uint64_t sz, T &) { return false; }
  /// Wandeln in MTime
  static bool c_from_mtime(int64_t i, T &t) { return false; }
  /// Einlesen von MTime
  static bool c_to_mtime(T t, int64_t &) { return false; }
  /// Ausgabe enum als string
  static std::string c_to_str(int) { return {}; }
};

template <typename T>
/// Standard Konvertierungs-Klasse für Serialisierung von und nach std::string
class StrConv : public StrConvBaseT<T> {
public:
  /// liest eine Variable aus einem \c std::string
  static inline bool c_string2x(const std::string &str, T &t, const ConvFromStrHint &) { return mobs::string2x(str, t); }
  /// liest eine Variable aus einem \c std::wstring
  static inline bool c_wstring2x(const std::wstring &wstr, T &t, const ConvFromStrHint &cth) { return c_string2x(mobs::to_string(wstr), t, cth); }
  /// Wandelt eine Variable in einen \c std::string um
  static inline std::string c_to_string(const T &t, const ConvToStrHint &) { return to_string(t); };
  /// Wandelt eine Variable in einen \c std::wstring um
  static inline std::wstring c_to_wstring(const T &t, const ConvToStrHint &) { return to_wstring(t); };
//  static inline bool c_blob_ref(const T &t, const char *&ptr, size_t &sz) { return false; }
  /// Angabe, ob die Ausgabe als Text erfolgt (quoting, escaping nötig)
  static inline bool c_is_chartype(const ConvToStrHint &) { return mobschar(T()); }
  /// Größter möglicher Wert
  static inline uint64_t c_max() { uint64_t u; int64_t i; if (c_to_int64(std::numeric_limits<T>::max(), i)) return uint64_t(i);
    return c_to_uint64(std::numeric_limits<T>::max(), u) ? u : 0; }
  /// Kleinster möglicher Wert
  static inline int64_t c_min() { uint64_t u; int64_t i; if (c_to_int64(std::numeric_limits<T>::min(), i)) return i;
    return c_to_uint64(std::numeric_limits<T>::min(), u) ? int64_t(u) : 0; }
  /// Wandeln in uint64
  static inline bool c_to_uint64(T t, uint64_t &u) { return to_uint64(t, u); }
  /// Wandeln in int64
  static inline bool c_to_int64(T t, int64_t &i) { return to_int64(t, i); }
  /// Einlesen von int_64_t
  static inline bool c_from_int(int64_t i, T &t) { return from_number(i, t); }
  /// Einlesen von int_64_t
  static inline bool c_from_uint(uint64_t u, T &t) { return from_number(u, t); }
};

template <>
/// \private
class StrConv<bool> : public StrConvBaseT<bool> {
public:
  /// \private
  static inline bool c_string2x(const std::string &str, bool &t, const ConvFromStrHint &) { return mobs::string2x(str, t); }
  /// \private
  static inline bool c_wstring2x(const std::wstring &wstr, bool &t, const ConvFromStrHint &cth) { return c_string2x(mobs::to_string(wstr), t, cth); }
  /// \private
  static inline std::string c_to_string(const bool &t, const ConvToStrHint &) { return to_string(t); };
  /// \private
  static inline std::wstring c_to_wstring(const bool &t, const ConvToStrHint &) { return to_wstring(t); };
  /// \private
  static inline bool c_is_chartype(const ConvToStrHint &) { return false; }
  /// \private
  static inline uint64_t c_max() { return 1; }
  /// \private
  static inline bool c_to_uint64(bool t, uint64_t &u) { u = t ? 1:0; return true; }
  /// \private
  static inline bool c_from_uint(uint64_t u, bool &t) { t = (u != 0); return true; }
};

template <>
/// \private
class StrConv<float> : public StrConvBaseT<float> {
public:
  /// \private
  static  bool c_string2x(const std::string &str, float &t, const ConvFromStrHint &);
  /// \private
  static  bool c_wstring2x(const std::wstring &wstr, float &t, const ConvFromStrHint &cth);
  /// \private
  static inline std::string c_to_string(const float &t, const ConvToStrHint &) { return to_string(t); };
  /// \private
  static inline std::wstring c_to_wstring(const float &t, const ConvToStrHint &) { return to_wstring(t); };
  /// \private
  static inline bool c_is_chartype(const ConvToStrHint &) { return false; }
  /// \private
  static inline bool c_to_double(float t, double &d) { d = t; return true; }
  /// \private
  static inline bool c_from_double(double d, float &t) { t = static_cast<float>(d); return true; }
};



template <>
/// \private
class StrConv<double> : public StrConvBaseT<double> {
public:
  /// \private
  static bool c_string2x(const std::string &str, double &t, const ConvFromStrHint &);
  /// \private
  static bool c_wstring2x(const std::wstring &wstr, double &t, const ConvFromStrHint &cth);
  /// \private
  static inline std::string c_to_string(const double &t, const ConvToStrHint &) { return to_string(t); };
  /// \private
  static inline std::wstring c_to_wstring(const double &t, const ConvToStrHint &) { return to_wstring(t); };
  /// \private
  static inline bool c_is_chartype(const ConvToStrHint &) { return false; }
  /// \private
  static inline bool c_to_double(double t, double &d) { d = t; return true; }
  /// \private
  static inline bool c_from_double(double d, double &t) { t = d; return true; }
};


template <>
/// \private
class StrConv<std::vector<u_char>> : public StrConvBaseT<std::vector<u_char>> {
public:
  /// \private
  static bool c_string2x(const std::string &str, std::vector<u_char> &t, const ConvFromStrHint &);
  /// \private
  static bool c_wstring2x(const std::wstring &wstr, std::vector<u_char> &t, const ConvFromStrHint &);
  /// \private
  static std::string c_to_string(const std::vector<u_char> &t, const ConvToStrHint &);
  /// \private
  static std::wstring c_to_wstring(const std::vector<u_char> &t, const ConvToStrHint &);
  /// \private
  static inline bool c_is_blob() { return true; }
  /// \private
  static bool c_to_blob(const std::vector<u_char> &t, const void *&p, uint64_t &sz);
  /// \private
  static bool c_from_blob(const void *p, uint64_t sz, std::vector<u_char> &t);
};

template <typename T>
/// Konvertierungs-Klasse für enums mit Ein-/Ausgabe als \c int
class StrIntConv : public StrConvBaseT<T> {
public:
  /// \private
  static inline bool c_string2x(const std::string &str, T &t, const ConvFromStrHint &) { int i; if (not mobs::string2x(str, i)) return false; t = T(i); return true; }
  /// \private
  static inline bool c_wstring2x(const std::wstring &wstr, T &t, const ConvFromStrHint &) { int i; if (not mobs::wstring2x(wstr, i)) return false; t = T(i); return true; }
  /// \private
  static inline std::string c_to_string(const T &t, const ConvToStrHint &) { return std::to_string(int(t)); }
  /// \private
  static inline std::wstring c_to_wstring(const T &t, const ConvToStrHint &) { return std::to_wstring(int(t)); };
  /// \private
  static inline bool c_is_chartype(const ConvToStrHint &) { return false; }
  /// \private
  static inline bool c_is_specialized() { return false; }
  /// \private
  static inline uint64_t c_max() { return std::numeric_limits<unsigned short int>::max(); }
  /// \private
  static inline T c_empty() { return T(0); }
};


}

#pragma clang diagnostic pop

#endif

