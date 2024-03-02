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
#include <array>
#include <iomanip>
#include <algorithm>


#ifdef STREAMLOG
#define CSBLOG(x, y) LOG(x, y)
#else
#define CSBLOG(x, y)
#endif

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


#define INPUT_BUFFER_SIZE 4096
// local-char to wide-char buffer size
#define OUTPUT_BUFFER_SIZE 2048
// Buffer für Base64-Kodierung, soll <= 75% INPUT_BUFFER_SIZE sein
#define C_IN_BUF_SZ  3072

#if (INPUT_BUFFER_SIZE % 4)
#error INPUT_BUFFER_SIZE must be multiple of 4
#endif

class CryptIstrBufData {
public:
  CryptIstrBufData(std::istream &istr, CryptBufBase *cbbp) : inStb(istr), cbb(cbbp) {
    if (not cbbp)
      cbb = std::unique_ptr<CryptBufBase>(new CryptBufBase);
    cbb->setIstr(istr);
  }

  ~CryptIstrBufData() = default;

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
  std::unique_ptr<CryptBufBase> cbb = nullptr;
  std::mbstate_t state{};
  std::array<CryptIstrBuf::char_type, INPUT_BUFFER_SIZE> buffer;
  CryptIstrBuf::pos_type pos = 0;
  std::unique_ptr<std::vector<char>> rest;
};


CryptIstrBuf::CryptIstrBuf(std::istream &istr, CryptBufBase *cbbp) : Base() {
  TRACE("");
  data = std::unique_ptr<CryptIstrBufData>(new CryptIstrBufData(istr, cbbp));
  // Buffer zu Beginn leer
  Base::setg(data->buffer.begin(), data->buffer.begin(), data->buffer.begin());
}

CryptIstrBuf::~CryptIstrBuf() {
  TRACE("");
//  data->print_buffer(Base::gptr(), Base::egptr());
}


bool CryptIstrBuf::bad() const {
  if (data->cbb and data->cbb->bad())
    return true;
  return false;
}

void CryptIstrBuf::swapBuffer(std::unique_ptr<CryptBufBase> &newBuffer) {
  //overflow(Traits::eof());
  CSBLOG(LM_DEBUG, "CryptIstrBuf::swapBuffer Buffer: " << std::distance(Base::pbase(), Base::pptr())
                                                       << " avail=" << data->cbb->in_avail());
  if (not newBuffer)
    newBuffer = std::unique_ptr<CryptBufBase>(new CryptBufBase);
  newBuffer->setIstr(data->inStb);
  //data->cbb->finalize();
  data->cbb.swap(newBuffer);
}


