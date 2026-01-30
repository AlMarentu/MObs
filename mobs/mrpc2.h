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

/** \file mrpc.h
\brief Framework für Client-Server Modul  */

#ifndef MOBS_MRPC2_H
#define MOBS_MRPC2_H

#include "xmlparser.h"
#include "xmlread.h"
#include "xmlwriter.h"
#include "mrpcsession.h"
#include <iostream>

namespace mobs {



/// Fehler des Clients beim Verbindungsaufbau
class Mrpc2ConnectException : public std::runtime_error {
public:
  explicit Mrpc2ConnectException(const std::string &what) : std::runtime_error(what) {}
};

/** \brief Klasse für Client-Server Modul über XML-RPC-Calls
 *
 * die Verschlüsselung der Nutzdaten ist anhand RFC 4051 implementiert.
 * Der Schlüsselaustausch erfolgt mittels Diffie-Hellman auf Basis elliptischer Kurven.
 * Zusätzlich besteht die Möglichkeit Roh-Daten zwischen den XML-Paketen zu übermitteln
 *
 * Ist im Server eine reuseTime gesetzt, so wird vom Client versucht eine bestehende Session zu verwenden.
 * gelingt dies nicht, so wird ein neuer Login-Vorgang gestartet. Der Server-Kontext geht dabei verloren.
 * Dies erlaubt eine schnelle Wiederverwendung einer bestehenden Session inklusive deren Kontext.
 * Wenn die Variable sessionReuseSpeedup gesetzt ist, so wird bei einer erfolgreichen Session-Reuse sofort ein Kommando gesendet.
 * Ist bei sessionReuseSpeedup der Session-Reuse nicht erfolgreich, so wird eine Exception geworfen.
 */
class Mrpc2 : public XmlReader {
  enum State { fresh, getPubKey, connectingServerAuthorized, connectingServer, connectingServerConfirmed, connectingClient, connected, readyRead, closing };
public:
  /** \brief Konstruktor für Client
   *
   * Soll der Server einen Reconnect anbieten, so muss session->sessionReuseTime gesetzt sein und der Server die
   * reconnectReceived-Methode implementieren.
   *
   * Bei einem Client Aufruf entscheidet der vorangegangene Aufruf darüber, ob ein reconnect versucht wird.
   * Soll die verhindert werden so muss mrpcSession->sessionId = 0 gesetzt werden.
   * Ist der mode != dontKeep, so wird, falls vom Server erlaubt, die reconnect-Info in der mrpcSession vermerkt.
   *
   * Ist der mode == speedup, so scheitert der Connect, falls die Session serverseitig nicht mehr existiert.
   * @param inStr Eingabestream
   * @param outStr Ausgabestream
   * @param mrpcSession Zeiger auf die Session-Info, darf nicht null sein
   * @param nonBlocking true, wenn statt blocking read nur die im Stream vorhandenen Daten gelesen werden sollen
   */
  Mrpc2(std::istream &inStr, std::ostream &outStr, MrpcSession *mrpcSession, bool nonBlocking);
  ~Mrpc2() override = default;

  /** \brief senden eines einzelnen Objektes mit Verschlüsselung und sync()
   *
   * @param obj zu sendendes Objekt
   */
  void sendSingle(const ObjectBase &obj);
  /// starte Verschlüsselung
  void encrypt();
  /// beende Verschlüsselung
  void stopEncrypt();

  /// für den non-blocking Modus: Rückgabe, ob ein Byte-Stream verfügbar ist
  bool inByteStreamAvail();
  /// Einlesen eines Byte-streams der Größe sz
  std::istream &inByteStream(size_t sz);
  /// Senden eines Byte-streams; der XML-Stream darf dabei nicht verschlüsselt sein
  std::ostream &outByteStream();
 /** \brief Senden eines Byte-streams beenden (ohne flush())
  *
  * Die gesendete Anzahl der Bytes sollte überprüft werden.
  * @return Anzahl der übertragenen Bytes oder -1 wenn vom darunterliegenden Stream nicht unterstützt
  */
  std::streamsize closeOutByteStream();


