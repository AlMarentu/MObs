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


#include "csb.h"
#include "converter.h"
#include "logging.h"

#include <locale>
#include <vector>
#include <iomanip>

namespace mobs {
//
//class StrFilterBase {
//public:
//  StrFilterBase() = default;
//  virtual ~StrFilterBase() = default;
//  size_t put(const char *buf, size_t sz);
//  size_t put(const  u_char *inp, size_t len, u_char *key)
//  void finalize();
//
//};


#define INPUT_BUFFER_SIZE 1024

#if (INPUT_BUFFER_SIZE % 4)
#error INPUT_BUFFER_SIZE must be multiple of 4
#endif

class CryptIstrBufData {
public:
  CryptIstrBufData(std::istream &istr, CryptBufBase *cbbp)  : inStb(istr), cbb(cbbp) {
    if (not cbbp)
      cbb = new CryptBufBase;
    cbb->setIstr(istr);
  }

  ~CryptIstrBufData() {
      delete cbb;
  }

#if 0
  void print_buffer(const CryptIstrBuf::char_type *begin, const CryptIstrBuf::char_type *end)
  {
    std::cout << "_____ PRINT _______\n";
    for (auto ip = begin; ip < end; ip++) {
      auto i = *ip;
      if (i == 0) {
        std::cout << "NULL";
      } else if (i < 128 and i > 31) {
        std::cout << char(i);
      } else {
        std::cout << std::hex << int(u_char(i)) << std::dec;
      }
      std::cout << " ";
    }
    std::cout << "\n--------------------------------------\n";
  }
#endif

  std::istream &inStb;
  CryptBufBase *cbb = nullptr;
  std::mbstate_t state{};
  std::array<CryptIstrBuf::char_type, INPUT_BUFFER_SIZE> buffer;
  CryptIstrBuf::pos_type pos = 0;
};


CryptIstrBuf::CryptIstrBuf(std::istream &istr, CryptBufBase *cbbp) : Base() {
  TRACE("");
  data = new CryptIstrBufData(istr, cbbp);
  // Buffer zu beginn leer
  Base::setg(data->buffer.begin(), data->buffer.begin(), data->buffer.begin());
}

CryptIstrBuf::~CryptIstrBuf() {
  TRACE("");
//  data->print_buffer(Base::gptr(), Base::egptr());
  delete data;
}


bool CryptIstrBuf::bad() const {
  if (data->cbb and data->cbb->bad())
    return true;
  return false;
}

CryptIstrBuf::int_type CryptIstrBuf::underflow() {
  TRACE("");
  try {
    std::array<char, INPUT_BUFFER_SIZE> buf; // NOLINT(cppcoreguidelines-pro-type-member-init)
    std::locale lo = this->getloc();
//    std::cout << "underflow\n";
    std::streamsize sz = 0;
    if (data->cbb) { // wird der buffer nicht explizit auf null gesetzt ist immer einer vorhanden
      std::streamsize rd = buf.size();
      auto av = data->cbb->in_avail();
      if (av == 0) {
        if (data->cbb->underflow() != CryptBufBase::Traits::eof())
          av = data->cbb->in_avail();
      }
      if (av > 0) {
        rd = av;
        LOG(LM_DEBUG, "READSOME CIB " << av);
      }
      sz = data->cbb->sgetn(&buf[0], rd);
    } else {
      if (data->inStb.eof())
        return Traits::eof();
      std::istream::sentry sen(data->inStb, true);
      if (sen) {
        data->inStb.read(&buf[0], buf.size());
        sz = data->inStb.gcount();
        if (not sz) {
          Base::setg(data->buffer.begin(), data->buffer.begin(), data->buffer.begin());
          return Traits::eof();
        }
      }
    }
//    std::cout << "GC = " << sz << "  " << std::string(&buf[0], sz) << std::endl;
    char_type *bit;
    const char *bp;
    std::use_facet<std::codecvt<char_type, char, std::mbstate_t>>(lo).in(data->state, &buf[0], &buf[sz], bp,
                                                                         &data->buffer[0],
                                                                         &data->buffer[INPUT_BUFFER_SIZE], bit);
    if (bp != &buf[sz])
      LOG(LM_ERROR, u8"charset locale conversion error " << std::distance(&buf[0], (char *)bp) << " " << sz);
    Base::setg(&data->buffer[0], &data->buffer[0], bit);
    if (Base::gptr() == Base::egptr())
      return Traits::eof();
    data->pos += off_type(std::distance(Base::gptr(), Base::egptr()));

//    data->print_buffer(Base::gptr(), Base::egptr());

    return Traits::to_int_type(*Base::gptr());
  } catch (std::exception &e) {
    LOG(LM_ERROR, "exception: " << e.what());
    Base::setg(data->buffer.begin(), data->buffer.begin(), data->buffer.begin());
    data->inStb.setstate(std::ios_base::failbit);
    return Traits::eof();
  }
}


//CryptIstrBuf::pos_type CryptIstrBuf::seekpos(CryptIstrBuf::pos_type pos, std::ios_base::openmode which) {
//  TRACE(PARAM(pos) << PARAM(which));
//  return basic_streambuf::seekpos(pos, which);
//}

// für ausschließlich tellg verwenden
CryptIstrBuf::pos_type CryptIstrBuf::seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which) {
  TRACE(PARAM(off) << PARAM(dir) << PARAM(which));
  if (not (which & std::ios_base::in) or dir != std::ios_base::cur or off != 0)
    return pos_type(off_type(-1));
  return pos_type(data->pos - off_type(std::distance(Base::gptr(), Base::egptr())));
}

