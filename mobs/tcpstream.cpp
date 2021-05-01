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


#include "tcpstream.h"
#include "logging.h"

#include <array>
#include <mutex>
#include <sys/types.h>
#ifdef __MINGW32__
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#endif
#include <unistd.h>

static std::string hostIp(const struct sockaddr &sa, socklen_t len)
{
  char host[NI_MAXHOST];
  int ret = getnameinfo(&sa, len, host, sizeof(host), nullptr, 0, NI_NUMERICHOST);
  if (ret != 0)
  {
    LOG(LM_ERROR, "getnameinfo liefert " << ret);
    return "???";
  }
  return host;
}

static std::string hostName(const struct sockaddr &sa, socklen_t len)
{
  char host[NI_MAXHOST];
  int ret = getnameinfo(&sa, len, host, sizeof(host), nullptr, 0, NI_NOFQDN);
  if (ret != 0)
  {
    LOG(LM_ERROR, "getnameinfo liefert " << ret);
    return "???";
  }
  return host;
}

#ifdef __MINGW32__
#undef errno
#define errno WSAGetLastError()
#else
#define closesocket(s) ::close(s)
#define SOCKET_ERROR -1
#endif

namespace mobs {

socketHandle TcpAccept::initService(const std::string &service) {
  struct addrinfo hints{};
  struct addrinfo *servinfo;
#ifdef __MINGW32__
  WinSock::get();
#endif
  hints.ai_family = PF_UNSPEC; // use AF_INET6 to force IPv6
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE; // use my IP address

  if (auto rv = getaddrinfo(nullptr, service.c_str(), &hints, &servinfo)) {
    LOG(LM_ERROR, "getaddrinfo: " << gai_strerror(rv));
    return invalidSocket;
  }
  // loop through all the results and bind to the first we can
  for(auto p = servinfo; p; p = p->ai_next)
    LOG(LM_INFO, "TRY " << hostIp(*p->ai_addr, p->ai_addrlen));
  for(auto p = servinfo; p; p = p->ai_next) {
    if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == invalidSocket) {
      LOG(LM_ERROR, "Fehler bei socket: " <<  strerror(errno));
      continue;
    }
    int para = 1;
    char *parp = (char *)&para;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, parp, sizeof(para)) < 0)
      LOG(LM_ERROR, "setsockopt SO_REUSEADDR " << strerror(errno));
#ifndef __MINGW32__
    if (setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &para, sizeof(para)) < 0)
      LOG(LM_ERROR, "setsockopt SO_NOSIGPIPE " << strerror(errno));
    #define SOCKET_ERROR -1
#endif
    int tries = 3;
    for (int i = 1; ; i++) {
      if (bind(fd, p->ai_addr, p->ai_addrlen) == SOCKET_ERROR)
      {
        LOG(LM_ERROR, "Fehler bei bind: " <<  strerror(errno) << " " << errno);
        if (errno == EADDRINUSE and i < tries)
        {
          sleep(1);
          continue;
        }
        closesocket(fd);
        fd = invalidSocket;
      }
      break;
    }
    if (fd != invalidSocket) {
      LOG(LM_INFO, "CONNECTED " << hostIp(*p->ai_addr, p->ai_addrlen));
      break;
    }
  }
  freeaddrinfo(servinfo);
  if (fd < 0) {
    return invalidSocket;
  }

  if (listen(fd, 10) == SOCKET_ERROR) {
    LOG(LM_ERROR, "Fehler bei listen: " << strerror(errno));
    return invalidSocket;
  }
  return fd;
}


socketHandle TcpAccept::acceptConnection(struct sockaddr &addr, size_t &len) const {
  static std::mutex mutex;
  std::lock_guard<std::mutex> guard(mutex);

  socklen_t addrLen = len;
  LOG(LM_INFO, "Accepting " << fd);
  int fdneu = ::accept(fd, &addr, &addrLen);
  len = addrLen;
  LOG(LM_INFO, "Accept " << fdneu);
  if (fdneu != invalidSocket) {
    LOG(LM_INFO, "accept: from Host: " << hostIp(addr, addrLen));
  }
  else
    LOG(LM_ERROR, "accept failed " << errno << " " << strerror(errno));

  return fdneu;
}


