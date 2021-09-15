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

#include "rsa.h"
#include "logging.h"
#include "converter.h"
#include "digest.h"

#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <utility>
#include <vector>
#include <cstring>



#define KEYBUFLEN 32
#define INPUT_BUFFER_LEN 1024

namespace mobs_internal {

std::string openSslGetError();


std::string openSslGetError() {
  u_long e;
  std::string res = "OpenSSL: ";
  while ((e = ERR_get_error())) {
    static bool errLoad = false;
    if (not errLoad) {
      errLoad = true;
//      ERR_load_crypto_strings(); // nur crypto-fehler laden
      SSL_load_error_strings(); // NOLINT(hicpp-signed-bitwise)
      ERR_load_BIO_strings();
      atexit([]() { ERR_free_strings(); });
    }
    res += ERR_error_string(e, nullptr);
  }
  return res;
}

}

namespace {

class openssl_exception : public std::runtime_error {
public:
  openssl_exception(const std::string &log = "") : std::runtime_error(log + " " + mobs_internal::openSslGetError()) {
    LOG(LM_DEBUG, "openssl: " << what());
  }
};


}









class mobs::CryptBufRsaData { // NOLINT(cppcoreguidelines-pro-type-member-init)
public:
  ~CryptBufRsaData()
  {
    if (ctx)
      EVP_CIPHER_CTX_free(ctx);
    EVP_PKEY_free(priv_key);
  }


  static RSA *readPrivateKey(const std::string &file, const std::string &passphrase) {
    BIO *bp;
    if (file.length() > 10 and file.compare(0, 10, "-----BEGIN") == 0) {
      bp = BIO_new_mem_buf(file.c_str(), file.length());
      if (not bp)
        THROW("alloc mem");
    }
    else {
      bp = BIO_new(BIO_s_file());;
      if (not bp)
        return nullptr;
      if (1 != BIO_read_filename(bp, file.c_str()))
        return nullptr;
    }

    // Load the RSA key from the BIO
    RSA* rsaPrivKey = nullptr;
    // wenn nullptr statt passphrase, wird auf der Konsole nachgefragt
    if (not PEM_read_bio_RSAPrivateKey( bp, &rsaPrivKey, nullptr, (void *)passphrase.c_str() )) {
      BIO_free_all( bp );
      throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
    }
    BIO_free_all( bp );
    return rsaPrivKey;
  }

  static RSA *readPublicKey(const std::string &file) {
    BIO *bp;
    if (file.length() > 10 and file.compare(0, 10, "-----BEGIN") == 0) {
      bp = BIO_new_mem_buf(file.c_str(), file.length());
      if (not bp)
        THROW("alloc mem");
    }
    else {
      bp = BIO_new(BIO_s_file());;
      if (not bp)
        return nullptr;
      if (1 != BIO_read_filename(bp, file.c_str()))
        return nullptr;
    }

    // Load the RSA key from the BIO
    RSA* rsaPubKey = nullptr;
    if (not PEM_read_bio_RSAPublicKey( bp, &rsaPubKey, nullptr, nullptr )) {
      BIO_free( bp );
      throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
    }
    BIO_free( bp );
    return rsaPubKey;
  }

  void initPubkeys(const std::list<CryptBufRsa::PubKey> &pupkeys) {

    if (not(ctx = EVP_CIPHER_CTX_new()))
      throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));

    for (auto &k:pupkeys) {
      RSA *rsaPupKey = readPublicKey(k.filename);
      if (not rsaPupKey)
        THROW("can't load public key " << k.filename);
      EVP_PKEY *pub_key = EVP_PKEY_new();
      if (1 != EVP_PKEY_set1_RSA(pub_key, rsaPupKey))
        throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));

      reciepients.emplace_back(pub_key, k.id);
    }

    std::vector<EVP_PKEY *> pub_keys(reciepients.size());
    std::vector<int> encrypted_key_len(reciepients.size());
    std::vector<u_char *> encrypted_key(reciepients.size());

    for (size_t i = 0; i < reciepients.size(); i++) {
      encrypted_key[i] = &reciepients[i].cipher[0];
      pub_keys[i] = reciepients[i].key;
    }

    if (not EVP_SealInit(ctx, EVP_aes_256_cbc(), &encrypted_key[0], &encrypted_key_len[0], &iv[0],
                         &pub_keys[0], reciepients.size()))
      throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
//  RSA_free(rsaPupKey);
    for (size_t i = 0; i < reciepients.size(); i++)
      reciepients[i].cipher.resize(encrypted_key_len[i]);
