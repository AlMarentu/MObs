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

/** \file xmlread.h
\brief optionale Klasse um Objekte aus einen XML-String auszulesen */

#include "objgen.h"

namespace mobs {

class XmlReadData;
/// KLasse um Objekte aus XML einzulesen
class XmlRead {
  public:
  /// Konstruktor mit XML initialisieren
    XmlRead(const std::string &input);
    ~XmlRead();
  /// Objekt aus Daten füllen
    void fill(ObjectBase &obj);
  private:
    XmlReadData *data;
    void parse();

};

}
