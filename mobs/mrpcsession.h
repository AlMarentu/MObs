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

/** \file mrpcsession.h
\brief Session-Info für Client-Server Modul mrpc */


#ifndef MOBS_MRPCSESSION_H
#define MOBS_MRPCSESSION_H


#include <string>

namespace mobs {

class MrpcSession {
public:
  MrpcSession() = default;
  /** \brief Initialisierung bei Client mit Host-Adresse
   *
   * Wird eine sessionReuseTime benötigt, sollte die keyValidTime mindestens das doppelte betragen.
   * Die keyValidTime sollte mindestens 10 Sekunden betragen. In der Regel sind  Werte von 1 Stunde sinnvoll.
   * @param hostname hostName/IPAddress[:serviceName/port]
   */
  explicit MrpcSession(const std::string &hostname) : server(hostname) {};
  ~MrpcSession() = default;
  /// gibt den Hostnamen zurück (Teil bis :)
  std::string host() const;
  /// gibt den Port zurück (Teil nach :)
  std::string port() const;
  /// löscht die Session-bezogenen Informationen
  void clear();
  /// Anzahl Sekunden, die der Key noch gültig ist
  unsigned int keyValid() const;
  /// ist der Key/Session bereits abgelaufen
  bool expired() const;
  /// key-valid-time ist zu 80% abgelaufen
  bool keyNeedsRefresh() const;

  std::string server; ///< hostname[:Port]; die Verwaltung erfolgt in der Client-Anwendung.
  std::vector<u_char> sessionKey{}; ///< session-Key; wird vom Mrpc verwaltet
  std::string keyName; ///< Name des session-Keys; wird im Server vom Mrpc verwaltet
  u_int sessionId = 0; ///< session-Key; wird vom Mrpc verwaltet; im Server muss sie explizit im Login-Vorgang gesetzt werden
  time_t last = 0; ///< letzte Verwendung; wird vom Mrpc verwaltet
  time_t generated = 0; ///< Erzeugung des Keys; wird vom Mrpc verwaltet
  std::string info; ///< Info über Login-Informationen im Server, enthält im Client die Cipher, die an den Server gesendet wurde
  std::string publicServerKey; ///< hier kann der öffentliche Schlüssel als PEM abgelegt werden; nur in der Client-Anwendung verwendet
  int sessionReuseTime = 0; ///< Zeit in Sekunden, die eine Session nach letzter Benutzung wiederverwendet werden kann, wenn > 0; muss im Server gesetzt werden, im Client wird sie automatisch verwaltet
  int keyValidTime = 0; ///< Zeit in Sekunden, die der sessionKey seit Erzeugung gültig ist wenn > 0; muss im Server gesetzt werden, im Client wird sie automatisch verwaltet
};

}
#endif //MOBS_MRPCSESSION_H
