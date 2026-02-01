// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2023 Matthias Lautner
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
#include "aes.h"
#include "rsa.h"
#include "crypt.h"

#include "objtypes.h"
#include "nbuf.h"
#include "objgen.h"
#include "xmlout.h"
#include "xmlwriter.h"
#include "xmlread.h"
#include "xmlparser.h"
#include "mrpcsession.h"
#include "mrpc.h"
#include "mrpcec.h"
#include "tcpstream.h"

#include <stdio.h>
#include <sstream>
#include <gtest/gtest.h>
#include <codecvt>

using namespace std;


namespace {


class MrpcPerson : virtual public mobs::ObjectBase {
public:
  ObjInit(MrpcPerson);
  MemVar(std::string, name);
};
ObjRegister(MrpcPerson);

class MrpcPing : virtual public mobs::ObjectBase {
public:
  ObjInit(MrpcPing);
  MemVar(std::string, comment);
};
ObjRegister(MrpcPing);


class MrpcServer : public mobs::Mrpc {
public:
  MrpcServer(std::istream &iStr, std::ostream &oStr, const std::string &pub, const std::string &priv) :
      Mrpc(iStr, oStr, &mrpcSession, false), pubKey(pub), privKey(priv) {}

  std::string loginReceived(const std::vector<u_char> &cipher, std::string &info) override {
    std::string login;
    std::string software;
    std::string hostname;
    std::string keyId = receiveLogin(cipher, privKey, "", login, software, hostname);
    info = "welcome";
    LOG(LM_INFO, "SRV LOGIN RECEIVED " << keyId);
    if (keyId != "testkey")
    {
      info = "unauthorized";
      return {};
    }

    session->sessionId = 1;
    session->sessionReuseTime = 120;
    return pubKey; // identisch mit clientkey
  }

  bool reconnectReceived(u_int newId, std::string &error) override {
    // hier bei Bedarf Session umschalten

    if (session->sessionId != 1)
      return false;
    return true;
  }

  void getPupKeyReceived(std::string &key, std::string &info) override {
    key = pubKey;
    info = "server up and running";
  }


  mobs::MrpcSession mrpcSession{};
  std::string pubKey;
  std::string privKey;
};


class MrpcServer2 : public mobs::MrpcEc {
public:
  MrpcServer2(std::istream &iStr, std::ostream &oStr, const std::string &pub, const std::string &priv) :
      MrpcEc(iStr, oStr, &mrpcSession, false), pubKey(pub), privKey(priv) {}

  std::string getSenderPublicKey(const std::string &keyId) override {
    if (keyId != "testkey")
      return {};
    return pubKey;
  }

  void loginReceived(const std::vector<u_char> &cipher, const std::string &keyId) override {

    LOG(LM_INFO, "SRV LOGIN RECEIVED " << keyId);
    std::vector<u_char> key;
    session->sessionId = 2;
    session->sessionReuseTime = 120;
    session->keyValidTime = 5400;
    static string lastCipher;
    static vector<u_char> lastKey;
    if (lastKey.empty() or lastCipher != mobs::to_string_base64(cipher)) {
      setEcdhSessionKey(cipher, privKey, "");
      lastKey = session->sessionKey;
      lastCipher = mobs::to_string_base64(cipher);
    } else {
      session->sessionKey = lastKey;
      LOG(LM_INFO, "REUSE OLD SESSION");
    }
  }

  void authenticated(const std::string &login, const std::string &host, const std::string &software) override {
    LOG(LM_INFO, "AUTH " << login << '@' << host << ' ' << software);
  }

  std::string getServerPublicKey() override {
    return mobs::readPublicKey(pubKey);
  }


