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


#include "objtypes.h"
#include "aes.h"
#include "logging.h"

#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
//#include <openssl/err.h>
#include <vector>
#include <iomanip>
#include <cstring>

#ifdef STREAMLOG
#define CSBLOG(x, y) LOG(x, y)
#else
#define CSBLOG(x, y)
#endif

#define KEYBUFLEN 32
#define INPUT_BUFFER_LEN 4096

namespace mobs_internal {

// -> rsa.cpp
std::string openSslGetError();

}

namespace {

//std::string getError() {
//  u_long e;
//  std::string res = "OpenSSL: ";
//  while ((e = ERR_get_error())) {
//    static bool errLoad = false;
//    if (not errLoad) {
//      errLoad = true;
////      ERR_load_crypto_strings(); // nur crypto-fehler laden
//      SSL_load_error_strings(); // NOLINT(hicpp-signed-bitwise)
//      atexit([](){ ERR_free_strings(); });
//    }
//    res += ERR_error_string(e, nullptr);
//  }
//  return res;
//}

class openssl_exception : public std::runtime_error {
public:
  explicit openssl_exception(const std::string &log = "") : std::runtime_error(log + " " + mobs_internal::openSslGetError()) {
    CSBLOG(LM_DEBUG, "openssl: " << what());
  }
};

}

class mobs::CryptBufAesData { // NOLINT(cppcoreguidelines-pro-type-member-init)
public:
  ~CryptBufAesData()
  {
    if (ctx)
      EVP_CIPHER_CTX_free(ctx);
    if (mdctx)
      EVP_MD_CTX_free(mdctx);
  }
  void initAES()
  {
    // This initially zeros out the Key and IV, and then uses the EVP_BytesToKey to populate these two data structures.
    // In this case we are using Sha1 as the key-derivation function and the same password used when we encrypted the
    // plaintext. We use a single iteration (the 6th parameter).
//    key.fill(0);
//    iv.fill(0);
    if (not EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), &salt[0], (u_char*)passwd.c_str(), passwd.length(), 1, &key[0], &iv[0]))
      throw openssl_exception(LOGSTR("mobs::CryptBufAes"));
  }

  void newSalt()
  {
    int r = RAND_bytes(&salt[0], salt.size());
    if (r < 0)
      throw openssl_exception(LOGSTR("mobs::CryptBufAes"));
  }

  void md_init() {
    if (not md_algo.empty()) {
      md = EVP_get_digestbyname(md_algo.c_str());
      if (not md)
        THROW("hash algorithm " << md_algo << " not available");
      mdctx = EVP_MD_CTX_new();
      EVP_DigestInit_ex(mdctx, md, nullptr);
    }
  }

  std::array<mobs::CryptBufAes::char_type, INPUT_BUFFER_LEN + EVP_MAX_BLOCK_LENGTH> buffer;
  std::array<u_char, INPUT_BUFFER_LEN + EVP_MAX_BLOCK_LENGTH> inputBuf; // NOLINT(cppcoreguidelines-pro-type-member-init)
  u_char *inputStart = &inputBuf[0];
  std::array<u_char, 8> salt{};
  std::array<u_char, KEYBUFLEN> iv{};
  std::array<u_char, KEYBUFLEN> key{};
  std::vector<u_char> md_value{}; // hash-wert
  EVP_CIPHER_CTX *ctx = nullptr;
  EVP_MD_CTX *mdctx = nullptr; // hash context
  const EVP_MD *md = nullptr; // hash algo
  std::string md_algo;
  std::string passwd;
  std::string id;
  bool initIV = false;  // iv aus stream lesen
  bool salted = false;  // iv aus salt generieren und schreiben
  bool finished = false;
};

const std::vector<u_char> &mobs::CryptBufAes::hash() const {
  return data->md_value;
}

std::string mobs::CryptBufAes::hashStr() const {
  std::stringstream x;
  for (auto c:data->md_value)
    x << std::hex << std::setfill('0') << std::setw(2) << u_int(c);
  return x.str();
}

void mobs::CryptBufAes::hashAlgorithm(const std::string &algo) {
  data->md_algo = algo;
}

void mobs::CryptBufAes::getRand(std::vector<u_char> &rand) {
  int r = RAND_bytes(&rand[0], rand.size());
  if (r < 0)
    throw openssl_exception(LOGSTR("mobs::CryptBufAes"));
}


mobs::CryptBufAes::CryptBufAes(const std::string &pass, const std::string &id) : CryptBufBase() {
  TRACE("");
  data = std::unique_ptr<CryptBufAesData>(new mobs::CryptBufAesData);
  data->passwd = pass;
  data->id = id;
  data->salted = true;
}

mobs::CryptBufAes::CryptBufAes(const std::vector<u_char> &key, const std::vector<u_char> &iv, const std::string &id, bool writeIV) {
  TRACE("");
  data = std::unique_ptr<CryptBufAesData>(new mobs::CryptBufAesData);
  data->id = id;
  data->initIV = writeIV;
  memcpy(&data->key[0], &key[0], std::min(key.size(), size_t(KEYBUFLEN)));
  memcpy(&data->iv[0], &iv[0], std::min(iv.size(), size_t(KEYBUFLEN)));
}