//  LOG(LM_DEBUG, "KEYLEN " << encrypted_key_len[0] << " " << EVP_PKEY_size(pub_key));
  }



class Receipt {
  public:
    Receipt(EVP_PKEY *k, std::string name) : id(std::move(name)), key(k) { cipher.resize(EVP_PKEY_size(key)); }
    Receipt() { EVP_PKEY_free(key); }
    std::string id;
    EVP_PKEY *key = nullptr;
    std::vector<u_char> cipher;
  };
  std::vector<Receipt> reciepients;
  std::array<mobs::CryptBufRsa::char_type, INPUT_BUFFER_LEN> buffer;
  std::array<u_char, KEYBUFLEN> iv{};
  EVP_CIPHER_CTX *ctx = nullptr;
  EVP_PKEY *priv_key = nullptr;
  std::vector<u_char> cipher;
  bool init = true;
  bool finished = false;
};


mobs::CryptBufRsa::CryptBufRsa(const std::list<PubKey> &keys) {
  TRACE("");
  data = new mobs::CryptBufRsaData;
  data->initPubkeys(keys);
}


mobs::CryptBufRsa::CryptBufRsa(const std::string &filename, const std::string &id) : CryptBufBase() {
  TRACE("");
  data = new mobs::CryptBufRsaData;
  std::list<PubKey> pk;
  pk.emplace_back(filename, id);
  data->initPubkeys(pk);

}

mobs::CryptBufRsa::CryptBufRsa(const std::string &filename, const std::vector<u_char> &cipher, const std::string &passphrase) : CryptBufBase() {
  TRACE("");
  data = new mobs::CryptBufRsaData;
  data->cipher = cipher;
  RSA *rsaPrivKey = data->readPrivateKey(filename, passphrase);
  data->priv_key = EVP_PKEY_new();
  if (1 != EVP_PKEY_set1_RSA(data->priv_key, rsaPrivKey))
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));

  if (not (data->ctx = EVP_CIPHER_CTX_new()))
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
//  memcpy(&data->iv[0], &g_iv[0], KEYBUFLEN);
}


mobs::CryptBufRsa::~CryptBufRsa() {
  TRACE("");
  delete data;
}


const std::vector<u_char> &mobs::CryptBufRsa::getRecipientKey(size_t pos) const {
  if (pos >= data->reciepients.size())
    THROW("pos exceeds size");
  return data->reciepients[pos].cipher;
}

size_t mobs::CryptBufRsa::recipients() const {
  return data->reciepients.size();
}

std::string mobs::CryptBufRsa::getRecipientId(size_t pos) const {
  if (pos >= data->reciepients.size())
    THROW("pos exceeds size");
  return data->reciepients[pos].id;
}

std::string mobs::CryptBufRsa::getRecipientKeyBase64(size_t pos) const {
  if (pos >= data->reciepients.size())
    THROW("pos exceeds size");
  return mobs::to_string_base64(data->reciepients[pos].cipher);
}



// first 16 bytes is IV
mobs::CryptBufRsa::int_type mobs::CryptBufRsa::underflow() {
  TRACE("");
//  std::cout << "underflow3 " << "\n";
  try {
    if (data->finished)
      return Traits::eof();
    std::array<u_char, INPUT_BUFFER_LEN + EVP_MAX_BLOCK_LENGTH> buf{}; // NOLINT(cppcoreguidelines-pro-type-member-init)

    size_t sz = doRead((char *) &buf[0], buf.size() - EVP_MAX_BLOCK_LENGTH);
    // Input Buffer wenigsten halb voll kriegen
    if (sz) {
      while (sz < buf.size() / 2) {
        auto szt = doRead((char *) &buf[sz], buf.size() - sz);
        if (not szt) {
          data->finished = true;
          break;
        }
        sz += szt;
      }
    }
    else
      data->finished = true;
    u_char *start = &buf[0];
    if (data->init) {
      int is = EVP_CIPHER_iv_length(EVP_aes_256_cbc());
      if (sz < is)
        THROW("data missing");
      memcpy(&data->iv[0], start, is);
      if (1 != EVP_OpenInit(data->ctx, EVP_aes_256_cbc(), &data->cipher[0], data->cipher.size(), &data->iv[0], data->priv_key))
        throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
      start += is;
      sz -= is;
      data->init = false;
    }
    if (not data->ctx)
      THROW("context is invalid");
    int len = 0;
//      if (1 != EVP_DecryptUpdate(data->ctx, (u_char *) &data->buffer[0], &len, start, sz))
    if (1 != EVP_OpenUpdate(data->ctx, (u_char *) &data->buffer[0], &len, start, sz))
      throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
//    LOG(LM_DEBUG, "GC2 = " << sz << " " << len << " " << std::string(&data->buffer[0], len));

    if (data->finished) {
      int lenf = 0;
      if (1 != EVP_OpenFinal(data->ctx, (u_char *) &data->buffer[len], &lenf))
        throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
      len += lenf;
      //    EVP_CIPHER_CTX_reset(data->ctx);
      EVP_CIPHER_CTX_free(data->ctx);
//      LOG(LM_DEBUG, "AES done");
      data->ctx = nullptr;
    }
    Base::setg(&data->buffer[0], &data->buffer[0], &data->buffer[len]);
    if (len == 0) {
      if (data->ctx)
        throw std::runtime_error("Keine Daten obwohl Quelle nicht leer");
      return Traits::eof();
    }
    return Traits::to_int_type(*Base::gptr());
  } catch (std::exception &e) {
    LOG(LM_ERROR, "Encryption error " << (data->init ? u8"in init ": u8"") << e.what());
    if (data->ctx) {
      EVP_CIPHER_CTX_free(data->ctx);
      data->ctx = nullptr;
    }
    setBad();
    return Traits::eof();
  }
}

