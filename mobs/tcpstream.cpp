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
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/ip.h>

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

namespace mobs {

int TcpAccept::initService(const std::string &service) {
  struct addrinfo hints{};
  struct addrinfo *servinfo;

  hints.ai_family = PF_UNSPEC; // use AF_INET6 to force IPv6
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE; // use my IP address

  int rv;
  if ((rv = getaddrinfo(nullptr, service.c_str(), &hints, &servinfo)) != 0) {
    LOG(LM_ERROR, "getaddrinfo: " << gai_strerror(rv));
    return -1;
  }
  // loop through all the results and bind to the first we can
  for(auto p = servinfo; p; p = p->ai_next)
    LOG(LM_INFO, "TRY " << hostIp(*p->ai_addr, p->ai_addrlen));
  for(auto p = servinfo; p; p = p->ai_next) {
    if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      LOG(LM_ERROR, "Fehler bei socket: " <<  strerror(errno));
      continue;
    }
    int para = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &para, sizeof(para)) < 0)
      LOG(LM_ERROR, "setsockopt SO_REUSEADDR " << strerror(errno));
    if (setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &para, sizeof(para)) < 0)
      LOG(LM_ERROR, "setsockopt SO_NOSIGPIPE " << strerror(errno));
    int tries = 3;
    for (int i = 1; ; i++) {
      if (bind(fd, p->ai_addr, p->ai_addrlen) == -1)
      {
        LOG(LM_ERROR, "Fehler bei bind: " <<  strerror(errno) << " " << errno);
        if (errno == EADDRINUSE and i < tries)
        {
          sleep(1);
          continue;
        }
        close(fd);
        fd = -1;
      }
      break;
    }
    if (fd != -1) {
      LOG(LM_INFO, "CONNECTED " << hostIp(*p->ai_addr, p->ai_addrlen));
      break;
    }
  }
  freeaddrinfo(servinfo);
  if (fd < 0) {
    return -1;
  }

  if (listen(fd, 10) < 0) {
    LOG(LM_ERROR, "Fehler bei listen: " << strerror(errno));
    return -1;
  }
  LOG(LM_INFO, "FD " << fd);
  return fd;
}


int TcpAccept::acceptConnection(struct sockaddr &addr, size_t &len) const {
  static std::mutex mutex;
  std::lock_guard<std::mutex> guard(mutex);

  socklen_t addrLen = len;
  LOG(LM_INFO, "Accepting " << fd);
  int fdneu = ::accept(fd, &addr, &addrLen);
  len = addrLen;
  LOG(LM_INFO, "Accept " << fdneu);
  if (fdneu >= 0) {
    LOG(LM_INFO, "accept: from Host: " << hostIp(addr, addrLen));
  }
  else
    LOG(LM_ERROR, "accept failed " << strerror(errno));

  return fdneu;
}


class TcpStBufData { // NOLINT(cppcoreguidelines-pro-type-member-init)
public:
  void connect(const std::string &host, uint32_t port) {
    struct sockaddr_in addr{};

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host.c_str()); // try if it's an numeric address
    if (addr.sin_addr.s_addr == INADDR_NONE) {
      struct hostent *rechner = gethostbyname(host.c_str());
      if (not rechner) {
        LOG(LM_ERROR, "can't resolve host " << host);
        return;
      }
      memcpy((char *) &addr.sin_addr, rechner->h_addr_list[0], sizeof(addr.sin_addr));
    }

    struct protoent *ppe;
    if (not(ppe = getprotobyname("tcp")))
      THROW("can't get \"tcp\" protocol entry");

    fd = socket(AF_INET, SOCK_STREAM, ppe->p_proto);
    if (fd < 0) {
      LOG(LM_ERROR, "can't bind socket");
      return;
    }

