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


#include <pwd.h>
#include <sys/utsname.h>
#include <unistd.h>
#include "mrpc.h"
#include "xmlwriter.h"
#include "xmlout.h"
#include "aes.h"
#include "rsa.h"

namespace mobs {

void Mrpc::encrypt()
{
  std::vector<u_char> iv;
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
  // weiteres parsen anhalten
  stop();
}

void Mrpc::Encrypt(const std::string &algorithm, const std::string &keyName, const std::string &cipher,
                   CryptBufBase *&cryptBufp)
{
  LOG(LM_DEBUG, "Encryption " << algorithm << " keyName " << keyName << " cipher " << cipher);
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
    throw std::runtime_error("result object bereits vorhanden");
  resultObj = std::unique_ptr<mobs::ObjectBase>(obj);
  // parsen anhalten
  stop();
}

void Mrpc::StartTag(const std::string &element)
{
  LOG(LM_DEBUG, "start " << element);
  // Wenn passendes Tag gefunden, dann Objekt einlesen
  auto o = ObjectBase::createObj(element);
  if (o)
    fill(o);
}

void Mrpc::EndTag(const std::string &element)
{
  LOG(LM_DEBUG, "end " << element << " lev " << level());
}

std::istream &Mrpc::inByteStream(size_t sz)
{
  LOG(LM_DEBUG, "Mrpc::inByteStream " << mobs::CryptBufAes::aes_size(sz));
  return byteStream(mobs::CryptBufAes::aes_size(sz), new mobs::CryptBufAes(session->sessionKey));
}

std::ostream &Mrpc::outByteStream()
{
  std::vector<u_char> iv;
  iv.resize(mobs::CryptBufAes::iv_size());
  mobs::CryptBufAes::getRand(iv);
  return writer.byteStream("\200", new mobs::CryptBufAes(session->sessionKey, iv, "", true));
}

void Mrpc::closeOutByteStream()
{
  writer.closeByteStream();
}

namespace {
class MrpcSessionLoginResult : virtual public mobs::ObjectBase {
public:
  ObjInit(MrpcSessionLoginResult);

  MemVar(std::vector<u_char>, key);
  MemVar(u_int, sessId);
};

class MrpcSessionLoginData : virtual public mobs::ObjectBase {
public:
  ObjInit(MrpcSessionLoginData);

  MemVar(std::string, login);
  MemVar(std::string, software);
  MemVar(std::string, hostname);
  MemVar(std::string, key);
};
}


Mrpc::Mrpc(std::istream &inStr, std::ostream &outStr, MrpcSession *mrpcSession, bool nonBlocking) :
        XmlReader(iStr), streambufI(inStr), streambufO(outStr),
        iStr(&streambufI), oStr(&streambufO),
        writer(oStr, mobs::XmlWriter::CS_utf8, true),
        session(mrpcSession)
{
  readTillEof(false);
  readNonBlocking(nonBlocking);
  oStr.exceptions(std::wostream::failbit | std::wostream::badbit);
}


void Mrpc::receiveSessionKey(const std::vector<u_char> &cipher, const std::string &privkey, const std::string &passwd)
{
  std::vector<u_char> sessInhalt;
  mobs::decryptPrivateRsa(cipher, sessInhalt, privkey, passwd);
  std::string buf((char *)&sessInhalt[0], sessInhalt.size());
  MrpcSessionLoginResult result;
  LOG(LM_INFO, "LRES " << buf);
  mobs::string2Obj(buf, result, mobs::ConvObjFromStr());
  session->sessionKey = result.key();
  session->sessionId = result.sessId();
}

std::vector<u_char> Mrpc::generateSessionKey(MrpcSession &session, const std::string &clientkey)
{
  std::vector<u_char> cipher;
  MrpcSessionLoginResult result;
  result.sessId(session.sessionId);
  session.sessionKey.resize(mobs::CryptBufAes::key_size());
  mobs::CryptBufAes::getRand(session.sessionKey);
  result.key(session.sessionKey);

  std::string buffer = result.to_string(mobs::ConvObjToString().exportJson().noIndent());
  std::vector<u_char> inhalt;
  copy(buffer.begin(), buffer.end(), back_inserter(inhalt));
  encryptPublicRsa(inhalt, cipher, clientkey);
  return cipher;
}

std::vector<u_char> Mrpc::generateLoginInfo(const std::string &keyId, const std::string &software,
                                            const std::string &serverkey)
{
  static std::string nodename;
  if (nodename.empty())
  {
    struct utsname uts;
    if (::uname(&uts) == -1)
      nodename = "none";
    else
      nodename = uts.nodename;
  }

  MrpcSessionLoginData loginData;
  //data.login(fingerprint);
  loginData.software(software);
  loginData.hostname(nodename);
  loginData.key(keyId);
  struct passwd *pw = ::getpwuid(geteuid());
  if (pw == nullptr)
    throw std::runtime_error("can't et pwd entry");
  loginData.login(pw->pw_name);
  std::string buffer = loginData.to_string(mobs::ConvObjToString().exportJson().noIndent());
  std::vector<u_char> inhalt;
  copy(buffer.begin(), buffer.end(), back_inserter(inhalt));
  std::vector<u_char> cipher;
  mobs::encryptPublicRsa(inhalt, cipher, serverkey);
  return cipher;
}

void Mrpc::xmlOut(const ObjectBase &obj)
{
  XmlOut xo(&writer, mobs::ConvObjToString().exportXml());
  obj.traverse(xo);
}

void Mrpc::sendSingle(const ObjectBase &obj)
{
  encrypt();
  xmlOut(obj);
  stopEncrypt();
  writer.sync();
}


} // mobs