class TcpStBufData { // NOLINT(cppcoreguidelines-pro-type-member-init)
public:
  void connect(const std::string &host, const std::string &service) {
#ifdef __MINGW32__
    WinSock::get();
#endif
    struct addrinfo hints, *res, *res0;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (auto error = getaddrinfo(host.c_str(), service.c_str(), &hints, &res0)) {
      LOG(LM_ERROR, "getaddrinfo: " << gai_strerror(error));
      return;
    }
    fd = invalidSocket;
    for (res = res0; res; res = res->ai_next) {
      LOG(LM_INFO, "TRY " << hostIp(*res->ai_addr, res->ai_addrlen));
      if ((fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == invalidSocket) {
        LOG(LM_ERROR, "socket failed " << errno);
        continue;
      }
      if (::connect(fd, res->ai_addr, res->ai_addrlen) == SOCKET_ERROR) {
        if (errno == WSAECONNREFUSED)
          LOG(LM_ERROR, "connection refused " << hostIp(*res->ai_addr, res->ai_addrlen));
        else
          LOG(LM_ERROR, "connect failed " << errno << " " << hostIp(*res->ai_addr, res->ai_addrlen));
        closesocket(fd);
        fd = invalidSocket;
        continue;
      }
      break;
    }
    if (fd == invalidSocket) {
      LOG(LM_ERROR, "can't connect");
      return;
    }
    LOG(LM_INFO, "Connected " << hostIp(*res->ai_addr, res->ai_addrlen));

    int i = 1;
    char *parp = (char *)&i;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, parp, sizeof(i));
  }

  std::streamsize readBuf() {
    if (fd == invalidSocket or bad)
      return 0;
#ifdef __WIN32__
    auto res = recv(fd, &rdBuf[0], int(rdBuf.size()), 0); // immer < INT_MAX
#else
    auto res = read(fd, &rdBuf[0], rdBuf.size());
#endif
    if (res < 0) {
      LOG(LM_ERROR, "read error " << errno);
      bad = true;
      return 0;
    }
//    if (res < 200)
//      LOG(LM_INFO, "READ TCP " << res << " " << std::string(&rdBuf[0], res));
//    else
//      LOG(LM_INFO, "READ TCP " << res << " " << std::string(&rdBuf[0], 100) << " ... " << std::string(&rdBuf[res-100], 100));
//    LOG(LM_DEBUG, "READ TCP " << res);
    rdPos += TcpStBuf::off_type(res);
    return res;
  }

  void writeBuf(std::streamsize sz) {
    if (fd == invalidSocket)
      bad = true;
    if (bad)
      return;
    TcpStBuf::char_type *cp = &wrBuf[0];
    while (sz > 0) {
#ifdef __WIN32__
      auto res = send(fd, cp, int(sz), 0); // buffersize immer < INT_MAX
#else
      auto res = write(fd, cp, sz);
#endif
      LOG(LM_INFO, "WRITE TCP " << res );
      if (res <= 0) {
        LOG(LM_ERROR, "write error " << errno);
        if (res == -1 and errno == EPIPE)
          LOG(LM_ERROR, "got sigpipe");
        bad = true;
        break;
      }
      wrPos += res;
      sz -= res;
      cp += res;
    }
  }

  std::string getRemoteHost() const {
    return hostName((sockaddr &)remoteAddr, addrLen);
  }

  std::string getRemoteIp() const {
    return hostIp((sockaddr &)remoteAddr, addrLen);
  }


  socketHandle fd = invalidSocket;
  bool bad = false;
  std::array<TcpStBuf::char_type, 8192> rdBuf;
  std::array<TcpStBuf::char_type, 8192> wrBuf;
  std::streamsize rdPos = 0;
  std::streamsize wrPos = 0;
  sockaddr_storage remoteAddr{};
  size_t addrLen = sizeof(remoteAddr);
};

TcpStBuf::TcpStBuf() : Base() {
  data = new TcpStBufData;
}

TcpStBuf::TcpStBuf(TcpAccept &accept) {
  data = new TcpStBufData;
  data->fd = accept.acceptConnection((sockaddr &)data->remoteAddr, data->addrLen);
  if (data->fd == invalidSocket)
    data->bad = true;
  Base::setp(data->wrBuf.begin(), data->wrBuf.end());
}

TcpStBuf::TcpStBuf(const std::string &host, const std::string &service) : Base() {
  data = new TcpStBufData;
  data->connect(host, service);
  Base::setp(data->wrBuf.begin(), data->wrBuf.end());
}

TcpStBuf::~TcpStBuf() {
  if (not bad() and is_open())
    close();
  delete data;
}

bool TcpStBuf::open(const std::string &host, const std::string &service) {
  TRACE(PARAM(host) << PARAM(service));
  data->connect(host, service);
  return is_open();
}

bool TcpStBuf::is_open() const {
  return data->fd != invalidSocket;
}

bool TcpStBuf::bad() const {
  return data->bad;
}

TcpStBuf::int_type TcpStBuf::overflow(TcpStBuf::int_type ch) {
  TRACE(PARAM(ch));
  data->writeBuf(std::distance(Base::pbase(), Base::pptr()));
  Base::setp(data->wrBuf.begin(), data->wrBuf.end()); // buffer zurücksetzen
  if (bad())
    return Traits::eof();
  if (not Traits::eq_int_type(ch, Traits::eof()))
    Base::sputc(Traits::to_char_type(ch));
  return ch;
}

TcpStBuf::int_type TcpStBuf::underflow() {
  TRACE("");
  auto sz = data->readBuf();
  Base::setg(&data->rdBuf[0], &data->rdBuf[0], &data->rdBuf[sz]);
  if (sz == 0)
    return Traits::eof();
  return Traits::to_int_type(*Base::gptr());
}

