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

/** \file mrpc.h
\brief Framework für Client-Server Modul  */

#ifndef MOBS_MRPC_H
#define MOBS_MRPC_H

class virtuell;

#include "xmlparser.h"
#include "xmlread.h"
#include "xmlwriter.h"
#include "mrpcsession.h"
#include <iostream>

namespace mobs {


class Mrpc : public XmlReader {
public:
  Mrpc(std::istream &inStr, std::ostream &outStr, MrpcSession *mrpcSession, bool nonBlocking);
  ~Mrpc() override = default;

  void sendSingle(const ObjectBase &obj);
  void encrypt();
  void stopEncrypt();
  std::istream &inByteStream(size_t sz);
  std::ostream &outByteStream();
  void closeOutByteStream();


  void StartTag(const std::string &element) override;
  void EndTag(const std::string &element) override;
  void Encrypt(const std::string &algorithm, const std::string &keyName, const std::string &cipher, CryptBufBase *&cryptBufp) override;
  void EncryptionFinished() override;
  void filled(mobs::ObjectBase *obj, const std::string &error) override;

  // nach xmlOut writer.sync()
  void xmlOut(const mobs::ObjectBase &obj);
  bool isEncrypted() const { return  encrypted; }
  void receiveSessionKey(const std::vector<u_char> &cipher, const std::string &privkey, const std::string &pass);
  /// erzeugt Sessionkey, übernimmt sessionId; returniert cipher für receiveSessionKey()
  static std::vector<u_char> generateSessionKey(MrpcSession &session, const std::string &clientkey);
  static std::vector<u_char> generateLoginInfo(const std::string &keyId,
                                               const std::string &software, const std::string &serverkey);

  mobs::CryptIstrBuf streambufI;
  mobs::CryptOstrBuf streambufO;
  std::wistream iStr;
  std::wostream oStr;
  XmlWriter writer;
  MrpcSession *session = nullptr;
  std::unique_ptr<mobs::ObjectBase> resultObj;

private:;
  bool encrypted = false;

};

} // mobs

#endif //MOBS_MRPC_H
