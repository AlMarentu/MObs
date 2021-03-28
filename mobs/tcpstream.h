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

struct sockaddr;
namespace mobs {


/** \brief Klasse um eine passive TCP-Verbindung zu öffnen
 *
 */
class TcpAccept {
  friend class TcpStBuf;
public:
  /** \brief öffnet eine Port, um eine TCP-Verbindung anzunehmen \see mobs::tcpstream
   *
   * @param service TCP-Port
   * @return -1 im Fehlerfall
   */
  int initService(const std::string &service);

private:
  int acceptConnection(struct sockaddr &sa, size_t &len) const;
  int fd = -1;
};

class TcpStBufData;

/// streambuffer für TCP-Verbindungen
class TcpStBuf : public std::basic_streambuf<char> {
public:
  using Base = std::basic_streambuf<char>;
  using char_type = typename Base::char_type;
  using Traits = std::char_traits<char_type>;
  using int_type = typename Base::int_type;

  /// default Konstruktor
  TcpStBuf();

  /** \brief Konstruktor für passive TCP-Verbindung
   *
   * Es wird eine mobs::TcpAccept Klasse benötigt, deren eingehende Verbindung angenommen wird.
   * Der Aufruf blockiert, bis eine Verbindung eingeht
   */
  TcpStBuf(TcpAccept &accept);

  /** \brief Konstruktor für TCP-Verbindung
   *
   * @param host Hostname oder IP
   * @param port Port
   */
  TcpStBuf(const std::string &host, uint32_t port);

  ~TcpStBuf() override;

  /** \brief offnet eine TCP-Verbindung
   *
   * @param host Host oder IP
   * @param port Posrt
   * @return true, wenn Verbindung offen
   */
  bool open(const std::string &host, uint32_t port);

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
  pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which) override;
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
  tcpstream(TcpAccept &accept);

  /** \brief Konstruktor für TCP-Verbindung
   *
   * @param host Hostname oder IP
   * @param port Port
   */
  tcpstream(const std::string &host, uint32_t port);

  ~tcpstream() override;

  /** \brief offnet eine TCP-Verbindung
   *
   * @param host Hostname oder IP
   * @param port Port
   */
  void open(const std::string &host, uint32_t port);

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

};


}

#endif //MOBS_TCPSTREAM_H
