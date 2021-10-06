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

/** \file mchrono.h
 \brief Optional: Wrapper für die Zeitpunkte aus std::chrono \c time_point */

#ifndef MOBS_MCHRONO_H
#define MOBS_MCHRONO_H

#include <chrono>
#include <string>

#include "objtypes.h"


namespace mobs {

using MDays = std::chrono::duration<int, std::ratio<86400>>;
using MDate = std::chrono::time_point<std::chrono::system_clock, MDays>;

/// Aktuelles Datum für MDate
MDate MDateNow();

/// \private
template <> bool string2x(const std::string &str, MDate &t);


/// Konvertier-Funktion \c MDate nach \c std::string im Format ISO8601
std::string to_string(MDate);


/// Konvertier-Funktion \c MDate nach \c std::string im Format ISO8601
inline std::wstring to_wstring(MDate t) {
  return to_wstring(to_string(t));
}

/// Konvertierung Tage nach Epoche in MDate
template<> bool from_number(int64_t , MDate &t);
/// Konvertierung MDate nach Tage nach Epoche
template<> bool to_int64(MDate t, int64_t &i);


/// \private
template<>
class StrConv<MDate> : public StrConvBaseT<MDate> {
public:
  /// \private
  static bool c_string2x(const std::string &str, MDate &t, const ConvFromStrHint &);

  /// \private
  static inline bool c_wstring2x(const std::wstring &wstr, MDate &t, const ConvFromStrHint &cth) { return c_string2x(mobs::to_string(wstr), t, cth); }

  /// \private
  static std::string c_to_string(const MDate &t, const ConvToStrHint &cth);

  /// \private
  static std::wstring c_to_wstring(const MDate &t, const ConvToStrHint &cth);

  /// \private
  static inline bool c_is_chartype(const ConvToStrHint &cth) { return not cth.compact(); }

  /// \private
  static inline uint64_t c_time_granularity() { return 86400000000; } // returning Seconds

  /// \private
  static inline MDate c_empty() { return MDate{}; }

  /// \private
  static inline uint64_t c_max() { return INT32_MAX; }

  /// \private
  static inline int64_t c_min() { return INT32_MIN; }

  /// \private
  static bool c_from_int(int64_t i, MDate &t);

  /// \private
  static bool c_to_int64(MDate t, int64_t &i);

  /// \private
  static bool c_from_mtime(int64_t i, MDate &t);

  /// \private
  static bool c_to_mtime(MDate t, int64_t &i);
};




using MTime = std::chrono::time_point<std::chrono::system_clock, std::chrono::microseconds>;

/// Aktuelle Zeit für MTime
MTime MTimeNow();

/** \brief Konvertiere Zeit-String in Ansi/ISO8601 nach MTime
 *
 * ist keine Zeitzone angegeben, so wird dir lokale Zeit angenommen.
 * @param str zu konvertierende Zeichenkette
 * @param t Rückgabe MTime
 * @return true, wenn Umwandlung fehlerfrei
 */
template <> bool string2x(const std::string &str, MTime &t);

/// Konvertier-Funktion \c MTime nach \c std::wstring im Format ISO8601
std::string to_string(MTime t);

/// Konvertier-Funktion \c MTime nach \c std::string im Format ISO8601
std::wstring to_wstring(MTime t);

/// Konvertierung Mikrosekunden nach Epoche in MTime
template<> bool from_number(int64_t , MTime &t);
/// Konvertierung MTime nach Mikrosekunden nach Epoche
template<> bool to_int64(MTime t, int64_t &i);


/// Enums für to_string Methoden bei Typ MTime
enum MTimeFract {MYear, MMonth, MDay, MHour, MMinute, MSecond, MF1, MF2, MF3, MF4, MF5, MF6};
/** \brief Konvertier-Funktion \c MTime nach \c std::string im Format ISO8601
 *
 * @param t Zeitpunkt
 * @param f Fraktion, bis zu der ausgegeben werden soll
 * @return Text der lokalen Zeit im Format ISO8601
 */
std::string to_string_iso8601(MTime t, MTimeFract f = MF6);

/** \brief Konvertier-Funktion \c MTime nach \c std::string im Format ISO8601
 *
 * @param t Zeitpunkt
 * @param f Fraktion, bis zu der ausgegeben werden soll
 * @return Text GMT im Format ISO8601
 */
 std::string to_string_gmt(MTime t, MTimeFract f = MF6);

/** \brief Konvertier-Funktion \c MTime nach \c std::string im Format ISO8601
 *
 * @param t Zeitpunkt
 * @param f Fraktion, bis zu der ausgegeben werden soll
 * @return Text der lokalen Zeit im Format Ansi
 */
 std::string to_string_ansi(MTime t, MTimeFract f = MF6);



/// \private
template<>
class StrConv<MTime> : public StrConvBaseT<MTime> {
public:
  /// \private
  static bool c_string2x(const std::string &str, MTime &t, const ConvFromStrHint &);

  /// \private
  static inline bool c_wstring2x(const std::wstring &wstr, MTime &t, const ConvFromStrHint &cth) { return c_string2x(mobs::to_string(wstr), t, cth); }

  /// \private
  static std::string c_to_string(const MTime &t, const ConvToStrHint &cth);

  /// \private
  static std::wstring c_to_wstring(const MTime &t, const ConvToStrHint &cth);

  /// \private
  static inline bool c_is_chartype(const ConvToStrHint &cth) { return not cth.compact(); }

  /// \private
  static inline uint64_t c_time_granularity() { return 1; }

  /// \private
  static inline MTime c_empty() { return MTime{}; }

  /// \private
  static inline uint64_t c_max() { return INT64_MAX; }

  /// \private
  static inline int64_t c_min() { return INT64_MIN; }

  /// \private
  static bool c_from_int(int64_t i, MTime &t) { return c_from_mtime(i, t); }

  /// \private
  static bool c_to_int64(MTime t, int64_t &i) { return c_to_mtime(t, i); }

  /// \private
  static bool c_from_mtime(int64_t i, MTime &t);

  /// \private
  static bool c_to_mtime(MTime t, int64_t &i);

};

}


#endif //MOBS_MCHRONO_H
