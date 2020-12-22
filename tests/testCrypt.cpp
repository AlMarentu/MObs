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
#include "objtypes.h"

#include <stdio.h>
#include <sstream>
#include <gtest/gtest.h>

using namespace std;


namespace {



TEST(cryptTest, aes1) {
  EXPECT_EQ("Guten Tag", mobs::from_aes_string("U2FsdGVkX19ACrvmZL5NXmtnoX4yH4wJkOTSYk+ZCSM=", "12345"));
  EXPECT_EQ("", mobs::from_aes_string("U2FsdGVkX18kKGguEw9kaylIrxvjzwnl5ncwmab9WoQ=", "12345"));
  EXPECT_EQ("Otto", mobs::from_aes_string(mobs::to_aes_string("Otto", "12345"), "12345"));
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

}

