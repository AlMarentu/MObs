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

#include <fstream>

#include "csb.h"
#include "crypt.h"

#include "objtypes.h"
#include "objgen.h"
#include "xmlwriter.h"
#include "xmlread.h"
#include "xmlparser.h"
#include "mrpcsession.h"
#include "mrpcec.h"
#include "tcpstream.h"

#include <getopt.h>
#include <sstream>
#include <sys/stat.h>
#include <thread>
#include <mutex>

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

class Progress : virtual public mobs::ObjectBase
{
public:
  ObjInit(Progress);

  MemVar(int, percent);
  MemVar(std::string, comment);
};

class LangeListe : virtual public mobs::ObjectBase
{
public:
  ObjInit(LangeListe);

  MemVar(std::string, name);
  MemVar(std::string, comment);
};
ObjRegister(LangeListe);

class LoadFile : virtual public mobs::ObjectBase
{
public:
  ObjInit(LoadFile);

  MemVar(long int, length);
  MemVar(std::string, name);
};
ObjRegister(LoadFile);

class BigDat : virtual public mobs::ObjectBase
{
public:
  ObjInit(BigDat);

  MemVar(long int, length);
  MemVar(std::string, name);
};
ObjRegister(BigDat);


class MrpcServer : public mobs::MrpcEc {
public:
  explicit MrpcServer(mobs::tcpstream &tcpstr, const std::string &priv = "") :
      MrpcEc(tcpstr, tcpstr, &mrpcSession, false), tcpstream(tcpstr), privKey(priv) {}

  std::string getSenderPublicKey(const std::string &keyId) override {
    string pub = keyId + ".pub";
    struct stat st{};
    if (stat(pub .c_str(), &st)) {
      LOG(LM_ERROR, "kein public key für " << keyId);
      return {};
    }
    return pub;
  }

  void keyChanged(const std::vector<u_char> &cipher, const std::string &keyId) override {
    LOG(LM_INFO, "SRV KEYCHANGE RECEIVED " << keyId);
    string cipherStr = mobs::to_string_base64(cipher);
    {
      std::lock_guard<std::mutex> guard(mutex);
      lastSessions.erase(cipherStr);
    }
    if (not session)
      throw std::runtime_error("session missing");
    setEcdhSessionKey(cipher, privKey, "");
    lastSessions.emplace(cipherStr, *session);
    clientCipher = cipherStr;
  }

  void loginReceived(const std::vector<u_char> &cipher, const std::string &keyId) override {
    LOG(LM_INFO, "SRV LOGIN RECEIVED " << keyId);
    std::vector<u_char> key;

    string cipherStr = mobs::to_string_base64(cipher);
    auto it = lastSessions.find(cipherStr);
    if (it == lastSessions.end() or (not keyId.empty() and keyId != it->second.keyName)) {
      if (not session)
        throw std::runtime_error("session missing");
      setEcdhSessionKey(cipher, privKey, "");

      {
        static int snr = 0;
        session->sessionId = ++snr;
        session->sessionReuseTime = 120;
        session->keyValidTime = 10;
        LOG(LM_INFO, "NEW    " << session->sessionId);
      }
      //session->generated = time(nullptr);
      {
        std::lock_guard<std::mutex> guard(mutex);
        if (it == lastSessions.end())
          it = lastSessions.emplace(cipherStr, *session).first;
        else
          it->second = *session;
        // abgelaufenen Sessions aufräumen
        for (auto i = lastSessions.begin(); i != lastSessions.end(); ) {
          LOG(LM_INFO, "VSEC " << i->second.keyValid() << " sec: "  << i->second.sessionId);
          if (i->second.expired()) {
            LOG(LM_INFO, "ERASE old session " << i->second.sessionId);
            i = lastSessions.erase(i);
          }
          else
            i++;
        }
      }
      LOG(LM_INFO, "CURRENT SESSIONS " << lastSessions.size());
      clientCipher = cipherStr;
    }
    else {
      *session = it->second;
      LOG(LM_INFO, "REUSE OLD SESSION KEY " << session->sessionId);
    }
  }

  void authenticated(const std::string &login, const std::string &host, const std::string &software) override {
    LOG(LM_INFO, "AUTH " << session->info);
    if (software == "qttest")
      session->keyValidTime = 30;
  }



  mobs::tcpstream &tcpstream;
  mobs::MrpcSession mrpcSession{};
  std::string clientCipher;
  std::string privKey;
  static std::mutex mutex;
  static map<string, mobs::MrpcSession> lastSessions;
};