    if (::connect(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
      LOG(LM_ERROR, "can't connect");
      ::close(fd);
      fd = -1;
      bad = true;
      return;
    }

    int i = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i));

  }

  std::streamsize readBuf() {
    if (fd < 0 or bad)
      return 0;
    auto res = read(fd, &rdBuf[0], rdBuf.size());
//    LOG(LM_DEBUG, "READ TCP " << res << " " << std::string(&rdBuf[0], res));
//    LOG(LM_DEBUG, "READ TCP " << res);
    if (res < 0) {
      LOG(LM_ERROR, "read error");
      bad = true;
      return 0;
    }
    rdPos += TcpStBuf::off_type(res);
    return res;
  }

  void writeBuf(std::streamsize sz) {
    if (fd < 0)
      bad = true;
    if (bad)
      return;
    TcpStBuf::char_type *cp = &wrBuf[0];
    while (sz > 0) {
      auto res = write(fd, cp, sz);
      LOG(LM_INFO, "WRITE TCP " << res );
      if (res <= 0) {
        LOG(LM_ERROR, "write error");
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
    return hostName(remoteAddr, addrLen);
  }

  std::string getRemoteIp() const {
    return hostIp(remoteAddr, addrLen);
  }


  int fd = -1;
  bool bad = false;
  std::array<TcpStBuf::char_type, 8192> rdBuf;
  std::array<TcpStBuf::char_type, 8192> wrBuf;
  std::streamsize rdPos = 0;
  std::streamsize wrPos = 0;
  struct sockaddr remoteAddr{};
  size_t addrLen = sizeof(remoteAddr);
};

TcpStBuf::TcpStBuf() : Base() {
  data = new TcpStBufData;
}

TcpStBuf::TcpStBuf(TcpAccept &accept) {
  data = new TcpStBufData;
  data->fd = accept.acceptConnection(data->remoteAddr, data->addrLen);
  Base::setp(data->wrBuf.begin(), data->wrBuf.end());
}

TcpStBuf::TcpStBuf(const std::string &host, uint32_t port) : Base() {
  data = new TcpStBufData;
  data->connect(host, port);
  Base::setp(data->wrBuf.begin(), data->wrBuf.end());
}

TcpStBuf::~TcpStBuf() {
  if (not bad() and is_open())
    close();
  delete data;
}

bool TcpStBuf::open(const std::string &host, uint32_t port) {
  TRACE(PARAM(host) << PARAM(port));
  data->connect(host, port);
  return is_open();
}

bool TcpStBuf::is_open() const {
  return data->fd >= 0;
}

bool TcpStBuf::bad() const {
  return data->bad;
}

TcpStBuf::int_type TcpStBuf::overflow(TcpStBuf::int_type ch) {
  TRACE(PARAM(ch));
  data->writeBuf(std::distance(Base::pbase(), Base::pptr()));
  Base::setp(data->wrBuf.begin(), data->wrBuf.end()); // buffer zurücksetzen
  if (not Traits::eq_int_type(ch, Traits::eof()))
    Base::sputc(ch);
  if (not bad())
    return ch;
  return Traits::eof();
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
  int res = ::close(data->fd);
  data->fd = -1;
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
  return pos_type(off_type(-1));
}

void TcpStBuf::shutdown(std::ios_base::openmode which) {
  if (which & std::ios_base::out)
    pubsync();
  if (bad() or not is_open())
    return;
  int how = 0;
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

tcpstream::tcpstream(const std::string &host, uint32_t port) : std::iostream ( new TcpStBuf(host, port)) {
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

void tcpstream::open(const std::string &host, uint32_t port) {
  auto *tp = dynamic_cast<TcpStBuf *>(rdbuf());
  if (not tp) THROW("bad cast");
  tp->open(host, port);
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


//class SelectData {
//public:
//  class Process {
//  public:
//
//  };
//};
//
//int doSelect() {
//  struct timeval tv;
//  fd_set fds;
//  FD_ZERO(&fds);
//  FD_SET(fdneu, &fds);
//  tv.tv_sec = 1;
//  tv.tv_usec = 0;
//
//  g_sessions.clear();
//
//  res = select(fdneu +1, &fds, NULL, NULL, &tv);
//}


}
