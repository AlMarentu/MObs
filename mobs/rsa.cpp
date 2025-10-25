// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2025 Matthias Lautner
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
#include <openssl/decoder.h>
#include <openssl/encoder.h>
#include <openssl/engine.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <utility>
#include <array>
#include <vector>
#include <cstring>
#include <iomanip>


#define KEYBUFLEN 32
#define INPUT_BUFFER_LEN 1024

namespace mobs_internal {
std::string openSslGetError();
}


namespace {

class openssl_exception : public std::runtime_error {
public:
  explicit openssl_exception(const std::string &log = "") : std::runtime_error(log + " " + mobs_internal::openSslGetError()) {
    LOG(LM_ERROR, "openssl: " << what());
  }
};

class SSL_Delete {
  SSL_Delete() = default;
public:
  void operator()(BIO *p) { BIO_free_all(p); }
  void operator()(OSSL_ENCODER_CTX *p) { OSSL_ENCODER_CTX_free(p); }
  void operator()(EVP_PKEY *p) { EVP_PKEY_free(p); }
  void operator()(EVP_PKEY_CTX *p) { EVP_PKEY_CTX_free(p); }
  void operator()(EVP_CIPHER_CTX *p) { EVP_CIPHER_CTX_free(p); }
};

}



class mobs::CryptBufRsaData { // NOLINT(cppcoreguidelines-pro-type-member-init)
public:
  CryptBufRsaData() = default;
  ~CryptBufRsaData()
  {
    if (ctx)
      EVP_CIPHER_CTX_free(ctx);
  }


  static std::unique_ptr<EVP_PKEY, SSL_Delete> readPrivateKey2(const std::string &file, const std::string &passphrase) {
    std::unique_ptr<BIO, SSL_Delete> bp{nullptr, SSL_Delete{}};
    if (file.length() > 10 and file.compare(0, 10, "-----BEGIN") == 0) {
      bp.reset(BIO_new_mem_buf(file.c_str(), file.length()));
      if (not bp)
        THROW("alloc mem");
    }
    else {
      bp.reset(BIO_new(BIO_s_file()));
      if (not bp)
        return {nullptr, SSL_Delete{}};
      if (1 != BIO_read_filename(bp.get(), file.c_str()))
        return {nullptr, SSL_Delete{}};
    }

// Load the RSA key from the BIO
    EVP_PKEY* rsaPrivKey = nullptr;
// wenn nullptr statt passphrase, wird auf der Konsole nachgefragt
    if (not PEM_read_bio_PrivateKey( bp.get(), &rsaPrivKey, nullptr, (void *)passphrase.c_str() ))
      throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
    std::unique_ptr<EVP_PKEY, SSL_Delete> key(rsaPrivKey, SSL_Delete{});
    if (not EVP_PKEY_is_a(key.get(), "RSA") and not EVP_PKEY_is_a(key.get(), "EC"))
      THROW("IS NO RSA KEY: "); // << EVP_PKEY_get0_type_name(rsaPrivKey));
    return key;
  }

  static std::unique_ptr<EVP_PKEY, SSL_Delete> readPublicKey2(const std::string &file) {
    std::unique_ptr<BIO, SSL_Delete> bp{nullptr, SSL_Delete{}};
    if (file.length() > 10 and file.compare(0, 10, "-----BEGIN") == 0) {
      bp.reset(BIO_new_mem_buf(file.c_str(), file.length()));
      if (not bp)
        THROW("alloc mem");
    }
    else {
      bp.reset(BIO_new(BIO_s_file()));
      if (not bp)
        return {nullptr, SSL_Delete{}};
      if (1 != BIO_read_filename(bp.get(), file.c_str()))
        return {nullptr, SSL_Delete{}};
    }

    // Load the RSA key from the BIO
    EVP_PKEY* rsaPubKey = nullptr;
    if (not PEM_read_bio_PUBKEY(bp.get(), &rsaPubKey, nullptr, nullptr ))
      throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
    std::unique_ptr<EVP_PKEY, SSL_Delete> key(rsaPubKey, SSL_Delete{});

    if (not EVP_PKEY_is_a(key.get(), "RSA") and not EVP_PKEY_is_a(key.get(), "EC"))
      THROW("IS NO RSA KEY"); // << EVP_PKEY_get0_type_name(rsaPubKey));
    return key;
  }

