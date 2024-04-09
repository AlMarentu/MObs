// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2024 Matthias Lautner
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

#include <unistd.h>
#include "mrpc.h"
#include "xmlwriter.h"
#include "xmlout.h"
#include "aes.h"
#include "rsa.h"

namespace mobs {

namespace {
class MrpcSessionLoginResult : virtual public mobs::ObjectBase {
public:
  ObjInit(MrpcSessionLoginResult);

  MemVar(std::vector<u_char>, key);
  MemVar(u_int, sessId);
  MemVar(u_int, sessionReuseTime, USENULL);
  MemVar(u_int, sessionKeyValidTime, USENULL);
};

class MrpcSessionLoginData : virtual public mobs::ObjectBase {
public:
  ObjInit(MrpcSessionLoginData);

  MemVar(std::string, login);
  MemVar(std::string, software);
  MemVar(std::string, hostname);
  MemVar(std::string, key);
};

class MrpcSessionLogin : virtual public mobs::ObjectBase {
public:
  ObjInit(MrpcSessionLogin);
  MemVar(std::vector<u_char>, cipher);
  MemVar(std::string, info, USENULL);
  // soll keine Session wiederverwendet werden, dann info setzen
  MemVar(bool, dontKeep, USENULL);
};

class MrpcGetPublickey : virtual public mobs::ObjectBase {
public:
  ObjInit(MrpcGetPublickey);
  MemVar(std::string, pubKey);
  MemVar(std::string, info);
};

class MrpcSessionUse : virtual public mobs::ObjectBase {
public:
  ObjInit(MrpcSessionUse);
  MemVar(u_int, id);
  MemVar(bool, verify, USENULL);
  MemVar(std::string, info, USENULL);
};

class MrpcSessionRefreshKey : virtual public mobs::ObjectBase {
public:
  ObjInit(MrpcSessionRefreshKey);
  MemVar(std::vector<u_char>, key, USENULL);
  MemVar(u_int, sessionReuseTime, USENULL);
  MemVar(u_int, sessionKeyValidTime, USENULL);
  MemVar(std::string, info, USENULL);
};

class MrpcSessionReturnError : virtual public mobs::ObjectBase {
public:
  ObjInit(MrpcSessionReturnError);
  MemVar(std::string, error);
};

class MrpcSessionTestConnection : virtual public mobs::ObjectBase {
public:
  ObjInit(MrpcSessionTestConnection);
  MemVar(std::string, info, USENULL);
};


}


std::string MrpcSession::host() const
{
  auto pos = server.find(':');
  if (pos == std::string::npos)
    return server;
  return server.substr(0, pos);
}

std::string MrpcSession::port() const
{
  auto pos = server.find(':');
  if (pos == std::string::npos)
    return {};
  return server.substr(pos + 1);
}

void Mrpc::encrypt()
{
  std::vector<u_char> iv;
  if (not session)
    throw std::runtime_error("session missing");
  if (writer.cryptingLevel() == 0)
  {
    iv.resize(mobs::CryptBufAes::iv_size());
    mobs::CryptBufAes::getRand(iv);
    writer.startEncrypt(new mobs::CryptBufAes(session->sessionKey, iv, "", true));
  }
}

void Mrpc::stopEncrypt()
{
  writer.stopEncrypt();
}

void Mrpc::EncryptionFinished()
{
  LOG(LM_DEBUG, "Encryption finished " << level());
  encrypted = false;
  if (state == connected and level() == 2)
    state = readyRead;
  // weiteres parsen anhalten
  stop();
}

void Mrpc::Encrypt(const std::string &algorithm, const std::string &keyName, const std::string &cipher,
                   CryptBufBase *&cryptBufp)
{
  LOG(LM_DEBUG, "Encryption " << algorithm << " keyName " << keyName << " cipher " << cipher);
  if (not session)
    throw std::runtime_error("session missing");
  encrypted = true;
  if (algorithm == "aes-256-cbc") {
    cryptBufp = new mobs::CryptBufAes(session->sessionKey);
  }
  session->last = time(nullptr);
}

void Mrpc::filled(mobs::ObjectBase *obj, const std::string &error)
{
  LOG(LM_DEBUG, "filled " << obj->to_string() << (isEncrypted()? " OK":" UNENCRYPTED"));
  if (not error.empty())
    THROW("error in XML stream: " << error);
  if (resultObj)
    THROW("result object bereits vorhanden: " << resultObj->getObjectName());
  resultObj = std::unique_ptr<mobs::ObjectBase>(obj);
  // parsen anhalten
  stop();
}

void Mrpc::StartTag(const std::string &element)
{
  LOG(LM_DEBUG, "start " << element);
  // Wenn passendes Tag gefunden, dann Objekt einlesen
  if (state == connectingServer and element == "MrpcSessionUse")
    fill(new MrpcSessionUse);
  else if (state == connectingServer or state == connectingClient or state == getPubKey) { // ohne Login nur fixe Auswahl
    if (element == "MrpcSessionLogin")
      fill(new MrpcSessionLogin);
    else if (element == "MrpcSessionReturnError")
      fill(new MrpcSessionReturnError);
    else if (element == "MrpcGetPublickey")
      fill(new MrpcGetPublickey);
    else if (auto o = ObjectBase::createObj(element))
      fill(o);
    else if ((state != connectingServer or element != "methodCall") and (state != connectingClient or element != "methodResponse") and
             (state != getPubKey or element != "methodResponse"))
      LOG(LM_WARNING, "unknown element " << element);
  } else {
    auto o = ObjectBase::createObj(element);
    if (o)
      fill(o);
    else if (state == connected and element == "MrpcSessionRefreshKey")
      fill(new MrpcSessionRefreshKey);
    else if ((state == reconnectingClient or state == reconnectingClientTest) and element == "MrpcSessionReturnError")
      fill(new MrpcSessionReturnError);
    else if ((state == reconnectingClient or state == reconnectingClientTest) and element == "MrpcSessionTestConnection")
      fill(new MrpcSessionTestConnection);
    else if ((state != reconnectingClient and state != reconnectingClientTest) or element != "methodResponse")
      LOG(LM_WARNING, "unknown element " << element);
  }
}

void Mrpc::EndTag(const std::string &element)
{
  LOG(LM_DEBUG, "end " << element << " lev " << level());
  if (state == connected and not encrypted and level() == 2)
    state = readyRead;
}

bool Mrpc::inByteStreamAvail() {
  // es muss mindestens ein Zeichen im Puffer sein, für den Delimiter
  auto s = streambufI.getIstream().rdbuf()->in_avail();
  return s > 0;
}

std::istream &Mrpc::inByteStream(size_t sz)
{
  LOG(LM_DEBUG, "Mrpc::inByteStream " << mobs::CryptBufAes::aes_size(sz));
  if (not session)
    throw std::runtime_error("session missing");
  return byteStream(mobs::CryptBufAes::aes_size(sz), new mobs::CryptBufAes(session->sessionKey));
}

std::ostream &Mrpc::outByteStream()
{
  std::vector<u_char> iv;
  iv.resize(mobs::CryptBufAes::iv_size());
  mobs::CryptBufAes::getRand(iv);
  if (not session)
    throw std::runtime_error("session missing");
  return writer.byteStream("\200", new mobs::CryptBufAes(session->sessionKey, iv, "", true));
}

std::streamsize Mrpc::closeOutByteStream()
{
  return writer.closeByteStream();
}


Mrpc::Mrpc(std::istream &inStr, std::ostream &outStr, MrpcSession *mrpcSession, bool nonBlocking, SessionMode mode) :
    XmlReader(iStr), streambufI(inStr), streambufO(outStr),
    iStr(&streambufI), oStr(&streambufO),
    writer(oStr, mobs::XmlWriter::CS_utf8, false),
    session(mrpcSession), sessionMode(mode)
{
  readTillEof(false);
  readNonBlocking(nonBlocking);
  oStr.exceptions(std::wostream::failbit | std::wostream::badbit);
  iStr.exceptions(std::wostream::failbit | std::wostream::badbit);
}


void Mrpc::receiveSessionKey(const std::vector<u_char> &cipher, const std::string &privkey, const std::string &passwd)
{
  std::vector<u_char> sessInhalt;
  mobs::decryptPrivateRsa(cipher, sessInhalt, privkey, passwd);
  std::string buf((char *)&sessInhalt[0], sessInhalt.size());
  MrpcSessionLoginResult result;
  mobs::string2Obj(buf, result, mobs::ConvObjFromStr());
  if (not session)
    throw std::runtime_error("session missing");
  session->sessionKey = result.key();
  session->sessionId = result.sessId();
  session->sessionReuseTime = result.sessionReuseTime();
  session->generated = time(nullptr);
  session->keyValidTime = result.sessionKeyValidTime();
  LOG(LM_DEBUG, "Received Session Key " << session->sessionId << " " << session->info << " reuse="
                                           << session->sessionReuseTime << " valid=" << session->keyValidTime);
}

std::string Mrpc::receiveLogin(const std::vector<u_char> &cipher, const std::string &privkey, const std::string &passwd,
                               std::string &login, std::string &software, std::string &hostname)
{
  std::vector<u_char> sessInhalt;
  mobs::decryptPrivateRsa(cipher, sessInhalt, privkey, passwd);
  std::string buf((char *)&sessInhalt[0], sessInhalt.size());
  MrpcSessionLoginData data;
  mobs::string2Obj(buf, data, mobs::ConvObjFromStr());
  login = data.login();
  software = data.software();
  hostname = data.hostname();
  LOG(LM_DEBUG, "LOGIN = " << data.to_string());
  return data.key();
}

std::vector<u_char> Mrpc::generateSessionKey(const std::string &clientkey)
{
  std::vector<u_char> cipher;
  MrpcSessionLoginResult result;
  if (not session)
    throw std::runtime_error("session missing");
  if (not session->sessionId)
    throw std::runtime_error("sessionId empty");
  result.sessId(session->sessionId);
  if (session->sessionReuseTime > 0)
    result.sessionReuseTime(session->sessionReuseTime);
  if (session->keyValidTime > 0)
    result.sessionKeyValidTime(session->keyValidTime);
  session->sessionKey.resize(mobs::CryptBufAes::key_size());
  mobs::CryptBufAes::getRand(session->sessionKey);
  result.key(session->sessionKey);
  session->generated = time(nullptr);
  session->keyName = clientkey;

  std::string buffer = result.to_string(mobs::ConvObjToString().exportJson().noIndent());
  std::vector<u_char> inhalt;
  copy(buffer.begin(), buffer.end(), back_inserter(inhalt));
  encryptPublicRsa(inhalt, cipher, clientkey);
  return cipher;
}

void Mrpc::sendNewSessionKey() // Server
{
  MrpcSessionRefreshKey result;
  // neuen Key erzeugen und senden
  std::vector<u_char> newKey;
  newKey.resize(mobs::CryptBufAes::key_size());
  mobs::CryptBufAes::getRand(newKey);
  if (session->sessionReuseTime > 0)
    result.sessionReuseTime(session->sessionReuseTime);
  if (session->keyValidTime > 0)
    result.sessionKeyValidTime(session->keyValidTime);
  result.key(newKey);
  //result.info();
  LOG(LM_INFO, "Refresh session key " << session->sessionId << " " << session->info);
  sendSingle(result); // mit altem Schlüssel versenden, dann wird der neue Schlüssel verwendet
  session->sessionKey = newKey;
  session->generated = time(nullptr);
}

void Mrpc::refreshSessionKey()  // Client
{
  MrpcSessionRefreshKey result;
  sendSingle(result);
}


std::vector<u_char> Mrpc::generateLoginInfo(const std::string &keyId, const std::string &software,
                                            const std::string &serverkey)
{
  MrpcSessionLoginData loginData;
  //data.login(fingerprint);
  loginData.software(software);
  loginData.hostname(getNodeName());
  loginData.key(keyId);
  loginData.login(getLoginName());
  std::string buffer = loginData.to_string(mobs::ConvObjToString().exportJson().noIndent());
  std::vector<u_char> inhalt;
  copy(buffer.begin(), buffer.end(), back_inserter(inhalt));
  std::vector<u_char> cipher;
  mobs::encryptPublicRsa(inhalt, cipher, serverkey);
  return cipher;
}

void Mrpc::xmlOut(const ObjectBase &obj)
{
  XmlOut xo(&writer, mobs::ConvObjToString().exportXml().exportWoNull());
  obj.traverse(xo);
  //writer.putc('\n');
}

void Mrpc::sendSingle(const ObjectBase &obj)
{
  encrypt();
  xmlOut(obj);
  stopEncrypt();
  //writer.putc('\n');
  writer.sync();
}

void Mrpc::closeServer()
{
  writer.writeTagEnd();
  writer.sync();
}

void Mrpc::flush()
{
  writer.sync();
}

// Server
bool Mrpc::parseServer()
{
  LOG(LM_DEBUG, "parseServer " << int(state));
  if (level() <= 0 and state != fresh and state != closing) {
    writer.writeTagEnd();
    writer.sync();
    state = closing;
    return true;
  }
  switch (state) {
    case fresh:
      // XML-Header
      writer.writeHead();
      //writer.writeTagBegin(L"methodCall");
      writer.writeTagBegin(L"methodResponse");
      state = connectingServer;
          __attribute__ ((fallthrough));
    case connectingServer:
      parse();
      LOG(LM_DEBUG, "pars done " << std::boolalpha << bool(resultObj));
      if (auto *sess = dynamic_cast<MrpcSessionReturnError *>(resultObj.get())) {  // sollte der Server eigentlich nie bekommen
        LOG(LM_ERROR, "SESSIONERROR (ignored) " << sess->error.toStr(mobs::ConvObjToString()));
      } else if (auto *sess = dynamic_cast<MrpcSessionLogin *>(resultObj.get())) {
        LOG(LM_DEBUG, "LOGIN ");
        std::string info;
        std::string key;
        MrpcSessionLogin answer;
        try {
          key = loginReceived(sess->cipher(), info);
          if (not key.empty()) {
            if (not session)
              throw std::runtime_error("session missing");
            if (sess->dontKeep())
              session->sessionReuseTime = 0;
            answer.cipher(generateSessionKey(key));
          }
        } catch (std::exception &e) {
          LOG(LM_ERROR, "ParseServer exception " << e.what());
          info = "login procedure failed";
          key = "";
        }
        if (key.empty()) {
          MrpcSessionReturnError eanswer;
          eanswer.error(info);
          xmlOut(eanswer);
          writer.sync();
          resultObj = nullptr;
          throw std::runtime_error("login failed");
        }
        answer.info(info);
        LOG(LM_DEBUG, "Connection establised ID " << session->sessionId << " " << session->info);
        state = connected;
        xmlOut(answer);
        writer.sync();
      } else if (auto *sess = dynamic_cast<MrpcSessionUse *>(resultObj.get())) {
        std::string info;
        LOG(LM_INFO, "REUSE " << sess->id());
        if (reconnectReceived(sess->id(), info)) {
          if (not session or not session->sessionId or session->sessionKey.empty()) {
            MrpcSessionReturnError eanswer;
            eanswer.error(STRSTR("PLS_RELOG " << info));
            xmlOut(eanswer); // Achtung: unverschlüsselt
            writer.sync();
            throw std::runtime_error("reconnect: session invalid");
          }
          if (session->keyValidTime > 0 and session->generated + session->keyValidTime < time(nullptr)) {
            MrpcSessionReturnError eanswer;
            eanswer.error(STRSTR("KEY_EXPIRED"));
            xmlOut(eanswer); // Achtung: unverschlüsselt
            writer.sync();
            throw std::runtime_error("reconnect: session key expired");
          }
          LOG(LM_DEBUG, "Connection reestablished ID " << session->sessionId);
          state = connected;
          if (sess->verify()) {
            MrpcSessionTestConnection answer;
            answer.info("hello again");
            sendSingle(answer);
            LOG(LM_DEBUG, "verification sent");
          }
        } else {
          MrpcSessionReturnError eanswer;
          eanswer.error(STRSTR("PLS_RELOG " << info));
          xmlOut(eanswer); // Achtung: unverschlüsselt
          writer.sync();
          if (not sess->verify())
            throw std::runtime_error("reconnect failed");
        }
        resultObj = nullptr;
      } else if (auto *sess = dynamic_cast<MrpcGetPublickey *>(resultObj.get())) {
        std::string key;
        std::string info;
        try {
          getPupKeyReceived(key, info);
        } catch (std::exception &e) {
          info = "action failed";
          key.clear();
        }
        if (key.empty()) {
          MrpcSessionReturnError answer;
          answer.error(info);
          xmlOut(answer);
          writer.sync();
          resultObj = nullptr;
          break;
        }
        MrpcGetPublickey answer;
        answer.pubKey(key);
        answer.info(info);
        xmlOut(answer);
        writer.sync();
      }
      resultObj = nullptr;
      break;
    case readyRead:
      state = connected;
      // fallthrough  // kein break
    case connected:
      parse();
      LOG(LM_DEBUG, "pars done " << std::boolalpha << bool(resultObj));
      if (dynamic_cast<MrpcSessionRefreshKey *>(resultObj.get())) {
        resultObj = nullptr;
        sendNewSessionKey();
      } else if (session->keyValidTime > 0 and session->generated + session->keyValidTime < time(nullptr)) {
        MrpcSessionReturnError eanswer;
        eanswer.error(STRSTR("KEY_EXPIRED"));
        stopEncrypt();
        xmlOut(eanswer); // Achtung: unverschlüsselt
        writer.sync();
        throw std::runtime_error("reconnect: session key expired");
      }
      break;
    case closing:
      return false;
    case connectingClient:
    case reconnectingClient:
    case reconnectingClientTest:
    case getPubKey:
      throw std::runtime_error("error while connecting");
  }
  return state == connected or state == readyRead;
}

void Mrpc::tryReconnect()
{
  if (not session)
    throw std::runtime_error("session missing");
  if (session->sessionId and not session->sessionKey.empty()) {
    // XML-Header
    writer.writeHead();
    writer.writeTagBegin(L"methodCall");
    MrpcSessionUse cmd;
    cmd.id(session->sessionId);
    if (sessionMode != SessionMode::speedup)
      cmd.verify(true);
    xmlOut(cmd);
    writer.sync();
    state = sessionMode == SessionMode::speedup ? reconnectingClient : reconnectingClientTest;
  }
}

bool Mrpc::parseClient()
{
  LOG(LM_DEBUG, "parseClient " << int(state));
  if (level() <= 0 and state != fresh and state != reconnectingClient) {
    session->sessionId = 0;
    THROW("Session ended");
  }
  if (state != readyRead)
    parse();
  if (resultObj and state == reconnectingClient) {
    if (auto *sess = dynamic_cast<MrpcSessionReturnError *>(resultObj.get())) {
      LOG(LM_ERROR, "SESSIONERROR " << sess->error.toStr(mobs::ConvObjToString()));
      session->info = sess->error();
      session->sessionId = 0;
      resultObj = nullptr;
      throw MrpcConnectException(STRSTR("error received: " << session->info));
    }
    state = connected;
  }
  else if (auto *refKey = dynamic_cast<MrpcSessionRefreshKey *>(resultObj.get())) {
    LOG(LM_INFO, "New session key received " << session->sessionId << " " << refKey->info());
    session->sessionKey = refKey->key();
    session->sessionReuseTime = refKey->sessionReuseTime();
    session->keyValidTime = refKey->sessionKeyValidTime();
    session->info = refKey->info();
    session->generated = time(nullptr);
    LOG(LM_DEBUG, "Changed Session Key " << session->sessionId << " " << session->info << " reuse="
                                             << session->sessionReuseTime << " valid=" << session->keyValidTime);
    resultObj = nullptr;
    // readyRead-State verhindern
    if (state == readyRead)
      state = connected;
    else if (state == connected)
      state = reconnectingClient;
  }
  bool ret = state == readyRead;
  if (ret)
    state = connected;
  return ret;
}

// Client
bool Mrpc::waitForConnected(const std::string &keyId, const std::string &software, const std::string &privkey,
                            const std::string &passphrase, std::string &serverkey)
{
  if (not session)
    throw std::runtime_error("session missing");
  switch (state) {
    case fresh:
      LOG(LM_DEBUG, "Start Session " << session->sessionId  << " reuse="
                                           << session->sessionReuseTime << " valid=" << session->keyValidTime);
      if (session->sessionReuseTime > 0 and session->sessionId > 0 and not session->sessionKey.empty() and session->last and
          session->last + session->sessionReuseTime > time(nullptr)) {
        LOG(LM_DEBUG, "Reconnect");
        tryReconnect();
        break;  // Reconnect versuchen
      }
      if (oStr.tellp() == 0) { // wenn schon Output, dann nicht initialisieren
        if (serverkey.empty()) {
          // XML-Header
          writer.writeHead();
          writer.writeTagBegin(L"methodCall");
          MrpcGetPublickey cmd;
          xmlOut(cmd);
          writer.sync();
          state = getPubKey;
          break;
        }
        MrpcSessionLogin msg;
        msg.cipher(mobs::Mrpc::generateLoginInfo(keyId, software, serverkey));
        if (state == fresh) {
          // XML-Header
          writer.writeHead();
          writer.writeTagBegin(L"methodCall");
        }
        if (sessionMode == SessionMode::dontKeep)
          msg.dontKeep(true);
        xmlOut(msg);
        writer.sync();
      }
      state = connectingClient;
      break;
    case connectingClient:
    case reconnectingClientTest:
    case getPubKey:
      parse();
      if (not resultObj)
        break;
      if (dynamic_cast<MrpcSessionTestConnection *>(resultObj.get())) {
        LOG(LM_DEBUG, "Connection verified ID " << session->sessionId << " " << session->info);
        state = connected;
        resultObj = nullptr;
      } else if (auto *sess = dynamic_cast<MrpcSessionReturnError *>(resultObj.get())) {
        LOG(LM_ERROR, "SESSIONERROR " << sess->error());
        if (sessionMode == SessionMode::speedup or sess->error().find(u8"PLS_RELOG") == std::string::npos) {
          session->info = sess->error();
          resultObj = nullptr;
          throw MrpcConnectException(STRSTR("error received: " << session->info));
        }
        resultObj = nullptr;
        MrpcSessionLogin msg;
        msg.cipher(mobs::Mrpc::generateLoginInfo(keyId, software, serverkey));
        xmlOut(msg);
        writer.sync();
        state = connectingClient;
      } else if (auto *sess = dynamic_cast<MrpcSessionLogin *>(resultObj.get())) {
        receiveSessionKey(sess->cipher(), privkey, passphrase);
        state = connected;
        resultObj = nullptr;
      } else if (auto *sess = dynamic_cast<MrpcGetPublickey *>(resultObj.get())) {
        if (serverkey.empty())
          serverkey = sess->pubKey();
        session->info = sess->info();
        state = connectingClient;
        resultObj = nullptr;
        MrpcSessionLogin msg;
        LOG(LM_DEBUG, "received server public key " << serverkey);
        msg.cipher(mobs::Mrpc::generateLoginInfo(keyId, software, serverkey));
        xmlOut(msg);
        writer.sync();
      } else {
        LOG(LM_INFO, "WaitForConnected receive unknown " << resultObj->getObjectName() << ": " << resultObj->to_string());
        resultObj = nullptr;
      }
      break;
    case reconnectingClient:
    case connected:
    case readyRead:
    case closing:
      return true;
    case connectingServer:
      throw std::runtime_error("error while connecting");
  }
  if (state == reconnectingClient and sessionMode == SessionMode::speedup)
    return true;
  return state == connected or state == readyRead;
}

bool Mrpc::isConnected() const
{
  if (state == reconnectingClient and sessionMode == SessionMode::speedup)
    return true;
  return state == connected or state == readyRead;
}

bool Mrpc::clientAboutToRead() const
{
  switch (state) {
    case connectingClient:
    case getPubKey:
    case connected:
    case reconnectingClient:
    case reconnectingClientTest:
    case readyRead:
      return true;
    case fresh:
    case closing:
    case connectingServer:
      break;
  }

  return false;
}

bool Mrpc::serverKeepSession() const
{
  return session and session->sessionReuseTime > 0;
}


} // mobs