  /** \brief Arbeitsroutine des Clients
  *
  * die Routine muss solange aufgerufen werden bis true zurückgeliefert wird.
  * Danaach ist mindestens 1 Objekt empfangen und die XML-Ebene auf den Grundzustand zurückgesetzt.
  * @return true, wenn ein Objekt empfangen wurde und die Kommunikation abgeschlossen ist
  * \throws runtime_error wenn ein Fehler im Stream bzw im Login/Reconnect aufgetreten ist;
  */
  bool parseClient();

  /** \brief Client-Kommando zum Schließen der Kommando-Sequence
   *
   */
  void closeServer();


  /** \brief Arbeitsroutine des Server
   *
   * @return Connected-Status; d.H. der Server darf normale Anfragen bearbeiten
   * \throws runtime_error wenn ein Fehler im Stream bzw im Login/Reconnect aufgetreten ist;
   */
  bool parseServer();

  /** \brief callback für Server: Eingang einer Login-Anforderung
   *
   * Die Login-Anforderung cipher muss mit setSessionKey(cipher, keyId, serverPrivKey, passwd) quittiert werden.
   * Über die Callback-Methode getSenderPublicKey muss aus der keyId der public key des Senders ermittelt werden können
.  *
   * Bei einer exception wird die Login-Anforderung abgelehnt
   * @param cipher verschlüsselte Info des Logins
   * @param keyId id zum public key des Senders
   * \throws runtime_error bei Fehler
   */
  virtual void loginReceived(const std::vector<u_char> &cipher, const std::string &keyId) { throw std::runtime_error("loginReceived not implemented"); }

  /** \brief callback für Server wenn der Schlüssel bei bestehender Verbindung geändert wurde
  *
  * Die Login-Anforderung cipher muss mit setSessionKey(cipher, keyId, serverPrivKey, passwd) quittiert werden.
  * Über die Callback-Methode getSenderPublicKey muss aus der keyId der public key des Senders ermittelt werden können
  *
  * zusätzlich müssen in der Session-Struktur die sessionId, sessionReuseTime sowie keyValidTime gesetzt werden
. *
  * Die Methode muss nur implementiert werden, wenn ein Schlüsselwechsel benötigt wird und wenn eine abweichende Behandlung
  * zum login von Nöten ist
  * @param cipher verschlüsselte Info des Logins
  * @param keyId id zum public key des Senders
  * \throws runtime_error bei Fehler
  */
  virtual void keyChanged(const std::vector<u_char> &cipher, const std::string &keyId) { loginReceived(cipher, keyId); }

  /** \brief Callback für Server zur Ermittlung des public Keys eines Clients
   *
   * @param keyId Id des Schlüssels
   * @return PEM-String oder Filename des Schlüssels oder Leerstring, wenn unbekannt
   */
  virtual std::string getSenderPublicKey(const std::string &keyId) { return {}; }

  /** \brief callback für Server: Anfrage des Public keys
   *
   * Wird kein Schlüssel zurückgeliefert so erhält der Client eine Fehlermeldung
   * \return Rückgabe des PupKeys im PEM-Format "-----BEGIN ..." oder leer bei Fehler
   */
  virtual std::string getServerPublicKey() { return {}; }

  /** \brief Ermittle Session-Informationen aus Cipher (für Server)
   *
   * Der Schlüssel wird in der Session-Variable publicServerKey gespeichert
   * Der Server muss eine gültige Session haben, dann werden dort sessionKey, keyName (falls nicht leer), last und
   * generated (falls leer) gesetzt
   * @param cipher cipher der Client-Message
   * @param keyId Id der Client-Schlüssels (falls authentication über Client-Signatur ab openSSL 3.2)
   * @param privKey private key des Servers
   * @param passwd zugehöriges Passwort
   * \throw std::runtime_error im Fehlerfall
   */
  void setSessionKey(const std::vector<u_char> &cipher, const std::string &keyId, const std::string &privKey, const std::string &passwd);


