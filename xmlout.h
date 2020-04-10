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
  /// @param indent erzeugt Einrückungen, wenn \c true
  XmlOut(bool indent = true);
  ~XmlOut();
  /// \private
  virtual void doObjBeg(ObjTravConst &ot, const ObjectBase &obj);
  /// \private
  virtual void doObjEnd(ObjTravConst &ot, const ObjectBase &obj);
  /// \private
  virtual void doArrayBeg(ObjTravConst &ot, const MemBaseVector &vec);
  /// \private
  virtual void doArrayEnd(ObjTravConst &ot, const MemBaseVector &vec);
  /// \private
  virtual void doMem(ObjTravConst &ot, const MemberBase &mem);
  /// liefert Ergebnis-XML
  std::string getString();
  /// löscht Buffer
  void clear();
  /// Angabe eines Start-Tokens falls mehrere Objekte einer Liste gelesen werden
  void startList(std::string name);
private:
  XmlOutData *data;

};

/// Ausgabe eines Objektes im  XML-Format ohne whitespace
std::string to_xml(const ObjectBase &obj);

}