CryptIstrBuf::int_type CryptIstrBuf::underflow() {
  TRACE("");
  try {
    std::array<char, INPUT_BUFFER_SIZE> buf; // NOLINT(cppcoreguidelines-pro-type-member-init)
    std::locale lo = this->getloc();
    CSBLOG(LM_DEBUG, "CryptIstrBuf::underflow ");
    std::streamsize sz = 0;
    std::streamsize restSize = 0;
    if (data->rest) {
      restSize = data->rest->size();
      if (restSize > 6) { // bei mehr als 6 extra-Bytes wird kein gültiges Zeichen mehr entstehen
        CSBLOG(LM_DEBUG, "CryptIstrBuf::underflow Rest SZ=" << data->rest->size());
        //TODO Schalter ob invalid char eof oder bad liefert
        //  throw std::ios_base::failure("invalid charset", std::io_errc::stream);
        return Traits::eof();
      }
      std::copy_n(data->rest->begin(), restSize, buf.begin());
      data->rest = nullptr;
      data->state = std::mbstate_t{};
      CSBLOG(LM_DEBUG, "CryptIstrBuf::underflow restoring " << restSize << " chars: " << std::string(&buf[0], restSize));
    }
    if (data->cbb) {
      std::streamsize rd = buf.size() - restSize;
      auto av = data->cbb->in_avail();
      if (av == 0) {
        // evt. auf neue Zeichen warten
        if (data->cbb->underflow() == EOF)
        {
          if (restSize) // bereits unkodierbare Zeichen vorhanden
            throw std::ios_base::failure("invalid charset (trailing chars)", std::io_errc::stream);
          return Traits::eof();
        }
        av = data->cbb->in_avail();
      }
      if (av > 0 and av < rd) {
        rd = av;
        CSBLOG(LM_DEBUG, "READSOME CIB " << av);
      }
      sz = restSize + data->cbb->sgetn(&buf[restSize], rd);
    } else {
      CSBLOG(LM_DEBUG, "CryptIstrBuf READ ohne cbb");
      if (data->inStb.eof())
        return Traits::eof();
      std::istream::sentry sen(data->inStb, true);
      if (sen) {
        data->inStb.read(&buf[restSize], buf.size() - restSize);
        sz = data->inStb.gcount();
        if (not sz) {
          if (restSize) // bereits unkodierbare Zeichen vorhanden
            throw std::ios_base::failure("invalid charset (trailing chars)", std::io_errc::stream);
          Base::setg(data->buffer.begin(), data->buffer.begin(), data->buffer.begin());
          return Traits::eof();
        }
      }
    }
//    std::cout << "GC = " << sz << "  " << std::string(&buf[0], sz) << std::endl;
    char_type *bit;
    const char *bp;

    std::use_facet<std::codecvt<char_type, char, std::mbstate_t>>(lo).in(
        data->state, &buf[0], &buf[sz], bp,
        &data->buffer[0],
        &data->buffer[INPUT_BUFFER_SIZE], bit);
    if (bp != &buf[sz]) { // Rest merken
      data->rest = std::unique_ptr<std::vector<char>>(
          new std::vector<char>(const_cast<char *>(bp), &buf[sz])); //std::distance(bp, &buf[sz]))
      CSBLOG(LM_DEBUG, "CryptIstrBuf saving " << data->rest->size() << " chars: "
                                              << std::string(&(*data->rest)[0], data->rest->size()));
      if (bp == &buf[0])
      {
        LOG(LM_ERROR,
            "CryptIstrBuf::underflow facet failed chars = " << int(bp[0]) << ", " << int(bp[1]) << ", " << int(bp[2])
                                                            << " Size " << std::distance(&buf[0], (char *) bp) << " != "
                                                            << sz);
        throw std::ios_base::failure("invalid charset", std::io_errc::stream);
      }
    }
    Base::setg(&data->buffer[0], &data->buffer[0], bit);
    if (Base::gptr() == Base::egptr())
      return Traits::eof();
    data->pos += off_type(std::distance(Base::gptr(), Base::egptr()));

//    data->print_buffer(Base::gptr(), Base::egptr());

    return Traits::to_int_type(*Base::gptr());
  } catch (std::exception &e) {
    LOG(LM_ERROR, "exception: " << e.what());
    Base::setg(data->buffer.begin(), data->buffer.begin(), data->buffer.begin());
    throw std::ios_base::failure(e.what());
    //data->inStb.setstate(std::ios_base::failbit);
    //return Traits::eof();
  }
}


//CryptIstrBuf::pos_type CryptIstrBuf::seekpos(CryptIstrBuf::pos_type pos, std::ios_base::openmode which) {
//  TRACE(PARAM(pos) << PARAM(which));
//  return basic_streambuf::seekpos(pos, which);
//}

// für ausschließlich tellg verwenden
CryptIstrBuf::pos_type CryptIstrBuf::seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which) {
  TRACE(PARAM(off) << PARAM(dir) << PARAM(which));
  if (not(which & std::ios_base::in) or dir != std::ios_base::cur or off != 0)
    return pos_type(off_type(-1));
  return pos_type(data->pos - off_type(std::distance(Base::gptr(), Base::egptr())));
}