  void initPubkeys(const std::list<CryptBufRsa::PubKey> &pubkeys) {

    if (not(ctx = EVP_CIPHER_CTX_new()))
      throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));

    for (auto &k:pubkeys) {
      auto rsaPupKey = readPublicKey2(k.filename);
      if (not rsaPupKey)
        THROW("can't load public key " << k.filename);
      recipients.emplace_back(rsaPupKey, k.id);
    }

    std::vector<EVP_PKEY *> pub_keys(recipients.size());
    std::vector<int> encrypted_key_len(recipients.size());
    std::vector<u_char *> encrypted_key(recipients.size());

    for (size_t i = 0; i < recipients.size(); i++) {
      encrypted_key[i] = &recipients[i].cipher[0];
      pub_keys[i] = recipients[i].key.get();
    }

    size_t ivlen = EVP_CIPHER_get_iv_length(EVP_aes_256_cbc());
    //iv.resize(ivlen);
    if (not EVP_SealInit(ctx, EVP_aes_256_cbc(), &encrypted_key[0], &encrypted_key_len[0], &iv[0],
                         &pub_keys[0], recipients.size()))
      throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
    for (size_t i = 0; i < recipients.size(); i++) {
      auto &rec = recipients[i];
      rec.cipher.resize(encrypted_key_len[i]);
      rec.key = nullptr;
      std::stringstream x;
      for (auto c:rec.cipher)
        x << std::hex << std::setfill('0') << std::setw(2) << u_int(c);
      x << " iv: ";
      for (auto c:iv)
        x << std::hex << std::setfill('0') << std::setw(2) << u_int(c);
      LOG(LM_INFO, "SSSS " << ivlen << " " << rec.cipher.size() << " " << encrypted_key_len[i] << " " << x.str());
    }
    //  LOG(LM_DEBUG, "KEYLEN " << encrypted_key_len[0] << " " << EVP_PKEY_size(pub_key));
  }

// EVP_CIPHER_get_iv_length(type)

  class Receipt {
  public:
    Receipt(std::unique_ptr<EVP_PKEY, SSL_Delete> &k, std::string name) : id(std::move(name)), key(std::move(k)) { cipher.resize(EVP_PKEY_get_size(key.get())); }
    std::string id;
    std::unique_ptr<EVP_PKEY, SSL_Delete> key;
    std::vector<u_char> cipher;
  };
  std::vector<Receipt> recipients;
  std::array<mobs::CryptBufRsa::char_type, INPUT_BUFFER_LEN> buffer;
  std::array<u_char, KEYBUFLEN> iv{};
  EVP_CIPHER_CTX *ctx = nullptr;
  std::unique_ptr<EVP_PKEY, SSL_Delete> priv_key {nullptr, SSL_Delete{}};
  std::vector<u_char> cipher;
  bool init = true;
  bool finished = false;
};


mobs::CryptBufRsa::CryptBufRsa(const std::list<PubKey> &keys) {
  TRACE("");
  data = std::unique_ptr<mobs::CryptBufRsaData>(new mobs::CryptBufRsaData);
  data->initPubkeys(keys);
}


mobs::CryptBufRsa::CryptBufRsa(const std::string &filename, const std::string &id) : CryptBufBase() {
  TRACE("");
  data = std::unique_ptr<mobs::CryptBufRsaData>(new mobs::CryptBufRsaData);
  std::list<PubKey> pk;
  pk.emplace_back(filename, id);
  data->initPubkeys(pk);

}

