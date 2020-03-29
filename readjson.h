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

/** \file readjson.h
\brief optionale Klasse um Objekte aus einen JSON-String auszulesen, verwendet rapidjson */


#include "objgen.h"

namespace mobs {

class JsonReadData;
/// JSON-Parser für Objekte (verwendet rapidjson)
class JsonRead {
  public:
  /// Pareser mit JASO-String initialisieren
    JsonRead(const std::string &input);
    ~JsonRead();
  /// Objekt extrahieren
    void fill(ObjectBase &obj);
  private:
    JsonReadData *data;
    void parse();

};

}
