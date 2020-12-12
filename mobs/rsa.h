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

/** \file rsa.h
 *
 *  \brief Plugins und Funktionen für RSA/AES-Verschlüsselung
 */

#ifndef MOBS_RSA_H
#define MOBS_RSA_H


#include <utility>


#include "csb.h"

namespace mobs {

class CryptBufRsaData;

/** \brief Stream-Buffer zur Basisklasse CryptBufBase mit RSA-Verschlüsselung
 *
 * Dient als Plugin für mobs::CryptIstrBuf oder mobs::CryptOstrBuf
 *
 * Methode: openssl aes-256-cbc Verschlüsselung mit RSA-verschlüsselten Keys
 * Die Verschlüsselung kann für mehrere Empfänger vorgenommen werden;
 * der 16-Byte IV wird der Cipher vorangestellt
 */
class CryptBufRsa : public CryptBufBase {
public:
  using Base = std::basic_streambuf<char>;
  using char_type = typename Base::char_type;
  using Traits = std::char_traits<char_type>;
  using int_type = typename Base::int_type;

  /// Klasse für public-Key Informationen
  class PubKey {
  public:
    PubKey(std::string f, std::string i) : filename(std::move(f)), id(std::move(i)) {}
    std::string filename; ///< Dateiname der public-key Datei im PEM-Format
    std::string id; ///< Bezeichnung des Empfängers
  };
  /** \brief Konstruktor für Verschlüsselung an einen Empfänger
   *
   * @param filename Dateiname des public-keys im PEM-Format
   * @param id Bezeichnung des Empfängers
   */
  explicit CryptBufRsa(const std::string &filename, const std::string &id = "");
  /** \brief Konstruktor für Verschlüsselung an mehrere Empfänger
   *
   * @param keys Liste mit private-key-file - Id - Paaren
   */
  explicit CryptBufRsa(const std::list<PubKey> &keys);
  /** \brief Konstruktor für Entschlüsselung mit dem pub-ley
   *
   * @param filename Dateiname des private-keys im PEM-Format
   * @param cipher Schlüssel in base64 Format
   * @param passphrase Passphrase zum private-key
   */
  CryptBufRsa(const std::string &filename, const std::vector<u_char> &cipher, const std::string &passphrase);
  ~CryptBufRsa() override;
  /// Bezeichnung des Algorithmus der Verschlüsselung
  std::string name() const override { return u8"rsa-1_5"; }

  /// \private
  int_type overflow(int_type ch) override;
  /// \private
  int_type underflow() override;

  /// \private
  void finalize() override;

  /** \brief Anzahl der Empfänger für diese Verschlüsselung
    *
    * @return Anzahl der vorhandenen Empfängereinträage
    */
  size_t recipients() const override;
  /** \brief Abfrage der Id des Empfängers
   *
   * @param pos Nr. des Empfänger-Eintrages 0..(recipients() -1)
   * @return Id
   */
  std::string getRecipientId(size_t pos) const override;
  /** \brief Key zum Entschlüsseln der Nachricht eines Empfängers
   *
   * @param pos Nr. des Empfänger-Eintrages 0..(recipients() -1)
   * @return Schlüssel in base64, falls vorhanden
   */
  std::string getRecipientKeyBase64(size_t pos) const override;
  /** \brief Key zum Entschlüsseln der Nachricht eines Empfängers
   *
   * @param pos Nr. des Empfänger-Eintrages 0..(recipients() -1)
   * @return Schlüssel in binärer Form, falls vorhanden
   */
  const std::vector<u_char> &getRecipientKey(size_t pos) const;

protected:
//  virtual int sync() override;
//  virtual std::streamsize xsputn( const char_type* s, std::streamsize count ) override;

private:
  CryptBufRsaData *data;
};

/** \brief Erzeugung eines RSA Schlüsselpaares
 *
 * erzeugt ein RSA-Schlüsselpaar mit 2048 Bit. Der private-key ist des_ede3_cbc verschlüsselt
 * @param filePriv Dateiname für die zu erzeugende privat-key Datei im PEM-Format
 * @param filePup Dateiname für die zu erzeugende public-key Datei im PEM-Format
 * @param passphrase Passphrase für den private-key
 */
void generateRsaKey(const std::string &filePriv, const std::string &filePup, const std::string &passphrase);


}



#endif //MOBS_RSA_H
