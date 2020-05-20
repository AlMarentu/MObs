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

/** \file xmlout.h
\brief Klasse zur Ausgabe von Mobs-Objekte im XML-Format */

#ifndef MOBS_XMLOUT_H
#define MOBS_XMLOUT_H

#include <utility>

#include "objgen.h"

namespace mobs {

class XmlWriter;

/// KLasse zum Erzeugen von XML aus Objekten. benötigt einen XML-Writer
class XmlOut  : virtual public ObjTravConst {
public:
  /// Konstruktor
  /// @param xwr XML-Writer-Objekt
  /// @param cth Konvertierung-Hinweis
  XmlOut(XmlWriter *xwr, ConvObjToString cth) : cth(std::move(cth)), data(xwr) { };
  /// \private
  bool doObjBeg(const ObjectBase &obj) override;
  /// \private
  void doObjEnd(const ObjectBase &obj) override;
  /// \private
  bool doArrayBeg(const MemBaseVector &vec) override;
  /// \private
  void doArrayEnd(const MemBaseVector &vec) override;
  /// \private
  void doMem(const MemberBase &mem) override;

protected:
  /// \private
  const ConvObjToString cth;
private:
  XmlWriter *data;
  std::stack<std::wstring> elements;
};



}

#endif
