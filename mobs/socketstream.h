// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2023 Matthias Lautner
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


/** \file socketstream.h
 *
 *  \brief ioBuffer socketstream
 */

#ifndef MOBS_SOCKETSTREAM_H
#define MOBS_SOCKETSTREAM_H

#include <iostream>
#include <memory>
#ifdef __MINGW32__
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

class SocketStBufData;

/// streambuffer für TCP-Verbindungen
class SocketStBuf : public std::basic_streambuf<char> {
public:
  using Base = std::basic_streambuf<char>; ///< Basis-Typ
  using char_type = typename Base::char_type; ///< Element-Typ
  using Traits = std::char_traits<char_type>; ///< Traits-Typ
  using int_type = typename Base::int_type; ///< zugehöriger int-Typ


  /** \brief Konstruktor für TCP-Verbindung
   *
   * @param host Hostname oder IP
   * @param service Port als Zahl oder als Service-Name (z.B.: "http")
   */
  SocketStBuf(socketHandle &socket);

  ~SocketStBuf() override;

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
  /// \private
  std::streamsize showmanyc() override;

private:
  std::unique_ptr<SocketStBufData> data;
};

/** \brief iostream für TCP-Verbindungen
 *
 */
class socketstream : public std::iostream {
public:

  /** \brief Konstruktor für TCP-Verbindung
   *
   * @param host Hostname oder IP
   * @param service Port
   */
  socketstream(socketHandle &socket);

  ~socketstream() override;

  /// Schließen einer Verbindung
  void close();

  /// Rückgabe, ob Verbindung geöffnet wurde
  bool is_open() const;

  /** \brief prüft, ob Zugriff entsprechend openmode möglich ist
   *
   * der Aufruf ist immer nicht blockierend;
   * im Fehlerfall wird der Status bad() gesetzt
   * @param which Richtung auf die geprüft werden soll
   * @return True wenn Zugriff möglich
   * \throws mobs::tcpstream::failure wen ioexceptions aktiviert und ein Fehler vorliegt
   */
  bool poll(std::ios_base::openmode which = std::ios_base::in | std::ios_base::out);

  /** \brief Beendet die Kommunikation in einer oder beiden Richtungen
   *
   * @param which Richtung, die beendet werden soll
   */
  void shutdown(std::ios_base::openmode which = std::ios_base::in | std::ios_base::out);

};


}

#endif //MOBS_TCPSTREAM_H
