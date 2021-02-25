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

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedGlobalDeclarationInspection"
namespace mobs {

class CryptBufBase;
class XmlReadData;
/// KLasse um Objekte aus XML einzulesen
/// \throw runtime_error wenn in der Struktur des XML ein Fehler ist
class XmlReader {
public:
  /// Konstruktor mit Übergabe eines \c std::string
  /// @param input XML
  /// @param c conversion-hints
  /// @param charsetUnknown default ist der übergebene String in UTF-8 codiert, wird hier false übergeben, wird der Zeichensatz ermittelt (UTF-8, ISO-8869-1, -9, -15)
  explicit XmlReader(const std::string &input, const ConvObjFromStr &c = ConvObjFromStr(), bool charsetUnknown = false);
  /// Konstruktor mit XML und conversion-hints initialisieren
  explicit XmlReader(const std::wstring &input, const ConvObjFromStr &c = ConvObjFromStr());
  /// Konstruktor mit XML und conversion-hints initialisieren
  explicit XmlReader(std::wistream &str, const ConvObjFromStr &c = ConvObjFromStr());
  ~XmlReader();

  /// Callback für Null-Tag
  virtual void NullTag(const std::string &element) { EndTag(element); }
  /// Callback für Attribut
  virtual void Attribute(const std::string &element, const std::string &attribut, const std::wstring &value) { }
  /// Callback für Werte
  virtual void Value(const std::wstring &value) { }
  /// Callback für Cdata-Element
  virtual void Cdata(const std::wstring &value) { Value(value); }
  /** \brief Callback-Function: Ein CDATA-Elemet mit base64 codiertem Inhalt
   
      nur, wenn setBase64(true) gesetzt wurde
   @param base64 Inhalt des base64 codierden Wertes
   */
  virtual void Base64(const std::vector<u_char> &base64) { }
  /// Callback für Start-Tag
  virtual void StartTag(const std::string &element) { }
  /// Callback für Ende-Tag
  virtual void EndTag(const std::string &element) { }
  /// Callback für gelesenes Objekt
  /// @param obj Zeiger auf das mit \c fill übergebene Objekt
  /// @param error ist bei Fehler gefüllt, ansonsten leer
  virtual void filled(ObjectBase *obj, const std::string &error) = 0;
  /** \brief Callback-Funktion: Ein Element "EncryptedData" wurde gefunden und ein decrypt-Modul wird benötigt
   *
   * Es wird  "https://www.w3.org/2001/04/xmlenc#Element" unterstützt
   * @param algorithm Algorithmus um xmlns bereinigt zB.: aes-256-cbc, oder "https://www.w3.org/2001/04/xmlenc#aes-256-cbc"/
   * @param keyName KeyInfo-Element
   * @param cryptBufp ein mit new erzeugtes Encryption-Module; wird automatisch freigegeben
   */
  virtual void Encrypt(const std::string &algorithm, const std::string &keyName, const std::string &cipher, mobs::CryptBufBase *&cryptBufp) { }

  /// Encryption-Element abgeschlossen
  virtual void EncryptionFinished() { }

    /// setzte ein XML-Prefix
  void setPrefix(const std::string &pf);
  /// entferne das Prefix
  /// \throw runtime_error falls das prefix nicht übereinstimmt
  std::string elementRemovePrefix(const std::string &element) const;
  /// Aktiviere automatische base64 erkennung
  /// \see Base64
  void setBase64(bool b);
  /// Einstellung: Lese bis EOF, ansonsten stoppe beim letzten Ene-Tag
  void readTillEof(bool s);
  /// ist beim Parsen das Ende der Datei erreicht
  bool eof() const;
  /// ist beim Parsen das letzte Tag erreicht (level() == 0)
  bool eot() const;
  /// Parse-Level - root-element ist level 1
  size_t level() const;
  /// verlasse beim nächsten End-Tag den parser
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
  void StartTag(const std::string &element) override {
    if (element == "root") {
      fill(&object);
      done = true;
    }
  }
  /// \private
  void filled(ObjectBase *obj, const std::string &error) override {
    if (not error.empty())
      throw std::runtime_error(error);
  }
  /// wurde überhaupt eine Wurzel gefunden
  bool found() const { return done; }
private:
  ObjectBase &object;
  bool done = false;
};

}

#pragma clang diagnostic pop