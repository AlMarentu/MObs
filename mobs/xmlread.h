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

/** \file xmlread.h
\brief  Klasse um Objekte aus einen XML-String/File auszulesen */

#ifndef MOBS_XMLREAD_H
#define MOBS_XMLREAD_H

#include "objgen.h"
#include <memory>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnusedGlobalDeclarationInspection"
namespace mobs {


class Cipher : virtual public mobs::ObjectBase
{
public:
  ObjInit(Cipher);
  MemVar(std::string, CipherValue);
};

class KeyInfo : virtual public mobs::ObjectBase
{
public:
  ObjInit(KeyInfo);
  MemVar(std::string, KeyName, USENULL);
  MemObj(Cipher, CipherData, USENULL);
};



class CryptBufBase;
class XmlReadData;
/** \brief KLasse um Objekte aus XML einzulesen
 *
 */
class XmlReader {
public:
  /** \brief Konstruktor mit Übergabe eines \c std::string
   *
   * @param input XML
   * @param c conversion-hints
   * @param charsetUnknown default ist der übergebene String in UTF-8 codiert, wird hier false übergeben, wird der Zeichensatz ermittelt (UTF-8, ISO-8869-1, -9, -15)
   * \throw runtime_error wenn in der Struktur des XML ein Fehler ist
   */
  explicit XmlReader(const std::string &input, const ConvObjFromStr &c = ConvObjFromStr(), bool charsetUnknown = false);
  /** \brief Konstruktor mit Übergabe eines \c std::wstring
   *
   * @param input XML
   * @param c conversion-hints
   * \throw runtime_error wenn in der Struktur des XML ein Fehler ist
   */
  explicit XmlReader(const std::wstring &input, const ConvObjFromStr &c = ConvObjFromStr());
  /** \brief Konstruktor mit Übergabe eines \c std::wistream
   *
   * @param str XML
   * @param c conversion-hints
   * \throw runtime_error wenn in der Struktur des XML ein Fehler ist
   */
  explicit XmlReader(std::wistream &str, const ConvObjFromStr &c = ConvObjFromStr());
  virtual ~XmlReader();

