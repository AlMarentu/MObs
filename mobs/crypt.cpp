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

#include "crypt.h"
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
#include <openssl/provider.h>
#include <openssl/crypto.h>
#include <utility>
#include <array>
#include <vector>
#include <cstring>
#include <iomanip>


#define KEYBUFLEN 32
#define INPUT_BUFFER_LEN 1024



namespace mobs_internal {
class SSL_Delete {
public:
  SSL_Delete() = default;
  void operator()(BIO *p) { BIO_free_all(p); }
  void operator()(OSSL_ENCODER_CTX *p) { OSSL_ENCODER_CTX_free(p); }
  void operator()(EVP_PKEY *p) { EVP_PKEY_free(p); }
  void operator()(EVP_PKEY_CTX *p) { EVP_PKEY_CTX_free(p); }
  void operator()(EVP_CIPHER_CTX *p) { EVP_CIPHER_CTX_free(p); }
};

std::string openSslGetError();
std::unique_ptr<EVP_PKEY, SSL_Delete> readPrivateKey(const std::string &file, const std::string &passphrase);
std::unique_ptr<EVP_PKEY, SSL_Delete> readPublicKey(const std::string &file);

class LegacyProvider {
  LegacyProvider() = default;
  static OSSL_PROVIDER *legacy_provider;
public:
  static void init() {
    if (legacy_provider)
      return;
    OPENSSL_init_crypto(OPENSSL_INIT_ADD_ALL_CIPHERS | OPENSSL_INIT_ADD_ALL_DIGESTS, nullptr);
    legacy_provider = OSSL_PROVIDER_load(NULL, "legacy");
    if (!legacy_provider)
      LOG(LM_ERROR, "cann't load OpenSSL legacy provider");
    LOG(LM_INFO, "OpenSSL legacy provider loaded");
  }
  static void close() {
    OSSL_PROVIDER_unload(legacy_provider);
    legacy_provider = nullptr;
    CONF_modules_free();
  }
};
OSSL_PROVIDER *LegacyProvider::legacy_provider{};
}

using mobs_internal::SSL_Delete;

namespace {

class openssl_exception : public std::runtime_error {
public:
  explicit openssl_exception(const std::string &log = "") : std::runtime_error(log + " " + mobs_internal::openSslGetError()) {
    LOG(LM_ERROR, "openssl: " << what());
  }
};


}

std::string mobs_internal::openSslGetError() {
  u_long e;
  std::string res = "OpenSSL: ";
  while ((e = ERR_get_error())) {
    static bool errLoad = false;
    if (not errLoad) {
      errLoad = true;
//      ERR_load_crypto_strings(); // nur crypto-fehler laden
      SSL_load_error_strings(); // NOLINT(hicpp-signed-bitwise)
      //ERR_load_BIO_strings();
      atexit([]() { ERR_free_strings(); });
    }
    res += ERR_error_string(e, nullptr);
  }
  return res;
}


std::unique_ptr<EVP_PKEY, SSL_Delete> mobs_internal::readPrivateKey(const std::string &file, const std::string &passphrase) {
  if (file.empty())
    return {nullptr, SSL_Delete{}};
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
    throw openssl_exception(LOGSTR("mobs::readPrivateKey"));
  std::unique_ptr<EVP_PKEY, SSL_Delete> key(rsaPrivKey, SSL_Delete{});
  //if (not EVP_PKEY_is_a(key.get(), "RSA") and not EVP_PKEY_is_a(key.get(), "EC"))
  //THROW("IS NO RSA KEY: "); // << EVP_PKEY_get0_type_name(rsaPrivKey));
  return key;
}

