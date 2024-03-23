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

#include "objtypes.h"
#include "nbuf.h"
#include "objgen.h"
#include "xmlout.h"
#include "xmlwriter.h"
#include "xmlread.h"
#include "xmlparser.h"
#include "mrpcsession.h"
#include "mrpc.h"

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

void exampleClient() {

  string serverPub;
  MrpcPerson p;
  p.name("OTTO");

  stringstream strOut;

  stringstream strIn("XXX");

  mobs::MrpcSession clientSession{};
  mobs::Mrpc client(strIn, strOut, &clientSession, false);
  for (;;) {
    if (client.waitForConnected("testkey", "googletest", "priv", "12345", serverPub))
      break;
  }
  client.sendSingle(p);

  for (;;) {
    client.parseClient();
    if (auto res = dynamic_cast<MrpcPerson *>(client.resultObj.get())) {
      break;
    }
  }


}

void exampleServer() {
  MrpcPerson p;
  p.name("OTTO");

  stringstream strOut;

  stringstream strIn("XXX");

  MrpcServer server(strIn, strOut, "", "");

  for (;;) {
    server.parseServer();
    if (auto res = dynamic_cast<MrpcPerson *>(server.resultObj.get())) {
      server.sendSingle(p);
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
  mobs::Mrpc client(strOut, strIn, &clientSession, false);
  client.sessionReuseSpeedup = true;

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
  client2.sessionReuseSpeedup = true;

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
  mobs::Mrpc client(strOut, strIn, &clientSession, false);
  client.sessionReuseSpeedup = true;

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
  client2.sessionReuseSpeedup = true;

  LOG(LM_INFO, "------- reconnect -----");

  clientConnected = false;
  LOG(LM_INFO, "CLI");
  client2.tryReconnect();
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
  mobs::Mrpc client(strOut, strIn, &clientSession, false);

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
  mobs::Mrpc client(strOut, strIn, &clientSession, false);

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
  EXPECT_TRUE(client.isConnected());}

}