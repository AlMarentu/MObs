// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2026 Matthias Lautner
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
#define MSG_DONTWAIT 0
#warning MSG_DONTWAIT
#include <windows.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <sys/poll.h>
#endif
#include <unistd.h>
#include <cstring>

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
#define MSG_NOSIGNAL 0
#else
#define closesocket(s) ::close(s)
#define SOCKET_ERROR (-1)
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
    LOG(LM_DEBUG, "TRY " << hostIp(*p->ai_addr, p->ai_addrlen));
  for(auto p = servinfo; p; p = p->ai_next) {
    if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == invalidSocket) {
      LOG(LM_ERROR, "Fehler bei socket: " <<  strerror(errno));
      continue;
    }
    int para = 1;
    char *parp = (char *)&para;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, parp, sizeof(para)) < 0)
      LOG(LM_ERROR, "setsockopt SO_REUSEADDR " << strerror(errno));
#if 0
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
      LOG(LM_DEBUG, "CONNECTED " << hostIp(*p->ai_addr, p->ai_addrlen));
      break;
    }
  }
  freeaddrinfo(servinfo);
  if (fd < 0) {
    return invalidSocket;
  }

  if (listen(fd, 128) == SOCKET_ERROR) {
    LOG(LM_ERROR, "Fehler bei listen: " << strerror(errno));
    return invalidSocket;
  }
  return fd;
}


socketHandle TcpAccept::acceptConnection(struct sockaddr &addr, size_t &len) const {
  static std::mutex mutex;
  std::lock_guard<std::mutex> guard(mutex);

  socklen_t addrLen = socklen_t(len);
  LOG(LM_DEBUG, "Accepting " << fd);
  int fdneu = ::accept(fd, &addr, &addrLen);
  len = addrLen;
  LOG(LM_DEBUG, "Accept " << fdneu);
  if (fdneu != invalidSocket) {
    LOG(LM_DEBUG, "accept: from Host: " << hostIp(addr, addrLen));
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
    struct addrinfo hints{};
    struct addrinfo *res, *res0;
    std::stringstream error;
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (auto error = getaddrinfo(host.c_str(), service.c_str(), &hints, &res0)) {
      LOG(LM_ERROR, "getaddrinfo: " << gai_strerror(error));
      return;
    }
    fd = invalidSocket;
    for (res = res0; res; res = res->ai_next) {
      LOG(LM_DEBUG, "TRY " << hostIp(*res->ai_addr, res->ai_addrlen));
      (sockaddr &)remoteAddr = *res->ai_addr;
      addrLen = res->ai_addrlen;
      if ((fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == invalidSocket) {
        LOG(LM_ERROR, "socket failed " << errno);
        continue;
      }
      if (timeout >= 0)
        setTimeout(timeout);
      if (::connect(fd, res->ai_addr, res->ai_addrlen) == SOCKET_ERROR) {
#ifdef __MINGW32__
        if (errno == WSAECONNREFUSED)
#else
        if (errno == ECONNREFUSED)
#endif
          error <<  " connection refused " << hostIp(*res->ai_addr, res->ai_addrlen);
        else
          error <<  "connect failed " << errno << " " << hostIp(*res->ai_addr, res->ai_addrlen);
        closesocket(fd);
        fd = invalidSocket;
        continue;
      }
      break;
    }
    if (fd == invalidSocket) {
      LOG(LM_ERROR, error.str());
      return;
    }

    int i = 1;
    char *parp = (char *)&i;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, parp, sizeof(i));
  }

  std::streamsize readBuf(bool nowait) {
    if (fd == invalidSocket or bad)
      return 0;
    //LOG(LM_DEBUG, "TcpBuffer::underflow " << (nowait ? "NOWAIT":"WAIT"));
    auto res = recv(fd, &rdBuf[0], int(rdBuf.size()), MSG_NOSIGNAL | (nowait? MSG_DONTWAIT: 0)); // immer < INT_MAX
    //LOG(LM_DEBUG, "TcpBuffer::underflow Done " << res << "/" << rdBuf.size());
    if (res < 0) {
      if (not nowait or errno != EAGAIN) {
        LOG(LM_ERROR, "read error " << errno << " " << strerror(errno));
        bad = true;
      }
      return 0;
    }
    //LOG(LM_DEBUG, "TcpStream read " << res << " soll " << rdBuf.size());
    //LOG(LM_DEBUG, "RCV: " << std::string(&rdBuf[0], res));
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
      auto res = send(fd, cp, int(sz), MSG_NOSIGNAL); // buffersize immer < INT_MAX
      //LOG(LM_DEBUG, "WRITE TCP " << res );
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
    return hostName((sockaddr &)remoteAddr, socklen_t(addrLen));
  }

  std::string getRemoteIp() const {
    return hostIp((sockaddr &)remoteAddr, socklen_t(addrLen));
  }

  bool setTOS(int tos) const {
#ifdef __MINGW32__
#define TOS_CAST (const char *)
#else
#define SOCK_CAST
#endif
    if (setsockopt(fd, IPPROTO_IP, IP_TOS, SOCK_CAST &tos, sizeof(tos)) < 0) {
      LOG(LM_ERROR, "setTOS IP_TOS " << strerror(errno));
      return false;
    }
    return true;
  }

  bool setTimeout(int milliseconds) {
    timeout = milliseconds; // wenn noch nicht offen, dann nur merken
    if (fd != invalidSocket)
    {
      struct timeval timeout_tv {milliseconds / 1000, (milliseconds % 1000) * 1000 };
      if (setsockopt (fd, SOL_SOCKET, SO_SNDTIMEO, SOCK_CAST &timeout_tv, sizeof timeout_tv) < 0)
      {
        LOG(LM_ERROR, "setTOS SO_SNDTIMEO " << strerror(errno));
        return false;
      }
      if (setsockopt (fd, SOL_SOCKET, SO_RCVTIMEO, SOCK_CAST &timeout_tv, sizeof timeout_tv) < 0)
      {
        LOG(LM_ERROR, "setTOS SO_RCVTIMEO " << strerror(errno));
        return false;
      }
    }
    return true;
  }


  socketHandle fd = invalidSocket;
  bool bad = false;
  std::array<TcpStBuf::char_type, 8192> rdBuf;
  std::array<TcpStBuf::char_type, 8192> wrBuf;
  std::streamsize rdPos = 0;
  std::streamsize wrPos = 0;
  sockaddr_storage remoteAddr{};
  size_t addrLen = sizeof(remoteAddr);
  int timeout = -1;
};

