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


#include "aes.h"
#include "aes.h"
#include "rsa.h"
#include "rsa.h"
#include "digest.h"
#include "digest.h"
#include "objtypes.h"
#include "objgen.h"

#include <stdio.h>
#include <sstream>
#include <gtest/gtest.h>

#include "logging.h"
using namespace std;


namespace {



TEST(cryptTest, aes1) {
  EXPECT_EQ("Guten Tag", mobs::from_aes_string("U2FsdGVkX19ACrvmZL5NXmtnoX4yH4wJkOTSYk+ZCSM=", "12345"));
  EXPECT_EQ("", mobs::from_aes_string("U2FsdGVkX18kKGguEw9kaylIrxvjzwnl5ncwmab9WoQ=", "12345"));
  EXPECT_EQ("Otto", mobs::from_aes_string(mobs::to_aes_string("Otto", "12345"), "12345"));
  ASSERT_NO_THROW(mobs::to_aes_string("", "12345"));
  EXPECT_EQ("", mobs::from_aes_string(mobs::to_aes_string("", "12345"), "12345"));
  EXPECT_ANY_THROW(mobs::from_aes_string("U2FsdGVkX19ACrvmZL5NXmtnoX4yH4wJkOTSYk+ZCSM=", "11111"));
  EXPECT_ANY_THROW(mobs::from_aes_string("", "12345"));

//EXPECT_EQ(R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
//<aaa><![CDATA[]]></aaa>
//)", w.getString());
}

static std::string to_aes(const std::string &s) {
  std::vector<u_char> key;
  std::vector<u_char> iv;
  key.resize(mobs::CryptBufAes::key_size(), '1');
  iv.resize(mobs::CryptBufAes::iv_size(), '0');

  std::stringstream ss;
  mobs::CryptOstrBuf streambuf(ss, new mobs::CryptBufAes(key, iv));
  std::wostream xStrOut(&streambuf);
  xStrOut << mobs::CryptBufAes::base64(true);
  xStrOut << mobs::to_wstring(s);
  streambuf.finalize();

  return ss.str();
}

static std::string from_aes(const std::string &s) {
  std::vector<u_char> key;
  std::vector<u_char> iv;
  key.resize(mobs::CryptBufAes::key_size(), '1');
  iv.resize(mobs::CryptBufAes::iv_size(), '0');

  std::stringstream ss(s);
  mobs::CryptIstrBuf streambuf(ss, new mobs::CryptBufAes(key, iv));
  std::wistream xStrIn(&streambuf);
  streambuf.getCbb()->setBase64(true);
  std::string res;
  wchar_t c;
  while (not xStrIn.get(c).eof())
    res += u_char(c);
  if (streambuf.bad())
    throw std::runtime_error("encryption failed");
  return res;
}

TEST(cryptTest, aes2) {
  EXPECT_EQ(32, mobs::CryptBufAes::key_size());
  EXPECT_EQ(16, mobs::CryptBufAes::iv_size());
  std::string res1, res2;
  ASSERT_NO_THROW(res1 = to_aes("Hallo!!!"));
  ASSERT_NO_THROW(res2 = from_aes(res1));
  EXPECT_EQ("Hallo!!!", res2);

}

TEST(cryptTest, rsa1) {
  ASSERT_NO_THROW(mobs::generateRsaKey("priv.pem", "pup.pem", "12345"));

  std::vector<u_char> sessionKey = {'H', 'a', 'L', 'L', 'o', '\0'};
  std::vector<u_char> cipher;
//  sessionKey.resize()
  ASSERT_NO_THROW(mobs::encryptPrivateRsa(sessionKey, cipher, "priv.pem", "12345"));
  EXPECT_EQ(256, cipher.size());
  std::vector<u_char> sessionKey2;
  ASSERT_NO_THROW(mobs::decryptPublicRsa(cipher, sessionKey2, "pup.pem"));
  EXPECT_EQ(sessionKey, sessionKey2);

  unlink("priv.pem");
  unlink("pub.pem");

}

TEST(cryptTest, rsa2) {
  ASSERT_NO_THROW(mobs::generateRsaKey("priv.pem", "pup.pem", "12345"));

  std::vector<u_char> sessionKey = {'H', 'a', 'L', 'L', 'o', '\0'};
  std::vector<u_char> cipher;
//  sessionKey.resize()
  ASSERT_NO_THROW(mobs::encryptPublicRsa(sessionKey, cipher, "pup.pem"));
  EXPECT_EQ(256, cipher.size());
  std::vector<u_char> sessionKey2;
  ASSERT_NO_THROW(mobs::decryptPrivateRsa(cipher, sessionKey2,  "priv.pem", "12345"));
  EXPECT_EQ(sessionKey, sessionKey2);

  unlink("priv.pem");
  unlink("pub.pem");

}


TEST(cryptTest, rsa3) {
  string priv, pub;
  ASSERT_NO_THROW(mobs::generateRsaKeyMem(priv, pub, "12345"));

  std::vector<u_char> sessionKey = {'H', 'a', 'L', 'L', 'o', '\0'};
  std::vector<u_char> cipher;
//  sessionKey.resize()
  ASSERT_NO_THROW(mobs::encryptPrivateRsa(sessionKey, cipher, priv, "12345"));
  EXPECT_EQ(256, cipher.size());
  std::vector<u_char> sessionKey2;
  ASSERT_NO_THROW(mobs::decryptPublicRsa(cipher, sessionKey2, pub));
  EXPECT_EQ(sessionKey, sessionKey2);

}

TEST(cryptTest, rsa4) {
  string priv, pub;
  ASSERT_NO_THROW(mobs::generateRsaKeyMem(priv, pub, "12345"));

  std::vector<u_char> sessionKey = {'H', 'a', 'L', 'L', 'o', '\0'};
  std::vector<u_char> cipher;
//  sessionKey.resize()
  ASSERT_NO_THROW(mobs::encryptPublicRsa(sessionKey, cipher, pub));
  EXPECT_EQ(256, cipher.size());
  std::vector<u_char> sessionKey2;
  ASSERT_NO_THROW(mobs::decryptPrivateRsa(cipher, sessionKey2,  priv, "12345"));
  EXPECT_EQ(sessionKey, sessionKey2);

}


TEST(cryptTest, rsa5) {
  string priv, pub;
  ASSERT_NO_THROW(mobs::generateRsaKeyMem(priv, pub));

  std::vector<u_char> sessionKey = {'H', 'a', 'L', 'L', 'o', '\0'};
  std::vector<u_char> cipher;
//  sessionKey.resize()
  ASSERT_NO_THROW(mobs::encryptPublicRsa(sessionKey, cipher, pub));
  EXPECT_EQ(256, cipher.size());
  std::vector<u_char> sessionKey2;
  ASSERT_NO_THROW(mobs::decryptPrivateRsa(cipher, sessionKey2,  priv, "beliebig da unverschlüsselt"));
  EXPECT_EQ(sessionKey, sessionKey2);

}

TEST(cryptTest, rsaCheck) {
  string priv, pub;
  ASSERT_NO_THROW(mobs::generateRsaKeyMem(priv, pub, "12345"));
  EXPECT_TRUE(mobs::checkPasswordRsa(priv, "12345"));
//  std::cerr << mobs::printRsa(priv, "12345") << std::endl;
  EXPECT_FALSE(mobs::checkPasswordRsa(priv, "54321"));
  auto hash = mobs::getRsaFingerprint(pub);
  //std::cerr << hash << std::endl;
  EXPECT_EQ(32, hash.length());
}

TEST(cryptTest, rsaChange) {
  string priv, pub, priv2, pub2;
  ASSERT_NO_THROW(mobs::generateRsaKeyMem(priv, pub, "12345"));
  EXPECT_TRUE(mobs::checkPasswordRsa(priv, "12345"));
  ASSERT_NO_THROW(mobs::exportKey(priv, "12345", priv2, pub2, "55555"));
  EXPECT_TRUE(mobs::checkPasswordRsa(priv2, "55555"));
  EXPECT_TRUE(pub == pub2);
}



TEST(cryptTest, digest1) {

  std::stringstream ss;
  auto md = new mobs::CryptBufDigest("sha1");
  mobs::CryptOstrBuf streambuf(ss, md);
  std::wostream xStrOut(&streambuf);
  xStrOut << L"Fischers Fritz fischt frische Fische";
  streambuf.finalize();

  EXPECT_EQ(ss.str(), "Fischers Fritz fischt frische Fische");
  EXPECT_EQ(md->hashStr(), "fa24fbd0c280509e2171aa5958b06b313a57e70e");

}

TEST(cryptTest, digest1e) {
  std::stringstream ss;
  auto md = new mobs::CryptBufDigest("gibtsnich");
  mobs::CryptOstrBuf streambuf(ss, md);
  std::wostream xStrOut(&streambuf);
  EXPECT_TRUE(xStrOut.good());
  EXPECT_FALSE(xStrOut.eof());
  // Fehler im overflow
  for (int i = 1; i < 300; i++)
    xStrOut << L"Fischers Fritz fischt frische Fische";
  EXPECT_FALSE(xStrOut.good());
  streambuf.finalize();
}

TEST(cryptTest, digest1ee) {
  std::stringstream ss;
  auto md = new mobs::CryptBufDigest("gibtsnich");
  mobs::CryptOstrBuf streambuf(ss, md);
  std::wostream xStrOut(&streambuf);
  EXPECT_TRUE(xStrOut.good());
  EXPECT_FALSE(xStrOut.eof());
  xStrOut << L"Fischers Fritz fischt frische Fische";
  // Fehler im flush
  xStrOut.flush();
  EXPECT_FALSE(xStrOut.good());
}

TEST(cryptTest, digest2) {
  std::stringstream ss("Fischers Fritz fischt frische Fische");
  auto md = new mobs::CryptBufDigest("sha1");
  mobs::CryptIstrBuf streambuf(ss, md);
  std::wistream xStrIn(&streambuf);
  EXPECT_TRUE(xStrIn.good());
  std::string res;
  wchar_t c;
  while (xStrIn and not xStrIn.get(c).eof()) {
    res += u_char(c);
  }

  EXPECT_TRUE(xStrIn.eof());
  EXPECT_FALSE(xStrIn.bad());
  EXPECT_FALSE(streambuf.bad());
  EXPECT_EQ(res, "Fischers Fritz fischt frische Fische");
  EXPECT_EQ(md->hashStr(), "fa24fbd0c280509e2171aa5958b06b313a57e70e");
}

TEST(cryptTest, faileof) {
  std::wstringstream ss(L"Fischers Fritz fischt frische Fische");
  std::string res;
  wchar_t c;
  while (ss and not ss.get(c).eof()) {
    res += u_char(c);
  }
  EXPECT_TRUE(ss.eof());
  EXPECT_FALSE(ss.bad());
  EXPECT_TRUE(ss.fail());
  EXPECT_EQ(res, "Fischers Fritz fischt frische Fische");
}

TEST(cryptTest, digest2e) {
  std::stringstream ss("Fischers Fritz fischt frische Fische");
  auto md = new mobs::CryptBufDigest("gibtsnicht");
  mobs::CryptIstrBuf streambuf(ss, md);
  std::wistream xStrIn(&streambuf);
  EXPECT_TRUE(xStrIn.good());
  std::string res;
  wchar_t c;
  while (xStrIn and not xStrIn.get(c).eof()) {
    LOG(LM_INFO, "FAIL " << xStrIn.fail());
    res += u_char(c);
  }
  EXPECT_TRUE(streambuf.bad());
  EXPECT_TRUE(xStrIn.bad());
  EXPECT_TRUE(xStrIn.fail());
  //EXPECT_TRUE(xStrIn.eof());
  EXPECT_FALSE(xStrIn.good());
}


TEST(cryptTest, digest3) {
  mobs::digestStream ds("sha1");
  ASSERT_FALSE(ds.bad());
  ds << "Fischers Fritz fischt frische Fische";
  EXPECT_EQ(ds.hashStr(), "fa24fbd0c280509e2171aa5958b06b313a57e70e");
  EXPECT_TRUE(ds.eof());
  EXPECT_EQ(ds.hashStr(), "fa24fbd0c280509e2171aa5958b06b313a57e70e");
  EXPECT_TRUE(ds.eof());

  mobs::digestStream dsx("gibsnich");
  EXPECT_TRUE(dsx.bad());

}

TEST(cryptTest, digest4) {
  std::string s = "Fischers Fritz fischt frische Fische";
  EXPECT_EQ(mobs::hash_value(s), "fa24fbd0c280509e2171aa5958b06b313a57e70e");
  std::vector<u_char> buf(s.begin(), s.end());
  EXPECT_EQ(mobs::hash_value(buf), "fa24fbd0c280509e2171aa5958b06b313a57e70e");

  EXPECT_ANY_THROW(mobs::hash_value(buf, "gibsnich"));
  EXPECT_ANY_THROW(mobs::hash_value(s, "gibsnich"));
}

TEST(cryptTest, uuid) {
  mobs::digestStream ds("md5");
  std::vector<u_char> domain({0x6b, 0xa7, 0xb8, 0x10, 0x9d, 0xad, 0x11, 0xd1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8});
  ds << string((char *)&domain[0], domain.size());
  ds << "www.example.org";
  std::string uuid = ds.uuid();
  EXPECT_EQ(36, uuid.length());
  EXPECT_EQ('-', uuid[8]);
  EXPECT_EQ('-', uuid[13]);
  EXPECT_EQ('3', uuid[14]);
  EXPECT_EQ('-', uuid[18]);
  EXPECT_EQ('-', uuid[23]);

  mobs::digestStream ds2;
  ds2 << string((char *)&domain[0], domain.size());
  ds2 << "www.example.org";
  EXPECT_EQ("74738ff5-5367-5958-9aee-98fffdcd1876", ds2.uuid());

}

//Objektdefinitionen
class Fahrzeug : virtual public mobs::ObjectBase
{
public:
  ObjInit(Fahrzeug);
  MemVar(string, typ);
  MemVar(int, achsen, USENULL);
  MemVar(bool, antrieb);
};

class Gespann : virtual public mobs::ObjectBase
{
public:
  ObjInit(Gespann);
  MemVar(int, id, KEYELEMENT1);
  MemVar(string, typ);
  MemVar(string, fahrer, XMLENCRYPT);
  MemObj(Fahrzeug, zugmaschine);
  MemVector(Fahrzeug, haenger);
};


TEST(cryptTest, xml) {
  Gespann f1, f2;

  f1.id(1);
  f1.typ("Brauereigespann");
  f1.fahrer("Otto");
  f1.zugmaschine.typ("Sechsspänner");
  f1.zugmaschine.achsen(0);
  f1.zugmaschine.antrieb(true);
  f1.haenger[0].typ("Bräuwagen");
  f1.haenger[0].achsen(2);

  mobs::ConvObjToString cth = mobs::ConvObjToString().exportXml().setEncryptor([](){return new mobs::CryptBufAes( "12345", "john");});
  string x = f1.to_string(cth);
  LOG(LM_INFO, x);
  EXPECT_EQ(string::npos, x.find("Otto"));
  EXPECT_NE(string::npos, x.find("EncryptedData"));
  EXPECT_NE(string::npos, x.find("zugmaschine"));

  mobs::ConvObjFromStr cfs = mobs::ConvObjFromStr().useXml().setDecryptor([&](const string& alg, const string& key) {
    EXPECT_EQ("aes-256-cbc", alg);
    EXPECT_EQ("john", key);
    return new mobs::CryptBufAes( "12345");
  });
  ASSERT_NO_THROW(mobs::string2Obj(x, f2, cfs));
  EXPECT_EQ("Brauereigespann", f2.typ());
  EXPECT_EQ("Otto", f2.fahrer());


}

}