void CryptIstrBuf::imbue(const std::locale &loc) {
  TRACE("");
  CSBLOG(LM_DEBUG, "CryptIstrBuf::imbue " << std::distance(Base::gptr(), Base::egptr()));
  //CSBLOG(LM_INFO, "__" << mobs::to_string(std::wstring(Base::gptr(), std::distance(Base::gptr(), Base::egptr()))));
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
    if (bp != bp2) {
      auto sz = std::distance(bp2, (const char *) bp);
      LOG(LM_ERROR,
          "CryptIstrBuf::imbue facet failed 2 chars = " << int(bp[0]) << ", " << int(bp[1]) << ", " << int(bp[2])
                                                        << " Size " << std::distance(&buf[0], (char *) bp) << " != "
                                                        << sz);
      if (bp2 != &buf[0]) { // Rest merken
        data->rest = std::unique_ptr<std::vector<char>>(new std::vector<char>((char *) bp2, bp));
        CSBLOG(LM_DEBUG, "CryptIstrBuf saving " << data->rest->size() << " chars: "
                                                << std::string(&(*data->rest)[0], data->rest->size()));
      } else
        throw std::ios_base::failure("invalid charset", std::io_errc::stream);
    }
    CSBLOG(LM_DEBUG,
           "locale change " << std::distance(Base::gptr(), Base::egptr()) << " -> " << std::distance(&buf[0], bp) << " -> "
                            << std::distance(&data->buffer[0], bit2));
    Base::setg(&data->buffer[0], &data->buffer[0], bit2);
    data->pos += off_type(std::distance(Base::gptr(), Base::egptr()));
    if (data->rest)
      return;
  }
  if (data->rest) {
    CSBLOG(LM_DEBUG, "CryptIstrBuf::imbue rest");
    std::array<char, INPUT_BUFFER_SIZE> buf; // NOLINT(cppcoreguidelines-pro-type-member-init)
    auto sz = data->rest->size();
    std::copy_n(data->rest->begin(), sz, buf.begin());
    data->rest = nullptr;
    CSBLOG(LM_DEBUG, "CryptIstrBuf restoring " << sz << " chars: " << std::string(&buf[0], sz));
    char_type *bit;
    const char *bp;
    std::use_facet<std::codecvt<char_type, char, std::mbstate_t>>(loc).in(data->state, &buf[0], &buf[sz], bp,
                                                                          Base::egptr(),
                                                                          &data->buffer[INPUT_BUFFER_SIZE], bit);
    if (bp != &buf[sz]) {
      LOG(LM_ERROR,
          "CryptIstrBuf::imbue facet failed chars = " << int(bp[0]) << ", " << int(bp[1]) << ", " << int(bp[2])
                                                      << " Size " << std::distance(&buf[0], (char *) bp) << " != "
                                                      << sz);
      if (bp != &buf[0]) { // Rest merken
        data->rest = std::unique_ptr<std::vector<char>>(
            new std::vector<char>(const_cast<char *>(bp), &buf[sz])); //std::distance(bp, &buf[sz]))
        CSBLOG(LM_DEBUG, "CryptIstrBuf saving " << data->rest->size() << " chars: "
                                                << std::string(&(*data->rest)[0], data->rest->size()));
      } else
        throw std::ios_base::failure("invalid charset", std::io_errc::stream);
    }
    Base::setg(&data->buffer[0], Base::egptr(), bit);
    data->pos += off_type(std::distance(Base::gptr(), Base::egptr()));
//    data->print_buffer(Base::gptr(), Base::egptr());
    return;
  }
  basic_streambuf::imbue(loc);
}

CryptBufBase *CryptIstrBuf::getCbb() {
  if (not data->cbb)
    LOG(LM_ERROR, "NO CBB");
  return data->cbb.get();
}

std::istream &CryptIstrBuf::getIstream() {
  return data->inStb;
}

std::streamsize CryptIstrBuf::showmanyc() {
  CSBLOG(LM_DEBUG, "CryptIstrBuf::showmanyc");

  if (not data->cbb)
    return 0;
  if (data->rest)
    return -1;
  return data->cbb->in_avail();
}