// first 16 bytes is IV
mobs::CryptBufRsa::int_type mobs::CryptBufRsa::overflow(mobs::CryptBufRsa::int_type ch) {
  TRACE("");
//  std::cout << "overflow3 " << int(ch) << "\n";
  if (Base::pbase() != Base::pptr()) {
    if (not data->ctx)
      THROW("context is invalid");
    int len = 0;
    std::array<u_char, INPUT_BUFFER_LEN + EVP_MAX_BLOCK_LENGTH + KEYBUFLEN> buf; // NOLINT(cppcoreguidelines-pro-type-member-init)
    u_char *start = &buf[0];
    if (data->init) {
      data->init = false;
      int is = EVP_CIPHER_iv_length(EVP_aes_256_cbc());
      memcpy(start, &data->iv[0], is);
      start += is;
    }
    if (1 != EVP_SealUpdate(data->ctx, start, &len, (u_char *)(Base::pbase()),
                               std::distance(Base::pbase(), Base::pptr())))
      throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
     len +=  int(start - &buf[0]);
//      LOG(LM_INFO, "Writing " << len << "  was " << std::distance(Base::pbase(), Base::pptr()) << std::string(Base::pbase(), std::distance(Base::pbase(), Base::pptr())));
    doWrite((char *)(&buf[0]), len);
    CryptBufBase::setp(data->buffer.begin(), data->buffer.end()); // buffer zurücksetzen
  }

  if (not Traits::eq_int_type(ch, Traits::eof()))
    Base::sputc(ch);
  if (isGood())
    return ch;
  return Traits::eof();
}

void mobs::CryptBufRsa::finalize() {
  TRACE("");
  if (data->ctx) {
    std::array<u_char, EVP_MAX_BLOCK_LENGTH> buf; // NOLINT(cppcoreguidelines-pro-type-member-init)
    int len;
    if (1 != EVP_SealFinal(data->ctx, &buf[0], &len))
      throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
//    EVP_CIPHER_CTX_reset(data->ctx);
    EVP_CIPHER_CTX_free(data->ctx);
    data->ctx = nullptr;
//    LOG(LM_DEBUG, "Writing. " << len);
    doWrite(reinterpret_cast<char *>(&buf[0]), len);
  }
  CryptBufBase::finalize();
}


void mobs::generateRsaKey(const std::string &filePriv, const std::string &filePub, const std::string &passphrase) {
  RSA *rsa = RSA_new();
  BIGNUM *bne = BN_new();
  unsigned long	e = RSA_F4;

  if (1 != BN_set_word(bne,e))
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
  if (not RSA_generate_key_ex(rsa, 2048, bne, nullptr))
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
  BN_free(bne);
  BIO	*bp_public = BIO_new_file(filePub.c_str(), "w+");
  if (1 != PEM_write_bio_RSAPublicKey(bp_public, rsa))
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
  BIO_free_all(bp_public);
  BIO	*bp_private = BIO_new_file(filePriv.c_str(), "w+");

//  if (1 != PEM_write_bio_RSAPrivateKey(bp_private, rsa, nullptr, nullptr, 0, nullptr, nullptr)) // unverschlüsselte Ausgabe
  if (1 != PEM_write_bio_RSAPrivateKey(bp_private, rsa, EVP_des_ede3_cbc(), nullptr, 0, nullptr, (void *)passphrase.c_str()))
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
  BIO_free_all(bp_private);

//  cout <<  RSA_check_key(rsa) << endl;

  RSA_free(rsa);
}

