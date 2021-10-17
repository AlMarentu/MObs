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


/** \file tcpstream.h
 *
 *  \brief ioBuffer tcpstream
 */

#ifndef MOBS_TCPSTREAM_H
#define MOBS_TCPSTREAM_H

#include <iostream>
#ifdef __MINGW32__
// für poll muss _WIN32_WINNT >= 0x0601 sein entspricht Win 7
#undef _WIN32_WINNT
#define _WIN32_WINNT  0x0601
#include <winsock2.h>
#endif

struct sockaddr;
namespace mobs {

#ifdef __MINGW32__
class WinSock {
private:
  WinSock();
  ~WinSock();
public:
  static const WinSock *get();
  static void deleteWinSock();
private:
  static WinSock *winSock;
};

typedef SOCKET socketHandle; ///< Systemunabhängiger typ eines TCP-Sockets
const socketHandle invalidSocket = INVALID_SOCKET;
#else
typedef int socketHandle; ///< Systemunabhängiger typ eines TCP-Sockets
const socketHandle invalidSocket = -1; ///< Konstante für uninitialisierten TCP-Socket
#endif

/** \brief Klasse um eine passive TCP-Verbindung zu öffnen
 *
 */
class TcpAccept {
  friend class TcpStBuf;
public:
  /** \brief öffnet eine Port, um eine TCP-Verbindung anzunehmen \see mobs::tcpstream
   *
   * @param service TCP-Port
   * @return socketHandle (Betriebssystem abhängig) invalidSocket im Fehlerfall
   */
  socketHandle initService(const std::string &service);

private:
  socketHandle acceptConnection(struct sockaddr &sa, size_t &len) const;
  socketHandle fd = invalidSocket;
};

class TcpStBufData;

/// streambuffer für TCP-Verbindungen
class TcpStBuf : public std::basic_streambuf<char> {
public:
  using Base = std::basic_streambuf<char>; ///< Basis-Typ
  using char_type = typename Base::char_type; ///< Element-Typ
  using Traits = std::char_traits<char_type>; ///< Traits-Typ
  using int_type = typename Base::int_type; ///< zugehöriger int-Typ

  /// default Konstruktor
  TcpStBuf();

  /** \brief Konstruktor für passive TCP-Verbindung
   *
   * Es wird eine mobs::TcpAccept Klasse benötigt, deren eingehende Verbindung angenommen wird.
   * Der Aufruf blockiert, bis eine Verbindung eingeht
   */
  explicit TcpStBuf(TcpAccept &accept);

  /** \brief Konstruktor für TCP-Verbindung
   *
   * @param host Hostname oder IP
   * @param service Port als Zahl oder als Service-Name (z.B.: "http")
   */
  TcpStBuf(const std::string &host, const std::string &service);

  ~TcpStBuf() override;

  /** \brief offnet eine TCP-Verbindung
   *
   * @param host Host oder IP
   * @param service Port als Zahl oder als Service-Name (z.B.: "http")
   * @return true, wenn Verbindung offen
   */
  bool open(const std::string &host, const std::string &service);

  /** \brief schließt die Verbindung
   *
   * @return true wenn fehlerfrei
   */
  bool close();

  /** \brief Beendet die Kommunikation in einer oder beiden Richtungen
   *
   * @param which Richtung, die beendet werden soll
   */
  void shutdown(std::ios_base::openmode which);

  /// prüfe status
  bool poll(std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) const;

  /// Rückgabe ob Verbindung offen
  bool is_open() const;

  /// liefert remote host bei passiver Verbindung
  std::string getRemoteHost() const;

  /// liefert remote ip bei passiver Verbindung
  std::string getRemoteIp() const;

  /// Rückgabe ob Fehlerstatus
  bool bad() const;

  /// \private
  int_type overflow(int_type ch) override;
  /// \private
  int_type underflow() override;

protected:
  /// \private
  std::basic_streambuf<char>::pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which) override;
  /// \private
  int sync() override;
private:
  TcpStBufData *data;
};

/** \brief iostream für TCP-Verbindungen
 *
 */
class tcpstream : public std::iostream {
public:
  /// default Konstruktor
  tcpstream();

  /** \brief Konstruktor für passive TCP-Verbindung
   *
   * Es wird eine mobs::TcpAccept Klasse benötigt, deren eingehende Verbindung angenommen wird.
   * Der Aufruf blockiert, bis eine Verbindung eingeht
   * @param accept Initialisierter mobs::TcpAccept
   */
  explicit tcpstream(TcpAccept &accept);

  /** \brief Konstruktor für TCP-Verbindung
   *
   * @param host Hostname oder IP
   * @param service Port
   */
  tcpstream(const std::string &host, const std::string &service);

  ~tcpstream() override;

  /** \brief offnet eine TCP-Verbindung
   *
   * @param host Hostname oder IP
   * @param service Port
   */
  void open(const std::string &host, const std::string &service);

  /// Schließen einer Verbindung
  void close();

  /** \brief Beendet die Kommunikation in einer oder beiden Richtungen
   *
   * @param which Richtung, die beendet werden soll
   */
  void shutdown(std::ios_base::openmode which = std::ios_base::in | std::ios_base::out);

  /// Rückgabe, ob Verbindung geöffnet wurde
  bool is_open() const;

  /// liefert remote host bei passiver Verbindung
  std::string getRemoteHost() const;

  /// liefert remote ip bei passiver Verbindung
  std::string getRemoteIp() const;

  /** \brief prüft, ob Zugriff entsprechend openmode möglich ist
   *
   * der Aufruf ist immer nicht blockierend;
   * im Fehlerfall wird der Status bad() gesetzt
   * @param which Richtung auf die geprüft werden soll
   * @return True wenn Zugriff möglich
   * \throws mobs::tcpstream::failure wen ioexceptions aktiviert und ein Fehler vorliegt
   */
  bool poll(std::ios_base::openmode which = std::ios_base::in | std::ios_base::out);
};


}

#endif //MOBS_TCPSTREAM_H