TcpStBuf::TcpStBuf() : Base() {
  data = std::unique_ptr<TcpStBufData>(new TcpStBufData);
}

TcpStBuf::TcpStBuf(TcpAccept &accept) {
  data = std::unique_ptr<TcpStBufData>(new TcpStBufData);
  data->fd = accept.acceptConnection((sockaddr &)data->remoteAddr, data->addrLen);
  if (data->fd == invalidSocket)
    data->bad = true;
  Base::setp(data->wrBuf.begin(), data->wrBuf.end());
}

TcpStBuf::TcpStBuf(const std::string &host, const std::string &service) : Base() {
  data = std::unique_ptr<TcpStBufData>(new TcpStBufData);
  data->connect(host, service);
  Base::setp(data->wrBuf.begin(), data->wrBuf.end());
}

TcpStBuf::~TcpStBuf() {
  if (not is_open())
    return;
  shutdown(std::ios_base::in | std::ios_base::out);
  close();
}

bool TcpStBuf::open(const std::string &host, const std::string &service) {
  TRACE(PARAM(host) << PARAM(service));
  data->connect(host, service);
  return is_open();
}

bool TcpStBuf::is_open() const {
  return data->fd != invalidSocket;
}

bool TcpStBuf::poll(std::ios_base::openmode which) const {
  if (data->fd == invalidSocket)
    return false;
  struct pollfd pf = { data->fd, 0, 0 };
  if (which & std::ios_base::in)
    pf.events |= POLLIN;
  if (which & std::ios_base::out)
    pf.events |= POLLOUT;
#ifdef __MINGW32__
  auto res = WSAPoll(&pf, 1, 0);
#else
  auto res = ::poll(&pf, 1, 0);
#endif
  if (res == SOCKET_ERROR) {
    LOG(LM_ERROR, "poll error " << errno);
    data->bad = true;
  } else if (pf.revents & (POLLERR | POLLHUP | POLLNVAL)) {
    data->bad = true;
  }
  else if (pf.revents & (POLLIN | POLLOUT))
    return true;
  return false;
}