void mobs::generateRsaKeyMem(std::string &priv, std::string &pub, const std::string &passphrase) {
  RSA *rsa = RSA_new();
  BIGNUM *bne = BN_new();
  unsigned long	e = RSA_F4;

  if (1 != BN_set_word(bne,e))
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
  if (not RSA_generate_key_ex(rsa, 2048, bne, nullptr))
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
  BN_free(bne);
  BIO	*bp_public = BIO_new(BIO_s_mem());
  if (not bp_public)
    THROW("alloc memory");
  if (1 != PEM_write_bio_RSAPublicKey(bp_public, rsa))
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
  char *bptr;
  auto n = BIO_get_mem_data(bp_public, &bptr);
  pub = std::string(bptr, n);
  BIO_free_all(bp_public);

  BIO	*bp_private = BIO_new(BIO_s_mem());
  if (passphrase.empty()) {
    if (1 != PEM_write_bio_RSAPrivateKey(bp_private, rsa, nullptr, nullptr, 0, nullptr, nullptr)) // unverschlüsselte Ausgabe
      throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
  } else {
    if (1 != PEM_write_bio_RSAPrivateKey(bp_private, rsa, EVP_des_ede3_cbc(), nullptr, 0, nullptr,
                                         (void *) passphrase.c_str()))
      throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
  }
  n = BIO_get_mem_data(bp_private, &bptr);
  priv = std::string(bptr, n);
  BIO_free_all(bp_private);

//  cout <<  RSA_check_key(rsa) << endl;

  RSA_free(rsa);
}


void
mobs::decryptPublicRsa(const std::vector<u_char> &cipher, std::vector<u_char> &sessionKey, const std::string &filePup) {
  RSA* rsaPubKey = mobs::CryptBufRsaData::readPublicKey(filePup);
  if (not rsaPubKey)
    THROW(u8"can't load pub key");
  if (cipher.size() != RSA_size(rsaPubKey))
    THROW(u8"cipher must have size of " << RSA_size(rsaPubKey));
  sessionKey.resize(RSA_size(rsaPubKey), 0);
  int sz = RSA_public_decrypt(cipher.size(), &cipher[0], &sessionKey[0], rsaPubKey, RSA_PKCS1_PADDING);
  if (sz < 0)
    throw openssl_exception(LOGSTR("mobs::decryptPublicRsa"));
  sessionKey.resize(sz);
  RSA_free(rsaPubKey);
}

void
mobs::encryptPrivateRsa(const std::vector<u_char> &sessionKey, std::vector<u_char> &cipher, const std::string &filePriv,
                        const std::string &passphrase) {
  RSA* rsaPrivKey = mobs::CryptBufRsaData::readPrivateKey(filePriv, passphrase);
  if (not rsaPrivKey)
    THROW(u8"can't load priv key");
  if (sessionKey.size() >= RSA_size(rsaPrivKey) - 11)
    THROW(u8"array to big");
  cipher.resize(RSA_size(rsaPrivKey), 0);
  if (0 > RSA_private_encrypt(sessionKey.size(), &sessionKey[0], &cipher[0], rsaPrivKey, RSA_PKCS1_PADDING))
    throw openssl_exception(LOGSTR("mobs::encryptPrivateRsa"));
  RSA_free(rsaPrivKey);
}

void
mobs::decryptPrivateRsa(const std::vector<u_char> &cipher, std::vector<u_char> &sessionKey, const std::string &filePriv,
                        const std::string &passphrase) {
  RSA* rsaPrivKey = mobs::CryptBufRsaData::readPrivateKey(filePriv, passphrase);
  if (not rsaPrivKey)
    THROW(u8"can't load priv key");
  if (cipher.size() != RSA_size(rsaPrivKey))
    THROW(u8"cipher must have size of " << RSA_size(rsaPrivKey));
  sessionKey.resize(RSA_size(rsaPrivKey), 0);
  int sz = RSA_private_decrypt(cipher.size(), &cipher[0], &sessionKey[0], rsaPrivKey, RSA_PKCS1_OAEP_PADDING);
  if (sz < 0)
    throw openssl_exception(LOGSTR("mobs::decryptPrivateRsa"));
  sessionKey.resize(sz);
  RSA_free(rsaPrivKey);
}