void CryptIstrBuf::imbue(const std::locale &loc) {
  TRACE("");
  std::locale lo = getloc();
  if (lo != loc and Base::gptr() != Base::egptr()) {
//    std::cout << "imbue" << std::endl;
    data->pos -= off_type(std::distance(Base::gptr(), Base::egptr()));
    std::array<char, INPUT_BUFFER_SIZE * 4> buf; // NOLINT(cppcoreguidelines-pro-type-member-init)
    char *bp;
    const char_type *bit;
    std::mbstate_t state{};
//    bool ncv = std::use_facet<std::codecvt<char_type, char, std::mbstate_t>>(lo).always_noconv();
    std::use_facet<std::codecvt<char_type, char, std::mbstate_t>>(lo).out(state, Base::gptr(), Base::egptr(), bit,
                                                                         &buf[0], &buf[sizeof(buf)], bp);
    if (bit != Base::egptr())
      LOG(LM_ERROR, u8"read buffer to small");
    data->state = std::mbstate_t{};
    const char *bp2;
    char_type *bit2;
//    ncv = std::use_facet<std::codecvt<char_type, char, std::mbstate_t>>(lo).always_noconv();
    std::use_facet<std::codecvt<char_type, char, std::mbstate_t>>(loc).in(data->state, &buf[0], bp, bp2,
                                                                         &data->buffer[0],
                                                                         &data->buffer[INPUT_BUFFER_SIZE], bit2);
    if (bp != bp2)
      LOG(LM_ERROR, u8"read buffer to small");
    LOG(LM_DEBUG, "locale change " << std::distance(Base::gptr(), Base::egptr()) << " -> " << std::distance(&buf[0], bp) << " -> " << std::distance(&data->buffer[0], bit2));
    Base::setg(&data->buffer[0], &data->buffer[0], bit2);
    data->pos += off_type(std::distance(Base::gptr(), Base::egptr()));
  }
  basic_streambuf::imbue(loc);
}

CryptBufBase *CryptIstrBuf::getCbb() {
  if (not data->cbb)
    LOG(LM_ERROR, "NO CBB");
  return data->cbb;
}

std::istream &CryptIstrBuf::getIstream() {
  return data->inStb;
}


class CryptOstrBufData {
public:
  CryptOstrBufData(std::ostream &ostr, CryptBufBase *cbbp)  : outStb(ostr), cbb(cbbp) {
    if (not cbbp)
      cbb = new CryptBufBase;
    cbb->setOstr(ostr);
  }

  ~CryptOstrBufData() {
      delete cbb;
  }

  std::ostream &outStb;
  CryptBufBase *cbb = nullptr;
  std::mbstate_t state{};
  std::array<CryptOstrBuf::char_type, 1024> buffer;
  CryptOstrBuf::pos_type pos = 0;
};


