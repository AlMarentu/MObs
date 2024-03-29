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


#include "socketstream.h"
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
#include <cstring>
#include <sys/poll.h>


#ifdef __MINGW32__
#undef errno
#define errno WSAGetLastError()
#define MSG_NOSIGNAL 0
#else
#define closesocket(s) ::close(s)
#define SOCKET_ERROR (-1)
#endif

namespace mobs {


class SocketStBufData { // NOLINT(cppcoreguidelines-pro-type-member-init)
public:
  SocketStBufData(socketHandle &socket) {
#ifdef __MINGW32__
    WinSock::get();
#endif
    int sv[2];
    int res = socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    if (res)
    {
      LOG(LM_ERROR, "startSubserver: socketpair failed " << errno << " " << strerror(errno));
      socket = invalidSocket;
      return;
    }
    LOG(LM_DEBUG, "SV " << sv[0] << " " << sv[1]);
    socket = sv[0];
    fd = sv[1];
  }

  std::streamsize readBuf(bool nowait) {
    if (fd == invalidSocket or bad)
      return 0;
    auto res = recv(fd, &rdBuf[0], int(rdBuf.size()), MSG_NOSIGNAL | (nowait? MSG_DONTWAIT: 0)); // immer < INT_MAX
    if (res < 0) {
      if (not nowait or errno != EAGAIN) {
        LOG(LM_ERROR, "read error " << errno << " " << strerror(errno));
        bad = true;
      }
      return 0;
    }
//    if (res < 200)
//      LOG(LM_DEBUG, "READ TCP " << res << " " << std::string(&rdBuf[0], res));
//    else
//      LOG(LM_INFO, "READ TCP " << res << " " << std::string(&rdBuf[0], 100) << " ... " << std::string(&rdBuf[res-100], 100));
//    LOG(LM_DEBUG, "READ TCP " << res);
    rdPos += SocketStBuf::off_type(res);
    return res;
  }

  void writeBuf(std::streamsize sz) {
    if (fd == invalidSocket)
      bad = true;
    if (bad)
      return;
    SocketStBuf::char_type *cp = &wrBuf[0];
    while (sz > 0) {
      auto res = send(fd, cp, int(sz), MSG_NOSIGNAL); // buffersize immer < INT_MAX
      LOG(LM_DEBUG, "WRITE TCP " << res );
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


  socketHandle fd = invalidSocket;
  bool bad = false;
  std::array<SocketStBuf::char_type, 8192> rdBuf;
  std::array<SocketStBuf::char_type, 8192> wrBuf;
  std::streamsize rdPos = 0;
  std::streamsize wrPos = 0;
  sockaddr_storage remoteAddr{};
  size_t addrLen = sizeof(remoteAddr);
};



SocketStBuf::SocketStBuf(socketHandle &socket) : Base() {
  data = std::unique_ptr<SocketStBufData>(new SocketStBufData(socket));
  Base::setp(data->wrBuf.begin(), data->wrBuf.end());
}

SocketStBuf::~SocketStBuf() {
  if (is_open()) {
    LOG(LM_DEBUG, "CLOSE " << int(data->fd));
    closesocket(data->fd);
  }
}

bool SocketStBuf::is_open() const {
  return data->fd != invalidSocket;
}

bool SocketStBuf::poll(std::ios_base::openmode which) const {
  if (data->fd == invalidSocket)
    return false;
  struct pollfd pf = { data->fd, 0, 0 };
  if (which & std::ios_base::in)
    pf.events += POLLIN;
  if (which & std::ios_base::out)
    pf.events += POLLOUT;
  auto res = ::poll(&pf, 1, 0);
  if (res < 0) {
    LOG(LM_ERROR, "poll error " << errno);
    data->bad = true;
  } else if (pf.revents & (POLLERR | POLLHUP | POLLNVAL)) {
    data->bad = true;
  }
  else if (pf.revents & (POLLIN | POLLOUT))
    return true;
  return false;
}

bool SocketStBuf::bad() const {
  return data->bad;
}

SocketStBuf::int_type SocketStBuf::overflow(SocketStBuf::int_type ch) {
  TRACE(PARAM(ch));
  data->writeBuf(std::distance(Base::pbase(), Base::pptr()));
  Base::setp(data->wrBuf.begin(), data->wrBuf.end()); // buffer zurücksetzen
  if (bad())
    return Traits::eof();
  if (not Traits::eq_int_type(ch, Traits::eof()))
    Base::sputc(Traits::to_char_type(ch));
  return ch;
}

std::streamsize SocketStBuf::showmanyc() {
  TRACE("");
  if (bad() or not is_open())
    return -1;
  auto sz = data->readBuf(true);
  if (bad())
    return -1;
  Base::setg(&data->rdBuf[0], &data->rdBuf[0], &data->rdBuf[sz]);
  return sz;
}

SocketStBuf::int_type SocketStBuf::underflow() {
  TRACE("");
  auto sz = data->readBuf(true);
  if (sz == 0)
  {
    LOG(LM_DEBUG, "SocketStBuf::underflow WAITING");
    sz = data->readBuf(false);
    LOG(LM_DEBUG, "SocketStBuf::underflow DONE " << sz);
  }
  Base::setg(&data->rdBuf[0], &data->rdBuf[0], &data->rdBuf[sz]);
  if (sz == 0)
    return Traits::eof();
  return Traits::to_int_type(*Base::gptr());
}

bool SocketStBuf::close() {
  pubsync();
  if (not is_open())
    return false;
  LOG(LM_DEBUG, "CLOSE " << int(data->fd));
  int res = closesocket(data->fd);
  data->fd = invalidSocket;
  return res == 0;
}

int SocketStBuf::sync() {
  if (Base::pbase() != Base::pptr())
    overflow(Traits::eof());
  return bad() ? -1: 0;
}

// wird im state eof or bad vom soketstream nicht mehr aufgerufen

std::basic_streambuf<char>::pos_type
SocketStBuf::seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which) {
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

void SocketStBuf::shutdown(std::ios_base::openmode which) {
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


socketstream::socketstream(socketHandle &socket) :
        std::iostream(new SocketStBuf(socket)) {
  if (not is_open())
    setstate(std::ios_base::badbit);
}


socketstream::~socketstream() {
  auto *tp = dynamic_cast<SocketStBuf *>(rdbuf());
  delete tp;
}


void socketstream::close() {
  auto *tp = dynamic_cast<SocketStBuf *>(rdbuf());
  if (not tp) THROW("bad cast");
  tp->close();
  if (tp->bad())
    setstate(std::ios_base::badbit);
}

bool socketstream::is_open() const {
  auto *tp = dynamic_cast<SocketStBuf *>(rdbuf());
  if (not tp) THROW("bad cast");
  return (not tp->bad() and tp->is_open());
}

bool socketstream::poll(std::ios_base::openmode which) {
  auto *tp = dynamic_cast<SocketStBuf *>(rdbuf());
  if (not tp) THROW("bad cast");
  bool res = tp->poll(which);
  if (tp->bad())
    setstate(std::ios_base::badbit);
  return res;
}

void socketstream::shutdown(std::ios_base::openmode which) {
  auto *tp = dynamic_cast<SocketStBuf *>(rdbuf());
  if (not tp) THROW("bad cast");
  tp->shutdown(which);
  if (tp->bad())
    setstate(std::ios_base::badbit);
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