mobs::CryptBufAes::CryptBufAes(const std::vector<u_char> &key, const std::string &id) {
  TRACE("");
  data = std::unique_ptr<CryptBufAesData>(new mobs::CryptBufAesData);
  data->id = id;
  data->initIV = true;
  memcpy(&data->key[0], &key[0], std::min(key.size(), size_t(KEYBUFLEN)));
}

mobs::CryptBufAes::~CryptBufAes() {
  TRACE("");
}


std::streamsize mobs::CryptBufAes::showmanyc() {
  if (data->finished)
    return -1;
  std::streamsize s = canRead();
  if (s == 0)
    return 0;
  return underflowWorker(true);
}

/*
int mobs::CryptBufAes::sync() {
  TRACE("");
  LOG(LM_INFO, "CryptBufAes::sync");
  CryptBufBase::sync();
  //finalize();
  return isGood() ? 0 : -1;
}
*/


mobs::CryptBufAes::int_type mobs::CryptBufAes::underflow() {
  TRACE("");
//  std::cout << "underflow3 " << "\n";
  try {
    if (data->finished)
      return Traits::eof();
    int len = underflowWorker(false);
 //    std::cout << "GC2 = " << len << " ";
    if (len)
      return Traits::to_int_type(*Base::gptr());
    if (data->ctx)
      throw std::runtime_error("Keine Daten obwohl Quelle nicht leer");
  } catch (std::exception &e) {
    LOG(LM_ERROR, "Exception " << e.what());
    if (data->ctx) {
      EVP_CIPHER_CTX_free(data->ctx);
      data->ctx = nullptr;
    }
    setBad();
    throw std::ios_base::failure(e.what(), std::io_errc::stream);
  }
  return Traits::eof();
}