CryptOstrBuf::CryptOstrBuf(std::ostream &ostr, CryptBufBase *cbbp) : Base() {
  TRACE("");
  data = new CryptOstrBufData(ostr, cbbp);
  Base::setp(data->buffer.begin(), data->buffer.end());
}

CryptOstrBuf::~CryptOstrBuf() {
  TRACE("");
//  if (seekoff(0, std::ios_base::cur) > 0)
//    finalize();
  delete data;
}

CryptOstrBuf::int_type CryptOstrBuf::overflow(CryptOstrBuf::int_type ch) {
   TRACE(PARAM(ch));
   if (Base::pbase() != Base::pptr()) {
    std::ostream::sentry s(data->outStb);
    if (s) {
      const std::locale &lo = this->getloc();
      const char_type *bit;
      std::vector<char> buf(2048);
      char *bp;
      std::use_facet<std::codecvt<char_type, char, std::mbstate_t>>(lo).out(data->state, Base::pbase(), Base::pptr(),
                                                                            bit, &buf[0], &buf[buf.size()], bp);
      auto ep = Base::pptr();
      data->pos += off_type(std::distance((const char_type *)&data->buffer[0], bit));
      Base::setp(data->buffer.begin(), data->buffer.end()); // buffer zurücksetzen
      while (bit < ep)  // übrig gebliebene Zeichen zurückschreiben
        Base::sputc(*bit++);
      if (data->cbb) {
        std::streamsize sz = std::distance(&buf[0], bp);
        std::streamsize wr = 0;
        while (wr < sz) {
          LOG(LM_DEBUG, "OUTSTB write " << sz - wr);
          auto n = data->cbb->sputn(&buf[wr], sz - wr);
          if (n <= 0)
            return Traits::eof();
          wr += n;
        }
      }
      else
        data->outStb.write(&buf[0], std::distance(&buf[0], bp));
#if 0
      // Debug:
      for (auto ip = &buf[0]; ip != bp; ip++) {
        auto const i = *ip;
        if (i == 0) {
          std::cout << "NULL";
        } else if (i > 0 and i > 31) {
          std::cout << i;
        } else {
          std::cout << std::setfill('0') << std::setw(2) << std::hex << int(u_char(i)) << std::dec;
        }
        std::cout << " ";
      }
      std::cout << "\n";
#endif
    }
    else
      return Traits::eof();
  }
  if (not Traits::eq_int_type(ch, Traits::eof()))
    Base::sputc(ch);
  if (data->outStb.good())
    return ch;
  return Traits::eof();
}


int CryptOstrBuf::sync() {
  TRACE("");
  overflow(Traits::eof());
  if (data->cbb) {
    if  (data->cbb->pubsync() < 0)
      return -1;
  }
  data->outStb.flush();
  return data->outStb.good() ? 0: -1;
}

void CryptOstrBuf::finalize() {
  TRACE("");
  pubsync();
  if (data->cbb)
    data->cbb->finalize();
}

//CryptOstrBuf::pos_type CryptOstrBuf::seekpos(CryptOstrBuf::pos_type pos, std::ios_base::openmode which) {
//  TRACE(PARAM(pos) << PARAM(which));
//  return basic_streambuf::seekpos(pos, which);
//}

// für ausschließlich tellp verwenden
CryptOstrBuf::pos_type CryptOstrBuf::seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which) {
  TRACE(PARAM(off) << PARAM(dir) << PARAM(which));
  if (not (which & std::ios_base::out) or dir != std::ios_base::cur or off != 0)
    return pos_type(off_type(-1));
  return pos_type(data->pos + off_type(std::distance(Base::pbase(), Base::pptr())));
}

void CryptOstrBuf::imbue(const std::locale &loc) {
  TRACE("");
  overflow(Traits::eof());
  // TODO ist hier ein unshift nötig?
#if 0
  std::vector<char> buf(128);
  char *bp;
  std::use_facet<std::codecvt<char_type, char, std::mbstate_t>>(getloc()).unshift(data->state, &buf[0], &buf[buf.size()], bp);
  if (bp > &buf[0]) {
    if (data->cbb) {
      std::streamsize sz = std::distance(&buf[0], bp);
      std::streamsize wr = 0;
      while (wr < sz) {
        wr += data->cbb->sputn(&buf[wr], sz - wr);
      }
    } else
      data->outStb.write(&buf[0], std::distance(&buf[0], bp));
  }
#endif
  basic_streambuf::imbue(loc);
}

