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


#include "objtypes.h"
#include "digest.h"
#include "logging.h"

#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <array>
#include <vector>
#include <iomanip>
//#include <openssl/err.h>


#define KEYBUFLEN 32
#define INPUT_BUFFER_LEN 1024

namespace mobs_internal {

// -> rsa.cpp
std::string openSslGetError();

}

namespace {


class openssl_exception : public std::runtime_error {
public:
  explicit openssl_exception(const std::string &log = "") : std::runtime_error(log + " " + mobs_internal::openSslGetError()) {
    LOG(LM_DEBUG, "openssl: " << what());
  }
};

}

class mobs::CryptBufDigestData { // NOLINT(cppcoreguidelines-pro-type-member-init)
public:
  ~CryptBufDigestData()
  {
    if (mdctx)
      EVP_MD_CTX_free(mdctx);
  }

  void md_init() {
    if (not md_algo.empty()) {
      md = EVP_get_digestbyname(md_algo.c_str());
      if (not md)
        THROW("hash algorithm '" << md_algo << "' not available");
      mdctx = EVP_MD_CTX_new();
      if (not EVP_DigestInit_ex(mdctx, md, nullptr))
        throw openssl_exception(LOGSTR("mobs::CryptBufDigest"));
    }
  }

  std::array<mobs::CryptBufDigest::char_type, INPUT_BUFFER_LEN> buffer;
  std::vector<u_char> md_value{}; // hash-wert
  EVP_MD_CTX *mdctx = nullptr; // hash context
  const EVP_MD *md = nullptr; // hash algo
  std::string md_algo;
  bool finished = false;
};



std::string mobs::CryptBufDigest::hashStr() const {
  std::stringstream x;
  for (auto c:data->md_value)
    x << std::hex << std::setfill('0') << std::setw(2) << u_int(c);
  return x.str();
}

void mobs::CryptBufDigest::hashAlgorithm(const std::string &algo) {
  data->md_algo = algo;
}


mobs::CryptBufDigest::CryptBufDigest(const std::string &algo) : CryptBufBase() {
  TRACE("");
  data = std::unique_ptr<mobs::CryptBufDigestData>(new mobs::CryptBufDigestData);
  data->md_algo = algo;
}


mobs::CryptBufDigest::~CryptBufDigest() {
  TRACE("");
}




mobs::CryptBufDigest::int_type mobs::CryptBufDigest::underflow() {
  TRACE("");
//  std::cout << "underflow3 " << "\n";
  try {
    if (data->finished or not isGood())
      return Traits::eof();
    size_t sz = doRead((char *) &data->buffer[0], data->buffer.size());
    // Input Buffer wenigstens halb voll kriegen
    if (sz) {
      while (sz < data->buffer.size() / 2) {
        auto szt = doRead((char *) &data->buffer[sz], data->buffer.size() - sz);
        if (not szt) {
          data->finished = true;
          break;
        }
        sz += szt;
      }
    }
    else
      data->finished = true;
    if (not data->mdctx)
      data->md_init();
    if (sz)
      EVP_DigestUpdate(data->mdctx, &data->buffer[0], sz);
    Base::setg(&data->buffer[0], &data->buffer[0], &data->buffer[sz]);
    if (data->finished) {
      data->md_value.resize(EVP_MAX_MD_SIZE);
      u_int md_len;
      EVP_DigestFinal_ex(data->mdctx, &data->md_value[0], &md_len);
      data->md_value.resize(md_len);
    }
//    std::cout << "GC2 = " << len << " ";
    if (sz)
      return Traits::to_int_type(*Base::gptr());
  } catch (std::exception &e) {
    LOG(LM_ERROR, "Exception " << e.what());
    Base::setg(&data->buffer[0], &data->buffer[0], &data->buffer[0]);
    setBad();
    throw std::ios_base::failure(e.what(), std::io_errc::stream);
  }
  return Traits::eof();
}