class CryptOstrBufData {
public:
  CryptOstrBufData(std::ostream &ostr, CryptBufBase *cbbp) : outStb(ostr), cbb(cbbp) {
    if (not cbbp)
      cbb = std::unique_ptr<CryptBufBase>(new CryptBufBase);
    cbb->setOstr(ostr);
  }

  ~CryptOstrBufData() = default;

  std::ostream &outStb;
  std::unique_ptr<CryptBufBase> cbb;
  std::mbstate_t state{};
  std::array<CryptOstrBuf::char_type, OUTPUT_BUFFER_SIZE> buffer;
  CryptOstrBuf::pos_type pos = 0;
};


CryptOstrBuf::CryptOstrBuf(std::ostream &ostr, CryptBufBase *cbbp) : Base() {
  TRACE("");
  data = std::unique_ptr<CryptOstrBufData>(new CryptOstrBufData(ostr, cbbp));
  Base::setp(data->buffer.begin(), data->buffer.end());
}

CryptOstrBuf::~CryptOstrBuf() {
  TRACE("");
//  if (seekoff(0, std::ios_base::cur) > 0)
//    finalize();
}

void CryptOstrBuf::swapBuffer(std::unique_ptr<CryptBufBase> &newBuffer) {
  overflow(Traits::eof());
  if (not newBuffer)
    newBuffer = std::unique_ptr<CryptBufBase>(new CryptBufBase);
  newBuffer->setOstr(data->outStb);
  data->cbb->finalize();
  data->cbb.swap(newBuffer);
}