CryptBufBase *CryptOstrBuf::getCbb() {
  return data->cbb;
}

std::ostream &CryptOstrBuf::getOstream() {
  return data->outStb;
}

// für Manipulator
void CryptBufBase::base64::set(mobs::CryptOstrBuf *rdp) const {
  if (rdp and rdp->getCbb())
    rdp->getCbb()->setBase64(b);
}

// für Manipulator
void CryptBufBase::base64::set(mobs::CryptIstrBuf *rdp) const {
  if (rdp and rdp->getCbb())
    rdp->getCbb()->setBase64(b);
}












#define C_IN_BUF_SZ  256

class CryptBufBaseData { // NOLINT(cppcoreguidelines-pro-type-member-init)
public:
  class Base64Info {
  public:

    int i = 0;
    unsigned int a = 0;
    int l = 0;
    std::string linebreak;
  };

  void setBad()  {
    bad = true;
//    if (inStb)
//      inStb->setstate(std::ios_base::failbit);
    if (outStb)
      outStb->setstate(std::ios_base::failbit);
  }

  bool isGood() const  {
    if (inStb)
      return inStb->good();
    if (outStb)
      return outStb->good();
    return false;
  }

  bool isEof() const  {
    if (inStb)
      return inStb->eof();
    return true;
  }

  void b64Start() {
    b64Cnt = 0;
    b64Value = 0;
  }


  void b64get(u_char c, CryptBufBase::char_type *&it) {
    int v = from_base64(c);
    if (v < 0) {
      if (c == '=') { // padding
        switch (b64Cnt) {
          case 3:
            *it++ = u_char(b64Value >> 10);
            *it++ = u_char((b64Value >> 2) & 0xff);
            // fall into
          case 100:
            b64Cnt = 999; // Wenn noch ein = kommt -> fehler
            break;
          case 2:
            *it++ = u_char(b64Value >> 4);
            b64Cnt = 100; // es darf noch 1 = kommen
            break;
          case 1: throw std::runtime_error("base64 unexpected end");
          default: throw std::runtime_error("base64 unexpected padding");
        }
      }
      else
        throw std::runtime_error("base64 padding");
    }
    else if (v < 64) {
      if (b64Cnt > 3)
        throw std::runtime_error("base64 invalid");
      b64Value = (b64Value << 6) + v;
      if (++b64Cnt == 4) {
        *it++ = u_char(b64Value >> 16);
        *it++ = u_char((b64Value >> 8) & 0xff);
        *it++ = u_char(b64Value & 0xff);
        b64Cnt = 0;
        b64Value = 0;
      }
    }
  }

  std::streamsize doRead(char *s, std::streamsize count)  {
    if (readLimit == 0)
      return 0;
    std::istream::sentry sen(*inStb, true);
    if (not sen)
      return 0;
    if (use64) {
      char *it = s;
      if (count > C_IN_BUF_SZ)
        count = C_IN_BUF_SZ;
      size_t c2 = count / 3 * 4;
      std::array<char, C_IN_BUF_SZ / 3 * 4 + 1> buf; // NOLINT(cppcoreguidelines-pro-type-member-init)
      if (readLimit >= 0 and readLimit < c2)
        c2 = readLimit;
      if (c2)
        inStb->read(&buf[0], c2);
      auto sr = c2 ? inStb->gcount() : 0;
      if (readLimit >= 0)
        readLimit -= sr;
      char *cp = &buf[0];
      if (sr)
        for (size_t i = 0; i < sr; i++)
          b64get(u_char(*cp++), it);
      else {
        if (b64Cnt > 0)
          for (; b64Cnt < 4; b64Cnt++)
            b64get(u_char('='), it);
      }
      LOG(LM_DEBUG, "CSB READ " << std::distance(s, it));
      return std::distance(s, it);
    }  else {
      if (readLimit >= 0 and readLimit < count)
        count = readLimit;
      auto av = inStb->rdbuf()->in_avail();
      if (av == 0 and readLimit < 0) {
        LOG(LM_DEBUG, "READSOME at end");
        inStb->peek();
        av = inStb->rdbuf()->in_avail();
      }
//      LOG(LM_DEBUG, "READSOME " << av << " soll " << count);
      if (av > 0 and av < count and readLimit == -1)
        count = av;
      std::streamsize n;
      if (Traits::eq(delimiter, Traits::eof())) {
        inStb->read(s, count);
        n = inStb->gcount();
      } else {
        char d = Traits::to_char_type(delimiter);
        for (n = 0; n < count; n++,s++) {
          if (not inStb->get(*s).good())
            break;
          if (*s == d) {
            LOG(LM_DEBUG, "delimiter found at " << n);
            inStb->unget();
            break;
          }
        }
      }

//      inStb->get(s, count, L'\0');
//      std::streamsize n = inStb->readsome(s, count);
//      if (n > 0) {
//        LOG(LM_DEBUG, "READSOME " << n);
//        if (readLimit >= 0)
//          readLimit -= n;
//        return n;
//      }
      if (readLimit >= 0)
        readLimit -= n;
      return n;
    }
  }