  /// senden eines Objektes ohne flush()
  void xmlOut(const mobs::ObjectBase &obj);
  /// senden der write-Buffers
  void flush();
  /// Rückgabe, ob das zuletzt ausgewertete Objekt verschlüsselt war
  bool isEncrypted() const { return  encrypted; }

  /** \brief Verbindung hergestellt
   * @return true, wenn die Verbindung hergestellt ist
   */
  bool isConnected() const;

  /// Rückgabe, ob der nächste Lesevorgang blockiert
  bool clientAboutToRead() const;

  /// Rückgabe, ob die Session wiederverwendet werden kann (für den Server)
  bool serverKeepSession() const;

  /** \brief Starte eine Verbindung zum Server
   *
   * Danach muss parseClient verwendet werden
   * @param keyId Id des Client-Schlüssels
   * @param software Info-String des Aufrufenden Programmes
   * @param privkey private Key des Clients
   * @param passphrase zugehöriges Passwort
   * @param serverPubKey public Key des Servers
   * @param keyAuth true, wenn die Authentifikation des Clients in der Cipher eingebettet werden soll (erst ab openSSL 3.2)
   */
  void startSession(const std::string &keyId, const std::string &software, const std::string &privkey,
                    const std::string &passphrase, std::string &serverPubKey, bool keyAuth = false);

  /** \brief Erzeuge einen neuen Schlüssel mit cipher und sende ihn an den Server
   *
   * Die Verbindung zum Server muss bestehen und in beide Richtungen Idle sein
   * @param privkey private Key des Clients
   * @param passphrase zugehöriges Passwort
   * @param serverPubKey public Key des Servers
   * @param keyAuth true, wenn die Authentifikation des Clients in der Cipher eingebettet werden soll (erst ab openSSL 3.2)
   */
  void clientRefreshKey(const std::string &privkey, const std::string &passphrase, std::string &serverPubKey, bool keyAuth = false); // erst ab openSSL 3.2

  /** \brief Sende eine Anfrage an den Server um dessen public Key abzufragen
   *
   * Achtung, die Authentizität des Servers/Schlüssels muss anderweitig geprüft werden
   */
  void getPublicKey();

  mobs::CryptIstrBuf streambufI; ///< \private
  mobs::CryptOstrBuf streambufO; ///< \private
  std::wistream iStr; ///< \private
  std::wostream oStr; ///< \private
  XmlWriter writer; ///< das Writer-Objekt für die Ausgabe
  MrpcSession *session; ///< Zeiger auf eine MrpcSession - darf nicht nullptr sein
  std::unique_ptr<mobs::ObjectBase> resultObj; ///< Das zuletzt empfangene Objekt muss nach Verwendung auf nullptr gesetzt werden

  template<class T>
  /** \brief Rückgabe des zuletzt empfangenen Objektes als unique_ptr
   *
   * Beispiel:
   * auto res = client.getResult<MrpcPerson>();
   * @return unique_ptr\<T\> oder nullptr, wenn das Objekt nicht vom Typ T ist
   */
  std::unique_ptr<T> getResult();

protected:
  /// \private
  void StartTag(const std::string &element) override;
  /// \private
  void EndTag(const std::string &element) override;
  /// \private
  void Encrypt(const std::string &algorithm, const std::string &keyName, const std::string &cipher, CryptBufBase *&cryptBufp) override;
  /// \private
  void EncryptionFinished() override;
  /// \private
  void filled(mobs::ObjectBase *obj, const std::string &error) override;

private:
  bool encrypted = false;
  State state = fresh;

};

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpotentially-evaluated-expression"
template<class T>
std::unique_ptr<T> Mrpc2::getResult()
{
  if (resultObj and typeid(T) == typeid(*resultObj))
    return std::unique_ptr<T>(dynamic_cast<T *>(resultObj.release()));
  return nullptr;
}
#pragma clang diagnostic pop
} // mobs

#endif //MOBS_MRPC2_H
