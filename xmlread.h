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
\brief  Klasse um Objekte aus einen XML-String/File auszulesen */

#include "objgen.h"

namespace mobs {

class XmlReadData;
/// KLasse um Objekte aus XML einzulesen
/// \throw runtime_error wenn in der Struktur des XML ein Fehler ist
class XmlReader {
public:
  /// Konstruktor mit XML und conversion-hints initialisieren
  XmlReader(const std::string &input, const ConvObjFromStr &c = ConvObjFromStr());
  /// Konstruktor mit XML und conversion-hints initialisieren
  XmlReader(const std::wstring &input, const ConvObjFromStr &c = ConvObjFromStr());
  /// Konstruktor mit XML und conversion-hints initialisieren
  XmlReader(std::wistream &str, const ConvObjFromStr &c = ConvObjFromStr());
  ~XmlReader();

  /// Callback für Null-Tag
  virtual void NullTag(const std::string &element) { EndTag(element); }
  /// Callback für Attribut
  virtual void Attribute(const std::string &element, const std::string &attribut, const std::wstring &value) { }
  /// Callback für Werte
  virtual void Value(const std::wstring &value) { }
  /// Callback für Cdata-Element
  virtual void Cdata(const std::wstring &value) { Value(value); }
  /// Callback für Start-Tag
  virtual void StartTag(const std::string &element) { }
  /// Callback für Ende-Tag
  virtual void EndTag(const std::string &element) { }
  /// Callback für gelesenes Objekt
  /// @param obj Zeiger auf das mit \c fill übergebene Objekt
  /// @param error ist bei Fehler gefüllt, ansonsten leer
  virtual void filled(ObjectBase *obj, const std::string &error) = 0;

  /// ist beim Parsen das Ende erreicht
  bool eof() const;
  /// verlasse bein nächsten End-Tag den parser
  void stop();
  /// parse den Input (weiter)
  void parse();
  /// Objekt aus Daten füllen
  void fill(ObjectBase *obj);
  private:
    XmlReadData *data;
//  XmlReadData(const std::string &input, const ConvObjFromStr &c) : XmlParserW(str), str(to_wstring(input)) { cfs = c; };

};

/// Liest ein Objekt aus einem XML-String
class XmlRead : public XmlReader {
public:
  /// Alles initialisieren
  XmlRead(const std::string &str, ObjectBase &obj, const ConvObjFromStr &c) : XmlReader(str, c), object(obj) { }
  /// \private
  virtual void StartTag(const std::string &element) {
    if (element == "root") {
      fill(&object);
      done = true;
    }
  }
  /// \private
  virtual void filled(ObjectBase *obj, const std::string &error) {
    if (not error.empty())
      throw std::runtime_error(error);
  }
  /// wurde überhaupt eine Wurzel gefunden
  bool found() { return done; }
private:
  ObjectBase &object;
  bool done = false;
};

}