  void b64put(const u_char *buf, size_t sz)  {
    auto first = buf;
    auto last = first + sz;
    while (first != last) {
      b64.a = (b64.a << 8) + (*first & 0xff);
      if (++b64.i == 3) {
        *outStb << char(to_base64(b64.a >> 18));
        *outStb << char(to_base64((b64.a >> 12) & 0x3f));
        *outStb << char(to_base64((b64.a >> 6) & 0x3f));
        *outStb << char(to_base64(b64.a & 0x3f));
        b64.i = 0;
        b64.a = 0;
        if (b64.l++ > 15) {
          for (auto c:b64.linebreak)
            *outStb << c;
          b64.l = 0;
        }
      }
      first++;
    }
  }
  void b64finalize()  {
    if (outStb) {
      if (b64.i == 2) {
        *outStb << char(to_base64(b64.a >> 10));
        *outStb << char(to_base64(b64.a >> 4 & 0x3f));
        *outStb << char(to_base64((b64.a & 0x0f) << 2));
        *outStb << ('=');
      }
      if (b64.i == 1) {
        *outStb << char(to_base64(b64.a >> 2));
        *outStb << char(to_base64((b64.a & 0x03) << 4));
        *outStb << ('=');
        *outStb << ('=');
      }
      b64.i = 0;
      b64.a = 0;
      b64.l = 0;
    }
  }

  void doWrite(const CryptBufBase::char_type *s, std::streamsize count)  {
    if (outStb and count > 0) {
      std::ostream::sentry se(*outStb);
      if (se) {
        if (use64)
          b64put(reinterpret_cast<const u_char *>(s), count);
        else {
          outStb->write(s, count);
        }
      } else
        LOG(LM_ERROR, "bad state");
    }
  }

  using Traits = std::char_traits<CryptBufBase::char_type>;

  std::ostream *outStb = nullptr;
  std::istream *inStb = nullptr;
  std::array<CryptBufBase::char_type, C_IN_BUF_SZ> buffer;
  bool use64 = false;
  bool bad = false;
  int delimiter = Traits::eof();

  int b64Value = 0;
  int b64Cnt = 0;
  Base64Info b64;
  std::streamsize readLimit = -1;
};




CryptBufBase::CryptBufBase() : Base() {
  TRACE("");
  data = new CryptBufBaseData;
}

CryptBufBase::~CryptBufBase() {
  TRACE("");
//  finalize();
  delete data;
}

void CryptBufBase::setOstr(std::ostream &ostr) {
  data->outStb = &ostr;
  Base::setp(data->buffer.begin(), data->buffer.end());
}

void CryptBufBase::setIstr(std::istream &istr) {
  data->inStb = &istr;
  Base::setg(data->buffer.begin(), data->buffer.begin(), data->buffer.begin());
}

CryptBufBase::int_type CryptBufBase::underflow() {
  TRACE("");
//  std::cout << "underflow\n";
  size_t sz = data->doRead(&data->buffer[0], data->buffer.size());
  Base::setg(&data->buffer[0], &data->buffer[0], &data->buffer[sz]);
//  std::cout << "GC2 = " << sz << "  ";
  if (not sz)
    return Traits::eof();
  return Traits::to_int_type(*Base::gptr());
}

