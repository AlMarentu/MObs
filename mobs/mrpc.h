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

/** \file mrpc.h
\brief Framework für Client-Server Modul  */

#ifndef MOBS_MRPC_H
#define MOBS_MRPC_H

#include "xmlparser.h"
#include "xmlread.h"
#include "xmlwriter.h"
#include "mrpcsession.h"
#include <iostream>

namespace mobs {



/// Fehler des Clients beim Verbindungsaufbau
class MrpcConnectException : public std::runtime_error {
public:
  explicit MrpcConnectException(const std::string &what) : std::runtime_error(what) {}
};

/** \brief Klasse für Client-Server Modul über XML-RPC-Calls
 *
 * die Verschlüsselung der Nutzdaten ist anhand RFC 4051 implementiert.
 * Zusätzlich besteht die Möglichkeit Roh-Daten zwischen den XML-Paketen zu übermitteln
 */
class Mrpc : public XmlReader {
public:
  enum State { fresh, getPubKey, connectingClient, connectingServer, reconnectingClient, connected, readyRead, closing };
  Mrpc(std::istream &inStr, std::ostream &outStr, MrpcSession *mrpcSession, bool nonBlocking);
  ~Mrpc() override = default;

  /** \brief senden eines einzelnen Objektes mit Verschlüsselung und sync()
   *
   * @param obj zu sendendes Objekt
   */
  void sendSingle(const ObjectBase &obj);
  /// starte Verschlüsselung
  void encrypt();
  /// beende Verschlüsselung
  void stopEncrypt();

  /// Einlesen eines Byte-streams der Größe sz
  std::istream &inByteStream(size_t sz);
  /// Senden eines Byte-streams
  std::ostream &outByteStream();
  /// Senden eines Byte-streams beenden
  void closeOutByteStream();

  /** \brief Arbeitsroutine des Clients für den Initialisierungsvorgang
   *
   * die Routine muss solange aufgerufen werden bis true zurückgeliefert wird.
   * Wird ein leerer public key des Servers angegeben, so wird dieser vom Server erfragt.
   *
   * ist eine sessionReuseTime gesetzt, so wird versucht eine bestehende Session zu verwenden.
   * @param keyId Schlüssel-Id für den Login-Vorgang
   * @param software Information über den Client
   * @param privkey private key der Clients
   * @param passphrase passphrase des private keys
   * @param serverkey public key des Servers, bei leer autudetection und Rückgabe des Keys im PEM-Format
   * @return connected wenn isConnected==false dürfen keine Anfragen bearbeitet werden
   */
  bool waitForConnected(const std::string &keyId, const std::string &software, const std::string &privkey,
                        const std::string &passphrase, std::string &serverkey);
  /** \brief Versuche einen Reconnect vom Client (muss vor waitForConnected aufgerufen werden)
   *
   * obsolet: sessionReuseTime verwenden \see waitForConnected
   */
  void tryReconnect();
  /** \brief Arbeitsroutine des Clients
  *
  * die Routine muss solange aufgerufen werden bis true zurückgeliefert wird.
  * Danaach ist mindestens 1 Objekt empfangen und die XML-Ebene auf den Grundzustand zurückgesetzt.
  * @return true, wenn ein Objekt empfangen wurde und die Kommunikation abgeschlossen ist
  * \throws runtime_error wenn ein Fehler im Stream bzw im Login/Reconnect aufgetreten ist;
  */
  bool parseClient();

  /** \brief Client-Kommando zum Schließen der Session, danach ist kein reconnect möglich
   *
   */
  void closeServer();


  /** \brief Arbeitsroutine des Server
   *
   * @return Connected-Status; d.H. der Server darf normale Anfragen bearbeiten
   * \throws runtime_error wenn ein Fehler im Stream bzw im Login/Reconnect aufgetreten ist;
   */
  bool parseServer();