map<string, mobs::MrpcSession> MrpcServer::lastSessions;
std::mutex MrpcServer::mutex;





#define TLOG(l, x) LOG(l, "T " << t << " " << x << std::flush)
void server(mobs::TcpAccept &tcpAccept, int t) {

  map<string, string> vornamen = { {"Goethe", "Johann Wolfgang von"},
                                   {"Mozart", "Wolfgang Amadeus"},
                                   {"Schiller", "Friedrich"},
                                   {"Lessing", "Gotthold Ephraim"},
                                   {"Shakespeare", "William"},
                                   {"Bach", "Johann Sebastian"},
                                   {"Weber", "Carl Maria von"}} ;
  for (;;) {
    try {
      TLOG(LM_INFO, "WAITING");
      mobs::tcpstream xstream(tcpAccept);
      xstream.exceptions(std::iostream::failbit | std::iostream::badbit);
      TLOG(LM_INFO, "Remote: " << xstream.getRemoteHost() << " " << xstream.getRemoteIp());

      MrpcServer server(xstream, mobs::readPrivateKey("srv.priv", "00000"));
      while (not server.eot()) {
        server.parseServer();
        TLOG(LM_INFO, "Parser");
        if (auto res = server.getResult<MrpcPing>()) {
          LOG(LM_INFO, "Received Ping");
          server.sendSingle(*res);
        }
        else if (auto res = server.getResult<MrpcPerson>()) {
          TLOG(LM_INFO, "Received " << res->name());
          auto it = vornamen.find(res->name());
          MrpcPerson p;
          p.name(it != vornamen.end() ? it->second : "unbekannt");
          server.sendSingle(p);
        }
        else if (auto res = server.getResult<LangeListe>()) {
          TLOG(LM_INFO, "Received " << res->name());
          server.encrypt();
          server.writer.writeTagBegin(L"liste");
          for (auto i = 0; i < 1000; i++) {
            Progress p;
            p.percent(i/10);
            p.comment("Bitte warten ...");
            usleep(5000);
            server.xmlOut(p);
          }
          server.writer.writeTagEnd();
          auto it = vornamen.find(res->name());
          LangeListe p;
          p.name(it != vornamen.end() ? it->second : "unbekannt");
          server.sendSingle(p);
        }
        else if (auto res = server.getResult<LoadFile>()) {
          LoadFile p;
          p.name("log");
          struct stat sbuf;
          if (::stat("log", &sbuf) != 0)
            THROW("stat failed");
          p.length(sbuf.st_size);
          server.sendSingle(p, sbuf.st_size);
          auto &str = server.outByteStream();
          ifstream istr(p.name());
          if (not istr.is_open())
            THROW("open failed");
          str << istr.rdbuf();
          istr.close();
          auto sz = server.closeOutByteStream();
          LOG(LM_INFO, "Bytes written " << sz);
          server.writer.putc('\n');
          server.writer.sync();
          server.flush();
        }
        else if (auto res = server.getResult<BigDat>()) {
          LOG(LM_INFO, "Received BigDat");
          while (server.isEncrypted() or not server.inByteStreamAvail()) {
            LOG(LM_INFO, "WAIT DATA STARTS " << server.getAttachmentLength());
            server.parseServer();
          }
          LOG(LM_INFO, "Start Attachment " << server.getAttachmentLength());
          auto &istr = server.inByteStream();
          std::ofstream ostr("raus");
          ostr << istr.rdbuf();
          ostr.close();
          LOG(LM_INFO, "DATA STORED");

          BigDat p;
          p.name("log");
          struct stat sbuf;
          p.length(sbuf.st_size);
          server.sendSingle(p);
        }
      }
      xstream.exceptions(std::iostream::goodbit);
      TLOG(LM_INFO, "Server beendet");
    } catch (exception &e) {
      TLOG(LM_ERROR, "Server EX: " << e.what());
    }
  }

}

void usage() {
  cerr << "usage: mrpcsrv \n"
       << " -P Port default = '4444'\n"
       << " -v Debug-Level\n";

  exit(1);
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
    mobs::generateCryptoKey(mobs::CryptECprime256v1, "srv.priv", "srv.pub", "00000");

    mobs::TcpAccept tcpAccept;
    if (tcpAccept.initService(port) < 0)
      THROW("Service not started");

    std::thread t1(server, std::ref(tcpAccept), 1);
    std::thread t2(server, std::ref(tcpAccept), 2);
    server(tcpAccept, 0);
  }
  catch (exception &e) {
    LOG(LM_ERROR, "EXCEPTION " << e.what());
  }
  return 0;
}