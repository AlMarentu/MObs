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

/** \file crypt.h
 *
 *  \brief Plugins und Funktionen für RSA/AES-Verschlüsselung
 */

#ifndef MOBS_CRYPT_H
#define MOBS_CRYPT_H


#include <utility>
#include <list>
#include <memory>


#include "csb.h"

namespace mobs {

/// Klasse für public-Key Informationen
class RecipientKey {
public:
  /** \brief Konstruktor
   *
   * @param f Dateiname der public-key Datei im PEM-Format
   * @param i Bezeichnung des Empfängers
   */
  RecipientKey(std::string pubKey, std::string from = "", std::string to = "") : keyFile(std::move(pubKey)), idFrom(std::move(from)), idTo(std::move(to)) {}
  std::string keyFile; ///< Dateiname oder PEM-Format des public-keys
  std::string idFrom; ///< Bezeichnung des Senders
  std::string idTo; ///< Bezeichnung des Empfängers
  std::vector<u_char> cipher; //< cipher
};

enum CryptKeyType {
  CryptRSA2048, //< RSA mit 2048 Bit
  CryptX25519,
  CryptECprime256v1, //< ephemeren Elliptic-Curve P256
  CryptECsecp256k1,
};

/** \brief Erzeugung eines Schlüsselpaares (Datei)
 *
 * erzeugt ein Schlüsselpaar vom angegeben Typ. Der private-key ist des_ede3_cbc verschlüsselt
 * @param type Typ des Schlüssels
 * @param filePriv Dateiname für die zu erzeugende privat-key Datei im PEM-Format
 * @param filePub Dateiname für die zu erzeugende public-key Datei im PEM-Format
 * @param passphrase Passphrase für den private-key
 * @param format Format für Ausgabe, default ist "PEM"
 */
void generateCryptoKey(enum CryptKeyType type, const std::string &filePriv, const std::string &filePub, const std::string &passphrase, const std::string &format = "PEM");

/** \brief Erzeugung eines Schlüsselpaares (String)
 *
 * erzeugt ein Schlüsselpaar vom angegeben Typ. Der private-key ist des_ede3_cbc verschlüsselt, falls eine passphrase
 * angegeben ist
 * @param type Typ des Schlüssels
 * @param priv String des erzeugten private-key im PEM-Format
 * @param pub String des erzeugten public-key im PEM-Format
 * @param passphrase Passphrase für den private-key, wenn leer dann unverschlüsselt
 */
void generateCryptoKeyMem(enum CryptKeyType type, std::string &priv, std::string &pub, const std::string &passphrase = "");

/** \brief Verschlüsselung eine Keys mit einem public Key
 *
 * Der zu verschlüsselnde Buffer darf maximal 214 Zeichen lang sein
 * @param sessionKey zu verschlüsselnde Zeichenkette
 * @param cipher verschlüsseltes Ergebnis
 * @param filePub Dateipfad eines public Keys oder der Schlüssel selbst im PEM-Format
 */
void encryptPublic(const std::vector<u_char> &sessionKey, std::vector<u_char> &cipher, const std::string &filePub);

/** \brief Entschlüsselung mit einem private Key
 *
 * @param cipher verschlüsselte Eingabe
 * @param sessionKey entschlüsselte Zeichenkette
 * @param filePriv Dateipfad eines private Keys oder der Schlüssel selbst im PEM-Format
 * @param passphrase Kennwort zum private Key
*/
void decryptPrivate(const std::vector<u_char> &cipher, std::vector<u_char> &sessionKey, const std::string &filePriv, const std::string &passphrase);

/** \brief Verschlüsselung eine Keys mit einem private Key
 *
 * Der zu verschlüsselnde Buffer darf maximal 214 Zeichen lang sein
 * @param sessionKey zu verschlüsselnde Zeichenkette
 * @param cipher verschlüsseltes Ergebnis
 * @param filePriv Dateipfad eines private Keys oder der Schlüssel selbst im PEM-Format
 * @param passphrase Kennwort zum private Key
 */void encryptPrivate(const std::vector<u_char> &sessionKey, std::vector<u_char> &cipher, const std::string &filePriv, const std::string &passphrase);

/** \brief Entschlüsselung mit einem public Key
 *
 * @param cipher verschlüsselte Eingabe
 * @param sessionKey entschlüsselte Zeichenkette
 * @param filePup Dateipfad eines public Keys oder der Schlüssel selbst im PEM-Format
 */
void decryptPublic(const std::vector<u_char> &cipher, std::vector<u_char> &sessionKey, const std::string &filePup);

/** \brief Test ob Passwort und Schlüssel OK
 *
 * @param filePriv Dateipfad eines private Keys oder der Schlüssel selbst im PEM-Format
 * @param passphrase Kennwort zum private Key
 * @return true, wenn privat key und Passwort korrekt
 */
bool checkPassword(const std::string &filePriv, const std::string &passphrase);

/** \brief Schlüsselpaar erneut ausgeben
 *
 * Aus dem privat key / File wird ein public/private Key-Paar erzeugt mit neuem Passwort.
 * So kann der öffentliche Schlüssel neu generiert werden oder das Passwort der privaten geändert
 * @param filePriv Dateipfad eines private Keys oder der Schlüssel selbst im PEM-Format
 * @param passphraseOld Kennwort zum private Key
 * @param priv String des erzeugten private-key im PEM-Format
 * @param pub String des erzeugten public-key im PEM-Format
 * @param passphraseNew neues Kennwort zum neu gespeicherten private Key
 */
void exportKey(const std::string &filePriv, const std::string &passphraseOld, std::string &priv, std::string &pub,
               const std::string &passphraseNew);

/** \brief privat key aus File in unverschlüsselten string lesen
 *
 * @param filePriv Dateipfad eines private Keys oder der Schlüssel selbst im PEM-Format
 * @param passphrase Kennwort zum private Key
 * @return priv String des erzeugten private-key im PEM-Format
 */
std::string readPrivateKey(const std::string &filePriv, const std::string &passphrase);

/** \brief public key File in string lesen
 *
 * @param filePub Dateipfad eines private Keys im PEM-Format
 * @return pub String des erzeugten public-key im PEM-Format
 * \throw runtime_error im Fehlerfall
*/
std::string readPublicKey(const std::string &filePub, std::string &pub);

/** \brief info zum Schlüssel ausgeben
 *
 * @param filePriv Dateipfad eines private Keys oder der Schlüssel selbst im PEM-Format
 * @param passphrase Kennwort zum private Key
 * @return Info in Textform oder Leerstring im Fehlerfall
 * \throw runtime_error im Fehlerfall
 */
std::string getKeyInfo(const std::string &filePriv, const std::string &passphrase);

/** \brief Fingerprint zum Public-Key
 *
 * @param filePub Dateipfad eines öffentlichen Keys oder der Schlüssel selbst im PEM-Format
 * @return MD5 Hash des Schlüssels (32 Zeichen)
 * \throw runtime_error im Fehlerfall
 */
std::string getKeyFingerprint(const std::string &filePub);

/** \brief Authentifizierungsschlüssel für Session-Key erzeugen
 *
 * @param cipher generierte Cipher die dem Server mitgeteilt wird
 * @param sessionKey generierter Session-Key der symmetrischen Verschlüsselung
 * @param filePup öffentlicher-Schlüssel des Servers
 * @param filePriv privater Schlüssel des Clients oder leer
 * @param passphrase Passwort zum privaten Schlüssel oder leer
 */
void encapsulatePublic(std::vector<u_char> &cipher, std::vector<u_char> &sessionKey, const std::string &filePup, const std::string &filePriv = "",
                       const std::string &passphrase = "");

/** \brief Authentifizierungsschlüssel für Session-Key entschlüsseln
 *
 * @param cipher Cipher die vom Client erhalten wurde
 * @param sessionKey generierter Session-Key der symmetrischen Verschlüsselung
 * @param filePriv privater Schlüssel des Server
 * @param passphrase Passwort zum privaten Schlüssel
 * @param filePup öffentlicher-Schlüssel des Clients zur Authentisierung oder leer
 */
void decapsulatePublic(const std::vector<u_char> &cipher, std::vector<u_char> &sessionKey, const std::string &filePriv,
                       const std::string &passphrase, const std::string &filePup = "");

}


#endif //MOBS_CRYPT_H