CryptOstrBuf::int_type CryptOstrBuf::overflow(CryptOstrBuf::int_type ch) {
  TRACE(PARAM(ch));
  if (Base::pbase() != Base::pptr()) {
    std::ostream::sentry s(data->outStb);
    if (s) {
      const std::locale lo = this->getloc();
      const char_type *bit;
      std::vector<char> buf(OUTPUT_BUFFER_SIZE * 2);
      char *bp;
      std::use_facet<std::codecvt<char_type, char, std::mbstate_t>>(lo).out(data->state, Base::pbase(), Base::pptr(),
                                                                            bit, &buf[0], &buf[buf.size()], bp);
      if (Base::pptr() != bit and std::distance(&buf[0], bp) < ssize_t(buf.size())) {
        LOG(LM_ERROR, "CryptOstrBuf::overflow facet failed char = " << int(*bit));
        throw std::ios_base::failure("invalid charset", std::io_errc::stream);
      }
      auto ep = Base::pptr();
      data->pos += off_type(std::distance((const char_type *) &data->buffer[0], bit));
      Base::setp(data->buffer.begin(), data->buffer.end()); // buffer zurücksetzen
      while (bit < ep)  // übrig gebliebene Zeichen zurückschreiben
        Base::sputc(*bit++);
      if (data->cbb) {
        std::streamsize sz = std::distance(&buf[0], bp);
        std::streamsize wr = 0;
        while (wr < sz) {
          CSBLOG(LM_DEBUG, "OUTSTB write " << sz - wr);
          auto n = data->cbb->sputn(&buf[wr], sz - wr);
          if (n <= 0)
            return Traits::eof();
          wr += n;
        }
      } else
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
    } else
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
    if (data->cbb->pubsync() < 0)
      return -1;
  }
  data->outStb.flush();
  return data->outStb.good() ? 0 : -1;
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
  if (not(which & std::ios_base::out) or dir != std::ios_base::cur or off != 0)
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
  return data->cbb.get();
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



class CryptBufBaseData { // NOLINT(cppcoreguidelines-pro-type-member-init)
public:
  class Base64Info {
  public:

    int i = 0;
    unsigned int a = 0;
    int l = 0;
    std::string linebreak;
  };

  void setBad() {
    bad = true;
//    if (outStb)
//      outStb->setstate(std::ios_base::badbit);
  }

  bool isGood() const {
    if (bad)
      return false;
    if (inStb)
      return inStb->good();
    if (outStb)
      return outStb->good();
    return false;
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
          case 1:
            throw std::runtime_error("base64 unexpected end");
          default:
            throw std::runtime_error("base64 unexpected padding");
        }
      } else
        throw std::runtime_error("base64 padding");
    } else if (v < 64) {
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

  std::streamsize canRead() const {
    CSBLOG(LM_DEBUG, "canRead");
    if (readLimit == 0)
      return -1;
    std::streamsize s = inStb->rdbuf()->in_avail();
    if (s <= 0 and inStb->eof())
      s = -1;
    else if (use64 and s > 0) {
      while (s < 4 - lookaheadCnt and (readLimit < 0 or readLimit > 3)) {
        // wenigstens 4 Zeichen zusammenbringen
        inStb->read(&lookahead[lookaheadCnt], s);
        auto sr = inStb->gcount();
        if (sr <= 0)
          break;
        lookaheadCnt += sr;
        if (readLimit > 0)
          readLimit -= sr;
        s = inStb->rdbuf()->in_avail();
        if (s < 0)
          return 1;
        if (s == 0)
          break;
      }
      s += lookaheadCnt;
      s = (s > 3) ? s / 4 * 3 : 0;
    }
    //CSBLOG(LM_DEBUG, "CryptBuf::canRead  " << inStb->rdbuf()->in_avail() << "->" << s << " EOF " << inStb->eof());
    if (readLimit > 0 and s > readLimit)
      return readLimit;
    return s;
  }


  // wird nie negativ
  std::streamsize doRead(char *s, std::streamsize countIn) {
    CSBLOG(LM_DEBUG, "doRead " << countIn << " B64 " << use64 << "." << " Limit " << readLimit);
    if (readLimit == 0)
      return 0;
    if (not inStb->good()) {
      //std::cerr << "BAD doRead " << std::hex << inStb->rdstate() << std::endl;
      return 0;
    }
    if (inStb->rdstate() == std::ios::eofbit) // sentry setzt fail bei eof
      return 0;
    auto av = inStb->rdbuf()->in_avail();
    if (av < 0)
      return 0;
    std::istream::sentry sen(*inStb, true);
    if (not sen)
      return 0;
    if (use64) {
      std::streamsize count = countIn;
      char *it = s;
      if (count > C_IN_BUF_SZ)
        count = C_IN_BUF_SZ;
      ssize_t c2 = count / 3 * 4 - lookaheadCnt;
      if (c2 + lookaheadCnt < 4)
        c2 = 4 - lookaheadCnt;
      if (av >= 4 and av < c2)
        c2 = av / 4 * 4;
      //CSBLOG(LM_DEBUG, "READSOME64 " << c2 << " soll " << countIn / 3 * 4);
      std::array<char, C_IN_BUF_SZ / 3 * 4 + 1> buf; // NOLINT(cppcoreguidelines-pro-type-member-init)
      if (readLimit >= 0 and readLimit < c2)
        c2 = readLimit;
      if (c2)
        inStb->read(&buf[0], c2);
      auto sr = c2 ? inStb->gcount() : 0;
      if (readLimit >= 0)
        readLimit -= sr;
      char *cp = &lookahead[0];
      for (size_t i = 0; i < lookaheadCnt; i++)
        b64get(u_char(*cp++), it);
      lookaheadCnt = 0;
      cp = &buf[0];
      if (sr)
        for (size_t i = 0; i < sr; i++)
          b64get(u_char(*cp++), it);
      else {
        if (b64Cnt > 0)
          for (; b64Cnt < 4; b64Cnt++)
            b64get(u_char('='), it);
      }
      //CSBLOG(LM_DEBUG, "CSB READ " << std::distance(s, it));
      return std::distance(s, it);
    } else {
      if (readLimit >= 0 and readLimit < countIn)
        countIn = readLimit;
      std::streamsize count = countIn;
      if (av > 0 and av < count) //  and readLimit == -1
        count = av;
      CSBLOG(LM_DEBUG, "CryptBufBaseData::doRead " << count << " soll " << countIn);
      std::streamsize n = inStb->readsome(s, count);
      if (n == 0) {
        inStb->read(s, 1);  // mindestens 1 Zeichen lesen mit warten, dann nachfassen
        n = inStb->gcount();
      }
      if (n > 0 and n < countIn) { // wenn möglich nachfassen
        av = inStb->rdbuf()->in_avail();
        CSBLOG(LM_DEBUG, "CryptBufBaseData::doRead nachfassen " << av);
        if (av > 0) {
          if (av > countIn - n)
            av = countIn - n;
          inStb->read(s + n, av);
          auto n2 = inStb->gcount();
          if (n2 > 0)
            n += n2;
          else
            LOG(LM_ERROR, "CryptBufBaseData::doRead gcount liefert " << n2);
        }
      }

      if (readLimit >= 0)
        readLimit -= n;
      return n;
    }
  }


  void b64put(const u_char *buf, size_t sz) {
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
          for (auto c: b64.linebreak)
            *outStb << c;
          b64.l = 0;
        }
      }
      first++;
    }
  }

  void b64finalize() {
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

  void doWrite(const CryptBufBase::char_type *s, std::streamsize count) {
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
  mutable std::array<CryptBufBase::char_type, 4> lookahead;
  bool use64 = false;
  bool bad = false;

  int b64Value = 0;
  int b64Cnt = 0;
  mutable int lookaheadCnt = 0;
  Base64Info b64;
  mutable std::streamsize readLimit = -1;
};


CryptBufBase::CryptBufBase() : Base() {
  TRACE("");
  data = std::unique_ptr<CryptBufBaseData>(new CryptBufBaseData);
}

CryptBufBase::~CryptBufBase() {
  TRACE("");
//  finalize();
}

void CryptBufBase::setOstr(std::ostream &ostr) {
  data->outStb = &ostr;
  Base::setp(data->buffer.begin(), data->buffer.end());
}

void CryptBufBase::setIstr(std::istream &istr) {
  data->inStb = &istr;
  Base::setg(data->buffer.begin(), data->buffer.begin(), data->buffer.begin());
}

std::streamsize CryptBufBase::showmanyc() {
  return data->canRead();
}

CryptBufBase::int_type CryptBufBase::underflow() {
  TRACE("");
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
    doWrite(Base::pbase(), std::distance(Base::pbase(), Base::pptr()));
    Base::setp(data->buffer.begin(), data->buffer.end()); // buffer zurücksetzen
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

std::streamsize CryptBufBase::canRead() const {
  return data->canRead();
}

int CryptBufBase::sync() {
  TRACE("");
  if (Base::pbase() != Base::pptr())
    overflow(Traits::eof());
  return data->isGood() ? 0 : -1;
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
    Base::setg(&ch, &ch, &ch + 1);
    return Traits::to_int_type(ch);
  }
  inStb.unget();
  Base::setg(&ch, &ch + 1, &ch + 1);
  atEof = true;
  return Traits::eof();
}

std::streamsize Base64IstBuf::showmanyc() {
  if (atEof)
    return -1;
  return inStb.rdbuf()->in_avail();
}


class BinaryIstrBufData {
public:
  BinaryIstrBufData(CryptIstrBufData *cid, size_t len) :
      binaryLength(len), inStb(cid->inStb), cbb(cid->cbb.get()) { }

  ~BinaryIstrBufData() = default;

  std::streamsize underflow() { // 0 == eof
    TRACE("");
    if (not binaryLength)
      return 0;
    std::streamsize sz = 0;
    if (cbb) {
      std::streamsize rd = buffer.size();
      auto av = cbb->in_avail();
      if (av == 0) {
        // evt. auf neue Zeichen warten
        if (cbb->underflow() == EOF)
          return 0;
        av = cbb->in_avail();
      }
      if (av > 0 and av < rd)
        rd = av;
      if (rd > binaryLength)
        rd = binaryLength;
      sz = cbb->sgetn(&buffer[0], rd);
    } else {
      CSBLOG(LM_DEBUG, "BinaryIstrBuf READ ohne cbb");
      if (inStb.eof())
        return 0;
      std::istream::sentry sen(inStb, true);
      if (sen) {
        std::streamsize rd = buffer.size();
        if (rd > binaryLength)
          rd = binaryLength;
        inStb.read(&buffer[0], rd);
        sz = inStb.gcount();
      }
    }
    if (sz > binaryLength)
      sz = binaryLength;
    binaryLength -= sz;
    return sz;
  }
  std::streamsize showmanyc() {
    if (not binaryLength)
      return -1;
    std::streamsize sz;
    if (cbb)
      sz = cbb->in_avail();
    else
      sz = inStb.rdbuf()->in_avail();
    if (sz > binaryLength)
      return binaryLength;
    return sz;
  }

  size_t binaryLength;
  std::istream &inStb;
  CryptBufBase *cbb;
  std::array<BinaryIstBuf::char_type, INPUT_BUFFER_SIZE> buffer;
  CryptIstrBuf::pos_type pos = 0;
};

BinaryIstBuf::BinaryIstBuf(CryptIstrBuf &ci, size_t len) {
  data = std::unique_ptr<BinaryIstrBufData>(new BinaryIstrBufData(ci.data.get(), len));
  // Buffer zu Beginn leer
  std::streamsize sz = 0;
  if (ci.data->rest) {
    sz = ci.data->rest->size();
    if (sz > data->binaryLength)
      sz = data->binaryLength;
    std::copy_n(ci.data->rest->begin(), sz, data->buffer.begin());
    if (ci.data->rest->size() > sz) {
      ci.data->rest->erase(ci.data->rest->begin(), ci.data->rest->begin() + sz);
      //std::copy_n(&(*ci.data->rest)[sz], ci.data->rest->size() - sz, ci.data->rest->begin());
      //ci.data->rest->resize(ci.data->rest->size() - sz);
      ci.imbue(ci.getloc());  // Rest wieder zurückschreiben
    } else
      ci.data->rest = nullptr;
    data->binaryLength -= sz;
    data->pos += off_type(sz);
  }
  Base::setg(&data->buffer[0], &data->buffer[0], &data->buffer[sz]);
}

BinaryIstBuf::~BinaryIstBuf() = default;

BinaryIstBuf::int_type BinaryIstBuf::underflow() {
  TRACE("");
  if (not data->binaryLength)
    return Traits::eof();
  std::streamsize sz = 0;
  try {
    auto sz = data->underflow(); // immer >= 0 oder exception
    Base::setg(&data->buffer[0], &data->buffer[0], &data->buffer[sz]);
    if (not sz)
      return Traits::eof();
  } catch (std::exception &e) {
    LOG(LM_ERROR, "exception: " << e.what());
    Base::setg(data->buffer.begin(), data->buffer.begin(), data->buffer.begin());
    throw std::ios_base::failure(e.what());
  }
  return Traits::to_int_type(*Base::gptr());
}

std::streamsize BinaryIstBuf::showmanyc() {
  CSBLOG(LM_DEBUG, "BinaryIstBuf::showmanyc");
  return data->showmanyc();
}






#if 0
std::streamsize CryptBufNone2::showmanyc() {
  int s = canRead();
  if (s > 0)
    return 1;
  return s;
}

CryptBufNone2::int_type CryptBufNone2::overflow(int_type ch) {
  if (Traits::not_eof(ch)) {
    char_type c = Traits::to_char_type(ch);
    doWrite(&c, 1);
  }
  return ch;
}

CryptBufNone2::int_type CryptBufNone2::underflow() {
  if (doRead(&ch, 1) == 1) {
    Base::setg(&ch, &ch, &ch + 1);
    return Traits::to_int_type(ch);
  }  else {
    Base::setg(&ch, &ch+1, &ch+1);
    return Traits::eof();
  }
}
#endif




}