  /** \brief callback für Server: Eingang einer Login-Anforderung return clientKey oder "" falls kei Login
   *
   * Die Login-Anforderung cipher muss mit receiveLogin(cipher, privKey, "", ...) entschlüsselt werden.
   * Bei einer exception wird die Login-Anforderung abgelehnt
   * @param cipher verschlüsselte Info des Logins
   * @param info allgemeine Info oder Fehlermeldung
   * @return Name des public clientKey bzw. der Key im PEM-Format;  "" falls kein Login möglich
   */
  virtual std::string loginReceived(const std::vector<u_char> &cipher, std::string &info) { info = "not implemented"; return {}; }
  /** \brief callback für Server: Eingang einer reconnect-Anforderung auf eine bestehende SessionId
   *
   * @param newId gewünschte SessionId
   * @param Fehlermeldung
   * @return true wenn reconnect Ok
   */
  virtual bool reconnectReceived(u_int newId, std::string &error) { error = "not implemented"; return false; }
  /** \brief callback für Server: Anfrage des Public keys
   *
   * @param key Rückgabe des PupKeys im PEM-Format "-----BEGIN ..." oder leer bei Fehler
   * @param info Rückgabe allgemeine Serverinfo oder Fehlermeldung
   */
  virtual void getPupKeyReceived(std::string &key, std::string &info) { info = "not implemented";  }

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

  /// senden eines Objektes ohne writer.sync()
  void xmlOut(const mobs::ObjectBase &obj);
  /// Rückgabe, ob das zuletzt ausgewertete Objekt verschlüsselt war
  bool isEncrypted() const { return  encrypted; }
  /// \private
  void receiveSessionKey(const std::vector<u_char> &cipher, const std::string &privkey, const std::string &pass);
  /** \brief analysiert die empfangene Login-Information Server-seitig
   *
   * anhand der loginId kann dann der public key des Clients (Pfad oder PEM) ermittelt werden.
   * @param cipher empfangene Imformation
   * @param privkey privater Schlüssel des Servers
   * @param passwd
   * @param login Rückgabe Loginname des Caller-Systems
   * @param software Rückgabe Software des Caller-Systems
   * @param hostname Rückgabe Hostname des Caller-Systems
   * @return login-Id der Anfrage
   */
  static std::string receiveLogin(const std::vector<u_char> &cipher, const std::string &privkey, const std::string &passwd,
                                  std::string &login, std::string &software, std::string &hostname);
  /// \private erzeugt Sessionkey für Server, übernimmt sessionId; returniert cipher für receiveSessionKey()
  std::vector<u_char> generateSessionKey(const std::string &clientkey);
  /// \private erzeuge Login-Info auf Client-Seite
  static std::vector<u_char> generateLoginInfo(const std::string &keyId,
                                               const std::string &software, const std::string &serverkey);

  /** \brief Verbindung hergestellt
   * @return true, wenn die Verbindung hergestellt ist und parseClient statt waitForConnected aufgerufen werden muss
   */
  bool isConnected() const { return state == connected or state == reconnectingClient or state == readyRead; }

  /// Rückgabe, ob der nächste Lesevorgang blockiert
  bool clientAboutToRead() const;

  mobs::CryptIstrBuf streambufI; ///< \private
  mobs::CryptOstrBuf streambufO; ///< \private
  std::wistream iStr; ///< \private
  std::wostream oStr; ///< \private
  XmlWriter writer; ///< das Writer-Objekt wür die Ausgabe
  MrpcSession *session; ///< Zeiger auf ein MrpcSession - Info; muss zwingend existieren
  std::unique_ptr<mobs::ObjectBase> resultObj; ///< Das zuletzt empfangene Objekt; muss nach Verwendung auf nullptr gesetzt werden

  static int sessionReuseTime; ///< Zeit in Sekunden, die eine Session wiederverwendet werden kann

private:
  bool encrypted = false;
  State state = fresh;
  // TODO int clientSessionReuseTime vom Server beim Login empfangen
  // TODO evtl auch Ablaufzeit der Session vom Server empfangen

};

} // mobs

#endif //MOBS_MRPC_H
