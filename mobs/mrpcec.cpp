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

#include <unistd.h>
#include "mrpcec.h"
#include "xmlwriter.h"
#include "xmlout.h"
#include "aes.h"
#include "crypt.h"
#include "digest.h"
#include "mrpcsession.h"


namespace mobs {

namespace {
class MrpcSessionLoginResult : virtual public mobs::ObjectBase {
public:
  ObjInit(MrpcSessionLoginResult);

  MemVar(u_int, sessId);
  MemVar(u_int, sessionReuseTime, USENULL);
  MemVar(u_int, sessionKeyValidTime, USENULL);
};

class MrpcSessionAuth : virtual public mobs::ObjectBase {
public:
  ObjInit(MrpcSessionAuth);

  MemVar(std::string, keyId);
  MemVar(std::string, login);
  MemVar(std::string, software);
  MemVar(std::string, hostname);
  MemVar(std::vector<u_char>, auth, USENULL);
};


class MrpcGetPublickey : virtual public mobs::ObjectBase {
public:
  ObjInit(MrpcGetPublickey);
  MemVar(std::string, pubkey);
};

class MrpcNewEphemeralKey : virtual public mobs::ObjectBase {
public:
  ObjInit(MrpcNewEphemeralKey);
  MemVar(std::vector<u_char>, key);
};


class MrpcSessionReturnError : virtual public mobs::ObjectBase {
public:
  ObjInit(MrpcSessionReturnError);
  MemVar(std::string, error); // Rückgabe für Fehlermeldungen. Wird nicht verschlüsselt!
};


}

#if 0
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
#endif

void MrpcSession::clear() {
  sessionKey.clear();
  sessionId = 0;
  generated = 0;
  info.clear();
}

unsigned int MrpcSession::keyValid() const {
  if (keyValidTime <= 0)
    return INT_MAX;
  if (not sessionId or sessionKey.empty())
    return 0;
  auto now = time(nullptr);
  if (generated + keyValidTime <= now)
    return 0;
  return generated + keyValidTime - now;
}

bool MrpcSession::expired() const {
  if (not sessionId or sessionKey.empty())
    return true;
  if (keyValidTime <= 0 and sessionReuseTime <= 0)
    return false;
  auto now = time(nullptr);
  if (keyValidTime > 0 and generated + keyValidTime <= now)
    return true;
  if (sessionReuseTime > 0 and last + sessionReuseTime <= now)
    return true;
  return false;
}

bool MrpcSession::keyNeedsRefresh() const {
  return keyValidTime >= 10 and keyValid() <= keyValidTime * 2 / 10;
}

void MrpcEc::encrypt()
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

void MrpcEc::stopEncrypt()
{
  writer.stopEncrypt();
}

void MrpcEc::EncryptionFinished()
{
  LOG(LM_DEBUG, "Encryption finished " << level());
  encrypted = false;
  if (state == connectingServerConfirmed and level() == 2) {
    // hier muss die Verschlüsselung abgeschlossen werden, wenn der client ebenfalls die Verschlüsselung beendet hat;
    // ansonsten kommt der nächste Befehl im selben Crypt-Element
    LOG(LM_INFO, "connection established with wait");
    stopEncrypt();
    flush();
    state = readyRead;
  }
  if (state == connected and level() == 2)
    state = readyRead;
  // weiteres parsen anhalten
  stop();
}

void MrpcEc::Encrypt(const std::string &algorithm, const std::string &keyName, const std::string &cipher,
                    CryptBufBase *&cryptBufp)
{
  LOG(LM_DEBUG, "Encryption " << algorithm << " keyName " << keyName << " cipher " << cipher);
  if (not session)
    throw std::runtime_error("session missing");
  if (state == connectingServerConfirmed) {
    LOG(LM_INFO, "connection established without wait");
    state = readyRead;
  }
  else if (state == connected)
    state = readyRead;
  encrypted = true;
  if (algorithm == "aes-256-cbc") {
    if (cipher.empty() and not session->sessionKey.empty()) // TODO state, da nur für Server
      cryptBufp = new mobs::CryptBufAes(session->sessionKey);
    else {
      std::vector<u_char> cip;
      mobs::from_string_base64(cipher, cip);
      try {
        loginReceived(cip, keyName);
      }
      catch (std::exception &e) {
        LOG(LM_ERROR, "login failed: " << e.what());
        session->sessionKey.clear();
      }
      if (session->sessionKey.empty()) {
        MrpcSessionReturnError eanswer;
        eanswer.error("login failed");
        xmlOut(eanswer);
        closeServer();
        resultObj = nullptr;
        throw std::runtime_error("login failed");
      }
      if (not session->generated)
        session->generated = time(nullptr);
      //LOG(LM_DEBUG, "New Session key retrieved " << mobs::to_string(session->sessionKey));
      cryptBufp = new mobs::CryptBufAes(session->sessionKey, keyName);
    }
  }
  session->last = time(nullptr);
}

void MrpcEc::filled(mobs::ObjectBase *obj, const std::string &error)
{
  LOG(LM_DEBUG, "filled " << obj->getObjectName() << ": " << obj->to_string(ConvObjToString().exportWoNull()) << (isEncrypted()? " OK":" UNENCRYPTED"));
  if (not error.empty())
    THROW("error in XML stream: " << error);
  if (auto err = dynamic_cast<MrpcSessionReturnError *>(obj)) {
    LOG(LM_ERROR, "Received MrpcSessionReturnError " << err->error());
    if (resultObj) {
      LOG(LM_WARNING, "overwrite result");
      resultObj = nullptr;
    }
  }
  if (resultObj)
    THROW("result object bereits vorhanden: " << resultObj->getObjectName());
  resultObj = std::unique_ptr<mobs::ObjectBase>(obj);
  // parsen anhalten
  stop();
}

void MrpcEc::StartTag(const std::string &element)
{
  LOG(LM_DEBUG, "start " << element);
  // Wenn passendes Tag gefunden, dann Objekt einlesen
  if (state == connectingServer or state == connectingClient or state == getPubKey) { // ohne Login nur fixe Auswahl
    if (element == "MrpcSessionLoginResult")
      fill(new MrpcSessionLoginResult);
    else if (element == "MrpcSessionAuth")
      fill(new MrpcSessionAuth);
    else if (element == "MrpcSessionReturnError")
      fill(new MrpcSessionReturnError);
    else if (element == "MrpcGetPublickey")
      fill(new MrpcGetPublickey);
    else if ((state != connectingServer or element != "methodCall") and (state != connectingClient or element != "methodResponse") and
             (state != getPubKey or element != "methodResponse")) {
      MrpcSessionReturnError eanswer;
      eanswer.error("login failed");
      xmlOut(eanswer);
      closeServer();
      THROW("unknown element " << element << " in initialization");
    }
  } else if (element == "MrpcSessionReturnError") {
    fill(new MrpcSessionReturnError);
  } else if (element == "MrpcNewEphemeralKey") {
    fill(new MrpcNewEphemeralKey);
  } else {
    auto o = ObjectBase::createObj(element);
    if (o)
      fill(o);
    else
      LOG(LM_WARNING, "unknown element " << element);
  }
}

void MrpcEc::EndTag(const std::string &element)
{
  LOG(LM_DEBUG, "end " << element << " lev " << level());
  if (state == connected and not encrypted and level() == 2)
    state = readyRead;
}

bool MrpcEc::inByteStreamAvail() {
  // es muss mindestens ein Zeichen im Buffer sein, für den Delimiter
  auto s = streambufI.getIstream().rdbuf()->in_avail();
  return s > 0;
}

std::istream &MrpcEc::inByteStream(size_t sz)
{
  LOG(LM_DEBUG, "Mrpc::inByteStream " << mobs::CryptBufAes::aes_size(sz));
  if (not session)
    throw std::runtime_error("session missing");
  return byteStream(mobs::CryptBufAes::aes_size(sz), new mobs::CryptBufAes(session->sessionKey));
}

std::ostream &MrpcEc::outByteStream()
{
  std::vector<u_char> iv;
  iv.resize(mobs::CryptBufAes::iv_size());
  mobs::CryptBufAes::getRand(iv);
  if (not session)
    throw std::runtime_error("session missing");
  return writer.byteStream("\200", new mobs::CryptBufAes(session->sessionKey, iv, "", true));
}

std::streamsize MrpcEc::closeOutByteStream()
{
  return writer.closeByteStream();
}


MrpcEc::MrpcEc(std::istream &inStr, std::ostream &outStr, MrpcSession *mrpcSession, bool nonBlocking) :
    XmlReader(iStr), streambufI(inStr), streambufO(outStr),
    iStr(&streambufI), oStr(&streambufO),
    writer(oStr, mobs::XmlWriter::CS_utf8, false),
    session(mrpcSession)
{
  readTillEof(false);
  readNonBlocking(nonBlocking);
  oStr.exceptions(std::wostream::failbit | std::wostream::badbit);
  iStr.exceptions(std::wostream::failbit | std::wostream::badbit);
}




void MrpcEc::xmlOut(const ObjectBase &obj)
{
  XmlOut xo(&writer, mobs::ConvObjToString().exportXml().exportWoNull());
  obj.traverse(xo);
  //writer.putc('\n');
}

void MrpcEc::sendSingle(const ObjectBase &obj)
{
  encrypt();
  xmlOut(obj);
  stopEncrypt();
  //writer.putc('\n');
  flush();
}

void MrpcEc::closeServer()
{
  writer.writeTagEnd();
  flush();
}

void MrpcEc::flush()
{
  writer.sync();
}

// Server
bool MrpcEc::parseServer()
{
  LOG(LM_DEBUG, "parseServer " << int(state));
  if (level() <= 0 and state != fresh and state != closing) {
    writer.writeTagEnd();
    flush();
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
      setMaxElementSize(4096);
      parse();
      LOG(LM_DEBUG, "pars done " << std::boolalpha << bool(resultObj));
      if (auto *sess = dynamic_cast<MrpcSessionReturnError *>(resultObj.get())) {  // sollte der Server eigentlich nie bekommen
        LOG(LM_ERROR, "SESSIONERROR (ignored) " << sess->error.toStr(mobs::ConvObjToString()));
      }
      else if (auto *sess = dynamic_cast<MrpcSessionAuth *>(resultObj.get())) {
        session->info = STRSTR(sess->login() << '@' << sess->hostname() << '/' << sess->software());
        session->keyName = sess->keyId();
        LOG(LM_DEBUG, "Connection establised ID " << session->sessionId << " " << session->info);
        std::string pubKey = getSenderPublicKey(sess->keyId());
        if (sess->auth().empty() or
            pubKey.empty() or
            not digestVerify(session->sessionKey, sess->auth(), pubKey)) {
          MrpcSessionReturnError eanswer;
          eanswer.error("auth failed");
          xmlOut(eanswer);
          flush();
          resultObj = nullptr;
          throw std::runtime_error("login failed");
        }
        LOG(LM_DEBUG, "Send MrpcSessionLoginResult");
        encrypt();
        MrpcSessionLoginResult answer;
        answer.sessId(session->sessionId);
        answer.sessionKeyValidTime(session->keyValidTime);
        answer.sessionReuseTime(session->sessionReuseTime);
        xmlOut(answer);
        // ist der State 'connectingServerConfirmed', dann wird der Ausgabestrom geflusht, wenn der Client ebenfalls
        // die Verschlüsselung flusht. (siehe isConnected() im client)
        // Ansonsten werden die Verschlüsselungen nicht unterbrochen
        state = connectingServerConfirmed;
        setMaxElementSize(256*1024*1024);
        // callback
        authenticated(sess->login(), sess->hostname(), sess->software());
      } else if (auto *gp = dynamic_cast<MrpcGetPublickey *>(resultObj.get())) {
        LOG(LM_INFO, "MrpcGetPublickey");
        MrpcGetPublickey answer;
        answer.pubkey(getServerPublicKey());
        if (not answer.pubkey().empty()) {
          xmlOut(answer);
          flush();
        } else {
          MrpcSessionReturnError eanswer;
          eanswer.error("getPubKey failed");
          xmlOut(eanswer);
          flush();
          resultObj = nullptr;
          throw std::runtime_error("getPubKey failed");
        }
      }
      resultObj = nullptr;
      break;
    case readyRead:
      state = connected;
      __attribute__ ((fallthrough));
    case connectingServerConfirmed:
    case connected:
      parse();
      LOG(LM_DEBUG, "pars done " << std::boolalpha << bool(resultObj));
      if (session->keyValidTime > 0 and session->generated + session->keyValidTime < time(nullptr)) {
        MrpcSessionReturnError eanswer;
        eanswer.error(STRSTR("KEY_EXPIRED"));
        stopEncrypt();
        xmlOut(eanswer); // Achtung: unverschlüsselt
        flush();
        throw std::runtime_error("reconnect: session key expired");
      }
      if (auto *ek = dynamic_cast<MrpcNewEphemeralKey *>(resultObj.get())) {
        keyChanged(ek->key(), session->keyName);
        resultObj = nullptr;
      }
      break;
    case closing:
      return false;
    case connectingClient:
    case getPubKey:
      throw std::runtime_error("error while connecting");
  }
  return state == connected or state == readyRead or state == connectingServerConfirmed;
}


bool MrpcEc::parseClient()
{
  LOG(LM_DEBUG, "parseClient " << int(state));
  if (level() <= 0 and state != fresh and state != connectingClient and state != getPubKey) {
    session->sessionId = 0;
    THROW("Session ended");
  }
  if (state != readyRead)
    parse();
  if (resultObj and state == connectingClient) {
    if (auto *sess = dynamic_cast<MrpcSessionLoginResult *>(resultObj.get())) {
      LOG(LM_DEBUG, "Session answer received " << sess->sessId());
      session->sessionId = sess->sessId();
      session->sessionReuseTime = sess->sessionReuseTime();
      session->keyValidTime = sess->sessionKeyValidTime();
      state = connected;
      resultObj = nullptr;
    }
  }
  else if (resultObj and state == getPubKey) {
    if (auto *gp = dynamic_cast<MrpcGetPublickey *>(resultObj.get())) {
      LOG(LM_INFO, "GetPublickey answer received " << gp->pubkey());
      session->publicServerKey = gp->pubkey();
      state = fresh;
      resultObj = nullptr;
      return false;
    }
  }
  else if (auto *err = dynamic_cast<MrpcSessionReturnError *>(resultObj.get())) {
      session->info = err->error();
    THROW("session error received: " << err->error());
  }

  bool ret = state == readyRead;
  state = connected;
  return ret;
}

void MrpcEc::clientRefreshKey(std::string &serverkey) {
  if (not session)
    THROW("no session");
  if (state == fresh) { // ohne Connection einfach nur den Reconnect verhindern
    session->sessionKey.clear();
    return;
  }
  // neuen ephemeren Schlüssel generieren
  std::vector<u_char> secret;
  ecdhGenerate(secret, session->info, serverkey);
  session->generated = time(nullptr);
  // Verschlüsselung mit altem Schlüssel starten
  encrypt();
  // aus shared secret den session-key erzeugen
  hash_value(secret, session->sessionKey, "sha256");
  MrpcNewEphemeralKey newKey;
  std::vector<u_char> v;
  from_string_base64(session->info, v);
  newKey.key(v);
  LOG(LM_INFO, "Sende MrpcNewEphemeralKey");
  xmlOut(newKey);
  // Verschlüsselung beenden, damit neuer Schlüssel wirksam wird
  stopEncrypt();
}

void MrpcEc::startSession(const std::string &keyId, const std::string &software, const std::string &privkey,
                         const std::string &passphrase, std::string &serverkey) {
  if (not session)
    THROW("no session");
  LOG(LM_DEBUG, "Start Session NewMode " << session->sessionId  << " reuse="
                                         << session->sessionReuseTime << " valid=" << session->keyValidTime);

  if (state == fresh and oStr.tellp() == 0) { // wenn schon Output, dann nicht initialisieren
    // XML-Header
    writer.writeHead();
    writer.writeTagBegin(L"methodCall");
  }
  if (state == fresh and not session->info.empty() and session->keyValid() > 0) {
    LOG(LM_DEBUG, "Reuse key unconnected");
  } else {
    // neuen SessionKey und Cipher generieren
    std::vector<u_char> secret;
    ecdhGenerate(secret, session->info, serverkey);
    // aus shared secret den session-key erzeugen
    hash_value(secret, session->sessionKey, "sha256");
    session->generated = time(nullptr);
    if (state == fresh) {
      session->keyName = keyId;
      session->sessionReuseTime = 0;
      session->keyValidTime = 0;
    }
  }
  std::vector<u_char> iv;
  iv.resize(mobs::CryptBufAes::iv_size());
  mobs::CryptBufAes::getRand(iv);
  writer.startEncrypt((new mobs::CryptBufAes(session->sessionKey, iv, "", true))->setRecipientKeyBase64(session->info));
  //LOG(LM_DEBUG, "New Session startet, cipher " << session->info);

  if (state != fresh) // MrpcSessionAuth nur beim Öffnen der Connection senden
    return;
  state = connectingClient;

  MrpcSessionAuth loginData;
  loginData.software(software);
  loginData.hostname(getNodeName());
  loginData.login(getLoginName());
  loginData.keyId(keyId);
  // Zu Bestätigung der Authentizität den Session-Key signieren
  std::vector<u_char> auth;
  digestSign(session->sessionKey, auth, privkey, passphrase);
  loginData.auth(auth);
  LOG(LM_DEBUG, "Send MrpcSessionAuth");
  xmlOut(loginData);
}

void MrpcEc::getPublicKey() {
  if (not session)
    THROW("no session");
  LOG(LM_DEBUG, "getPublicKey");

  if (state == fresh and oStr.tellp() == 0) { // wenn schon Output, dann nicht initialisieren
    // XML-Header
    writer.writeHead();
    writer.writeTagBegin(L"methodCall");
  }
  MrpcGetPublickey obj;
  xmlOut(obj);
  flush();
  state = getPubKey;
}

void MrpcEc::setEcdhSessionKey(const std::vector<u_char> &cipher, const std::string &privKey, const std::string &passwd) {
  if (not session)
    throw std::runtime_error("session missing");
  if (not session->sessionKey.empty()) {
    LOG(LM_DEBUG, "Refresh Key");
    session->generated = 0;
    session->sessionKey.clear();
  }
  // keyId leer, der Sender ist hier noch nicht bekannt; SessionAuth erfolgt später
  // pub-key vom DER ins PEM-Format bringen
  std::string ephemeralKey = mobs::getPublicKey(cipher);
  LOG(LM_DEBUG, "Ephemeral Key " << ephemeralKey);
  std::vector<u_char> secret;
  // aus ephemeral key das shared secret ermitteln
  deriveSharedSecret(secret, ephemeralKey, privKey, passwd);
  // aus shared secret den session-key erzeugen
  hash_value(secret, session->sessionKey, "sha256");
  session->last = time(nullptr);
  if (not session->generated)
    session->generated = session->last;
}


bool MrpcEc::isConnected() const
{
  return state == connected or state == readyRead or state == connectingServerConfirmed;
}

bool MrpcEc::clientAboutToRead() const
{
  switch (state) {
    case connectingClient:
    case getPubKey:
    case connected:
    case readyRead:
      return true;
    case fresh:
    case closing:
    case connectingServer:
    case connectingServerConfirmed:
      break;
  }

  return false;
}

bool MrpcEc::serverKeepSession() const
{
  return session and session->sessionReuseTime > 0;
}


} // mobs