mobs::CryptBufDigest::int_type mobs::CryptBufDigest::overflow(mobs::CryptBufDigest::int_type ch) {
  TRACE("");
  if (isGood())
    try {
      if (not data->mdctx)
        data->md_init();

      if (Base::pbase() != Base::pptr()) {
        if (data->mdctx)
          EVP_DigestUpdate(data->mdctx, (u_char *) Base::pbase(), std::distance(Base::pbase(), Base::pptr()));
        doWrite(Base::pbase(), std::distance(Base::pbase(), Base::pptr()));

        CryptBufBase::setp(data->buffer.begin(), data->buffer.end()); // buffer zurücksetzen
      }

      if (not Traits::eq_int_type(ch, Traits::eof()))
        Base::sputc(ch);
      if (isGood())
        return ch;
    } catch (std::exception &e) {
      LOG(LM_ERROR, "Exception " << e.what());
      setBad();
      throw std::ios_base::failure(e.what(), std::io_errc::stream);
    }
  return Traits::eof();
}

void mobs::CryptBufDigest::finalize() {
  //LOG(LM_DEBUG, "CryptBufDigest::finalize");
  TRACE("");
  pubsync();
  if (data->mdctx) {
    data->md_value.resize(EVP_MAX_MD_SIZE);
    u_int md_len;
    EVP_DigestFinal_ex(data->mdctx, &data->md_value[0], &md_len);
    data->md_value.resize(md_len);
  }
  CryptBufBase::finalize();
}

const std::vector<u_char> &mobs::CryptBufDigest::hash() const {
  return data->md_value;
}


namespace {
class Digest : public std::basic_streambuf<char> {
public:
  using Base = std::basic_streambuf<char>;
  using char_type = typename Base::char_type;
  using Traits = std::char_traits<char_type>;
  using int_type = typename Base::int_type;

  explicit Digest(const std::string &algo = "sha1") : algorithm(algo) {
    TRACE("");
    md = EVP_get_digestbyname(algo.c_str());
    if (md) {
      mdctx = EVP_MD_CTX_new();
      if (not mdctx or not EVP_DigestInit_ex(mdctx, md, nullptr)) {
        LOG(LM_ERROR, "mobs::CryptBufDigest");
        mdctx = nullptr;
      }
    }
    else
      LOG(LM_ERROR, "mobs::CryptBufDigest '" << algo << "' doesn't exist");
    setp(buffer.begin(), buffer.end());
  }

  ~Digest() override  {
    TRACE("");
    if (mdctx)
      EVP_MD_CTX_free(mdctx);
  }

  int sync() override {
    TRACE("");
    overflow(Traits::eof());
    return mdctx ? 0: -1;
  }

  const std::vector<u_char> &hash() {
    TRACE("");
    if (mdctx) {
      pubsync();
      md_value.resize(EVP_MAX_MD_SIZE);
      u_int md_len;
      EVP_DigestFinal_ex(mdctx, &md_value[0], &md_len);
      md_value.resize(md_len);
      EVP_MD_CTX_free(mdctx);
      mdctx = nullptr;
    }
    return md_value;
  }

  std::string hashStr() {
    TRACE("");
    hash();
    std::stringstream x;
    for (auto c:md_value)
      x << std::hex << std::setfill('0') << std::setw(2) << u_int(c);
    return x.str();
  }

  int_type overflow(int_type ch) override {
    TRACE("");
    if (not mdctx)
      return Traits::eof();
    if (Base::pbase() != Base::pptr()) {
      EVP_DigestUpdate(mdctx, (u_char *)Base::pbase(), std::distance(Base::pbase(), Base::pptr()));

      setp(buffer.begin(), buffer.end()); // buffer zurücksetzen
    }

    if (not Traits::eq_int_type(ch, Traits::eof()))
      Base::sputc(ch);
    if (md_value.empty())
      return ch;
    return Traits::eof();
  }

  bool good() const { return mdctx or not md_value.empty(); }

  const std::string algorithm;

private:
  std::array<mobs::CryptBufDigest::char_type, 2048> buffer;
  std::vector<u_char> md_value{}; // hash-wert
  EVP_MD_CTX *mdctx = nullptr; // hash context
  const EVP_MD *md = nullptr; // hash algo
};

}

mobs::digestStream::digestStream(const std::string &algo) : std::ostream(new Digest(algo)) {
  TRACE("");
  auto *tp = dynamic_cast<Digest *>(rdbuf());
  if (not tp or not tp->good())
    clear(std::ios::badbit);
}