mobs::CryptBufRsa::CryptBufRsa(const std::string &filename, const std::vector<u_char> &cipher, const std::string &passphrase) : CryptBufBase() {
  TRACE("");
  data = std::unique_ptr<mobs::CryptBufRsaData>(new mobs::CryptBufRsaData);
  data->cipher = cipher;
  if (not (data->priv_key = mobs::CryptBufRsaData::readPrivateKey2(filename, passphrase)))
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
  if (not (data->ctx = EVP_CIPHER_CTX_new()))
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
//  memcpy(&data->iv[0], &g_iv[0], KEYBUFLEN);
}


mobs::CryptBufRsa::~CryptBufRsa() {
  TRACE("");
}


const std::vector<u_char> &mobs::CryptBufRsa::getRecipientKey(size_t pos) const {
  if (pos >= data->recipients.size())
    THROW("pos exceeds size");
  return data->recipients[pos].cipher;
}

size_t mobs::CryptBufRsa::recipients() const {
  return data->recipients.size();
}

std::string mobs::CryptBufRsa::getRecipientId(size_t pos) const {
  if (pos >= data->recipients.size())
    THROW("pos exceeds size");
  return data->recipients[pos].id;
}

std::string mobs::CryptBufRsa::getRecipientKeyBase64(size_t pos) const {
  if (pos >= data->recipients.size())
    THROW("pos exceeds size");
  return mobs::to_string_base64(data->recipients[pos].cipher);
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
    // Input Buffer wenigstens halb voll kriegen
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
      if (1 != EVP_OpenInit(data->ctx, EVP_aes_256_cbc(), &data->cipher[0], data->cipher.size(), &data->iv[0], data->priv_key.get()))
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
    throw std::ios_base::failure(e.what(), std::io_errc::stream);
    //return Traits::eof();
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




static void exportPrivateKeyToFile(EVP_PKEY *pkey, const std::string &filePriv, const std::string &passphrase, const std::string &format = "PEM")
{
  // bei DER: *structure = "PrivateKeyInfo"; /* PKCS#8 structure */ oder EncryptedPrivateKeyInfo
  // OSSL_KEYMGMT_SELECT_KEYPAIR
  std::unique_ptr<BIO, SSL_Delete> bp_private(BIO_new_file(filePriv.c_str(), "w+"), SSL_Delete{});
  std::unique_ptr<OSSL_ENCODER_CTX, SSL_Delete> ectx(OSSL_ENCODER_CTX_new_for_pkey(pkey,
                               OSSL_KEYMGMT_SELECT_PRIVATE_KEY
                                       | OSSL_KEYMGMT_SELECT_DOMAIN_PARAMETERS,
                                       format.c_str(), nullptr,
                                       nullptr), SSL_Delete{});
  if (not ectx or not bp_private)
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa no suitable potential encoders found"));
  if (not passphrase.empty()) {
    OSSL_ENCODER_CTX_set_passphrase(ectx.get(), (const unsigned char *)passphrase.c_str(), passphrase.length());
    //OSSL_ENCODER_CTX_set_cipher(ectx.get(), "AES-256-CBC", nullptr);
    OSSL_ENCODER_CTX_set_cipher(ectx.get(), "DES-EDE3-CBC", nullptr);
  }
  if (not OSSL_ENCODER_to_bio(ectx.get(), bp_private.get()))
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa write failed"));
}

static void exportPublicKeyToFile(EVP_PKEY *pkey, const std::string &filePub, const std::string &format = "PEM")
{
  std::unique_ptr<BIO, SSL_Delete> bp_public(BIO_new_file(filePub.c_str(), "w+"), SSL_Delete{});
  std::unique_ptr<OSSL_ENCODER_CTX, SSL_Delete> ectx(OSSL_ENCODER_CTX_new_for_pkey(pkey,
                                       OSSL_KEYMGMT_SELECT_PUBLIC_KEY
                                       | OSSL_KEYMGMT_SELECT_DOMAIN_PARAMETERS,
                                       format.c_str(), nullptr,
                                       nullptr), SSL_Delete{});
  if (not ectx or not bp_public)
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa no suitable potential encoders found"));
  if (not OSSL_ENCODER_to_bio(ectx.get(), bp_public.get()))
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa write failed"));
}

std::string exportPrivateKey(EVP_PKEY *pkey, const std::string &passphrase)
{
  const char *format = "PEM";
  std::unique_ptr<OSSL_ENCODER_CTX, SSL_Delete> ectx(OSSL_ENCODER_CTX_new_for_pkey(pkey,
                                       OSSL_KEYMGMT_SELECT_PRIVATE_KEY
                                       | OSSL_KEYMGMT_SELECT_DOMAIN_PARAMETERS,
                                       format, nullptr,
                                       nullptr), SSL_Delete{});
  if (not ectx)
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa no suitable potential encoders found"));
  if (not passphrase.empty()) {
    OSSL_ENCODER_CTX_set_passphrase(ectx.get(), (const unsigned char *)passphrase.c_str(), passphrase.length());
    //OSSL_ENCODER_CTX_set_cipher(ectx.get(), "AES-256-CBC", nullptr);
    OSSL_ENCODER_CTX_set_cipher(ectx.get(), "DES-EDE3-CBC", nullptr);
  }
  unsigned char *data = nullptr;
  size_t datalen;
  if (not OSSL_ENCODER_to_data(ectx.get(), &data, &datalen))
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa write failed"));
  auto mem = std::string((char *)data, datalen);
  free(data);
  return mem;
}

std::string exportPublicKey(EVP_PKEY *pkey)
{
  const char *format = "PEM";
  std::unique_ptr<OSSL_ENCODER_CTX, SSL_Delete> ectx(OSSL_ENCODER_CTX_new_for_pkey(pkey,
                                       OSSL_KEYMGMT_SELECT_PUBLIC_KEY
                                       | OSSL_KEYMGMT_SELECT_DOMAIN_PARAMETERS,
                                       format, nullptr,
                                       nullptr), SSL_Delete{});
  if (not ectx)
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa no suitable potential encoders found"));
  unsigned char *data = nullptr;
  size_t datalen;
  if (not OSSL_ENCODER_to_data(ectx.get(), &data, &datalen))
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa write failed"));
  auto mem = std::string((char *)data, datalen);
  free(data);
  return mem;
}


void mobs::generateRsaKey(const std::string &filePriv, const std::string &filePub, const std::string &passphrase) {
  std::unique_ptr<EVP_PKEY_CTX, SSL_Delete> ctx(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr), SSL_Delete{});
  if (!ctx)
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
  if (EVP_PKEY_keygen_init(ctx.get()) <= 0)
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
  if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx.get(), 2048) <= 0)
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
  /* Generate key */
  EVP_PKEY *pkey = nullptr;
  if (EVP_PKEY_keygen(ctx.get(), &pkey) <= 0)
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
  std::unique_ptr<EVP_PKEY, SSL_Delete> key(pkey, SSL_Delete{});

  exportPublicKeyToFile(key.get(), filePub);
  exportPrivateKeyToFile(key.get(), filePriv, passphrase);
}

