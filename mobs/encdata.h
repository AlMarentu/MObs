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

/** \file encdata.h
\brief  Objekt-Definitionen für eine http://www.w3.org/2000/09/xmldsig#KeyInfo
*/

#ifndef MOBS_ENCDATA_H
#define MOBS_ENCDATA_H

#include "objgen.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedGlobalDeclarationInspection"
namespace mobs {

class Cipher : virtual public mobs::ObjectBase
{
public:
  ObjInit(Cipher);
  MemVar(std::string, CipherValue, NAMESPACE(xenc));
};

class Method : virtual public mobs::ObjectBase
{
public:
  ObjInit(Method);
  MemVar(std::string, Algorithm, XMLATTR);
};

class ConcatHKDF : virtual public mobs::ObjectBase
{
public:
  ObjInit(ConcatHKDF);
  MemObj(Method, PRF, NAMESPACE(dsig-more));
  MemVar(std::string, Salt, USENULL, NAMESPACE(dsig-more));
  MemVar(std::string, Info, USENULL, NAMESPACE(dsig-more));
  MemVar(int, KeyLength, NAMESPACE(dsig-more));
  void init() override {
    addNamespace("dsig-more", "http://www.w3.org/2021/04/xmldsig-more#");
  }
};

class ConcatKDF : virtual public mobs::ObjectBase
{
public:
  ObjInit(ConcatKDF);
  MemVar(std::string, AlgorithmID, XMLATTR);
  MemVar(std::string, PartyUInfo, XMLATTR);
  MemVar(std::string, PartyVInfo, XMLATTR);
  MemObj(Method, DigestMethod, NAMESPACE(ds));
};

class KeyDerivation : virtual public mobs::ObjectBase
{
public:
  ObjInit(KeyDerivation);
  MemVar(std::string, Algorithm, XMLATTR);
  MemObj(ConcatKDF, ConcatKDFParams, USENULL, NAMESPACE(xenc11));
  MemObj(ConcatHKDF, HKDFParams, NAMESPACE(dsig-more));
};

class KeyNC : virtual public mobs::ObjectBase
{
public:
  ObjInit(KeyNC);
  MemVar(std::string, URI, XMLATTR);
};

class KeyEC : virtual public mobs::ObjectBase
{
public:
  ObjInit(KeyEC);
  MemObj(KeyNC, NameCurve, NAMESPACE(dsig11));
  MemVar(std::string, PublicKey, NAMESPACE(dsig11));
  void init() override {
    addNamespace("dsig11", "http://www.w3.org/2009/xmldsig11#");
  }
};

class EcKey : virtual public mobs::ObjectBase
{
public:
  ObjInit(EcKey);
  MemObj(KeyEC, ECKeyValue, NAMESPACE(dsig11));
};

class X509D : virtual public mobs::ObjectBase
{
public:
   ObjInit(X509D);
   MemVar(std::string, X509SKI, NAMESPACE(ds));
};

class OrigRcpt : virtual public mobs::ObjectBase
{
public:
  ObjInit(OrigRcpt);
  MemObj(EcKey, KeyValue, USENULL, NAMESPACE(ds));
  MemObj(X509D, X509Data, USENULL, NAMESPACE(ds));
  MemVar(std::string, KeyName, USENULL, NAMESPACE(ds));

};


//  siehe auch HKDF [RFC5869]

class Agreement : virtual public mobs::ObjectBase
{
public:
  ObjInit(Agreement);
  MemVar(std::string, Algorithm, XMLATTR);
  MemObj(KeyDerivation, KeyDerivationMethod, NAMESPACE(xenc11));
  MemObj(OrigRcpt, OriginatorKeyInfo, NAMESPACE(xenc));
  MemObj(OrigRcpt, RecipientKeyInfo, NAMESPACE(xenc));
};

class KeyInfo2 : virtual public mobs::ObjectBase {
public:
  ObjInit(KeyInfo2);
  MemObj(Agreement, AgreementMethod, NAMESPACE(xenc));
};

class CryptedKey : virtual public mobs::ObjectBase
{
public:
  ObjInit(CryptedKey);
  MemObj(Method, EncryptionMethod, NAMESPACE(xenc));
  MemObj(KeyInfo2, KeyInfo, NAMESPACE(ds));
  MemObj(Cipher, CipherData, NAMESPACE(xenc));
};

class KeyInfo : virtual public mobs::ObjectBase
{
public:
  ObjInit(KeyInfo, NAMESPACE(ds), USEOBJTYPE);
  MemVar(std::string, KeyName, USENULL, NAMESPACE(ds));
  MemObj(Cipher, CipherData, USENULL, NAMESPACE(xenc));
  //MemObj(CryptedKey, EncryptedKey, USENULL, NAMESPACE(xenc));
  MemObj(Agreement, AgreementMethod, USENULL, NAMESPACE(xenc));
  void init() override {
    addNamespace("ds", "http://www.w3.org/2000/09/xmldsig#");
    addNamespace("xenc11", "http://www.w3.org/2009/xmlenc11#");
  }
};

}

#endif //MOBS_ENCDATA_H