  /// Callback für Null-Tag
  virtual void NullTag(const std::string &element) { EndTag(element); }
  /// Callback für Attribut
  virtual void Attribute(const std::string &element, const std::string &attribut, const std::wstring &value) { }
  /// Callback für Werte
  virtual void Value(const std::wstring &value) { }
  /** \brief Callback-Function: Ein CDATA-Element mit base64 codiertem Inhalt

      nur, wenn setBase64(true) gesetzt wurde
   @param base64 Inhalt des base64 codierten Wertes
   */
  virtual void Base64(const std::vector<u_char> &base64) { }
  /// Callback für Start-Tag
  virtual void StartTag(const std::string &element) { }
  /// Callback für Ende-Tag
  virtual void EndTag(const std::string &element) { }
  /// Callback für ProcessingInstruction
  virtual void ProcessingInstruction(const std::string &element, const std::string &attribut, const std::wstring &value) { }
  /// Callback für gelesenes Objekt
  /// @param obj Zeiger auf das mit \c fill übergebene Objekt
  /// @param error ist bei Fehler gefüllt, ansonsten leer
  virtual void filled(ObjectBase *obj, const std::string &error) = 0;
  /** \brief Callback-Funktion: Ein Element "EncryptedData" wurde gefunden und ein decrypt-Modul wird benötigt
   *
   * Es wird  "https://www.w3.org/2001/04/xmlenc#Element" unterstützt
   * @param algorithm Algorithmus um xmlns bereinigt zB.: aes-256-cbc, oder "https://www.w3.org/2001/04/xmlenc#aes-256-cbc"/
   * @param keyName KeyInfo-Element
   * @param cipher Schlüssel-Element
   * @param cryptBufp ein mit new erzeugtes Encryption-Module; wird automatisch freigegeben
   */
  virtual void Encrypt(const std::string &algorithm, const std::string &keyName, const std::string &cipher, mobs::CryptBufBase *&cryptBufp) { }
  /** \brief Callback-Funktion: Ein Element "EncryptedData" wurde gefunden und ein decrypt-Modul wird benötigt
   *
   * Es wird  "https://www.w3.org/2001/04/xmlenc#Element" unterstützt.
   * Bei mehreren Recipients muss nur bei einer Id eine Entschlüsselungs-Klasse zurückgeliefert werden.
   * Bei unbekannten Recipients kann nullptr zurückgegeben werden.
   * @param algorithm Algorithmus um xmlns bereinigt zB.: aes-256-cbc, oder "https://www.w3.org/2001/04/xmlenc#aes-256-cbc"/
   * @param keyInfo KeyInfo-Element
   * @returns cryptBufp ein mit new erzeugtes Encryption-Module; wird automatisch freigegeben oder nullptr, wenn unbekannt
   */
  virtual void Encrypt(const std::string &algorithm, const ObjectBase *keyInfo, mobs::CryptBufBase *&cryptBufp) {
    if (auto ki = dynamic_cast<const mobs::KeyInfo *>(keyInfo))
      Encrypt(algorithm, ki->KeyName(), ki->CipherData.CipherValue(), cryptBufp);
  }

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
  /// non blocking parsen
  void readNonBlocking(bool s);
  /// ist beim Parsen das Ende der Datei erreicht
  bool eof() const;
  /// ist beim Parsen das letzte Tag erreicht (level() == 0)
  bool eot() const;
  /// Parse-Level - root-element ist level 1
  size_t level() const;
  /// liefert aktuellen Namespace
  std::string currentXmlns() const;
  /// verlasse beim nächsten End-Tag den parser
  void stop();
  /// parse den Input (weiter); Rückgabe true, wenn Warten auf Daten
  bool parse();
  /// Objekt aus Daten füllen
  void fill(ObjectBase *obj);
  /// Referenz auf verwendeten input stream
  std::wistream &getIstr();
  /// Lese binäre Daten aus dem input stream, bei bedarf mit Verschlüsselung
  std::istream &byteStream(size_t len, CryptBufBase *cbbp = nullptr);
  /// setze maximale Elementgröße
  void setMaxElementSize(size_t s);
  /// ist der Stream verschlüsselt (läuft über CryptBuffer)
  bool encrypted() const;

private:
  std::unique_ptr<XmlReadData> data;
//  XmlReadData(const std::string &input, const ConvObjFromStr &c) : XmlParserW(str), str(to_wstring(input)) { cfs = c; };

};

/// Liest ein Objekt aus einem XML-String
class XmlRead : public XmlReader {
public:
  /// Alles initialisieren
  XmlRead(const std::string &str, ObjectBase &obj, const ConvObjFromStr &c) :
          XmlReader(str, c), object(obj), decrypFun(c.getDecFun()) { }
  /// \private
  void StartTag(const std::string &element) override {
    if (element == "root" or (object.hasFeature(OTypeAsXRoot) != Unset and element == object.getObjectName())) {
      fill(&object);
      done = true;
    }
  }
  /// \private
  void filled(ObjectBase *obj, const std::string &error) override {
    if (not error.empty())
      throw std::runtime_error(error);
  }
  /// \private
  void Encrypt(const std::string &algorithm, const std::string &keyName, const std::string &cipher, mobs::CryptBufBase *&cryptBufp) override {
    if (decrypFun)
      cryptBufp = decrypFun(algorithm, keyName);
  }

  /// wurde überhaupt eine Wurzel gefunden
  bool found() const { return done; }
private:
  ObjectBase &object;
  bool done = false;
  ConvObjFromStr::DecrypFun decrypFun;
};

}

#pragma clang diagnostic pop

#endif // MOBS_XMLREAD_H