void mobs::generateRsaKeyMem(std::string &priv, std::string &pub, const std::string &passphrase) {
  std::unique_ptr<EVP_PKEY_CTX, SSL_Delete> ctx(EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr), SSL_Delete{});
  if (!ctx)
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
  if (EVP_PKEY_keygen_init(ctx.get()) <= 0)
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
  if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx.get(), 2048) <= 0)
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
  /* Generate key */
  EVP_PKEY *pkey = nullptr;
  if (EVP_PKEY_keygen(ctx.get(), &pkey) <= 0)
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
  std::unique_ptr<EVP_PKEY, SSL_Delete> key(pkey, SSL_Delete{});

  pub = exportPublicKey(key.get());
  priv = exportPrivateKey(key.get(), passphrase);
}


void
mobs::decryptPublicRsa(const std::vector<u_char> &cipher, std::vector<u_char> &sessionKey, const std::string &filePup) {
  auto rsaPubKey = mobs::CryptBufRsaData::readPublicKey2(filePup);
  if (not rsaPubKey)
    THROW(u8"can't load pub key");
  if (cipher.size() != EVP_PKEY_size(rsaPubKey.get()))
    THROW(u8"cipher must have size of " << EVP_PKEY_size(rsaPubKey.get()) << " but has " << cipher.size());
  sessionKey.resize(EVP_PKEY_size(rsaPubKey.get()), 0);
  std::unique_ptr<EVP_PKEY_CTX, SSL_Delete>ctx(EVP_PKEY_CTX_new_from_pkey(nullptr, rsaPubKey.get(), nullptr), SSL_Delete{});
  if (!ctx)
    throw openssl_exception(LOGSTR("mobs::decryptPublicRsa"));
  if (1 != EVP_PKEY_public_check(ctx.get()))
    THROW("IS NO PUBLIC KEY");

  if (EVP_PKEY_verify_recover_init(ctx.get()) <= 0)
    throw openssl_exception(LOGSTR("mobs::decryptPublicRsa"));

  //if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0)
  //  throw openssl_exception(LOGSTR("mobs::decryptPublicRsa"));
  size_t sz = sessionKey.size();
  if (EVP_PKEY_verify_recover(ctx.get(), &sessionKey[0], &sz, &cipher[0], cipher.size()) <= 0)
    throw openssl_exception(LOGSTR("mobs::decryptPublicRsa"));
  sessionKey.resize(sz);
}