std::unique_ptr<EVP_PKEY, SSL_Delete> mobs_internal::readPublicKey(const std::string &file) {
  if (file.empty())
    return {nullptr, SSL_Delete{}};
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
    throw openssl_exception(LOGSTR("mobs::readPublicKey"));
  std::unique_ptr<EVP_PKEY, SSL_Delete> key(rsaPubKey, SSL_Delete{});

  //if (not EVP_PKEY_is_a(key.get(), "RSA") and not EVP_PKEY_is_a(key.get(), "EC"))
  //  THROW("IS NO RSA KEY"); // << EVP_PKEY_get0_type_name(rsaPubKey));
  return key;
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
    throw openssl_exception(LOGSTR("mobs::exportPrivateKeyToFile no suitable potential encoders found"));
  if (not passphrase.empty()) {
    OSSL_ENCODER_CTX_set_passphrase(ectx.get(), (const unsigned char *)passphrase.c_str(), passphrase.length());
    //OSSL_ENCODER_CTX_set_cipher(ectx.get(), "AES-256-CBC", nullptr);
    OSSL_ENCODER_CTX_set_cipher(ectx.get(), "DES-EDE3-CBC", nullptr);
  }
  if (not OSSL_ENCODER_to_bio(ectx.get(), bp_private.get()))
    throw openssl_exception(LOGSTR("mobs::exportPrivateKeyToFile write failed"));
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
    throw openssl_exception(LOGSTR("mobs::exportPublicKeyToFile no suitable potential encoders found"));
  if (not OSSL_ENCODER_to_bio(ectx.get(), bp_public.get()))
    throw openssl_exception(LOGSTR("mobs::exportPublicKeyToFile write failed"));
}

static std::string exportPrivateKey(EVP_PKEY *pkey, const std::string &passphrase)
{
  const char *format = "PEM";
  std::unique_ptr<OSSL_ENCODER_CTX, SSL_Delete> ectx(OSSL_ENCODER_CTX_new_for_pkey(pkey,
                                       OSSL_KEYMGMT_SELECT_PRIVATE_KEY
                                       | OSSL_KEYMGMT_SELECT_DOMAIN_PARAMETERS,
                                       format, nullptr,
                                       nullptr), SSL_Delete{});
  if (not ectx)
    throw openssl_exception(LOGSTR("mobs::exportPrivateKey no suitable potential encoders found"));
  if (not passphrase.empty()) {
    OSSL_ENCODER_CTX_set_passphrase(ectx.get(), (const unsigned char *)passphrase.c_str(), passphrase.length());
    //OSSL_ENCODER_CTX_set_cipher(ectx.get(), "AES-256-CBC", nullptr);
    OSSL_ENCODER_CTX_set_cipher(ectx.get(), "DES-EDE3-CBC", nullptr);
  }
  unsigned char *data = nullptr;
  size_t datalen;
  if (not OSSL_ENCODER_to_data(ectx.get(), &data, &datalen))
    throw openssl_exception(LOGSTR("mobs::exportPrivateKey write failed"));
  auto mem = std::string((char *)data, datalen);
  free(data);
  return mem;
}

static std::string exportPublicKey(EVP_PKEY *pkey)
{
  const char *format = "PEM";
  std::unique_ptr<OSSL_ENCODER_CTX, SSL_Delete> ectx(OSSL_ENCODER_CTX_new_for_pkey(pkey,
                                       OSSL_KEYMGMT_SELECT_PUBLIC_KEY
                                       | OSSL_KEYMGMT_SELECT_DOMAIN_PARAMETERS,
                                       format, nullptr,
                                       nullptr), SSL_Delete{});
  if (not ectx)
    throw openssl_exception(LOGSTR("mobs::exportPublicKey no suitable potential encoders found"));
  unsigned char *data = nullptr;
  size_t datalen;
  if (not OSSL_ENCODER_to_data(ectx.get(), &data, &datalen))
    throw openssl_exception(LOGSTR("mobs::exportPublicKey write failed"));
  auto mem = std::string((char *)data, datalen);
  free(data);
  return mem;
}