mobs::digestStream::~digestStream() {
  delete rdbuf();
}

const std::vector<u_char> &mobs::digestStream::hash() {
  TRACE("");
  auto *tp = dynamic_cast<Digest *>(rdbuf());
  if (not tp) THROW("bad cast");
  clear(tp->good() ? std::ios::eofbit : std::ios::badbit);
  return tp->hash();
}

std::string mobs::digestStream::hashStr() {
  TRACE("");
  auto *tp = dynamic_cast<Digest *>(rdbuf());
  if (not tp) THROW("bad cast");
  clear(tp->good() ? std::ios::eofbit : std::ios::badbit);
  return tp->hashStr();
}

std::string mobs::digestStream::uuid() {
  TRACE("");
  auto *tp = dynamic_cast<Digest *>(rdbuf());
  if (not tp) THROW("bad cast");
  clear(tp->good() ? std::ios::eofbit : std::ios::badbit);
  if (not tp->good())
    THROW("can't create hash");
  auto it = tp->hash().begin();
  std::stringstream ss;
  int i;
  ss << std::hex;
  for (i = 0; i < 4; i++) {
    ss << std::setfill('0') << std::setw(2) << u_int(*it++);
  }
  ss << '-';
  for (i = 0; i < 2; i++) {
    ss << std::setfill('0') << std::setw(2) << u_int(*it++);
  }
  ss << '-';
  if (tp->algorithm == "sha1")
    ss << '5';
  else if (tp->algorithm == "md5")
    ss << '3';
  else
    THROW("uuid: only sha1 or md5 allowed");
  ss << std::setw(1) << (u_int(*it++) & 0xf);
  ss << std::setfill('0') << std::setw(2) << u_int(*it++);
  ss << '-';
  ss << std::setfill('0') << std::setw(2) << (( u_int(*it++) & 0x5f) | 0x80);
  ss << std::setfill('0') << std::setw(2) << u_int(*it++);
  ss << '-';
  for (i = 0; i < 6; i++) {
    ss << std::setfill('0') << std::setw(2) << u_int(*it++);
  };
  return ss.str();
}



std::string mobs::hash_value(const std::string &s, const std::string &algo) {
  mobs::digestStream ds(algo);
  ds.exceptions(std::ostream::failbit | std::ostream::badbit);
  ds << s;
  return ds.hashStr();
}

std::string mobs::hash_value(const std::vector<u_char> &s, const std::string &algo) {
  auto md = EVP_get_digestbyname(algo.c_str());
  if (not md)
    throw std::runtime_error(LOGSTR("mobs::CryptBufDigest '" << algo << "' doesn't exist"));
  auto mdctx = EVP_MD_CTX_new();
  if (not EVP_DigestInit_ex(mdctx, md, nullptr))
    throw openssl_exception(LOGSTR("mobs::CryptBufDigest"));
  EVP_DigestUpdate(mdctx, &s[0], s.size());

  std::vector<u_char> md_value;
  md_value.resize(EVP_MAX_MD_SIZE);
  u_int md_len;
  EVP_DigestFinal_ex(mdctx, &md_value[0], &md_len);
  md_value.resize(md_len);
  std::stringstream x;
  for (auto c:md_value)
    x << std::hex << std::setfill('0') << std::setw(2) << u_int(c);
  return x.str();
}

void mobs::hash_value(const std::vector<u_char> &s, std::vector<u_char> &hash, const std::string &algo) {
  auto md = EVP_get_digestbyname(algo.c_str());
  if (not md)
    throw std::runtime_error(LOGSTR("mobs::CryptBufDigest '" << algo << "' doesn't exist"));
  auto mdctx = EVP_MD_CTX_new();
  if (not EVP_DigestInit_ex(mdctx, md, nullptr))
    throw openssl_exception(LOGSTR("mobs::CryptBufDigest"));
  EVP_DigestUpdate(mdctx, &s[0], s.size());

  hash.resize(EVP_MAX_MD_SIZE);
  u_int md_len;
  EVP_DigestFinal_ex(mdctx, &hash[0], &md_len);
  hash.resize(md_len);
}