void
mobs::encryptPrivateRsa(const std::vector<u_char> &sessionKey, std::vector<u_char> &cipher, const std::string &filePriv,
                        const std::string &passphrase) {
  auto rsaPrivKey = mobs::CryptBufRsaData::readPrivateKey2(filePriv, passphrase);
  if (not rsaPrivKey)
    THROW(u8"can't load priv key");
  if (sessionKey.size() >= EVP_PKEY_size(rsaPrivKey.get()) - 11)
    THROW(u8"array to big");
  cipher.resize(EVP_PKEY_size(rsaPrivKey.get()), 0);
  std::unique_ptr<EVP_PKEY_CTX, SSL_Delete>ctx(EVP_PKEY_CTX_new_from_pkey(nullptr, rsaPrivKey.get(), nullptr), SSL_Delete{});
  if (!ctx)
    throw openssl_exception(LOGSTR("mobs::encryptPrivateRsa"));
  if (1 != EVP_PKEY_private_check(ctx.get()))
    THROW("IS NO PRIVATE KEY");
  if (EVP_PKEY_sign_init(ctx.get()) <= 0)
    throw openssl_exception(LOGSTR("mobs::encryptPrivateRsa"));
  //if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0)
    //throw openssl_exception(LOGSTR("mobs::encryptPrivateRsa"));
  size_t sz = cipher.size();
  if (EVP_PKEY_sign(ctx.get(), &cipher[0], &sz, &sessionKey[0], sessionKey.size()) <= 0)
    throw openssl_exception(LOGSTR("mobs::encryptPrivateRsa"));
}

void
mobs::decryptPrivateRsa(const std::vector<u_char> &cipher, std::vector<u_char> &sessionKey, const std::string &filePriv,
                        const std::string &passphrase) {
  auto rsaPrivKey = mobs::CryptBufRsaData::readPrivateKey2(filePriv, passphrase);
  if (not rsaPrivKey)
    THROW(u8"can't load priv key");
  if (cipher.size() != EVP_PKEY_size(rsaPrivKey.get()))
    THROW(u8"cipher must have size of " << EVP_PKEY_size(rsaPrivKey.get()));
  sessionKey.resize(EVP_PKEY_size(rsaPrivKey.get()), 0);

  std::unique_ptr<EVP_PKEY_CTX, SSL_Delete>ctx(EVP_PKEY_CTX_new_from_pkey(nullptr, rsaPrivKey.get(), nullptr), SSL_Delete{});
  if (!ctx)
    throw openssl_exception(LOGSTR("mobs::decryptPrivateRsa"));
  if (1 != EVP_PKEY_private_check(ctx.get()))
    THROW("IS NO PRIVATE KEY");
  if (EVP_PKEY_decrypt_init(ctx.get()) <= 0)
    throw openssl_exception(LOGSTR("mobs::decryptPrivateRsa"));
  if (EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_PKCS1_OAEP_PADDING) <= 0)
    throw openssl_exception(LOGSTR("mobs::decryptPrivateRsa"));
  size_t sz = sessionKey.size();
  if (EVP_PKEY_decrypt(ctx.get(), &sessionKey[0], &sz, &cipher[0], cipher.size()) <= 0)
    throw openssl_exception(LOGSTR("mobs::decryptPrivateRsa"));
  sessionKey.resize(sz);
}