CryptBufBase::int_type CryptBufBase::overflow(CryptBufBase::int_type ch) {
  TRACE(PARAM(ch));
//  std::cout << "overflow2 " << int(ch) << "\n";
  if (Base::pbase() != Base::pptr()) {
//      const char_type *bit = Base::pbase();
//      const std::locale &lo = this->getloc();
//      std::vector<char> buf(2048);
//      char *bp;
//      std::use_facet<std::codecvt<char_type, char, std::mbstate_t>>(lo).out(data->state, Base::pbase(), Base::pbase(),
//                                                                            bit, &buf[0], &buf[buf.size()], bp);
//        doWrite(&buf[0], std::distance(&buf[0], bp));

    doWrite(Base::pbase(), std::distance(Base::pbase(), Base::pptr()));
//      std::cout << "overflow2 schreibe " << std::distance(Base::pbase(), Base::pptr()) << "\n";

//      auto ep = Base::pptr();
    Base::setp(data->buffer.begin(), data->buffer.end()); // buffer zurücksetzen
//      while (bit < ep)  // übrig gebliebene Zeichen zurückschreiben
//        Base::sputc(*bit++);


#if 0
      // Debug:
      for (auto ip = &buf[0]; ip != bp; ip++) {
        auto const i = *ip;
        if (i == 0) {
          std::cout << "NULL";
        } else if (i > 0 and i > 31) {
          std::cout << i;
        } else {
          std::cout << std::setfill('0') << std::setw(2) << std::hex << int(u_char(i)) << std::dec;
        }
        std::cout << " ";
      }
      std::cout << "\n";
#endif

  }
//  else
//    std::cout << "empty2\n";

  if (not Traits::eq_int_type(ch, Traits::eof()))
    Base::sputc(ch);
  if (data->isGood())
    return ch;
  return Traits::eof();
}


void CryptBufBase::doWrite(const CryptBufBase::char_type *s, std::streamsize count) {
  data->doWrite(s, count);
}

std::streamsize CryptBufBase::doRead(char *s, std::streamsize count) {
  return data->doRead(s, count);
}

int CryptBufBase::sync() {
  TRACE("");
//  LOG(LM_INFO, "CryptBufBase::sync()");
  if (Base::pbase() != Base::pptr())
    overflow(Traits::eof());
  return data->isGood() ? 0: -1;
}

void CryptBufBase::finalize() {
  TRACE("");
  if (data->outStb) {
//    std::cout << "finalize2\n";
    pubsync();
    if (data->use64)
      data->b64finalize();
  }
}


std::streamsize CryptBufBase::xsputn(const CryptBufBase::char_type *s, std::streamsize count) {
  TRACE(PARAM(count));
//  std::cout << "xsputn2 " << count << "\n";
  auto n = basic_streambuf::xsputn(s, count);
//  std::cout << "xsputn2 -> " << n << "\n";
  return n;
}

bool CryptBufBase::isGood() const {
  TRACE("");
  return data->isGood();
}

void CryptBufBase::setBad() {
  TRACE("");
  return data->setBad();
}

void CryptBufBase::setBase64(bool on) {
  TRACE("");
  if (data->outStb and data->use64 != on and seekoff(0, std::ios_base::cur, std::ios_base::out) > 0)
    finalize();
  if (on and not data->use64)
    data->b64Start();
  data->use64 = on;
}

bool CryptBufBase::bad() const {
  return data->bad;
}

void CryptBufBase::setReadLimit(std::streamsize bytes) {
  data->readLimit = bytes;
}

std::streamsize CryptBufBase::getLimitRemain() const {
  return data->readLimit;
}

void CryptBufBase::setReadDelimiter(CryptBufBase::char_type c) {
  data->delimiter = Traits::to_int_type(c);
}

void CryptBufBase::setReadDelimiter() {
  data->delimiter = Traits::eof();
}


Base64IstBuf::Base64IstBuf(std::wistream &istr) : Base(), inStb(istr) {
  TRACE("");
  Base::setg(&ch, &ch, &ch);
}

Base64IstBuf::int_type Base64IstBuf::underflow() {
  TRACE("");
  wchar_t c;
  if (inStb.get(c).eof())
    return Traits::eof();
  if (c == '=' or from_base64(c) >= 0) {
    ch = char(c);
    Base::setg(&ch, &ch, &ch+1);
    return Traits::to_int_type(ch);
  }
  inStb.unget();
  return Traits::eof();
}


}

