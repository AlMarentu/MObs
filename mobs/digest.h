// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2021 Matthias Lautner
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

/** \file digest.h
 *
 *  \brief Plugins und Funktionen für HashWert
 */

#ifndef MOBS_DIGEST_H
#define MOBS_DIGEST_H

#include "csb.h"

namespace mobs {

class CryptBufDigestData;

/** \brief Stream-Buffer als Filter für CryptBufBase ohne Veränderung mit Hash-Wert-Berechnung
 *
 * Dient als Plugin für mobs::CryptIstrBuf oder mobs::CryptOstrBuf
 *
 * Methoden siehe openssl list-message-digest-commands
 * zB.: "sha1", "sha244", "md5", ...
 *
 */
class CryptBufDigest : public CryptBufBase {
public:
  using Base = std::basic_streambuf<char>; ///< Basis-Typ
  using char_type = typename Base::char_type;  ///< Element-Typ
  using Traits = std::char_traits<char_type>; ///< Traits-Typ
  using int_type = typename Base::int_type; ///< zugehöriger int-Typ


  explicit CryptBufDigest(const std::string &algo = "");

  ~CryptBufDigest() override;

  /** \brief ermittle einen Hash-Wert über die bearbeiteten Daten
   *
   * als Algorithmen sind alle der ssh-lib zugelassenen erlaubt.
   * zB.: "sha1", "sha244", "md5", ...
   * Erlaubte Werte: openssl list-message-digest-commands
   * @param algo Algorithmus zur Hashwert-Ermittlung
   */
  void hashAlgorithm(const std::string &algo);

  /** \brief Ausgabe des ermittelten Hash-Wertes
   *
   * @return Hash-Wert als Byte-Array
   */
  const std::vector<u_char> &hash() const;

 /** \brief Ausgabe des ermittelten Hash-Wertes
   *
   * @return Hash-Wert als string
   */
  std::string hashStr() const;

    /// \private
  int_type overflow(int_type ch) override;
  /// \private
  int_type underflow() override;

  /// \private
  void finalize() override;

private:
  CryptBufDigestData *data;
};


/// ostream-Klasse zum ermitteln von Hash-Werten
class digestStream : public std::ostream {
public:
  /** \brief Konstruktor
   *
   * Methoden siehe openssl list-message-digest-commands
   * zB.: "sha1", "sha244", "md5", ...
   * @param algo Hash-Methode
   */
  explicit digestStream(const std::string &algo = "sha1");
  ~digestStream() override;

  /** \brief Ausgabe des ermittelten Hash-Wertes
 *
 * @return Hash-Wert als Byte-Array
 */
  const std::vector<u_char> &hash();

  /** \brief Ausgabe des ermittelten Hash-Wertes
    *
    * @return Hash-Wert als string
    */
  std::string hashStr();

  /** \brief Ausgabe des ermittelten Hash-Wertes als UUID
    *
    * als UUID sind nur 'sha1' (UUID Version 5) oder 'md5' (UUID Version 3) erlaubt.
    * Die Ausgabe erfolgt nach RFC 4122. Dazu muss zuerst der Identifier des Namensraumes, gefolgt vom Nutzinhalt
    * gehasht werden.
    * @return UUID String
    */
  std::string uuid();
};

/** \brief hash wert eines Strings ermitteln
 *
 * @param s String
 * @param algo Algorithmus
 * @return hashwert
 * \throws runtime_error im Fehlerfall
 */
std::string hash_value(const std::string &s, const std::string &algo = "sha1");

/** \brief hash wert eines Char-Buffers ermitteln
 *
 * @param s String
 * @param algo Algorithmus
 * @return hashwert
 * \throws runtime_error im Fehlerfall
 */std::string hash_value(const std::vector<u_char > &s, const std::string &algo = "sha1");



}


#endif //MOBS_DIGEST_H
