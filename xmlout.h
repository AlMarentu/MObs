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
\brief optionale Klasse um Objekte in einen XML-String umzuwandeln */

#include "objgen.h"

namespace mobs {

class XmlOutData;
/// KLasse zum Erzeugen von XML aud Objekten
class XmlOut : virtual public ObjTravConst {
public:
  /// Konstruktor
  /// @param cth Konvertierungs-Hinweis
  XmlOut(ConvObjToString cth);
  ~XmlOut();
  /// \private
  virtual bool doObjBeg(const ObjectBase &obj);
  /// \private
  virtual void doObjEnd(const ObjectBase &obj);
  /// \private
  virtual bool doArrayBeg(const MemBaseVector &vec);
  /// \private
  virtual void doArrayEnd(const MemBaseVector &vec);
  /// \private
  virtual void doMem(const MemberBase &mem);
  /// liefert Ergebnis-XML
  std::string getString();
  /// löscht Buffer
  void clear();
  /// Angabe eines Start-Tokens falls mehrere Objekte einer Liste gelesen werden
  void startList(std::string name);
private:
  XmlOutData *data;

};


}