bool TcpStBuf::close() {
  pubsync();
  if (bad() or not is_open())
    return false;
  int res = closesocket(data->fd);
  data->fd = invalidSocket;
  return res == 0;
}

int TcpStBuf::sync() {
  if (Base::pbase() != Base::pptr())
    overflow(Traits::eof());
  return bad() ? -1: 0;
}

// wird im state eof or bad vom tcpstream nicht mehr aufgerufen
std::fpos<mbstate_t> TcpStBuf::seekoff(long long int off, std::ios_base::seekdir dir, std::ios_base::openmode which) {
  if (which & std::ios_base::in) {
    if ((which & std::ios_base::out) or dir != std::ios_base::cur or off != 0)
      return pos_type(off_type(-1));
    return pos_type(data->rdPos - off_type(std::distance(Base::gptr(), Base::egptr())));
  }
  if (which & std::ios_base::out) {
    if ((which & std::ios_base::in) or dir != std::ios_base::cur or off != 0)
      return pos_type(off_type(-1));
    return pos_type(data->wrPos + off_type(std::distance(Base::pbase(), Base::pptr())));
  }
  return {off_type(-1)};
}

void TcpStBuf::shutdown(std::ios_base::openmode which) {
  if (which & std::ios_base::out)
    pubsync();
  if (bad() or not is_open())
    return;
  int how = 0;
#ifdef __MINGW32__
#define SHUT_RDWR SD_BOTH
#define SHUT_WR SD_SEND
#define SHUT_RD SD_RECEIVE
#endif
  if ((which & (std::ios_base::in | std::ios_base::out)) == (std::ios_base::in | std::ios_base::out))
    how = SHUT_RDWR;
  else if (which & std::ios_base::out)
    how = SHUT_WR;
  else if (which & std::ios_base::in)
    how = SHUT_RD;
  else
    return;
  if (::shutdown(data->fd, how) != 0) {
    LOG(LM_ERROR, "shutdown error " << strerror(errno));
    data->bad = true;
  }
}

std::string TcpStBuf::getRemoteHost() const {
  return data->getRemoteHost();
}

std::string TcpStBuf::getRemoteIp() const {
  return data->getRemoteIp();
}


tcpstream::tcpstream() : std::iostream ( new TcpStBuf()) {}

tcpstream::tcpstream(const std::string &host, const std::string &service) :
        std::iostream(new TcpStBuf(host, service)) {
  if (not is_open())
    setstate(std::ios_base::badbit);
}

tcpstream::tcpstream(TcpAccept &accept) : std::iostream ( new TcpStBuf(accept)) {
  if (not is_open())
    setstate(std::ios_base::badbit);
}

tcpstream::~tcpstream() {
  auto *tp = dynamic_cast<TcpStBuf *>(rdbuf());
  delete tp;
}

void tcpstream::open(const std::string &host, const std::string &service) {
  auto *tp = dynamic_cast<TcpStBuf *>(rdbuf());
  if (not tp) THROW("bad cast");
  tp->open(host, service);
  if (not is_open())
    setstate(std::ios_base::badbit);
}

void tcpstream::close() {
  auto *tp = dynamic_cast<TcpStBuf *>(rdbuf());
  if (not tp) THROW("bad cast");
  tp->close();
  if (tp->bad())
    setstate(std::ios_base::badbit);
}

bool tcpstream::is_open() const {
  auto *tp = dynamic_cast<TcpStBuf *>(rdbuf());
  if (not tp) THROW("bad cast");
  return (not tp->bad() and tp->is_open());
}

void tcpstream::shutdown(std::ios_base::openmode which) {
  auto *tp = dynamic_cast<TcpStBuf *>(rdbuf());
  if (not tp) THROW("bad cast");
  tp->shutdown(which);
  if (tp->bad())
    setstate(std::ios_base::badbit);
}

std::string tcpstream::getRemoteHost() const {
  auto *tp = dynamic_cast<TcpStBuf *>(rdbuf());
  if (not tp) THROW("bad cast");
  return tp->getRemoteHost();
}

std::string tcpstream::getRemoteIp() const {
  auto *tp = dynamic_cast<TcpStBuf *>(rdbuf());
  if (not tp) THROW("bad cast");
  return tp->getRemoteIp();
}


#ifdef __MINGW32__

WinSock::WinSock() {
  WSADATA wsaData;
  if (auto res = WSAStartup(MAKEWORD(2,2), &wsaData))
    THROW("WSAStartup failed with error: " << res);

  atexit(WinSock::deleteWinSock);
}

WinSock::~WinSock() {
  WSACleanup();
}

const WinSock *WinSock::get() {
  if (not winSock)
    winSock = new WinSock;
  return winSock;
}

void WinSock::deleteWinSock() {
  if (not winSock)
    return;
  delete winSock;
  winSock = nullptr;
}

WinSock *WinSock::winSock = nullptr;
#endif
}