void
mobs::encryptPublicRsa(const std::vector<u_char> &sessionKey, std::vector<u_char> &cipher, const std::string &filePup) {
  auto rsaPubKey = mobs::CryptBufRsaData::readPublicKey2(filePup);
  if (not rsaPubKey)
    THROW(u8"can't load pub key");
  if (sessionKey.size() >= EVP_PKEY_size(rsaPubKey.get()) - 41)
    THROW(u8"array to big");
  cipher.resize(EVP_PKEY_size(rsaPubKey.get()), 0);
  std::unique_ptr<EVP_PKEY_CTX, SSL_Delete>ctx(EVP_PKEY_CTX_new_from_pkey(nullptr, rsaPubKey.get(), nullptr), SSL_Delete{});
  if (!ctx)
    throw openssl_exception(LOGSTR("mobs::encryptPublicRsa"));
  if (1 != EVP_PKEY_public_check(ctx.get()))
    THROW("mobs::encryptPublicRsa: THIS IS NO PUBLIC KEY");
  if (EVP_PKEY_encrypt_init(ctx.get()) <= 0)
    throw openssl_exception(LOGSTR("mobs::encryptPublicRsa"));
  if (EVP_PKEY_CTX_set_rsa_padding(ctx.get(), RSA_PKCS1_OAEP_PADDING) <= 0)
    throw openssl_exception(LOGSTR("mobs::encryptPublicRsa"));
  size_t sz = cipher.size();
  if (EVP_PKEY_encrypt(ctx.get(), &cipher[0], &sz, &sessionKey[0], sessionKey.size()) <= 0)
    throw openssl_exception(LOGSTR("mobs::encryptPublicRsa"));
}

void
mobs::encapsulatePublicRsa(std::vector<u_char> &cipher, std::vector<u_char> &key, const std::string &filePup, const std::string &filePriv,
                           const std::string &passphrase) {
  auto rsaPubKey = mobs::CryptBufRsaData::readPublicKey2(filePup);
  if (not rsaPubKey)
    THROW(u8"can't load pub key");
  auto rsaPrivKey = mobs::CryptBufRsaData::readPrivateKey2(filePriv, passphrase);
  if (not rsaPrivKey)
    THROW(u8"can't load priv key");
  //if (sessionKey.size() >= EVP_PKEY_size(rsaPubKey) - 41)
  //  THROW(u8"array to big");
  //cipher.resize(EVP_PKEY_size(rsaPubKey), 0);
  std::unique_ptr<EVP_PKEY_CTX, SSL_Delete>ctx(EVP_PKEY_CTX_new_from_pkey(nullptr, rsaPubKey.get(), nullptr), SSL_Delete{});
  if (!ctx)
    throw openssl_exception(LOGSTR("mobs::encryptPublicRsa"));
  if (1 != EVP_PKEY_public_check(ctx.get()))
    THROW("mobs::encryptPublicRsa: THIS IS NO PUBLIC KEY");
  const EVP_MD *md = EVP_MD_fetch(nullptr, "SHA512", NULL); // Example with SM3
  if (!md)
    throw openssl_exception(LOGSTR("mobs::encryptPublicRsa"));

  if (EVP_PKEY_CTX_set_signature_md(ctx.get(), md) <= 0)
    throw openssl_exception(LOGSTR("mobs::encryptPublicRsa"));

  if (EVP_PKEY_CTX_set_kem_op(ctx.get(), "DHKEM") <= 0) // RSASVE
    throw openssl_exception(LOGSTR("mobs::encryptPublicRsa"));
  if (EVP_PKEY_auth_encapsulate_init(ctx.get(), rsaPrivKey.get(), nullptr) <= 0)
  //if (EVP_PKEY_encapsulate_init(ctx, nullptr) <= 0)
    throw openssl_exception(LOGSTR("mobs::encryptPublicRsa"));

  size_t sz = cipher.size();
  size_t secretlen = 0, outlen = 0;
  unsigned char *out = NULL, *secret = NULL;

  if (EVP_PKEY_encapsulate(ctx.get(), nullptr, &outlen, nullptr, &secretlen) <= 0)
    throw openssl_exception(LOGSTR("mobs::encryptPublicRsa"));
  cipher.resize(outlen);
  key.resize(secretlen);
  if (EVP_PKEY_encapsulate(ctx.get(), &cipher[0], &outlen, &key[0], &secretlen) <= 0)
    throw openssl_exception(LOGSTR("mobs::encryptPublicRsa"));
  cipher.resize(outlen);
  key.resize(secretlen);

  std::cout << mobs::to_string_base64(cipher) << "   " << mobs::to_string_base64(key) << std::endl;

}

