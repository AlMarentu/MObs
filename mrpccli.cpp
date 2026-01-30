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
#include "mrpc2.h"
#include "tcpstream.h"

#include <stdio.h>
#include <getopt.h>
#include <sstream>
#include <codecvt>
#include <thread>

using namespace std;



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


int errors = 0;
int queries = 0;

void clientWorker(mobs::MrpcSession &clientSession) {
  try {
    mobs::tcpstream xstream(clientSession.host(), clientSession.port());
    if (not xstream.is_open())
      throw runtime_error("cann't connect");
    xstream.exceptions(std::iostream::failbit | std::iostream::badbit);
    LOG(LM_INFO, "CONNECTED");
    mobs::Mrpc2 client(xstream, xstream, &clientSession, false);
    string id = clientSession.keyName;
    LOG(LM_INFO, "KVALID " << clientSession.keyValid());
    client.startSession(id, "test", id + ".priv", "12345", clientSession.publicServerKey);
//#define WAIT4CONNECTED
    // ist WAIT4CONNECTED gesetzt, so wird jede Verbindung geprüft, bevor Kommandos gesendet werden, ansonsten nur bei Key refresh
#ifndef WAIT4CONNECTED
    // wenn 80% der Key-Time abgelaufen, Session verlängern; kann nur stattfinden, wenn Connection bereits besteht und idle ist
    if (clientSession.keyNeedsRefresh()) {
#endif
    client.stopEncrypt();
    client.flush();
    while (not client.isConnected()) {
      LOG(LM_INFO, "WAIT for connected");
      client.parseClient();
    }
#ifdef WAIT4CONNECTED
    // wenn 80% der Key-Time abgelaufen, Session verlängern; kann nur stattfinden, wenn Connection bereits besteht und idle ist
    if (clientSession.keyNeedsRefresh()) {
#endif
      LOG(LM_INFO, "AUTOREFRESH");
      client.clientRefreshKey(id + ".priv", "12345", clientSession.publicServerKey);
    }

    MrpcPerson p1;
    p1.name("Goethe");
    client.sendSingle(p1);
    for (;;) {
      LOG(LM_INFO, "Parser");
      client.parseClient();
      if (auto res = client.getResult<MrpcPerson>()) {
        LOG(LM_INFO, "Received 1 " << res->name());
        queries++;
        break;
      }
    }

    p1.name("Lessing");

    client.sendSingle(p1);
    for (;;) {
      LOG(LM_INFO, "Parser");
      client.parseClient();
      if (auto res = client.getResult<MrpcPerson>()) {
        LOG(LM_INFO, "Received 2 " << res->name());
        queries++;
        break;
      }
    }

    client.closeServer();
    xstream.exceptions(std::iostream::goodbit);
    xstream.shutdown();
    LOG(LM_INFO, "CLS " << clientSession.keyName << " " << clientSession.sessionId << " " << clientSession.keyValidTime << " "
                        << clientSession.sessionReuseTime);
  } catch (exception &e) {
    LOG(LM_ERROR, "EXCEPTION " << clientSession.keyName << " " << e.what());
    clientSession.clear();
    errors++;
  }
}


void usage() {
  cerr << "usage: mrpcsrv \n"
       << " -P Port default = '4444'\n"
       << " -v Debug-Level\n";

  exit(1);
}

void doClient(string name, int)
{
  try {
    mobs::generateCryptoKey(mobs::CryptECprime256v1, name + ".priv", name + ".pub", "12345");

    mobs::MrpcSession clientSession("localhost:4444");
    clientSession.publicServerKey = "srv.pub";
    clientSession.keyName = name;

    time_t finish = time(nullptr) + 10;
    while (time(nullptr) <= finish) {
      clientWorker(clientSession);
      //usleep(100000);
    }
  } catch (exception &e) {
      LOG(LM_ERROR, "T " << name << " EXCEPTION " << e.what());
  }
}

int main(int argc, char* argv[]) {
  logging::currentLevel = logging::lm_info;
  string port = "4444";

  try {
    int ch;
    while ((ch = getopt(argc, argv, "P:v")) != -1) {
      switch (ch) {
        case 'P':
          port = optarg;
          break;
        case 'v':
          logging::currentLevel = logging::lm_debug;
          break;
        case '?':
        default:
          usage();
      }
    }

    std::thread t1(doClient, "aaa", 1);
#if 1
    std::thread t2(doClient, "bbb", 2);
    std::thread t3(doClient, "ccc", 3);
    std::thread t4(doClient, "ddd", 4);
    std::thread t5(doClient, "eee", 5);
    std::thread t6(doClient, "fff", 6);
    doClient("cli", 0);
    t6.join();
    t5.join();
    t4.join();
    t3.join();
    t2.join();
#endif
    t1.join();
    LOG(LM_INFO, "Errors = " << errors << " Queries = " << queries);
  }
  catch (exception &e) {
    LOG(LM_ERROR, "EXCEPTION " << e.what());
  }
  return 0;
}