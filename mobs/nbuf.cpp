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


#include "objtypes.h"
#include "nbuf.h"
#include "logging.h"

#include <array>
#include <vector>
#include <iomanip>
#include <cstring>


#define INPUT_BUFFER_LEN 32



class mobs::CryptBufNoneData { // NOLINT(cppcoreguidelines-pro-type-member-init)
public:
  ~CryptBufNoneData() = default;


  std::array<mobs::CryptBufNone::char_type, INPUT_BUFFER_LEN> inputBuf; // NOLINT(cppcoreguidelines-pro-type-member-init)
  std::array<mobs::CryptBufNone::char_type, INPUT_BUFFER_LEN> buffer; // NOLINT(cppcoreguidelines-pro-type-member-init)
  mobs::CryptBufNone::char_type *inputStart = &inputBuf[0];
  std::string id;
  bool finished = false;
};


mobs::CryptBufNone::CryptBufNone(const std::string &id) : CryptBufBase() {
  TRACE("");
  data = std::unique_ptr<CryptBufNoneData>(new mobs::CryptBufNoneData);
  data->id = id;
}


mobs::CryptBufNone::~CryptBufNone() {
  TRACE("");
}


std::streamsize mobs::CryptBufNone::showmanyc() {
  if (data->finished)
    return -1;
  std::streamsize s = canRead();
  if (s <= 0)
    return s;
  return underflowWorker(true);
}


mobs::CryptBufNone::int_type mobs::CryptBufNone::underflow() {
  TRACE("");
//  std::cout << "underflow3 " << "\n";
  try {
    if (data->finished)
      return Traits::eof();
    std::streamsize len = underflowWorker(false);
    //    std::cout << "GC2 = " << len << " ";
    if (len)
      return Traits::to_int_type(*Base::gptr());
  } catch (std::exception &e) {
    LOG(LM_ERROR, "CryptBufNone Exception " << e.what());
    setBad();
    throw std::ios_base::failure(e.what(), std::io_errc::stream);
  }
  return Traits::eof();
}

std::streamsize mobs::CryptBufNone::underflowWorker(bool nowait) {
  std::streamsize sz = std::distance(&data->inputBuf[0], data->inputStart);
  do {
    std::streamsize s = nowait ? canRead() : data->inputBuf.size() - sz;
    if (s > data->inputBuf.size() - sz)
      s = data->inputBuf.size() - sz;
    if (nowait and s <= 0)
      break;
    std::streamsize n = doRead((char *)data->inputStart, s);
    if (not n) {
      data->finished = true;
      break;
    }
    data->inputStart += n;
    sz += n;
  } while (sz < data->inputBuf.size() / 2); // Buffer wenigstens halb voll kriegen
  data->inputStart = &data->inputBuf[0];
  Base::setg(&data->inputBuf[0], &data->inputBuf[0], &data->inputBuf[sz]);
  return sz;
}


mobs::CryptBufNone::int_type mobs::CryptBufNone::overflow(mobs::CryptBufNone::int_type ch) {
  TRACE("");
//  std::cout << "overflow3 " << int(ch) << "\n";
  try {
    if (Base::pbase() != Base::pptr()) {
      int len;
      std::array<u_char, INPUT_BUFFER_LEN> buf; // NOLINT(cppcoreguidelines-pro-type-member-init)
      size_t ofs = 0;

//    size_t  s = std::distance(Base::pbase(), Base::pptr());
//    char *cp =  (Base::pbase());
//    LOG(LM_DEBUG, "Writing " << len << "  was " << std::distance(Base::pbase(), Base::pptr()));
      doWrite( Base::pbase(), std::distance(Base::pbase(), Base::pptr()));
//      std::cout << "overflow2 schreibe " << std::distance(Base::pbase(), Base::pptr()) << "\n";

      CryptBufBase::setp(data->buffer.begin(), data->buffer.end()); // buffer zurücksetzen
    }
    if (not Traits::eq_int_type(ch, Traits::eof()))
      Base::sputc(ch);
    if (isGood())
      return ch;
  } catch (std::exception &e) {
    LOG(LM_ERROR, "Exception " << e.what());
    throw std::ios_base::failure(e.what(), std::io_errc::stream);
  }
  return Traits::eof();
}

void mobs::CryptBufNone::finalize() {
  TRACE("");
//  std::cout << "finalize3\n";
  // bei leerer Eingabe, hier Verschlüsselung beginnen
  pubsync();
  CryptBufBase::finalize();
}


std::string mobs::CryptBufNone::getRecipientId(size_t pos) const {
  return data->id;
}