bool mobs::checkPasswordRsa(const std::string &filePriv, const std::string &passphrase) {
  try {
    auto rsaPrivKey = mobs::CryptBufRsaData::readPrivateKey2(filePriv, passphrase);
    if (not rsaPrivKey)
      return false;
    std::unique_ptr<EVP_PKEY_CTX, SSL_Delete>ctx(EVP_PKEY_CTX_new_from_pkey(nullptr, rsaPrivKey.get(), nullptr), SSL_Delete{});
    if (!ctx)
      return false;
    return 1 == EVP_PKEY_private_check(ctx.get());
  } catch (openssl_exception &e) {
    return false;
  }
}


void mobs::exportKey(const std::string &filePriv, const std::string &passphraseOld, std::string &priv, std::string &pub, const std::string &passphraseNew) {
  auto rsa = mobs::CryptBufRsaData::readPrivateKey2(filePriv, passphraseOld);
  if (not rsa)
    throw openssl_exception(LOGSTR("mobs::exportKey"));

  pub = exportPublicKey(rsa.get());
  priv = exportPrivateKey(rsa.get(), passphraseNew);
}

std::string mobs::getRsaInfo(const std::string &filePriv, const std::string &passphrase) {
  try {
    auto rsaPrivKey = mobs::CryptBufRsaData::readPrivateKey2(filePriv, passphrase);
    if (not rsaPrivKey)
      throw openssl_exception(LOGSTR("mobs::getRsaInfo"));
    std::unique_ptr<BIO, SSL_Delete> bp_private(BIO_new(BIO_s_mem()), SSL_Delete{});
    //if (1 != RSA_print(bp_private, rsaPrivKey, 3))
    if (1 != EVP_PKEY_print_params(bp_private.get(), rsaPrivKey.get(), 2, nullptr))
      throw openssl_exception(LOGSTR("mobs::getRsaInfo"));
    char *bptr;
    auto n = BIO_get_mem_data(bp_private.get(), &bptr);
    auto res = std::string(bptr, n);
    return res;
  } catch (openssl_exception &e) {
    return {};
  }
}

std::string mobs::getRsaFingerprint(const std::string &filePub) {
  auto rsaPubKey = mobs::CryptBufRsaData::readPublicKey2(filePub);
  if (not rsaPubKey)
    throw openssl_exception(LOGSTR("mobs::getRsaFingerprint"));
  BIGNUM *modulus{};
  if (not EVP_PKEY_get_bn_param(rsaPubKey.get(), "n", &modulus))
    throw openssl_exception(LOGSTR("mobs::getRsaFingerprint"));
  std::vector<unsigned char> buf;
  unsigned int sz = BN_num_bytes(modulus);
  //std::cerr << sz << " " << BN_bn2hex(modulus) << std::endl;
  buf.resize(sz);
  BN_bn2bin(modulus, &buf[0]);
  std::string res = mobs::hash_value(buf, "md5");
  BN_free(modulus);
  return res;
}