int mobs::CryptBufAes::underflowWorker(bool nowait) {
  int len;
  std::streamsize sz = std::distance(&data->inputBuf[0], data->inputStart);
  do {
    std::streamsize s = nowait ? canRead() : data->inputBuf.size() - EVP_MAX_BLOCK_LENGTH - sz;
    if (s > data->inputBuf.size() - EVP_MAX_BLOCK_LENGTH - sz)
      s = data->inputBuf.size() - EVP_MAX_BLOCK_LENGTH - sz;
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

  {
    u_char *start = &data->inputBuf[0];
    if (not data->ctx) {
//        LOG(LM_INFO, "AES init");
      if (data->initIV) {
        size_t is = iv_size();
        if (sz < is) {
          if (nowait)
            return 0;
          THROW("data missing");
        }
        memcpy(&data->iv[0], start, is);
        start += is;
        sz -= is;
      } else if (data->salted and sz >= 16 and std::string((char *) &data->inputBuf[0], 8) == "Salted__") {
        memcpy(&data->salt[0], start + 8, 8);
        start += 16;
        sz -= 16;
        data->initAES();
      }
      else if (nowait and data->salted and sz < 16)
        return 0;

      if (not(data->ctx = EVP_CIPHER_CTX_new()))
        throw openssl_exception(LOGSTR("mobs::CryptBufAes"));
      if (1 != EVP_DecryptInit_ex(data->ctx, EVP_aes_256_cbc(), nullptr, &data->key[0], &data->iv[0]))
        throw openssl_exception(LOGSTR(""));
      data->md_init();
      data->initIV = false;
    }
    data->inputStart = &data->inputBuf[0];
    if (1 != EVP_DecryptUpdate(data->ctx, (u_char *) &data->buffer[0], &len, start, sz))
      throw openssl_exception(LOGSTR("mobs::CryptBufAes"));
  }
  if (data->finished) {
    int lenf;
    if (1 != EVP_DecryptFinal_ex(data->ctx, (u_char *) &data->buffer[len], &lenf))
      throw openssl_exception(LOGSTR("mobs::CryptBufAes"));
    len += lenf;
    //    EVP_CIPHER_CTX_reset(data->ctx);
    EVP_CIPHER_CTX_free(data->ctx);
//      LOG(LM_INFO, "AES done");
    data->ctx = nullptr;
  }
  if (data->mdctx) {
    EVP_DigestUpdate(data->mdctx, &data->buffer[0], len);
    if (data->finished) {
      data->md_value.resize(EVP_MAX_MD_SIZE);
      u_int md_len;
      EVP_DigestFinal_ex(data->mdctx, &data->md_value[0], &md_len);
      data->md_value.resize(md_len);
    }
  }
  Base::setg(&data->buffer[0], &data->buffer[0], &data->buffer[len]);
  return len;
}

void mobs::CryptBufAes::ctxInit() {
  if (not data->ctx) {
    if (data->salted) {
      CSBLOG(LM_DEBUG, "AES init");
      openSalt();
      data->initAES();
    }
    if (not (data->ctx = EVP_CIPHER_CTX_new()))
      throw openssl_exception(LOGSTR("mobs::CryptBufAes"));
    if (1 != EVP_EncryptInit_ex(data->ctx, EVP_aes_256_cbc(), nullptr, &data->key[0], &data->iv[0]))
      throw openssl_exception(LOGSTR("mobs::CryptBufAes"));
    data->md_init();
  }
}

mobs::CryptBufAes::int_type mobs::CryptBufAes::overflow(mobs::CryptBufAes::int_type ch) {
  TRACE("");
//  std::cout << "overflow3 " << int(ch) << "\n";
  try {
    ctxInit();

    if (Base::pbase() != Base::pptr()) {
      int len;
      std::array<u_char, INPUT_BUFFER_LEN + EVP_MAX_BLOCK_LENGTH> buf; // NOLINT(cppcoreguidelines-pro-type-member-init)
      size_t ofs = 0;
      if (data->initIV) {
        data->initIV = false;
        ofs = iv_size();
        memcpy(&buf[0], &data->iv[0], ofs);
      }
      if (data->mdctx)
        EVP_DigestUpdate(data->mdctx, (u_char *) Base::pbase(), std::distance(Base::pbase(), Base::pptr()));

//    size_t  s = std::distance(Base::pbase(), Base::pptr());
//    char *cp =  (Base::pbase());
      if (1 != EVP_EncryptUpdate(data->ctx, &buf[ofs], &len, (u_char *) (Base::pbase()),
                                 std::distance(Base::pbase(), Base::pptr())))
        throw openssl_exception(LOGSTR("mobs::CryptBufAes"));
//    CSBLOG(LM_DEBUG, "Writing " << len << "  was " << std::distance(Base::pbase(), Base::pptr()));
      len += ofs;
      doWrite((char *) (&buf[0]), len);
//      std::cout << "overflow2 schreibe " << std::distance(Base::pbase(), Base::pptr()) << "\n";

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

void mobs::CryptBufAes::finalize() {
  TRACE("");
  // bei leerer Eingabe, hier Verschlüsselung beginnen
  ctxInit();
  if (data->initIV) {
    data->initIV = false;
    doWrite((char *)(&data->iv[0]), iv_size());
  }
  pubsync();
  if (data->ctx) {
    std::array<u_char, EVP_MAX_BLOCK_LENGTH> buf; // NOLINT(cppcoreguidelines-pro-type-member-init)
    int len;
    if (1 != EVP_EncryptFinal_ex(data->ctx, &buf[0], &len))
      throw openssl_exception(LOGSTR("mobs::CryptBufAes"));
//    EVP_CIPHER_CTX_reset(data->ctx);
    EVP_CIPHER_CTX_free(data->ctx);
    data->ctx = nullptr;
    //CSBLOG(LM_DEBUG, "Writing. " << len);
    doWrite(reinterpret_cast<char *>(&buf[0]), len);
    if (data->mdctx) {
      data->md_value.resize(EVP_MAX_MD_SIZE);
      u_int md_len;
      EVP_DigestFinal_ex(data->mdctx, &data->md_value[0], &md_len);
      data->md_value.resize(md_len);
    }
  }
  CryptBufBase::finalize();
}

void mobs::CryptBufAes::openSalt() {
  TRACE("");
  data->newSalt();
  doWrite("Salted__", 8);
  doWrite(reinterpret_cast<char *>(&data->salt[0]), data->salt.size());
  //CSBLOG(LM_DEBUG, "Writing salt " << 8 + data->salt.size());
}

std::string mobs::CryptBufAes::getRecipientId(size_t pos) const {
  return data->id;
}

size_t mobs::CryptBufAes::iv_size() {
  int i = EVP_CIPHER_iv_length(EVP_aes_256_cbc());
  if (i < 0)
    throw openssl_exception(LOGSTR("mobs::CryptBufAes"));
  return size_t(i);
}


std::string mobs::to_aes_string(const std::string &s, const std::string &pass) {
  TRACE("");
  std::stringstream ss;

  mobs::CryptOstrBuf streambuf(ss, new mobs::CryptBufAes(pass));
  std::wostream xStrOut(&streambuf);
  xStrOut << mobs::CryptBufAes::base64(true);
  xStrOut << mobs::to_wstring(s);
  streambuf.finalize();

  return ss.str();
}

std::string mobs::from_aes_string(const std::string &s, const std::string &pass) {
  TRACE("");
  std::stringstream ss(s);
  mobs::CryptIstrBuf streambuf(ss, new mobs::CryptBufAes(pass));
  std::wistream xStrIn(&streambuf);
  xStrIn.exceptions(std::ios::badbit);
  streambuf.getCbb()->setBase64(true);
  std::string res;
  wchar_t c;
  while (not xStrIn.get(c).eof())
    res += u_char(c);
  return res;
 }