bool TcpStBuf::bad() const {
  return data->bad;
}

TcpStBuf::int_type TcpStBuf::overflow(TcpStBuf::int_type ch) {
  TRACE(PARAM(ch));
  //LOG(LM_DEBUG, "write " << std::string(Base::pbase(), std::distance(Base::pbase(), Base::pptr())));
  data->writeBuf(std::distance(Base::pbase(), Base::pptr()));
  Base::setp(data->wrBuf.begin(), data->wrBuf.end()); // buffer zurücksetzen
  if (bad())
    return Traits::eof();
  if (not Traits::eq_int_type(ch, Traits::eof()))
    Base::sputc(Traits::to_char_type(ch));
  return ch;
}

std::streamsize TcpStBuf::showmanyc() {
  TRACE("");
  if (bad() or not is_open())
    return -1;
  auto sz = data->readBuf(true);
  if (bad())
    return -1;
  Base::setg(&data->rdBuf[0], &data->rdBuf[0], &data->rdBuf[sz]);
//  if (not Traits::eq(delimiter, Traits::eof())) {
//    auto pos = std::find(gptr(), egptr(),  delimiter);
//    if (pos != egptr() and pos != gptr()) { // als erstes Zeichen erlaubt
//      sz = std::distance(gptr(), pos);
//      LOG(LM_DEBUG, "Found Delimiter size " << sz);
//    }
//  }
  return sz;
}

TcpStBuf::int_type TcpStBuf::underflow() {
  TRACE("");
  auto sz = data->readBuf(false);
  Base::setg(&data->rdBuf[0], &data->rdBuf[0], &data->rdBuf[sz]);
  if (sz == 0)
    return Traits::eof();
  return Traits::to_int_type(*Base::gptr());
}

bool TcpStBuf::close() {
  if (not is_open())
    return false;
  pubsync();
  int res = closesocket(data->fd);
  data->fd = invalidSocket;
  if (bad())
    return false;
  return res == 0;
}

int TcpStBuf::sync() {
  if (Base::pbase() != Base::pptr())
    overflow(Traits::eof());
  return bad() ? -1: 0;
}

// wird im state eof or bad vom tcpstream nicht mehr aufgerufen

std::basic_streambuf<char>::pos_type
TcpStBuf::seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which) {
  if (which & std::ios_base::in) {
    if ((which & std::ios_base::out) or dir != std::ios_base::cur or off != 0)
      return {off_type(-1)};
    return {data->rdPos - off_type(std::distance(Base::gptr(), Base::egptr()))};
  }
  if (which & std::ios_base::out) {
    if ((which & std::ios_base::in) or dir != std::ios_base::cur or off != 0)
      return {off_type(-1)};
    return {data->wrPos + off_type(std::distance(Base::pbase(), Base::pptr()))};
  }
  return {off_type(-1)};
}

void TcpStBuf::shutdown(std::ios_base::openmode which) {
  if (not is_open())
    return;
  if ((which & std::ios_base::out) and not bad())
    pubsync();
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
    if (errno != ENOTCONN)
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

bool TcpStBuf::setTOS(int tos) const {
  return data->setTOS(tos);
}

bool TcpStBuf::setTimeout(int milliseconds)
{
  return data->setTimeout(milliseconds);
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
  clear(); // Fehlerstatus löschen
  auto *tp = dynamic_cast<TcpStBuf *>(rdbuf());
  if (not tp or not tp->open(host, service))
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
  if (tp->bad() or not tp->is_open())
    return false;

  return (not tp->bad() and tp->is_open());
}

bool tcpstream::poll(std::ios_base::openmode which) {
  auto *tp = dynamic_cast<TcpStBuf *>(rdbuf());
  if (not tp) THROW("bad cast");
  bool res = tp->poll(which);
  if (tp->bad())
    setstate(std::ios_base::badbit);
  return res;
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

bool tcpstream::setTOS(int tos) const {
  auto *tp = dynamic_cast<TcpStBuf *>(rdbuf());
  if (not tp) THROW("bad cast");
  return tp->setTOS(tos);
}

bool tcpstream::setTimeout(int milliseconds)
{
  auto *tp = dynamic_cast<TcpStBuf *>(rdbuf());
  if (not tp) THROW("bad cast");
  return tp->setTimeout(milliseconds);
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
