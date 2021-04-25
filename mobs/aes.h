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

/** \file aes.h
 *
 *  \brief Plugins und Funktionen für AES-Verschlüsselung
 */

#ifndef MOBS_AES_H
#define MOBS_AES_H

#include "csb.h"
#include <vector>

#ifdef __MINGW32__
typedef unsigned char u_char;
#endif

namespace mobs {

class CryptBufAesData;

/** \brief Stream-Buffer zur Basisklasse CryptBufBase mit AES-Verschlüsselung
 *
 * Dient als Plugin für mobs::CryptIstrBuf oder mobs::CryptOstrBuf
 *
 * Methode: openssl aes-256-cbc -md sha1
 *
 * entspricht dem Aufruf
 * openssl aes-256-cbc  -d -in file.xml  -md sha1 -k password
 * Bei base64-Format mit zusätzlichem Parametern -a -A
 */
class CryptBufAes : public CryptBufBase {
public:
  using Base = std::basic_streambuf<char>;
  using char_type = typename Base::char_type;
  using Traits = std::char_traits<char_type>;
  using int_type = typename Base::int_type;

  /** \brief Konstruktor für Verschlüsselung mit AES-256
   *
   * Es wird die Verschlüsselungsmethode aes-256-cb mit sha1 gehashter Passphrase verwendet
   * es wird das Prefix "SALTED__" und ein 8-Zeichen Salt vorangestellt
   * @param pass Passwort
   * @param id Id des Empfängers (falls für Export benötigt)
   * @param writeIV schreibe IV zu Beginn des Ausgabestroms
   */
  explicit CryptBufAes(const std::string &pass, const std::string &id = "");
  /** \brief Konstruktor für Verschlüsselung mit AES-256 und passphrase
   *
   * Key unf IV sind vorgegeben, Ver- und Entschlüsselung möglich
   * @param key 32-Byte Schlüssel (key_size())
   * @param iv Initialer Vektor der Größe iv_size()
   * @param id Id des Empfängers (falls für Export benötigt)
   * @param writeIV wenn true, wird der iv am Anfang der Ausgebe gesetzt
   */
  explicit CryptBufAes(const std::vector<u_char> &key, const std::vector<u_char> &iv, const std::string &id = "", bool writeIV = false);
  /** \brief Konstruktor für Verschlüsselung mit AES-256
   *
   * der IV wird aus dem Beginn der Cipher extrahiert, nur für Entschlüsselung geeignet
   * @param key key 32-Byte Schlüssel (key_size())
   * @param id  Id des Empfängers (nur informativ)
   */
  explicit CryptBufAes(const std::vector<u_char> &key, const std::string &id = "");
  ~CryptBufAes() override;
  /// Länge des keys
  static size_t key_size() { return 32; };
  /// Länge des iv
  static size_t iv_size();
  /** \brief fülle alle Elemente des Vektors mit Zufallszahlen
   *
   * @param rand Vektor mit entsprechender Größe
   */
  static void getRand(std::vector<u_char> &rand);

  /// Bezeichnung der Verschlüsselung
  std::string name() const override { return u8"aes-256-cbc"; }
  /// Anzahl der Empfänger-Ids ist immer 1
  size_t recipients() const override { return 1; }
  /// liefert bei pos==0 die Id des des Empfängers wie im Konstruktor angegeben
  std::string getRecipientId(size_t pos) const override;

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

  void openSalt();

protected:
//  virtual int sync() override;
//  virtual std::streamsize xsputn( const char_type* s, std::streamsize count ) override;

private:
  CryptBufAesData *data;

  void ctxInit();

};

/** \brief verschlüsselt einen String mit AES und gibt ihn Base64 aus
 *
 * Es wird die Verschlüsselungsmethode aes-256-cb mit sha1 gehashter Passphrase verwendet
 * @param s zu verschlüsselnder Text
 * @param pass Passphrase
 * @return verschlüsselter Text in base64
 */
std::string to_aes_string(const std::string &s, const std::string &pass);

/** \brief entschlüsselt einen base64-codierten Text
 *
 * Es wird die Verschlüsselungsmethode aes-256-cb mit sha1 gehashter Passphrase verwendet
 * @param s zu entschlüsselnder Text in base64
 * @param pass Passphrase
 * @return Klartext
 * \throw runtime_error im Fehlerfall
 */
std::string from_aes_string(const std::string &s, const std::string &pass);


}


#endif //MOBS_AES_H