void mobs::generateCryptoKey(enum CryptKeyType type, const std::string &filePriv, const std::string &filePub, const std::string &passphrase, const std::string &format) {
  auto id = EVP_PKEY_RSA;
  auto nid = NID_X9_62_prime256v1;
  switch(type) {
    case CryptRSA2048: id = EVP_PKEY_RSA; break;
    case CryptX25519: id = EVP_PKEY_X25519; break;
    case CryptECprime256v1: id = EVP_PKEY_EC; nid = NID_X9_62_prime256v1; break;
    case CryptECsecp256k1: id = EVP_PKEY_EC; nid = NID_X9_62_prime256v1; break;
  }
  std::unique_ptr<EVP_PKEY_CTX, SSL_Delete> ctx(EVP_PKEY_CTX_new_id(id, nullptr), SSL_Delete{});
  if (!ctx)
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
  if (EVP_PKEY_keygen_init(ctx.get()) <= 0)
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
  if (id == EVP_PKEY_RSA) {
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx.get(), 2048) <= 0)
      throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
  }
  else if (id == EVP_PKEY_EC)
  {
    if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx.get(), nid) <= 0)
      throw openssl_exception(LOGSTR("mobs::CryptBufEcc"));
  }
  /* Generate key */
  EVP_PKEY *pkey = nullptr;
  if (EVP_PKEY_keygen(ctx.get(), &pkey) <= 0)
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
  std::unique_ptr<EVP_PKEY, SSL_Delete> key(pkey, SSL_Delete{});

  exportPublicKeyToFile(key.get(), filePub, "PEM");
  exportPrivateKeyToFile(key.get(), filePriv, passphrase, "PEM");
}

void mobs::generateCryptoKeyMem(enum CryptKeyType type, std::string &priv, std::string &pub, const std::string &passphrase) {
  auto id = EVP_PKEY_RSA;
  auto nid = NID_X9_62_prime256v1;
  switch(type) {
    case CryptRSA2048: id = EVP_PKEY_RSA; break;
    case CryptX25519: id = EVP_PKEY_X25519; break;
    case CryptECprime256v1: id = EVP_PKEY_EC; nid = NID_X9_62_prime256v1; break;
    case CryptECsecp256k1: id = EVP_PKEY_EC; nid = NID_X9_62_prime256v1; break;
  }
  std::unique_ptr<EVP_PKEY_CTX, SSL_Delete> ctx(EVP_PKEY_CTX_new_id(id, nullptr), SSL_Delete{});
  if (!ctx)
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
  if (EVP_PKEY_keygen_init(ctx.get()) <= 0)
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
  if (id == EVP_PKEY_RSA) {
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx.get(), 2048) <= 0)
      throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
  }
  else if (id == EVP_PKEY_EC)
  {
    if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx.get(), nid) <= 0)
      throw openssl_exception(LOGSTR("mobs::CryptBufEcc"));
  }
  /* Generate key */
  EVP_PKEY *pkey = nullptr;
  if (EVP_PKEY_keygen(ctx.get(), &pkey) <= 0)
    throw openssl_exception(LOGSTR("mobs::CryptBufRsa"));
  std::unique_ptr<EVP_PKEY, SSL_Delete> key(pkey, SSL_Delete{});

  pub = exportPublicKey(key.get());
  priv = exportPrivateKey(key.get(), passphrase);
}