  mobs::MrpcSession mrpcSession{};
  std::string pubKey;
  std::string privKey;
};

void exampleClient() {

  string passphrase = "12345";
  string keystore = "keystore";
  string keyname = "client";
  string server = "localhost:5555";

  try
  {
    unique_ptr<mobs::ObjectBase> query;

    query = unique_ptr<mobs::ObjectBase>(new MrpcPing);
    dynamic_cast<MrpcPing *>(query.get())->comment("Hallo Welt");


    if (keystore.rfind('/') != keystore.length() - 1)
      keystore += '/';
    string serverPub = keystore + "server.pem";
    string privkey = keystore + keyname + "_priv.pem";
    mobs::MrpcSession clientSession(server);

    mobs::tcpstream con(clientSession.host(), clientSession.port());
    if (not con.is_open())
      THROW("can't connect " << errno << " " << strerror(errno));

    mobs::Mrpc client(con, con, &clientSession, true);
    for (;;) {
      if (client.waitForConnected("testkey", "googletest", privkey, passphrase, serverPub))
        break;
    }

    client.sendSingle(*query);

    bool readyRead = false;
    while (not readyRead) {
      readyRead = client.parseClient();
      if (auto res = dynamic_cast<MrpcPing *>(client.resultObj.get())) {
        LOG(LM_INFO, "Received PING " << res->comment());
        client.closeServer(); // Server-Session mitteilen, dass keine weiteren Anfragen kommen (auch kein Reconect)
        break; // keine Geduld zum Warten auf readyRead
      }
    }
  }
  catch (const std::exception &e)
  {
    LOG(LM_ERROR, "EXCEPTION " << e.what());
  }
}

void exampleServer() {
  stringstream strOut;
  stringstream strIn("");

  MrpcServer server(strIn, strOut, "", "");

  for (;;) {
    server.parseServer();
    if (auto res = dynamic_cast<MrpcPing *>(server.resultObj.get())) {
      server.sendSingle(*res);
    }
    server.resultObj = nullptr;
  }


}


TEST(mrpcTest, MrpcClientServer) {
  string priv, pub;
  mobs::generateRsaKeyMem(priv, pub);

  stringstream strOut;
  stringstream strIn;

  MrpcServer server(strIn, strOut, pub, priv);

  mobs::MrpcSession clientSession{};
  mobs::Mrpc client(strOut, strIn, &clientSession, false);

  bool clientConnected = false;
  LOG(LM_INFO, "CLI");
  ASSERT_NO_THROW(clientConnected = client.waitForConnected("testkey", "googletest", priv, "", pub));
  ASSERT_FALSE(clientConnected);
  LOG(LM_INFO, "XXX C->S " << strIn.str());

  LOG(LM_INFO, "SRV");
  ASSERT_NO_THROW(server.parseServer());
  LOG(LM_INFO, "XXX S->C " << strOut.str());

  LOG(LM_INFO, "CLI");
  ASSERT_NO_THROW(clientConnected = client.waitForConnected("testkey", "googletest", priv, "", pub));
  ASSERT_TRUE(clientConnected);

  LOG(LM_INFO, "CLI");
  MrpcPerson p1;
  client.sendSingle(p1);
  LOG(LM_INFO, "XXX C->S " << strIn.str());

  LOG(LM_INFO, "SRV");
  ASSERT_NO_THROW(server.parseServer());
  EXPECT_TRUE(bool(server.resultObj));
  EXPECT_NE(dynamic_cast<MrpcPerson *>(server.resultObj.get()), nullptr);
  MrpcPerson p2;
  p2.name("Heinrich");
  server.sendSingle(p2);
  client.resultObj = nullptr;
  //ASSERT_NO_THROW(server.parseServer());
  LOG(LM_INFO, "XXX S->C " << strOut.str());

  LOG(LM_INFO, "CLI");
  ASSERT_NO_THROW(client.parseClient());
  ASSERT_TRUE(bool(client.resultObj));
  to_string(*client.resultObj);
  ASSERT_NE(dynamic_cast<MrpcPerson *>(client.resultObj.get()), nullptr);
  EXPECT_EQ(dynamic_cast<MrpcPerson *>(client.resultObj.get())->name(), "Heinrich");
  auto res1 = client.getResult<MrpcPing>();
  EXPECT_FALSE(res1);
  auto res2 = client.getResult<MrpcPerson>();
  ASSERT_TRUE(res2);
  auto res3 = client.getResult<MrpcPerson>();
  EXPECT_FALSE(res3);
  EXPECT_EQ(res2->name(), "Heinrich");

}

TEST(mrpcTest, MrpcClientServerEcc) {
  string cpriv, cpub, spriv, spub;
  mobs::generateCryptoKeyMem(mobs::CryptECprime256v1, spriv, spub);
  mobs::generateCryptoKeyMem(mobs::CryptECprime256v1, cpriv, cpub);

  stringstream strStoC;
  stringstream strCtoS;

  MrpcServer2 server(strCtoS, strStoC, cpub, spriv);

  LOG(LM_INFO, "CLI");
  mobs::MrpcSession clientSession{};
  mobs::MrpcEc client(strStoC, strCtoS, &clientSession, false);

  ASSERT_NO_THROW(client.startSession("testkey", "googletest", cpriv, "", spub));

  MrpcPerson p1;
  client.sendSingle(p1);
  LOG(LM_INFO, "XXX C->S " << strCtoS.str());


  LOG(LM_INFO, "SRV");
  for  (int i= 0; i < 5; i++) {
    ASSERT_NO_THROW(server.parseServer());
    LOG(LM_INFO, "LLL S=" << server.level() << " E=" << server.isEncrypted() << " con=" << server.isConnected());
    LOG(LM_INFO, "XXX S->C " << strStoC.str());
    if (server.resultObj)
      break;
  }
  EXPECT_TRUE(server.isConnected());

  EXPECT_TRUE(bool(server.resultObj));
  EXPECT_NE(dynamic_cast<MrpcPerson *>(server.resultObj.get()), nullptr);
  MrpcPerson p2;
  p2.name("Heinrich");
  server.sendSingle(p2);
  LOG(LM_INFO, "XXX S->C " << strStoC.str());

  LOG(LM_INFO, "CLI");
  for  (int i= 0; i < 5; i++) {
    ASSERT_NO_THROW(client.parseClient());
    LOG(LM_INFO, "C-CON " << client.isConnected());
    if (client.resultObj)
      break;
  }
  EXPECT_TRUE(client.isConnected());

  ASSERT_TRUE(bool(client.resultObj));
  to_string(*client.resultObj);
  ASSERT_NE(dynamic_cast<MrpcPerson *>(client.resultObj.get()), nullptr);
  EXPECT_EQ(dynamic_cast<MrpcPerson *>(client.resultObj.get())->name(), "Heinrich");
  auto res1 = client.getResult<MrpcPing>();
  EXPECT_FALSE(res1);
  auto res2 = client.getResult<MrpcPerson>();
  ASSERT_TRUE(res2);
  auto res3 = client.getResult<MrpcPerson>();
  EXPECT_FALSE(res3);
  EXPECT_EQ(res2->name(), "Heinrich");


  LOG(LM_INFO, "------- 2. Runde -----");


  //clientSession.clear();
  ASSERT_NO_THROW(client.startSession("testkey", "googletest", cpriv, "", spub));
  p1.name("Walther");
  client.sendSingle(p1);
  LOG(LM_INFO, "XXX C->S " << strCtoS.str());

  LOG(LM_INFO, "SRV");
  ASSERT_NO_THROW(server.parseServer());
  LOG(LM_INFO, "XXX S->C " << strStoC.str());

  //ASSERT_NO_THROW(server.parseServer());

  EXPECT_TRUE(bool(server.resultObj));
  EXPECT_NE(dynamic_cast<MrpcPerson *>(server.resultObj.get()), nullptr);


}

TEST(mrpcTest, MrpcClientServerGetPub) {
  string cpriv, cpub, spriv, spub;
  mobs::generateCryptoKeyMem(mobs::CryptECprime256v1, spriv, spub);
  mobs::generateCryptoKeyMem(mobs::CryptECprime256v1, cpriv, cpub);

  stringstream strStoC;
  stringstream strCtoS;

  MrpcServer2 server(strCtoS, strStoC, cpub, spriv);

  LOG(LM_INFO, "CLI");
  mobs::MrpcSession clientSession{};
  mobs::MrpcEc client(strStoC, strCtoS, &clientSession, false);

  client.getPublicKey();
  LOG(LM_INFO, "XXX C->S " << strCtoS.str());

  LOG(LM_INFO, "SRV");
  ASSERT_NO_THROW(server.parseServer());
  LOG(LM_INFO, "XXX S->C " << strStoC.str());

  LOG(LM_INFO, "CLI");
  ASSERT_NO_THROW(client.parseClient());


  ASSERT_NO_THROW(client.startSession("testkey", "googletest", cpriv, "", spub));

  MrpcPerson p1;
  client.sendSingle(p1);
  LOG(LM_INFO, "XXX C->S " << strCtoS.str());


  LOG(LM_INFO, "SRV");
  for  (int i= 0; i < 5; i++) {
    ASSERT_NO_THROW(server.parseServer());
    LOG(LM_INFO, "LLL S=" << server.level() << " E=" << server.isEncrypted() << " con=" << server.isConnected());
    LOG(LM_INFO, "XXX S->C " << strStoC.str());
    if (server.resultObj)
      break;
  }
  EXPECT_TRUE(server.isConnected());

  EXPECT_TRUE(bool(server.resultObj));
  EXPECT_NE(dynamic_cast<MrpcPerson *>(server.resultObj.get()), nullptr);
  MrpcPerson p2;
  p2.name("Heinrich");
  server.sendSingle(p2);
  LOG(LM_INFO, "XXX S->C " << strStoC.str());

  LOG(LM_INFO, "CLI");
  for  (int i= 0; i < 5; i++) {
    ASSERT_NO_THROW(client.parseClient());
    LOG(LM_INFO, "C-CON " << client.isConnected());
    if (client.resultObj)
      break;
  }
  EXPECT_TRUE(client.isConnected());

  ASSERT_TRUE(bool(client.resultObj));
  to_string(*client.resultObj);
  ASSERT_NE(dynamic_cast<MrpcPerson *>(client.resultObj.get()), nullptr);
  EXPECT_EQ(dynamic_cast<MrpcPerson *>(client.resultObj.get())->name(), "Heinrich");
  auto res1 = client.getResult<MrpcPing>();
  EXPECT_FALSE(res1);
  auto res2 = client.getResult<MrpcPerson>();
  ASSERT_TRUE(res2);
  auto res3 = client.getResult<MrpcPerson>();
  EXPECT_FALSE(res3);
  EXPECT_EQ(res2->name(), "Heinrich");


}

TEST(mrpcTest, MrpcClientServerEccWait) {
  string cpriv, cpub, spriv, spub;
  mobs::generateCryptoKeyMem(mobs::CryptECprime256v1, spriv, spub);
  mobs::generateCryptoKeyMem(mobs::CryptECprime256v1, cpriv, cpub);

  stringstream strStoC;
  stringstream strCtoS;

  MrpcServer2 server(strCtoS, strStoC, cpub, spriv);

  LOG(LM_INFO, "CLI");
  mobs::MrpcSession clientSession{};
  mobs::MrpcEc client(strStoC, strCtoS, &clientSession, false);
  ASSERT_NO_THROW(client.startSession("testkey", "googletest", cpriv, "", spub));

  // wird nur benötigt, wenn vor Senden des ersten Kommandos die Verbindung gecheckt werden soll
  client.stopEncrypt();
  client.flush();

  LOG(LM_INFO, "SRV");
  for  (int i= 0; i < 5; i++) {
    ASSERT_NO_THROW(server.parseServer());
    LOG(LM_INFO, "LLL S=" << server.level() << " E=" << server.isEncrypted() << " con=" << server.isConnected());
    LOG(LM_INFO, "XXX S->C " << strStoC.str());
    if (server.isConnected())
      break;
  }
  EXPECT_TRUE(server.isConnected());

  MrpcPerson p1;
  client.sendSingle(p1);
  LOG(LM_INFO, "XXX C->S " << strCtoS.str());


  LOG(LM_INFO, "SRV");
  for  (int i= 0; i < 5; i++) {
    ASSERT_NO_THROW(server.parseServer());
    LOG(LM_INFO, "LLL S=" << server.level() << " E=" << server.isEncrypted() << " con=" << server.isConnected());
    LOG(LM_INFO, "XXX S->C " << strStoC.str());
    if (server.resultObj)
      break;
  }
  EXPECT_TRUE(server.isConnected());

  EXPECT_TRUE(bool(server.resultObj));
  EXPECT_NE(dynamic_cast<MrpcPerson *>(server.resultObj.get()), nullptr);
  MrpcPerson p2;
  p2.name("Heinrich");
  server.sendSingle(p2);
  LOG(LM_INFO, "XXX S->C " << strStoC.str());

  LOG(LM_INFO, "CLI");
  for  (int i= 0; i < 5; i++) {
    ASSERT_NO_THROW(client.parseClient());
    LOG(LM_INFO, "C-CON " << client.isConnected());
    if (client.resultObj)
      break;
  }
  EXPECT_TRUE(client.isConnected());

  ASSERT_TRUE(bool(client.resultObj));
  to_string(*client.resultObj);
  ASSERT_NE(dynamic_cast<MrpcPerson *>(client.resultObj.get()), nullptr);
  EXPECT_EQ(dynamic_cast<MrpcPerson *>(client.resultObj.get())->name(), "Heinrich");
  auto res1 = client.getResult<MrpcPing>();
  EXPECT_FALSE(res1);
  auto res2 = client.getResult<MrpcPerson>();
  ASSERT_TRUE(res2);
  auto res3 = client.getResult<MrpcPerson>();
  EXPECT_FALSE(res3);
  EXPECT_EQ(res2->name(), "Heinrich");


}

TEST(mrpcTest, MrpcClientServerEccWOauth) {
  string cpriv, cpub, spriv, spub;
  mobs::generateCryptoKeyMem(mobs::CryptECprime256v1, spriv, spub);
  mobs::generateCryptoKeyMem(mobs::CryptECprime256v1, cpriv, cpub);

  stringstream strStoC;
  stringstream strCtoS;

  MrpcServer2 server(strCtoS, strStoC, cpub, spriv);

  LOG(LM_INFO, "CLI");
  mobs::MrpcSession clientSession{};
  mobs::MrpcEc client(strStoC, strCtoS, &clientSession, false);
  //ASSERT_NO_THROW(client.startSession("testkey", "googletest", cpriv, "", spub));
  client.writer.writeHead();
  client.writer.writeTagBegin(L"methodCall");
  // neuen SessionKey und Cipher generieren
  std::vector<u_char> cipher;
  mobs::encapsulatePublic(cipher, clientSession.sessionKey, spub);
  clientSession.info = mobs::to_string_base64(cipher);
  clientSession.generated = time(nullptr);
  clientSession.keyName = "testkey";
  clientSession.sessionReuseTime = 0;
  clientSession.keyValidTime = 0;
  std::vector<u_char> iv;
  iv.resize(mobs::CryptBufAes::iv_size());
  mobs::CryptBufAes::getRand(iv);
  client.writer.startEncrypt((new mobs::CryptBufAes(clientSession.sessionKey, iv, "", true))->setRecipientKeyBase64(clientSession.info));
  LOG(LM_INFO, "New Session startet, cipher " << clientSession.info);

  MrpcPerson p1;
  client.sendSingle(p1);
  client.sendSingle(p1);
  client.sendSingle(p1);
  LOG(LM_INFO, "XXX C->S " << strCtoS.str());



  LOG(LM_INFO, "SRV");
  ASSERT_ANY_THROW(server.parseServer());

  EXPECT_FALSE(server.isConnected());

  LOG(LM_INFO, "XXX S->C " << strStoC.str());

  LOG(LM_INFO, "CLI");
  ASSERT_ANY_THROW(client.parseClient());
  EXPECT_FALSE(client.isConnected());
  EXPECT_EQ(clientSession.info, "login failed");

}

TEST(mrpcTest, MrpcClientServerEccRecon) {
  string cpriv, cpub, spriv, spub;
  mobs::generateCryptoKeyMem(mobs::CryptECprime256v1, spriv, spub);
  mobs::generateCryptoKeyMem(mobs::CryptECprime256v1, cpriv, cpub);
  mobs::MrpcSession clientSession{};

  {
    stringstream strStoC;
    stringstream strCtoS;

    MrpcServer2 server(strCtoS, strStoC, cpub, spriv);

    LOG(LM_INFO, "CLI");
    mobs::MrpcEc client(strStoC, strCtoS, &clientSession, false);
    ASSERT_NO_THROW(client.startSession("testkey", "googletest", cpriv, "", spub));
    MrpcPerson p1;
    client.sendSingle(p1);
    LOG(LM_INFO, "XXX C->S " << strCtoS.str());


    LOG(LM_INFO, "SRV");
    ASSERT_NO_THROW(server.parseServer());
    LOG(LM_INFO, "XXX S->C " << strStoC.str());

    ASSERT_NO_THROW(server.parseServer());

    EXPECT_TRUE(bool(server.resultObj));
    EXPECT_NE(dynamic_cast<MrpcPerson *>(server.resultObj.get()), nullptr);
    server.resultObj = nullptr; // Server-Objekt entfernen;
    MrpcPerson p2;
    p2.name("Heinrich");
    server.sendSingle(p2);
    LOG(LM_INFO, "XXX S->C " << strStoC.str());

    LOG(LM_INFO, "CLI");
    ASSERT_NO_THROW(client.parseClient());
    ASSERT_NO_THROW(client.parseClient());
    ASSERT_TRUE(bool(client.resultObj));
    to_string(*client.resultObj);
    ASSERT_NE(dynamic_cast<MrpcPerson *>(client.resultObj.get()), nullptr);
    EXPECT_EQ(dynamic_cast<MrpcPerson *>(client.resultObj.get())->name(), "Heinrich");
    auto res1 = client.getResult<MrpcPing>();
    EXPECT_FALSE(res1);
    auto res2 = client.getResult<MrpcPerson>();
    ASSERT_TRUE(res2);
    auto res3 = client.getResult<MrpcPerson>();
    EXPECT_FALSE(res3);
    EXPECT_EQ(res2->name(), "Heinrich");
    EXPECT_FALSE(bool(client.resultObj));

    // jetzt eine 2. Datensatz senden

    MrpcPerson p3;
    p3.name("Goethe");
    client.sendSingle(p3);
    LOG(LM_INFO, "XXX C->S " << strCtoS.str());

    LOG(LM_INFO, "SRV");
    EXPECT_FALSE(server.resultObj);
    ASSERT_NO_THROW(server.parseServer());
    ASSERT_NO_THROW(server.parseServer());
    ASSERT_NO_THROW(server.parseServer());
    EXPECT_TRUE(bool(server.resultObj));
    auto res4 = server.getResult<MrpcPerson>();
    ASSERT_TRUE(res4);
    EXPECT_EQ(res4->name(), "Goethe");

    // Antwort bearbeiten und zurück
    MrpcPerson p4;
    p4.name("Johann Wolfgang von");
    server.sendSingle(p4);
    LOG(LM_INFO, "XXX S->C " << strStoC.str());

    LOG(LM_INFO, "CLI");
    ASSERT_NO_THROW(client.parseClient());
    ASSERT_NO_THROW(client.parseClient());
    EXPECT_TRUE(bool(client.resultObj));
    auto res5 = client.getResult<MrpcPerson>();
    ASSERT_TRUE(res5);
    EXPECT_EQ(res5->name(), "Johann Wolfgang von");
  }

  // jetzt neue Connection mit alter Session
  // damit ist die Cipher sowie der Session-Key identisch und kann am Server gecached werden.
  LOG(LM_INFO, "------- reconnect -----");

  {
    stringstream strStoC;
    stringstream strCtoS;

    MrpcServer2 server(strCtoS, strStoC, cpub, spriv);

    LOG(LM_INFO, "CLI");
    mobs::MrpcEc client(strStoC, strCtoS, &clientSession, false);
    ASSERT_NO_THROW(client.startSession("testkey", "googletest", cpriv, "", spub));
    MrpcPerson p1;
    client.sendSingle(p1);
    LOG(LM_INFO, "XXX C->S " << strCtoS.str());


    LOG(LM_INFO, "SRV");

    for (int i = 0; i < 5; i++) {
      ASSERT_NO_THROW(server.parseServer());
      LOG(LM_INFO, "XXX S->C " << strStoC.str());
      if (server.resultObj)
        break;
    }

    EXPECT_TRUE(bool(server.resultObj));
    EXPECT_NE(dynamic_cast<MrpcPerson *>(server.resultObj.get()), nullptr);
    MrpcPerson p2;
    p2.name("Heinrich");
    server.sendSingle(p2);
    LOG(LM_INFO, "XXX S->C " << strStoC.str());

    LOG(LM_INFO, "CLI");
    ASSERT_NO_THROW(client.parseClient());
    ASSERT_NO_THROW(client.parseClient());
    ASSERT_TRUE(bool(client.resultObj));
    to_string(*client.resultObj);
    ASSERT_NE(dynamic_cast<MrpcPerson *>(client.resultObj.get()), nullptr);
    EXPECT_EQ(dynamic_cast<MrpcPerson *>(client.resultObj.get())->name(), "Heinrich");
    auto res1 = client.getResult<MrpcPing>();
    EXPECT_FALSE(res1);
    auto res2 = client.getResult<MrpcPerson>();
    ASSERT_TRUE(res2);
    auto res3 = client.getResult<MrpcPerson>();
    EXPECT_FALSE(res3);
    EXPECT_EQ(res2->name(), "Heinrich");

  }

}

TEST(mrpcTest, MrpcClientServerEccRefresh) {
  string cpriv, cpub, spriv, spub;
  mobs::generateCryptoKeyMem(mobs::CryptECprime256v1, spriv, spub);
  mobs::generateCryptoKeyMem(mobs::CryptECprime256v1, cpriv, cpub);
  mobs::MrpcSession clientSession{};


  stringstream strStoC;
  stringstream strCtoS;

  MrpcServer2 server(strCtoS, strStoC, cpub, spriv);

  LOG(LM_INFO, "CLI");
  mobs::MrpcEc client(strStoC, strCtoS, &clientSession, false);
  ASSERT_NO_THROW(client.startSession("testkey", "googletest", cpriv, "", spub));
  MrpcPerson p1;
  client.sendSingle(p1);
  LOG(LM_INFO, "XXX C->S " << strCtoS.str());


  LOG(LM_INFO, "SRV");
  ASSERT_NO_THROW(server.parseServer());
  LOG(LM_INFO, "XXX S->C " << strStoC.str());

  ASSERT_NO_THROW(server.parseServer());

  EXPECT_TRUE(bool(server.resultObj));
  EXPECT_NE(dynamic_cast<MrpcPerson *>(server.resultObj.get()), nullptr);
  server.resultObj = nullptr; // Server-Objekt entfernen;
  MrpcPerson p2;
  p2.name("Heinrich");
  server.sendSingle(p2);
  LOG(LM_INFO, "XXX S->C " << strStoC.str());

  LOG(LM_INFO, "CLI");
  ASSERT_NO_THROW(client.parseClient());
  ASSERT_NO_THROW(client.parseClient());
  ASSERT_TRUE(bool(client.resultObj));
  to_string(*client.resultObj);
  ASSERT_NE(dynamic_cast<MrpcPerson *>(client.resultObj.get()), nullptr);
  EXPECT_EQ(dynamic_cast<MrpcPerson *>(client.resultObj.get())->name(), "Heinrich");
  auto res1 = client.getResult<MrpcPing>();
  EXPECT_FALSE(res1);
  auto res2 = client.getResult<MrpcPerson>();
  ASSERT_TRUE(res2);
  auto res3 = client.getResult<MrpcPerson>();
  EXPECT_FALSE(res3);
  EXPECT_EQ(res2->name(), "Heinrich");
  EXPECT_FALSE(bool(client.resultObj));

  // jetzt eine 2. Datensatz senden

  MrpcPerson p3;
  p3.name("Goethe");
  client.sendSingle(p3);
  LOG(LM_INFO, "XXX C->S " << strCtoS.str());

  LOG(LM_INFO, "SRV");
  EXPECT_FALSE(server.resultObj);
  ASSERT_NO_THROW(server.parseServer());
  ASSERT_NO_THROW(server.parseServer());
  ASSERT_NO_THROW(server.parseServer());
  EXPECT_TRUE(bool(server.resultObj));
  auto res4 = server.getResult<MrpcPerson>();
  ASSERT_TRUE(res4);
  EXPECT_EQ(res4->name(), "Goethe");

  // Antwort bearbeiten und zurück
  MrpcPerson p4;
  p4.name("Johann Wolfgang von");
  server.sendSingle(p4);
  LOG(LM_INFO, "XXX S->C " << strStoC.str());

  LOG(LM_INFO, "CLI");
  ASSERT_NO_THROW(client.parseClient());
  ASSERT_NO_THROW(client.parseClient());
  EXPECT_TRUE(bool(client.resultObj));
  auto res5 = client.getResult<MrpcPerson>();
  ASSERT_TRUE(res5);
  EXPECT_EQ(res5->name(), "Johann Wolfgang von");

  LOG(LM_INFO, "------- refresh -----");

  ASSERT_NO_THROW(client.clientRefreshKey(spub));
  {
    MrpcPerson p1;
    client.sendSingle(p1);
    LOG(LM_INFO, "XXX C->S " << strCtoS.str());


    LOG(LM_INFO, "SRV");

    for (int i = 0; i < 5; i++) {
      ASSERT_NO_THROW(server.parseServer());
      LOG(LM_INFO, "XXX S->C " << strStoC.str());
      if (server.resultObj)
        break;
    }

    EXPECT_TRUE(bool(server.resultObj));
    EXPECT_NE(dynamic_cast<MrpcPerson *>(server.resultObj.get()), nullptr);
    MrpcPerson p2;
    p2.name("Heinrich");
    server.sendSingle(p2);
    LOG(LM_INFO, "XXX S->C " << strStoC.str());

    LOG(LM_INFO, "CLI");
    ASSERT_NO_THROW(client.parseClient());
    ASSERT_NO_THROW(client.parseClient());
    ASSERT_TRUE(bool(client.resultObj));
    to_string(*client.resultObj);
    ASSERT_NE(dynamic_cast<MrpcPerson *>(client.resultObj.get()), nullptr);
    EXPECT_EQ(dynamic_cast<MrpcPerson *>(client.resultObj.get())->name(), "Heinrich");
    auto res1 = client.getResult<MrpcPing>();
    EXPECT_FALSE(res1);
    auto res2 = client.getResult<MrpcPerson>();
    ASSERT_TRUE(res2);
    auto res3 = client.getResult<MrpcPerson>();
    EXPECT_FALSE(res3);
    EXPECT_EQ(res2->name(), "Heinrich");
  }
}



TEST(mrpcTest, MrpcClientClosingServer) {
  string priv, pub;
  mobs::generateRsaKeyMem(priv, pub);

  stringstream strOut;
  stringstream strIn;

  MrpcServer server(strIn, strOut, pub, priv);

  mobs::MrpcSession clientSession{};
  mobs::Mrpc client(strOut, strIn, &clientSession, false);

  bool clientConnected = false;
  LOG(LM_INFO, "CLI");
  ASSERT_NO_THROW(clientConnected = client.waitForConnected("testkey", "googletest", priv, "", pub));
  ASSERT_FALSE(clientConnected);
  LOG(LM_INFO, "XXX C->S " << strIn.str());

  LOG(LM_INFO, "SRV");
  EXPECT_TRUE(server.parseServer());
  LOG(LM_INFO, "XXX S->C " << strOut.str());

  LOG(LM_INFO, "CLI");
  ASSERT_NO_THROW(clientConnected = client.waitForConnected("testkey", "googletest", priv, "", pub));
  ASSERT_TRUE(clientConnected);

  LOG(LM_INFO, "CLI");
  MrpcPerson p1;
  client.sendSingle(p1);
  LOG(LM_INFO, "XXX C->S " << strIn.str());

  LOG(LM_INFO, "SRV");
  EXPECT_TRUE(server.parseServer());
  EXPECT_TRUE(bool(server.resultObj));
  EXPECT_NE(dynamic_cast<MrpcPerson *>(server.resultObj.get()), nullptr);
  MrpcPerson p2;
  p2.name("Heinrich");
  server.sendSingle(p2);
  client.resultObj = nullptr;
  //ASSERT_NO_THROW(server.parseServer());
  LOG(LM_INFO, "XXX S->C " << strOut.str());

  LOG(LM_INFO, "CLI");
  ASSERT_NO_THROW(client.parseClient());
  ASSERT_TRUE(bool(client.resultObj));
  to_string(*client.resultObj);
  ASSERT_NE(dynamic_cast<MrpcPerson *>(client.resultObj.get()), nullptr);
  EXPECT_EQ(dynamic_cast<MrpcPerson *>(client.resultObj.get())->name(), "Heinrich");

  client.closeServer();
  LOG(LM_INFO, "ZZZ C->S " << strIn.str());

  LOG(LM_INFO, "SRV");
  EXPECT_EQ(1, server.level());
  EXPECT_TRUE(server.parseServer());
  EXPECT_TRUE(server.parseServer());
  EXPECT_TRUE(server.parseServer());
  EXPECT_EQ(0, server.level());
  LOG(LM_INFO, "ZZZ S->C " << strOut.str());
  EXPECT_FALSE(server.parseServer());
  EXPECT_FALSE(server.parseServer());
  EXPECT_FALSE(server.parseServer());

  LOG(LM_INFO, "CLI");
  EXPECT_EQ(1, client.level());
  ASSERT_NO_THROW(client.parseClient());
  ASSERT_NO_THROW(client.parseClient());
  EXPECT_EQ(0, client.level());
  ASSERT_ANY_THROW(client.parseClient());

}


TEST(mrpcTest, fetchServerkey) {
  string priv, pub;
  mobs::generateRsaKeyMem(priv, pub);
  string serverPub;

  stringstream strOut;
  stringstream strIn;

  MrpcServer server(strIn, strOut, pub, priv);

  mobs::MrpcSession clientSession{};
  mobs::Mrpc client(strOut, strIn, &clientSession, false);

  bool clientConnected = false;
  bool serverConnected = false;
  LOG(LM_INFO, "CLI");
  ASSERT_NO_THROW(clientConnected = client.waitForConnected("testkey", "googletest", priv, "", serverPub));
  ASSERT_FALSE(clientConnected);
  LOG(LM_INFO, "XXX C->S " << strIn.str());

  LOG(LM_INFO, "SRV");
  ASSERT_NO_THROW(serverConnected = server.parseServer());
  EXPECT_FALSE(serverConnected);
  LOG(LM_INFO, "XXX S->C " << strOut.str());

  LOG(LM_INFO, "CLI");
  ASSERT_NO_THROW(clientConnected = client.waitForConnected("testkey", "googletest", priv, "", serverPub));
  EXPECT_EQ(pub, serverPub);
  EXPECT_FALSE(clientConnected);
  LOG(LM_INFO, "Server Info: " << client.session->info);

  LOG(LM_INFO, "SRV");
  ASSERT_NO_THROW(serverConnected = server.parseServer());
  EXPECT_TRUE(serverConnected);
  LOG(LM_INFO, "XXX S->C " << strOut.str());

  LOG(LM_INFO, "CLI");
  ASSERT_NO_THROW(clientConnected = client.waitForConnected("testkey", "googletest", priv, "", serverPub));
  EXPECT_EQ(pub, serverPub);
  ASSERT_TRUE(clientConnected);

  LOG(LM_INFO, "CLI");
  MrpcPerson p1;
  client.sendSingle(p1);
  LOG(LM_INFO, "XXX C->S " << strIn.str());

  LOG(LM_INFO, "SRV");
  ASSERT_NO_THROW(serverConnected = server.parseServer());
  EXPECT_TRUE(serverConnected);
  EXPECT_TRUE(bool(server.resultObj));
  EXPECT_NE(dynamic_cast<MrpcPerson *>(server.resultObj.get()), nullptr);
  MrpcPerson p2;
  p2.name("Heinrich");
  server.sendSingle(p2);
  client.resultObj = nullptr;
  //ASSERT_NO_THROW(server.parseServer());
  LOG(LM_INFO, "XXX S->C " << strOut.str());

  LOG(LM_INFO, "CLI");
  ASSERT_NO_THROW(client.parseClient());
  ASSERT_TRUE(bool(client.resultObj));
  to_string(*client.resultObj);
  ASSERT_NE(dynamic_cast<MrpcPerson *>(client.resultObj.get()), nullptr);
  EXPECT_EQ(dynamic_cast<MrpcPerson *>(client.resultObj.get())->name(), "Heinrich");


}

TEST(mrpcTest, invalidLogin) {
  string priv, pub;
  mobs::generateRsaKeyMem(priv, pub);

  stringstream strOut;
  stringstream strIn;

  MrpcServer server(strIn, strOut, pub, priv);

  mobs::MrpcSession clientSession{};
  mobs::Mrpc client(strOut, strIn, &clientSession, false);

  bool clientConnected = false;
  LOG(LM_INFO, "CLI");
  ASSERT_NO_THROW(clientConnected = client.waitForConnected("invalid", "googletest", priv, "", pub));
  ASSERT_FALSE(clientConnected);
  LOG(LM_INFO, "XXX C->S " << strIn.str());
  LOG(LM_INFO, "SRV");
  EXPECT_ANY_THROW(server.parseServer());
  LOG(LM_INFO, "XXX S->C " << strOut.str());
  LOG(LM_INFO, "CLI");
  EXPECT_ANY_THROW(clientConnected = client.waitForConnected("invalid", "googletest", priv, "", pub));
  LOG(LM_INFO, "RES " << client.session->info);
  EXPECT_FALSE(clientConnected);
  EXPECT_EQ(client.session->info, "unauthorized");
  EXPECT_FALSE(client.isConnected());


}

TEST(mrpcTest, reconectSpeedup) {
  string priv, pub;
  mobs::generateRsaKeyMem(priv, pub);

  stringstream strOut;
  stringstream strIn;

  MrpcServer server(strIn, strOut, pub, priv);

  mobs::MrpcSession clientSession{};
  mobs::Mrpc client(strOut, strIn, &clientSession, false, MrpcServer::SessionMode::speedup);

  bool clientConnected = false;
  bool readyRead = false;
  LOG(LM_INFO, "CLI");
  EXPECT_FALSE(client.clientAboutToRead());
  ASSERT_NO_THROW(clientConnected = client.waitForConnected("testkey", "googletest", priv, "", pub));
  ASSERT_FALSE(clientConnected);
  LOG(LM_INFO, "XXX C->S " << strIn.str());

  LOG(LM_INFO, "SRV");
  ASSERT_NO_THROW(server.parseServer());
  EXPECT_TRUE(server.serverKeepSession());
  LOG(LM_INFO, "XXX S->C " << strOut.str());

  LOG(LM_INFO, "CLI");
  EXPECT_TRUE(client.clientAboutToRead());
  ASSERT_NO_THROW(clientConnected = client.waitForConnected("testkey", "googletest", priv, "", pub));
  ASSERT_TRUE(clientConnected);
  EXPECT_TRUE(client.clientAboutToRead());

  LOG(LM_INFO, "CLI");
  MrpcPerson p1;
  client.sendSingle(p1);
  LOG(LM_INFO, "XXX C->S " << strIn.str());

  LOG(LM_INFO, "SRV");
  ASSERT_NO_THROW(server.parseServer());
  EXPECT_TRUE(bool(server.resultObj));
  EXPECT_NE(dynamic_cast<MrpcPerson *>(server.resultObj.get()), nullptr);
  MrpcPerson p2;
  p2.name("Heinrich");
  server.sendSingle(p2);
  client.resultObj = nullptr;
  //ASSERT_NO_THROW(server.parseServer());
  LOG(LM_INFO, "XXX S->C " << strOut.str());

  LOG(LM_INFO, "CLI");
  EXPECT_TRUE(client.clientAboutToRead());
  ASSERT_NO_THROW(readyRead = client.parseClient());
  ASSERT_TRUE(bool(client.resultObj));
  to_string(*client.resultObj);
  ASSERT_NE(dynamic_cast<MrpcPerson *>(client.resultObj.get()), nullptr);
  EXPECT_EQ(dynamic_cast<MrpcPerson *>(client.resultObj.get())->name(), "Heinrich");

  EXPECT_FALSE(readyRead);
  EXPECT_TRUE(client.isConnected());
  ASSERT_NO_THROW(readyRead = client.parseClient());
  EXPECT_TRUE(readyRead);
  EXPECT_TRUE(client.isConnected());
  EXPECT_TRUE(client.clientAboutToRead());


  // jetzt reconnect
  stringstream strOut2;
  stringstream strIn2;
  MrpcServer server2(strIn2, strOut2, pub, priv);
  server2.session->sessionId = server.session->sessionId;
  server2.session->sessionKey = server.session->sessionKey;
  mobs::Mrpc client2(strOut2, strIn2, &clientSession, false, MrpcServer::SessionMode::speedup);

  LOG(LM_INFO, "------- reconnect -----");

  clientConnected = false;
  LOG(LM_INFO, "CLI");
  //client2.tryReconnect();
  EXPECT_FALSE(client2.clientAboutToRead());
  ASSERT_NO_THROW(clientConnected = client2.waitForConnected("testkey", "googletest", priv, "", pub));
  EXPECT_TRUE(clientConnected);
  LOG(LM_INFO, "XXX C->S " << strIn2.str());

  LOG(LM_INFO, "SRV");
  ASSERT_NO_THROW(server2.parseServer());
  LOG(LM_INFO, "XXX S->C " << strOut2.str());

  LOG(LM_INFO, "CLI");
  MrpcPerson p3;
  client2.sendSingle(p3);
  LOG(LM_INFO, "XXX C->S " << strIn2.str());

  LOG(LM_INFO, "SRV");
  ASSERT_NO_THROW(server2.parseServer());
  EXPECT_TRUE(bool(server2.resultObj));
  EXPECT_NE(dynamic_cast<MrpcPerson *>(server2.resultObj.get()), nullptr);
  MrpcPerson p4;
  p4.name("Chlodwig");
  server2.sendSingle(p4);
  client2.resultObj = nullptr;
  //ASSERT_NO_THROW(server2.parseServer());
  LOG(LM_INFO, "XXX S->C " << strOut2.str());

  LOG(LM_INFO, "CLI");
  ASSERT_NO_THROW(readyRead = client2.parseClient());
  ASSERT_TRUE(bool(client2.resultObj));
  to_string(*client2.resultObj);
  ASSERT_NE(dynamic_cast<MrpcPerson *>(client2.resultObj.get()), nullptr);
  EXPECT_EQ(dynamic_cast<MrpcPerson *>(client2.resultObj.get())->name(), "Chlodwig");

  EXPECT_FALSE(readyRead);
  EXPECT_TRUE(client.isConnected());
  ASSERT_NO_THROW(readyRead = client2.parseClient());
  EXPECT_TRUE(readyRead);
  EXPECT_TRUE(client.isConnected());

}

TEST(mrpcTest, reconectSpeedupFail) {
  string priv, pub;
  mobs::generateRsaKeyMem(priv, pub);

  stringstream strOut;
  stringstream strIn;

  MrpcServer server(strIn, strOut, pub, priv);

  mobs::MrpcSession clientSession{};
  mobs::Mrpc client(strOut, strIn, &clientSession, false, MrpcServer::SessionMode::speedup);

  bool clientConnected = false;
  bool readyRead = false;
  LOG(LM_INFO, "CLI");
  ASSERT_NO_THROW(clientConnected = client.waitForConnected("testkey", "googletest", priv, "", pub));
  ASSERT_FALSE(clientConnected);
  LOG(LM_INFO, "XXX C->S " << strIn.str());

  LOG(LM_INFO, "SRV");
  ASSERT_NO_THROW(server.parseServer());
  LOG(LM_INFO, "XXX S->C " << strOut.str());

  LOG(LM_INFO, "CLI");
  ASSERT_NO_THROW(clientConnected = client.waitForConnected("testkey", "googletest", priv, "", pub));
  ASSERT_TRUE(clientConnected);

  LOG(LM_INFO, "CLI");
  MrpcPerson p1;
  client.sendSingle(p1);
  LOG(LM_INFO, "XXX C->S " << strIn.str());

  LOG(LM_INFO, "SRV");
  ASSERT_NO_THROW(server.parseServer());
  EXPECT_TRUE(bool(server.resultObj));
  EXPECT_NE(dynamic_cast<MrpcPerson *>(server.resultObj.get()), nullptr);
  MrpcPerson p2;
  p2.name("Heinrich");
  server.sendSingle(p2);
  client.resultObj = nullptr;
  //ASSERT_NO_THROW(server.parseServer());
  LOG(LM_INFO, "XXX S->C " << strOut.str());

  LOG(LM_INFO, "CLI");
  ASSERT_NO_THROW(readyRead = client.parseClient());
  ASSERT_TRUE(bool(client.resultObj));
  EXPECT_FALSE(readyRead);
  to_string(*client.resultObj);
  ASSERT_NE(dynamic_cast<MrpcPerson *>(client.resultObj.get()), nullptr);
  EXPECT_EQ(dynamic_cast<MrpcPerson *>(client.resultObj.get())->name(), "Heinrich");

  ASSERT_NO_THROW(readyRead = client.parseClient());
  EXPECT_TRUE(readyRead);

  // jetzt reconnect
  stringstream strOut2;
  stringstream strIn2;
  MrpcServer server2(strIn2, strOut2, pub, priv);
  // Falsche Session Id
  server2.session->sessionId = server.session->sessionId + 1;
  server2.session->sessionKey = server.session->sessionKey;
  mobs::Mrpc client2(strOut2, strIn2, &clientSession, false, MrpcServer::SessionMode::speedup);

  LOG(LM_INFO, "------- reconnect -----");

  clientConnected = false;
  LOG(LM_INFO, "CLI");
  ASSERT_NO_THROW(clientConnected = client2.waitForConnected("testkey", "googletest", priv, "", pub));
  EXPECT_TRUE(clientConnected);
  MrpcPerson p3;
  client2.sendSingle(p3);
  LOG(LM_INFO, "XXX C->S " << strIn2.str());

  LOG(LM_INFO, "SRV");
  EXPECT_ANY_THROW(server2.parseServer());
  LOG(LM_INFO, "XXX S->C " << strOut2.str());

  LOG(LM_INFO, "SRV");
  EXPECT_ANY_THROW(server2.parseServer());

  LOG(LM_INFO, "CLI");
  ASSERT_ANY_THROW(client2.parseClient());
  EXPECT_EQ(0, client2.session->sessionId);


}


TEST(mrpcTest, reconect) {
  string priv, pub;
  mobs::generateRsaKeyMem(priv, pub);

  stringstream strOut;
  stringstream strIn;

  MrpcServer server(strIn, strOut, pub, priv);

  mobs::MrpcSession clientSession{};
  mobs::Mrpc client(strOut, strIn, &clientSession, false, MrpcServer::SessionMode::keep);

  bool clientConnected = false;
  bool readyRead = false;
  LOG(LM_INFO, "CLI");
  EXPECT_FALSE(client.clientAboutToRead());
  ASSERT_NO_THROW(clientConnected = client.waitForConnected("testkey", "googletest", priv, "", pub));
  ASSERT_FALSE(clientConnected);
  LOG(LM_INFO, "XXX C->S " << strIn.str());

  LOG(LM_INFO, "SRV");
  ASSERT_NO_THROW(server.parseServer());
  LOG(LM_INFO, "XXX S->C " << strOut.str());

  LOG(LM_INFO, "CLI");
  EXPECT_TRUE(client.clientAboutToRead());
  ASSERT_NO_THROW(clientConnected = client.waitForConnected("testkey", "googletest", priv, "", pub));
  ASSERT_TRUE(clientConnected);
  EXPECT_TRUE(client.clientAboutToRead());

  LOG(LM_INFO, "CLI");
  MrpcPerson p1;
  client.sendSingle(p1);
  LOG(LM_INFO, "XXX C->S " << strIn.str());

  LOG(LM_INFO, "SRV");
  ASSERT_NO_THROW(server.parseServer());
  EXPECT_TRUE(bool(server.resultObj));
  EXPECT_NE(dynamic_cast<MrpcPerson *>(server.resultObj.get()), nullptr);
  MrpcPerson p2;
  p2.name("Heinrich");
  server.sendSingle(p2);
  client.resultObj = nullptr;
  //ASSERT_NO_THROW(server.parseServer());
  LOG(LM_INFO, "XXX S->C " << strOut.str());

  LOG(LM_INFO, "CLI");
  EXPECT_TRUE(client.clientAboutToRead());
  ASSERT_NO_THROW(readyRead = client.parseClient());
  ASSERT_TRUE(bool(client.resultObj));
  to_string(*client.resultObj);
  ASSERT_NE(dynamic_cast<MrpcPerson *>(client.resultObj.get()), nullptr);
  EXPECT_EQ(dynamic_cast<MrpcPerson *>(client.resultObj.get())->name(), "Heinrich");

  EXPECT_FALSE(readyRead);
  EXPECT_TRUE(client.isConnected());
  ASSERT_NO_THROW(readyRead = client.parseClient());
  EXPECT_TRUE(readyRead);
  EXPECT_TRUE(client.isConnected());
  EXPECT_TRUE(client.clientAboutToRead());


  // jetzt reconnect
  stringstream strOut2;
  stringstream strIn2;
  MrpcServer server2(strIn2, strOut2, pub, priv);
  server2.session->sessionId = server.session->sessionId;
  server2.session->sessionKey = server.session->sessionKey;
  mobs::Mrpc client2(strOut2, strIn2, &clientSession, false);

  LOG(LM_INFO, "------- reconnect -----");

  clientConnected = false;
  LOG(LM_INFO, "CLI");
  EXPECT_FALSE(client2.clientAboutToRead());
  ASSERT_NO_THROW(clientConnected = client2.waitForConnected("testkey", "googletest", priv, "", pub));
  EXPECT_FALSE(clientConnected);
  LOG(LM_INFO, "XXX C->S " << strIn2.str());

  LOG(LM_INFO, "SRV");
  ASSERT_NO_THROW(server2.parseServer());
  LOG(LM_INFO, "XXX S->C " << strOut2.str());

  LOG(LM_INFO, "CLI");
  EXPECT_TRUE(client2.clientAboutToRead());
  ASSERT_NO_THROW(clientConnected = client2.waitForConnected("testkey", "googletest", priv, "", pub));
  EXPECT_TRUE(clientConnected);
  LOG(LM_INFO, "XXX C->S " << strIn2.str());

  LOG(LM_INFO, "CLI");
  MrpcPerson p3;
  client2.sendSingle(p3);
  LOG(LM_INFO, "XXX C->S " << strIn2.str());

  LOG(LM_INFO, "SRV");
  ASSERT_NO_THROW(server2.parseServer());
  EXPECT_TRUE(bool(server2.resultObj));
  EXPECT_NE(dynamic_cast<MrpcPerson *>(server2.resultObj.get()), nullptr);
  MrpcPerson p4;
  p4.name("Chlodwig");
  server2.sendSingle(p4);
  client2.resultObj = nullptr;
  //ASSERT_NO_THROW(server2.parseServer());
  LOG(LM_INFO, "XXX S->C " << strOut2.str());

  LOG(LM_INFO, "CLI");
  ASSERT_NO_THROW(readyRead = client2.parseClient());
  EXPECT_FALSE(bool(client2.resultObj));
  ASSERT_NO_THROW(readyRead = client2.parseClient());
  ASSERT_TRUE(bool(client2.resultObj));
  to_string(*client2.resultObj);
  ASSERT_NE(dynamic_cast<MrpcPerson *>(client2.resultObj.get()), nullptr);
  EXPECT_EQ(dynamic_cast<MrpcPerson *>(client2.resultObj.get())->name(), "Chlodwig");

  EXPECT_FALSE(readyRead);
  EXPECT_TRUE(client.isConnected());
  ASSERT_NO_THROW(readyRead = client2.parseClient());
  EXPECT_TRUE(readyRead);
  EXPECT_TRUE(client.isConnected());

}

TEST(mrpcTest, reconectReuse) {
  string priv, pub;
  mobs::generateRsaKeyMem(priv, pub);

  stringstream strOut;
  stringstream strIn;

  MrpcServer server(strIn, strOut, pub, priv);

  mobs::MrpcSession clientSession{};
  mobs::Mrpc client(strOut, strIn, &clientSession, false, MrpcServer::SessionMode::keep);

  bool clientConnected = false;
  bool readyRead = false;
  LOG(LM_INFO, "CLI");
  ASSERT_NO_THROW(clientConnected = client.waitForConnected("testkey", "googletest", priv, "", pub));
  ASSERT_FALSE(clientConnected);
  LOG(LM_INFO, "XXX C->S " << strIn.str());

  LOG(LM_INFO, "SRV");
  ASSERT_NO_THROW(server.parseServer());
  LOG(LM_INFO, "XXX S->C " << strOut.str());

  LOG(LM_INFO, "CLI");
  ASSERT_NO_THROW(clientConnected = client.waitForConnected("testkey", "googletest", priv, "", pub));
  ASSERT_TRUE(clientConnected);

  LOG(LM_INFO, "CLI");
  MrpcPerson p1;
  client.sendSingle(p1);
  LOG(LM_INFO, "XXX C->S " << strIn.str());

  LOG(LM_INFO, "SRV");
  ASSERT_NO_THROW(server.parseServer());
  EXPECT_TRUE(bool(server.resultObj));
  EXPECT_NE(dynamic_cast<MrpcPerson *>(server.resultObj.get()), nullptr);
  MrpcPerson p2;
  p2.name("Heinrich");
  server.sendSingle(p2);
  client.resultObj = nullptr;
  //ASSERT_NO_THROW(server.parseServer());
  LOG(LM_INFO, "XXX S->C " << strOut.str());

  LOG(LM_INFO, "CLI");
  ASSERT_NO_THROW(readyRead = client.parseClient());
  ASSERT_TRUE(bool(client.resultObj));
  EXPECT_FALSE(readyRead);
  to_string(*client.resultObj);
  ASSERT_NE(dynamic_cast<MrpcPerson *>(client.resultObj.get()), nullptr);
  EXPECT_EQ(dynamic_cast<MrpcPerson *>(client.resultObj.get())->name(), "Heinrich");

  ASSERT_NO_THROW(readyRead = client.parseClient());
  EXPECT_TRUE(readyRead);

  // jetzt reconnect
  stringstream strOut2;
  stringstream strIn2;
  MrpcServer server2(strIn2, strOut2, pub, priv);
  // Falsche Session Id
  server2.session->sessionId = server.session->sessionId + 1;
  server2.session->sessionKey = server.session->sessionKey;
  mobs::Mrpc client2(strOut2, strIn2, &clientSession, false);

  LOG(LM_INFO, "------- reconnect -----");

  clientConnected = false;
  LOG(LM_INFO, "CLI");
  EXPECT_FALSE(client2.clientAboutToRead());
  ASSERT_NO_THROW(clientConnected = client2.waitForConnected("testkey", "googletest", priv, "", pub));
  EXPECT_FALSE(clientConnected);
  LOG(LM_INFO, "XXX C->S " << strIn2.str());

  LOG(LM_INFO, "SRV");
  ASSERT_NO_THROW(server2.parseServer());
  LOG(LM_INFO, "XXX S->C " << strOut2.str());

  LOG(LM_INFO, "CLI");
  EXPECT_TRUE(client2.clientAboutToRead());
  ASSERT_NO_THROW(clientConnected = client2.waitForConnected("testkey", "googletest", priv, "", pub));
  EXPECT_FALSE(clientConnected);
  LOG(LM_INFO, "XXX C->S " << strIn2.str());

  LOG(LM_INFO, "SRV");
  ASSERT_NO_THROW(server2.parseServer());
  LOG(LM_INFO, "XXX S->C " << strOut2.str());

  LOG(LM_INFO, "CLI");
  EXPECT_TRUE(client2.clientAboutToRead());
  ASSERT_NO_THROW(clientConnected = client2.waitForConnected("testkey", "googletest", priv, "", pub));
  ASSERT_TRUE(clientConnected);
  LOG(LM_INFO, "XXX C->S " << strIn2.str());

  LOG(LM_INFO, "CLI");
  MrpcPerson p3;
  client2.sendSingle(p3);
  LOG(LM_INFO, "XXX C->S " << strIn2.str());

  LOG(LM_INFO, "SRV");
  ASSERT_NO_THROW(server2.parseServer());
  EXPECT_TRUE(bool(server2.resultObj));
  EXPECT_NE(dynamic_cast<MrpcPerson *>(server2.resultObj.get()), nullptr);
  MrpcPerson p4;
  p4.name("Chlodwig");
  server2.sendSingle(p4);
  client2.resultObj = nullptr;
  //ASSERT_NO_THROW(server2.parseServer());
  LOG(LM_INFO, "XXX S->C " << strOut2.str());

  LOG(LM_INFO, "CLI");
  ASSERT_NO_THROW(readyRead = client2.parseClient());
  ASSERT_TRUE(bool(client2.resultObj));
  to_string(*client2.resultObj);
  ASSERT_NE(dynamic_cast<MrpcPerson *>(client2.resultObj.get()), nullptr);
  EXPECT_EQ(dynamic_cast<MrpcPerson *>(client2.resultObj.get())->name(), "Chlodwig");

  EXPECT_FALSE(readyRead);
  EXPECT_TRUE(client.isConnected());
  ASSERT_NO_THROW(readyRead = client2.parseClient());
  EXPECT_TRUE(readyRead);
  EXPECT_TRUE(client.isConnected());
}


TEST(mrpcTest, MrpcRefreshKey) {
  string priv, pub;
  mobs::generateRsaKeyMem(priv, pub);

  stringstream strOut;
  stringstream strIn;

  MrpcServer server(strIn, strOut, pub, priv);

  mobs::MrpcSession clientSession{};
  mobs::Mrpc client(strOut, strIn, &clientSession, false, MrpcServer::SessionMode::keep);

  bool clientConnected = false;
  LOG(LM_INFO, "CLI");
  ASSERT_NO_THROW(clientConnected = client.waitForConnected("testkey", "googletest", priv, "", pub));
  ASSERT_FALSE(clientConnected);
  LOG(LM_INFO, "XXX C->S " << strIn.str());

  LOG(LM_INFO, "SRV");
  ASSERT_NO_THROW(server.parseServer());
  LOG(LM_INFO, "XXX S->C " << strOut.str());

  LOG(LM_INFO, "CLI");
  ASSERT_NO_THROW(clientConnected = client.waitForConnected("testkey", "googletest", priv, "", pub));
  ASSERT_TRUE(clientConnected);

  LOG(LM_INFO, "CLI");
  MrpcPerson p1;
  client.sendSingle(p1);
  LOG(LM_INFO, "XXX C->S " << strIn.str());

  LOG(LM_INFO, "SRV");
  ASSERT_NO_THROW(server.parseServer());
  EXPECT_TRUE(bool(server.resultObj));
  EXPECT_NE(dynamic_cast<MrpcPerson *>(server.resultObj.get()), nullptr);
  server.resultObj = nullptr;

  MrpcPerson p2;
  p2.name("Heinrich");
  server.sendSingle(p2);
  LOG(LM_INFO, "XXX S->C " << strOut.str());

  LOG(LM_INFO, "CLI");
  bool readyRead = false;
  ASSERT_NO_THROW(readyRead = client.parseClient());
  ASSERT_TRUE(bool(client.resultObj));
  EXPECT_FALSE(readyRead);

  //to_string(*client.resultObj);
  ASSERT_NE(dynamic_cast<MrpcPerson *>(client.resultObj.get()), nullptr);
  EXPECT_EQ(dynamic_cast<MrpcPerson *>(client.resultObj.get())->name(), "Heinrich");
  client.resultObj = nullptr;

  ASSERT_NO_THROW(readyRead = client.parseClient());
  EXPECT_TRUE(readyRead);

  LOG(LM_INFO, "Refresh Key");
  client.refreshSessionKey();  // Schlüssel aktualisieren
  LOG(LM_INFO, "XXX C->S " << strIn.str());

  LOG(LM_INFO, "SRV");
  ASSERT_NO_THROW(server.parseServer());
  EXPECT_FALSE(bool(server.resultObj));
  ASSERT_NO_THROW(server.parseServer());
  EXPECT_FALSE(bool(server.resultObj));
  LOG(LM_INFO, "XXX S->C " << strOut.str());

  LOG(LM_INFO, "CLI");
  ASSERT_NO_THROW(readyRead = client.parseClient());
  EXPECT_FALSE(bool(client.resultObj));
  EXPECT_FALSE(readyRead);


  LOG(LM_INFO, "CLI");
  client.sendSingle(p1);
  LOG(LM_INFO, "XXX C->S " << strIn.str());

  LOG(LM_INFO, "SRV");
  ASSERT_NO_THROW(server.parseServer());
  ASSERT_NO_THROW(server.parseServer());
  EXPECT_TRUE(bool(server.resultObj));
  EXPECT_NE(dynamic_cast<MrpcPerson *>(server.resultObj.get()), nullptr);
  server.resultObj = nullptr;

  p2.name("Gretchen");
  server.sendSingle(p2);
  LOG(LM_INFO, "XXX S->C " << strOut.str());




  LOG(LM_INFO, "CLI");
  ASSERT_NO_THROW(readyRead = client.parseClient());
  EXPECT_FALSE(readyRead);

  ASSERT_NO_THROW(readyRead = client.parseClient());
  ASSERT_TRUE(bool(client.resultObj));
  EXPECT_FALSE(readyRead);

  //to_string(*client.resultObj);
  ASSERT_NE(dynamic_cast<MrpcPerson *>(client.resultObj.get()), nullptr);
  EXPECT_EQ(dynamic_cast<MrpcPerson *>(client.resultObj.get())->name(), "Gretchen");
  server.resultObj = nullptr;

  ASSERT_NO_THROW(readyRead = client.parseClient());
  EXPECT_TRUE(readyRead);

}

}