void
mobs::encryptPublicRsa(const std::vector<u_char> &sessionKey, std::vector<u_char> &cipher, const std::string &filePup) {
  RSA* rsaPubKey = mobs::CryptBufRsaData::readPublicKey(filePup);
  if (not rsaPubKey)
    THROW(u8"can't load pub key");
  if (sessionKey.size() >= RSA_size(rsaPubKey) - 41)
    THROW(u8"array to big");
  cipher.resize(RSA_size(rsaPubKey), 0);
  if (0 > RSA_public_encrypt(sessionKey.size(), &sessionKey[0], &cipher[0], rsaPubKey, RSA_PKCS1_OAEP_PADDING))
    throw openssl_exception(LOGSTR("mobs::encryptPublicRsa"));
  RSA_free(rsaPubKey);
}

bool mobs::checkPasswordRsa(const std::string &filePriv, const std::string &passphrase) {
  try {
    RSA *rsaPrivKey = mobs::CryptBufRsaData::readPrivateKey(filePriv, passphrase);
    if (not rsaPrivKey)
      return false;
    int ret = RSA_check_key(rsaPrivKey);
    RSA_free(rsaPrivKey);
    return ret == 1;
  } catch (openssl_exception &e) {
    return false;
  }
}


void mobs::exportKey(const std::string &filePriv, const std::string &passphraseOld, std::string &priv, std::string &pub, const std::string &passphraseNew) {
  RSA *rsa = mobs::CryptBufRsaData::readPrivateKey(filePriv, passphraseOld);
  if (not rsa)
    throw openssl_exception(LOGSTR("mobs::exportKey"));

  BIO	*bp_public = BIO_new(BIO_s_mem());
  if (not bp_public)
    THROW("alloc memory");
  if (1 != PEM_write_bio_RSAPublicKey(bp_public, rsa))
    throw openssl_exception(LOGSTR("mobs::exportKey"));
  char *bptr;
  auto n = BIO_get_mem_data(bp_public, &bptr);
  pub = std::string(bptr, n);
  BIO_free_all(bp_public);

  BIO	*bp_private = BIO_new(BIO_s_mem());
  if (passphraseNew.empty()) {
    if (1 != PEM_write_bio_RSAPrivateKey(bp_private, rsa, nullptr, nullptr, 0, nullptr, nullptr)) // unverschlüsselte Ausgabe
      throw openssl_exception(LOGSTR("mobs::exportKey"));
  } else {
    if (1 != PEM_write_bio_RSAPrivateKey(bp_private, rsa, EVP_des_ede3_cbc(), nullptr, 0, nullptr,
                                         (void *) passphraseNew.c_str()))
      throw openssl_exception(LOGSTR("mobs::exportKey"));
  }
  n = BIO_get_mem_data(bp_private, &bptr);
  priv = std::string(bptr, n);
  BIO_free_all(bp_private);
  RSA_free(rsa);
}

std::string mobs::getRsaInfo(const std::string &filePriv, const std::string &passphrase) {
  try {
    RSA *rsaPrivKey = mobs::CryptBufRsaData::readPrivateKey(filePriv, passphrase);
    //    RSA *rsaPrivKey = mobs::CryptBufRsaData::readPublicKey(filePriv);
    if (not rsaPrivKey)
      throw openssl_exception(LOGSTR("mobs::exportKey"));
    BIO	*bp_private = BIO_new(BIO_s_mem());
    if (1 != RSA_print(bp_private, rsaPrivKey, 3))
      throw openssl_exception(LOGSTR("mobs::exportKey"));
    char *bptr;
    auto n = BIO_get_mem_data(bp_private, &bptr);
    auto res = std::string(bptr, n);
    BIO_free_all(bp_private);
    RSA_free(rsaPrivKey);
    return res;
  } catch (openssl_exception &e) {
    return "";
  }
}

std::string mobs::getRsaFingerprint(const std::string &filePub) {
  RSA *rsa = mobs::CryptBufRsaData::readPublicKey(filePub);
  if (not rsa)
    throw openssl_exception(LOGSTR("mobs::getRsaFingerprint"));
  const BIGNUM *modulus = RSA_get0_n(rsa);
  unsigned int sz = BN_num_bytes(modulus);
//    std::cerr << sz << " " << BN_bn2hex(n) << std::endl;
  std::vector<unsigned char> buf;
  buf.resize(sz);
  BN_bn2bin(modulus, &buf[0]);
  std::string res = mobs::hash_value(buf, "md5");
  RSA_free(rsa);
  return res;
}