void
mobs::decryptPublic(const std::vector<u_char> &cipher, std::vector<u_char> &sessionKey, const std::string &filePup) {
  auto rsaPubKey = mobs_internal::readPublicKey(filePup);
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
mobs::encryptPrivate(const std::vector<u_char> &sessionKey, std::vector<u_char> &cipher, const std::string &filePriv,
                        const std::string &passphrase) {
  auto rsaPrivKey = mobs_internal::readPrivateKey(filePriv, passphrase);
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
  //if (EVP_PKEY_CTX_set_signature_md(ctx.get(), EVP_sha256()) <= 0)
  //  throw openssl_exception(LOGSTR("mobs::encryptPrivateRsa"));
  size_t sz = cipher.size();
  if (EVP_PKEY_sign(ctx.get(), &cipher[0], &sz, &sessionKey[0], sessionKey.size()) <= 0)
    throw openssl_exception(LOGSTR("mobs::encryptPrivateRsa"));
}

void
mobs::decryptPrivate(const std::vector<u_char> &cipher, std::vector<u_char> &sessionKey, const std::string &filePriv,
                        const std::string &passphrase) {
  auto rsaPrivKey = mobs_internal::readPrivateKey(filePriv, passphrase);
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
mobs::encryptPublic(const std::vector<u_char> &sessionKey, std::vector<u_char> &cipher, const std::string &filePup) {
  auto rsaPubKey = mobs_internal::readPublicKey(filePup);
  if (not rsaPubKey)
    THROW(u8"can't load pub key");
  if (sessionKey.size() >= EVP_PKEY_size(rsaPubKey.get()) - 41)
    THROW(u8"array to big");
  LOG(LM_INFO, "MAX= " << EVP_PKEY_size(rsaPubKey.get()) - 41 << " res " << EVP_PKEY_size(rsaPubKey.get()));
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

#if 0


OSSL_PARAM params[] = {
        OSSL_PARAM_construct_int("bits", 2048),
        OSSL_PARAM_construct_end()
    };



#endif


// EVP_PKEY_ML_KEM (and specific sizes like ML-KEM-768)
// EVP_PKEY_X448

// cipher für Verschlüsselung, secret-key an empfänger
void
mobs::encapsulatePublic(std::vector<u_char> &cipher, std::vector<u_char> &key, const std::string &filePup, const std::string &filePriv,
                           const std::string &passphrase) {
  //mobs_internal::LegacyProvider::init();
  auto rsaPubKey = mobs_internal::readPublicKey(filePup);
  if (not rsaPubKey)
    THROW(u8"can't load pub key");
  auto rsaPrivKey = mobs_internal::readPrivateKey(filePriv, passphrase);
  //if (sessionKey.size() >= EVP_PKEY_size(rsaPubKey) - 41)
  //  THROW(u8"array to big");
  //cipher.resize(EVP_PKEY_size(rsaPubKey), 0);
  std::unique_ptr<EVP_PKEY_CTX, SSL_Delete>ctx(EVP_PKEY_CTX_new_from_pkey(nullptr, rsaPubKey.get(), nullptr), SSL_Delete{});
  if (!ctx)
    throw openssl_exception(LOGSTR("mobs::encapsulatePublic"));
  if (1 != EVP_PKEY_public_check(ctx.get()))
    THROW("mobs::encryptPublicRsa: THIS IS NO PUBLIC KEY");
  if (filePriv.empty()) {
     if (EVP_PKEY_encapsulate_init(ctx.get(), nullptr) <= 0)
      throw openssl_exception(LOGSTR("mobs::encapsulatePublic"));
  } else {
    if (not rsaPrivKey)
      THROW(u8"can't load priv key");
    // EVP_PKEY_ML_KEM "ML-KEM"
    if (EVP_PKEY_is_a(rsaPrivKey.get(), "RSA")) {
      if (EVP_PKEY_CTX_set_kem_op(ctx.get(), "RSASVE") <= 0) // DHKEM
        throw openssl_exception(LOGSTR("mobs::encapsulatePublic"));
    }
    if (EVP_PKEY_auth_encapsulate_init(ctx.get(), rsaPrivKey.get(), nullptr) <= 0)
      throw openssl_exception(LOGSTR("mobs::encapsulatePublic"));
  }

  size_t keylen = 0, outlen = 0;
  if (EVP_PKEY_encapsulate(ctx.get(), nullptr, &outlen, nullptr, &keylen) <= 0)
    throw openssl_exception(LOGSTR("mobs::encapsulatePublic"));
  cipher.resize(outlen);
  key.resize(keylen);
  if (EVP_PKEY_encapsulate(ctx.get(), &cipher[0], &outlen, &key[0], &keylen) <= 0)
    throw openssl_exception(LOGSTR("mobs::encapsulatePublic"));

  return;

  //const EVP_MD *md = EVP_MD_fetch(nullptr, "SHA512", NULL); // Example with SM3
  const EVP_MD* md = EVP_get_digestbyname("sha256");
  if (!md)
    throw openssl_exception(LOGSTR("mobs::encryptPublicRsa"));
  if (EVP_PKEY_CTX_set_signature_md(ctx.get(), md) <= 0)

    //if (EVP_PKEY_CTX_set_signature_md(ctx.get(), EVP_sha256()) <= 0)
    throw openssl_exception(LOGSTR("mobs::encryptPublicRsa"));


  //const EVP_MD *md = EVP_MD_fetch(nullptr, "SHA512", NULL); // Example with SM3
  //if (!md)
  //  throw openssl_exception(LOGSTR("mobs::encryptPublicRsa"));

  //if (EVP_PKEY_CTX_set_signature_md(ctx.get(), md) <= 0)
  //if (EVP_PKEY_CTX_set_signature_md(ctx.get(), EVP_sha256()) <= 0)
  //  throw openssl_exception(LOGSTR("mobs::encryptPublicRsa"));

   //if (EVP_PKEY_auth_encapsulate_init(ctx.get(), rsaPrivKey.get(), nullptr) <= 0)
  //if (EVP_PKEY_encapsulate_init(ctx, nullptr) <= 0)
    throw openssl_exception(LOGSTR("mobs::encryptPublicRsa"));


  std::cout << mobs::to_string_base64(cipher) << "   " << mobs::to_string_base64(key) << std::endl;

}

void
mobs::decapsulatePublic(const std::vector<u_char> &cipher, std::vector<u_char> &key, const std::string &filePriv,
                           const std::string &passphrase, const std::string &filePup) {
  auto rsaPrivKey = mobs_internal::readPrivateKey(filePriv, passphrase);
  if (not rsaPrivKey)
    THROW(u8"can't load priv key");
  auto rsaPubKey = mobs_internal::readPublicKey(filePup);
  std::unique_ptr<EVP_PKEY_CTX, SSL_Delete>ctx(EVP_PKEY_CTX_new_from_pkey(nullptr, rsaPrivKey.get(), nullptr), SSL_Delete{});
  if (!ctx)
    throw openssl_exception(LOGSTR("mobs::decryptPrivateRsa"));
  if (1 != EVP_PKEY_private_check(ctx.get()))
    THROW("IS NO PRIVATE KEY");
  if (filePup.empty()) {
    if (EVP_PKEY_decapsulate_init(ctx.get(), nullptr) <= 0)
      throw openssl_exception(LOGSTR("mobs::encryptPublicRsa"));
  } else {
    if (not rsaPubKey)
      THROW(u8"can't load pub key");
    if (EVP_PKEY_is_a(rsaPrivKey.get(), "RSA")) {
      if (EVP_PKEY_CTX_set_kem_op(ctx.get(), "RSASVE") <= 0) // DHKEM
        throw openssl_exception(LOGSTR("mobs::encryptPublicRsa"));
    }
    if (EVP_PKEY_auth_decapsulate_init(ctx.get(), rsaPubKey.get(), nullptr) <= 0)
      throw openssl_exception(LOGSTR("mobs::encryptPublicRsa"));
  }
  size_t secretlen = 0;
  if (EVP_PKEY_decapsulate(ctx.get(), nullptr, &secretlen, &cipher[0], cipher.size()) <= 0)
    throw openssl_exception(LOGSTR("mobs::encryptPublicRsa"));
  key.resize(secretlen);
  if (EVP_PKEY_decapsulate(ctx.get(), &key[0], &secretlen, &cipher[0], cipher.size()) <= 0)
    throw openssl_exception(LOGSTR("mobs::encryptPublicRsa"));
}

bool mobs::checkPassword(const std::string &filePriv, const std::string &passphrase) {
  try {
    auto rsaPrivKey = mobs_internal::readPrivateKey(filePriv, passphrase);
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
  auto rsa = mobs_internal::readPrivateKey(filePriv, passphraseOld);
  if (not rsa)
    throw openssl_exception(LOGSTR("mobs::exportKey"));

  pub = exportPublicKey(rsa.get());
  priv = exportPrivateKey(rsa.get(), passphraseNew);
}

std::string mobs::readPrivateKey(const std::string &filePriv, const std::string &passphrase) {
  auto rsa = mobs_internal::readPrivateKey(filePriv, passphrase);
  if (not rsa)
    throw openssl_exception(LOGSTR("mobs::readPrivateKey"));

  return exportPrivateKey(rsa.get(), "");
}

std::string mobs::readPublicKey(const std::string &filePub, std::string &pub) {
  auto rsa = mobs_internal::readPublicKey(filePub);
  if (not rsa)
    throw openssl_exception(LOGSTR("mobs::readPublicKey"));

  return exportPublicKey(rsa.get());
}

std::string mobs::getKeyInfo(const std::string &filePriv, const std::string &passphrase) {
  try {
    auto rsaPrivKey = mobs_internal::readPrivateKey(filePriv, passphrase);
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


