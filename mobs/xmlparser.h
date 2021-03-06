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

/** \file xmlparser.h
\brief EInfacher XML-Parser */


#ifndef MOBS_XMLPARSER_H
#define MOBS_XMLPARSER_H

#include "logging.h"
#include "objtypes.h"
#include "converter.h"
#include "csb.h"

#include<stack>
#include<exception>
#include<iostream>
#include <codecvt>
#include <utility>


namespace mobs {

/** \class XmlParser
 \brief Einfacher XML-Parser.
 Virtuelle Basisklasse mit Callback-Funktionen. Die Tags werden nativ geparst,
 es erfolgt keine Zeichenumwandlung (\&lt; usw.); Die werde werden implace zurückgeliefert
 Im Fehlerfall werden exceptions geworfen.
 \code
 concept XmlParser {
 XmlParser(const std::string &input);
 
 // Starte Parser
 void parse();
 
 // Callback Funktionen
 void NullTag(const std::string &element) override;
 void Attribute(const std::string &element, const std::string &attribut, const std::string &value) override;
 void Value(const std::string &value) override;
 void Cdata(const char *value, size_t len) override;
 void StartTag(const std::string &element) override;
 void EndTag(const std::string &element) override;
 void ProcessingInstruction(const std::string &element, const std::string &attribut, const std::string &value) override;
 };
 \endcode
 */




/// Einfacher XML-Parser - Basisklasse
class XmlParser  {
public:
  /*! Konstruktor der XML-Parser Basisklasse
   @param input XML-Text der geparst werden soll   */
  explicit XmlParser(const std::string &input) : Xml(input) {
    pos1 = pos2 = posS = posE = 0;
  };
  virtual ~XmlParser() = default;;
  
  /*! \brief Liefert XML-Puffer und aktuelle Position für detaillierte Fehlermeldung
   @param pos Position des Fehlers im Xml-Buffer
   @return zu parsender Text-Buffer
   */
  const std::string &info(size_t &pos) const {
    pos = pos1;
    return Xml;
  };
  /** \brief zugriff auf den Stack der Element-Struktur
   */
  const std::stack<std::string> &tagPath() const { return tags; };
  
  /** \brief Callback-Function: Ein Tag ohne Inhalt, impliziert EndTag(..)
   @param element Name des Elementes
   */
  virtual void NullTag(const std::string &element) = 0;
  /** \brief Callback-Function: Ein Atributwert einses Tags
   @param element Name des Elementes
   @param attribut Name des Attributes
   @param value Wert des Attributes
   */
  virtual void Attribute(const std::string &element, const std::string &attribut, const std::string &value) = 0;
  /** \brief Callback-Function: Ein Inhalt eines Tags
   @param value Inhalt des Tags
   */
  virtual void Value(const std::string &value) = 0;
  /** \brief Callback-Function: Ein CDATA-Elemet
   @param value Inhalt des Tags
   @param len Länge des Tags
   */
  virtual void Cdata(const char *value, size_t len) = 0;  // TODO bei C++17 besser std::string_view
  /** \brief Callback-Function: Ein Start-Tag
   @param element Name des Elementes
   */
  virtual void StartTag(const std::string &element) = 0;
  /** \brief Callback-Function: Ein Ende-Tag , jedoch nicht bei NullTag(..)
   @param element Name des Elementes
   */
  virtual void EndTag(const std::string &element) = 0;
  /** \brief Callback-Function: Eine Verarbeitungsanweisung z.B, "xml", "encoding", "UTF-8"
   @param element Name des Tags
   @param attribut Name der Verarbeitungsanweisung
   @param value Inhalt der Verarbeitungsanweisung
   */
  virtual void ProcessingInstruction(const std::string &element, const std::string &attribut, const std::string &value) = 0;
  
  /// Starte den Parser
  void parse() {
    TRACE("");
    parse2LT();
    if (pos2 != 0)
      throw std::runtime_error(u8"Syntax Head");
    eat('<');
    if (peek() == '?')
    {
      // Parse primäre Verarbeitungsanweisung
      eat();
      parse2GT();
      if (getValue() != "xml")
        throw std::runtime_error(u8"Syntax");
      for (;peek() != '?';)
      {
        eat(' ');
        parse2GT();
        std::string a = getValue();
        eat('=');
        char c = peek();
        if (c == '"')
          eat('"');
        else
          eat('\'');
        parse2Char(c);
        std::string v = getValue();
        eat(c);
        ProcessingInstruction("xml", a, v);
      }
      eat('?');
      eat('>');
      parse2LT();
    }
    // eigentliches Parsing
    for (;pos2 != std::string::npos;)
    {
      saveValue();
      eat('<');
      
      if (peek() == '/')
      {
        // Parse End-Tag
        eat();
        parse2GT();
        std::string element = getValue();
        if (element.empty())
          throw std::runtime_error("missing tag E");
        if (lastKey == element)
        {
          Value(decode(posS, posE));
          clearValue();
          lastKey = "";
        }
        EndTag(element);
        if (tags.empty())
          throw std::runtime_error(u8"unexpected closing tag " + element);
        if (tags.top() != element)
          throw std::runtime_error(u8"unmatching tag " + element + " expected " + tags.top());
        tags.pop();
        eat('>');
        parse2LT();
        continue;
      }
      else if (peek() == '!')
      {
        eat();
        // Parse CDATA Element
        if (peek() == '[')
        {
          eat('[');
          eat('C');
          eat('D');
          eat('A');
          eat('T');
          eat('A');
          eat('[');
          parse2CD();
          saveValue();
          Cdata(&Xml[posS], posE-posS);
          clearValue();
          lastKey = "";
          eat();
          eat();
        }
        else
        {
          // Parse Kommentar
          eat('-');
          eat('-');
          parse2Com();
        }
        eat('>');
        parse2LT();
        continue;
      }
      else if (peek() == '?')
      {
        // Parse Verarbeitungsanweisung
        eat();
        parse2GT();
        std::string element = getValue();
        for (;;)
        {
          if (peek() == '?')
          {
            eat();
            ProcessingInstruction(element, "", "");
            break;
          }
          eat(' ');
          parse2GT();
          std::string a = getValue();
          std::string v;
          if (peek() == '=')
          {
            eat('=');
            char c = peek();
            if (c == '"')
              eat('"');
            else
              eat('\'');
            parse2Char(c);
            v = getValue();
            eat(c);
          }
          ProcessingInstruction(element, a, v);
        }
        eat('>');
        parse2LT();
        continue;
      }
      // Parse Element-Beginn
      parse2GT();
      std::string element = getValue();
      if (element.empty())
        throw std::runtime_error("missing tag B");
      tags.push(element);
      StartTag(element);
      for (;;)
      {
        if (peek() == '>')  // Ende eines Starttags
        {
          eat();
          parse2LT();
          break;
        }
        else if (peek() == '/') // Leertag
        {
          eat();
          eat('>');
          NullTag(element);
          tags.pop();
          parse2LT();
          break;
        }
        eat(' ');
        parse2GT();
        std::string a = getValue();
        eat('=');
        char c = peek();
        if (c == '"')
          eat('"');
        else
          eat('\'');
        parse2Char(c);
        std::string v = getValue();
        eat(c);
        Attribute(element, a, v);
      }
      lastKey = element;
    }
    pos2 = Xml.length();
    saveValue();
    // nur noch für check Whitespace bis eof
    saveValue();
    if (not tags.empty())
      throw std::runtime_error(u8" expected tag at EOF: " + tags.top());
  };
  
private:
  
  void parse2LT() {
    pos2 = Xml.find('<', pos1);
    //cerr << "PLT " << pos2 << endl;
  };
  void parse2GT() {
    pos2 = Xml.find_first_of("/ <>=\"'?!", pos1);
    //cerr << "PGT " << pos2 << " " << pos1 << endl;
    if (pos2 == std::string::npos)
      throw std::runtime_error("Syntax");
  };
  void parse2Char(char c) {
    pos2 = Xml.find(c, pos1);
    //cerr << "PGT " << pos2 << " " << pos1 << endl;
    if (pos2 == std::string::npos)
      throw std::runtime_error("Syntax");
  };
  void parse2Com() {
    pos2 = Xml.find(u8"-->", pos1);
    //cerr << "P-- " << pos2 << " " << pos1 << endl;
    if (pos2 == std::string::npos)
      throw std::runtime_error("Syntax");
    pos1 = pos2 + 2;
  };
  void parse2CD() {
    pos2 = Xml.find(u8"]]>", pos1);
    //cerr << "PCD " << pos2 << " " << pos1 << endl;
    if (pos2 == std::string::npos)
      throw std::runtime_error("Syntax");
  };
  std::string getValue() {
    if (pos2 == std::string::npos)
      throw std::runtime_error(u8"unexpected EOF");
    size_t p = pos1;
    pos1 = pos2;
    return decode(p, pos2);
  };
  void clearValue() { posS = posE; }; // der Zwischenraum fand Verwendung
                                      /// Verwaltet den Zwischenraum zwischen den <..Tags..>
  void saveValue() {
    // wenn nicht verwendet, darf es nur white space sein
    if (posS != posE)
    {
      //cerr << "White '" <<  Xml.substr(posS, posE - posS) << "'" << endl;
      size_t p = Xml.find_first_not_of(" \n\r\t", posS);
      if (p < posE)
      {
        pos1 = p;
        throw std::runtime_error(u8"unexpected char");
      }
    }
    
    if (pos2 == std::string::npos)
      throw std::runtime_error(u8"unexpected EOF");
    posS = pos1;
    posE = pos2;
    pos1 = pos2;
  };
  void eat(char c) {
    if (Xml[pos1] != c)
      throw std::runtime_error(u8"Expected " + std::to_string(c) + " got " + std::to_string(Xml[pos1]));
    pos1++;
  };
  void eat() { pos1++; };
  char peek() {
    if (pos1 >= Xml.length())
      throw std::runtime_error(u8"unexpected EOF");
    //cerr << "Peek " << Xml[pos1] << " " << pos1 << endl;
    return Xml[pos1];
  };
  /// Wandelt einen Teilstring aus Xml von der HTML-Notation in ASCII zurück
  /// @param pos_S StartPosition
  /// @param pos_E EndePosition
  std::string decode(size_t pos_S, size_t pos_E) {
    // Da in XML Die Zeicxhe & und < immer escaped (in HTML) werden müssen, kann ein Rückwandlung
    // immer erfolgen, da ein '&' ansonsten nicht vorkommen sollte
    std::string result;
    for (;;)
    {
      size_t pos = Xml.find('&', pos_S);
      if (pos < pos_E) // & gefunden
      {
        result += std::string(&Xml[pos_S], pos - pos_S);
        pos_S = pos + 1;
        pos = Xml.find(';', pos_S);
        if (pos < pos_E and pos < pos_S + 16) // Token &xxxx; gefunden
        {
          std::string tok = std::string(&Xml[pos_S], pos - pos_S);
          //          std::cerr << "TOK " << tok << std::endl;
          char c = '\0';
          if (tok == "lt")
            c = '<';
          else if (tok == "gt")
            c = '>';
          else if (tok == "amp")
            c = '&';
          else if (tok == "quot")
            c = '"';
          else if (tok == "apos")
            c = '\'';
          if (c)
          {
            result += c;
            pos_S = pos + 1;
            continue;
          }
        }
        // wenn nichts passt dann einfach übernehmen
        result += '&';
      }
      else
      {
        result += std::string(&Xml[pos_S], pos_E - pos_S);
        break;
      }
    }
    return result;
  };
  const std::string &Xml;
  size_t pos1, pos2;  // current / search pointer for parsing
  size_t posS, posE;  // start / end pointer for last text
  std::stack<std::string> tags;
  std::string lastKey;
  
};







/** \class XmlParserW
\brief  XML-Parser  der mit wstream arbeitet.
Virtuelle Basisklasse mit Callback-Funktionen. Die Tags werden nativ geparst,
es erfolgt eine Zeichenumwandlung (\&lt; usw.);
 
Als Eingabe kann entweder ein \c std::wifstream dienen oder ein \c std::wistringstream
 
Bei einem \c wifstream wird anhand des BOM das Charset automatisch angepasst  (UTF-16 (LE) oder UTF-8).
Ohne BOM wird nativ \c ISO-8859-1 angenommen.  Über  <?xml ... encoding="UTF-8"  kann auch auf \c UTF-8 umgeschaltet werden.
 
Im Fehlerfall werden exceptions geworfen.
\code
concept XmlParserW {
XmlParserW(std::wistream &input);

// Starte Parser
void parse();

// Callback Funktionen
void NullTag(const std::string &element);
void Attribute(const std::string &element, const std::string &attribut, const std::wstring &value);
void Value(const std::wstring &value);
void Cdata(const std::wstring &value);
void StartTag(const std::string &element);
void EndTag(const std::string &element);
void ProcessingInstruction(const std::string &element, const std::string &attribut, const std::wstring &value);
};
\endcode
*/


class CryptBufBase;

class XmlParserW  {
  typedef std::char_traits<wchar_t> Traits;
public:
  /** Konstruktor der XML-Parser Basisklasse für std::wstring
   
     es kann z.B. ein \c std::wifstream dienen oder ein \c std::wistringstream übergeben werden
          Als Zeichensätze sind UTF-8, UTF-16, ISO8859-1, -9 und -15 erlaubt; Dateien dürfen mit einem BOM beginnen
   @param input XML-stream der geparst werden soll   */
  explicit XmlParserW(std::wistream &input) : istr(input), base64(base64data) { };
  virtual ~XmlParserW() = default;;
  /*! \brief Liefert XML-Puffer und aktuelle Position für detaillierte Fehlermeldung
   @param pos Position des Fehlers im Xml-Buffer
   @return zu parsender Text-Buffer
   */
  std::string info(size_t &pos) const {
    pos = istr.tellg();
//    std::cerr << "CUR = " << Traits::to_char_type(curr) << " " << pos << std::endl;
    std::wstring w;
    Traits::char_type c;
    w += Traits::to_char_type(curr);
    for (int i = 0; i < 50; i++) {
      if (Traits::not_eof((c = get())))
        w += c;
      else
        break;
    }
//    std::cerr << mobs::to_string(w) << std::endl;
    return to_string(w);
  };
//  /** \brief zugriff auf den Stack der Element-Struktur
//   */
//  const std::stack<std::string> &tagPath() const { return tags; };
  /** \brief aktuelle Ebene des Baums
   *
   * @return Level 1 = root
   */
  size_t currentLevel() const { return tags.size(); };
  std::string currentXmlns() const { return tags.empty() ? "" : tags.top().xmlns; };

  /** \brief Callback-Function: Ein Tag ohne Inhalt, impliziert EndTag(..)
   @param element Name des Elementes
   */
  virtual void NullTag(const std::string &element) = 0;
  /** \brief Callback-Function: Ein Attributwert eines Tags
   @param element Name des Elementes
   @param attribut Name des Attributes
   @param value Wert des Attributes
   */
  virtual void Attribute(const std::string &element, const std::string &attribut, const std::wstring &value) = 0;
  /** \brief Callback-Function: Ein Inhalt eines Tags
   @param value Inhalt des Tags
   */
  virtual void Value(const std::wstring &value) = 0;
  /** \brief Callback-Function: Ein CDATA-Element (optional) ansonsten wird auf \c Value abgebildet
   @param value Inhalt des Tags
   */
  virtual void Cdata(const std::wstring &value) { Value(value); }
  /** \brief Callback-Function: Ein CDATA-Element mit base64 codiertem Inhalt
   
      nur, wenn setBase64(true) gesetzt wurde
   @param input Inhalt des base64 codierten Wertes
   */
  virtual void Base64(const std::vector<u_char> &input) { }
  /** \brief Callback-Function: Ein Start-Tag
   @param element Name des Elementes
   */
  virtual void StartTag(const std::string &element) = 0;
  /** \brief Callback-Function: Ein Ende-Tag , jedoch nicht bei NullTag(..)
   @param element Name des Elementes
   */
  virtual void EndTag(const std::string &element) = 0;
  /** \brief Callback-Function: Eine Verarbeitungsanweisung z.B, "xml", "encoding", "UTF-8"
   @param element Name des Tags
   @param attribut Name der Verarbeitungsanweisung
   @param value Inhalt der Verarbeitungsanweisung
   */
  virtual void ProcessingInstruction(const std::string &element, const std::string &attribut, const std::wstring &value) = 0;
  /** \brief Callback-Funktion: Ein Element "EncryptedData" wurde gefunden und ein decrypt-Modul wird benötigt
   *
   * Es wird  "https://www.w3.org/2001/04/xmlenc#Element" unterstützt
   * Bei mehreren Recipients muss nur bei einer Id eine Entschlüsselungs-Klasse zurückgeliefert werden.
   * Bei unbekannten Recipients kann nullptr zurückgegeben werden.
   * @param algorithm Algorithmus um xmlns bereinigt zB.: aes-256-cbc, oder "https://www.w3.org/2001/04/xmlenc#aes-256-cbc"/
   * @param keyName KeyInfo-Element
   * @param cipher Schlüssel, falls vorhanden
   * @param cryptBufp ein mit new erzeugtes Encryption-Module; wird automatisch freigegeben
   */
  virtual void Encrypt(const std::string &algorithm, const std::string &keyName,  const std::string &cipher, CryptBufBase *&cryptBufp) = 0;
  /// Encryption-Element abgeschlossen
  virtual void EncryptionFinished() { }

  /// Einstellung: Lese bis EOF
  void readTillEof(bool s) {  reedEof = s; }
  /// ist beim Parsen das Ende der Datei erreicht
  bool eof() const { return endOfFile; }
  /// ist beim Parsen das letzte Tag erreicht
  bool eot() const { return running and tags.empty(); }
  /// verlasse beim nächsten End-Tag den parser
  void stop() { paused = true; }
  /// Aktiviere automatische base64 Erkennung
  /// \see Base64
  void setBase64(bool b) { useBase64 = b; }
  /// Referenz auf input stream
  std::wistream &getIstr() { return istr; };

  /// Starte den Parser
  void parse() {
    TRACE("");
    if (not running)
    {
      std::locale lo1 = std::locale(istr.getloc(), new codec_iso8859_1);
      istr.imbue(lo1);

      eat();  // erstes Zeichen einlesen
      /// BOM bearbeiten
      if (curr == 0xff)
      {
        std::locale lo;
        if ((curr = istr.get()) != 0xfe)
          throw std::runtime_error(u8"Error in BOM");
        lo = std::locale(istr.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>);
        istr.imbue(lo);
        encoding = u8"UTF-16"; // (LE)
        eat();
      }
      else if (curr == 0xfe)
      {
        std::locale lo;
        if ((curr = istr.get()) != 0xff)
          throw std::runtime_error(u8"Error in BOM");
        lo = std::locale(istr.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::codecvt_mode(0)>);
        istr.imbue(lo);
        encoding = u8"UTF-16"; // (BE)
        eat();
      }
      else if (curr == 0xef)
      {
        std::locale lo;
        if ((curr = istr.get()) == 0xbb and (curr = istr.get()) == 0xbf)
          lo = std::locale(istr.getloc(), new std::codecvt_utf8<wchar_t, 0x10ffff, std::little_endian>);
        else
          throw std::runtime_error(u8"Error in BOM");
        encoding = u8"UTF-8";
        istr.imbue(lo);
        eat();
      }

      buffer.clear();
      parse2LT();
      if (not Traits::eq_int_type(curr, '<'))
        THROW(u8"Syntax Head");
      // BOM überlesen
      if (not buffer.empty() and buffer != L"\u00EF\u00BB\u00BF" and buffer != L"\ufeff")
      {
//        for (auto c:buffer) std::cerr << '#' <<  int(c) << std::endl;
        THROW("invalid begin of File");
      }
      buffer.clear();
      running = true;
    } else if (paused) {
      paused = false;
      eat('>');
      parse2LT();
//      if (inParse2LT)
//        return;
//    }
//    else if (inParse2LT) {
//      while ((curr = get()) > 0) {
//        if (curr == '<') {
//          inParse2LT = false;
//          break;
//        }
//        if (try64)
//          base64.put(curr);
//        else
//          buffer += curr;
//        if (maxRead == 0)
//          return;
//      }
    }
    // eigentliches Parsing
    while (Traits::eq_int_type(curr, '<'))
    {
      saveValue();
//      saved = buffer;
      eat('<');

      if (peek() == '/')
      {
        // Parse End-Tag
        eat();
        parse2GT();
        decode(buffer);
        std::string element = to_string(buffer);
        if (element.empty())
          THROW("missing end tag");
        if (lastKey == element)
        {
          if (in64) {
            saveValue();
            in64 = false;
          } if (not cdata.empty()) {
            if (xmlEncState <= 0)
              Cdata(cdata);
            saveValue();
          } else {
            decode(saved);
            if (xmlEncState > 0) {
//            LOG(LM_DEBUG, "XV " << element << " " << xmlEncState);
              if (element == u8"CipherValue") {
                if (xmlEncState == 5) {
                  if (not encryptedData.cryptBufp)
                    encryptedData.cipher = to_string(saved);
                }
              } else if (element == u8"KeyName" and xmlEncState == 3) {
                if (not encryptedData.cryptBufp)
                  encryptedData.keyName = to_string(saved);
              }
            } else if (xmlEncState <= 0)
              Value(saved);
          }
          clearValue();
          lastKey = "";
        }
        cdata.clear();
        if (xmlEncState <= 0)
          EndTag(element);
        else {
//        LOG(LM_DEBUG, "EEE element " << element << " " << xmlEncState);
          xmlEncState--;
          if (element == u8"EncryptedData") {
            // Wenn CipherData vollständig dann wieder alles auf normal
            xmlEncState = 0;
            EncryptionFinished();
//          LOG(LM_DEBUG, "encrypting element " << xmlEncState);
          } else if (element == u8"KeyInfo") {
            if (not encryptedData.cryptBufp) {
              Encrypt(encryptedData.algorithm, encryptedData.keyName, encryptedData.cipher, encryptedData.cryptBufp);
              if (not encryptedData.cryptBufp) { // keine encryption, weitersuchen
                encryptedData.keyName = "";
                encryptedData.cipher = "";
              }
            }
          }
        }
        if (tags.empty())
          THROW(u8"unexpected closing tag " + element);
        if (tags.top().element != element) {
//        while (Traits::not_eof((curr = get()))) ;
          THROW(u8"unmatching tag " + element + " expected " + tags.top().element);
        }
        tags.pop();
        if (not reedEof and tags.empty())
          paused = true;
        if (paused)
          return;
        eat('>');
        parse2LT();
        continue;
      }
      else if (peek() == '!')
      {
        eat();
        // Parse CDATA Element
        if (peek() == '[')
        {
          eat('[');
          eat('C');
          eat('D');
          eat('A');
          eat('T');
          eat('A');
          eat('[');
          saveValue();  // nur whitespace prüfen
          cdata.swap(buffer);
          parse2CD();
          if (try64) {
            base64.done();
            Base64(base64data);
            cdata.clear();
            in64 = true;
          }
          else {
            buffer.resize(buffer.length() -2);
            if (buffer.empty()) {
              if (xmlEncState <= 0)
                Cdata(buffer);
              lastKey = "";
            } else
              cdata.swap(buffer);
          }
          base64.clear();
          try64 = false;
          clearValue();
        }
        else
        {
          // Parse Kommentar
          eat('-');
          eat('-');
          parse2Com();
        }
        if (paused)
          return;
        eat('>');
        parse2LT();
        continue;
      }
      else if (peek() == '?')
      {
        // Parse Verarbeitungsanweisung
        eat();
        parse2GT();
        decode(buffer);
        std::string element = to_string(buffer);
        for (;;)
        {
          if (peek() == '?')
          {
            eat();
            ProcessingInstruction(element, "", L"");
            break;
          }
          eat(' ');
          parse2GT();
          decode(buffer);
          std::string a = to_string(buffer);
          std::wstring v;
          if (peek() == '=')
          {
            eat('=');
            auto c = peek();
            if (c == '"')
              eat('"');
            else
              eat('\'');
            buffer.clear();
            parse2Char(c);
            decode(buffer);
            v = buffer;
            eat(c);
          }
          if (element == u8"xml" and a == u8"encoding" and not v.empty())
          {
            if (encoding.empty())
            {
              encoding = mobs::to_string(v);
              if (encoding == u8"UTF-8")
              {
                std::locale lo = std::locale(istr.getloc(), new std::codecvt_utf8<wchar_t, 0x10ffff, std::little_endian>);
                istr.imbue(lo);
              }
              else if (encoding == u8"ISO-8859-15")
              {
                std::locale lo = std::locale(istr.getloc(), new codec_iso8859_15);
                istr.imbue(lo);
              }
              else if (encoding == u8"ISO-8859-9")
              {
                std::locale lo = std::locale(istr.getloc(), new codec_iso8859_9);
                istr.imbue(lo);
              }
              else if (encoding != u8"ISO-8859-1")
                LOG(LM_WARNING, u8"unknown encoding: " << encoding << " using ISO-8859-1");
            }
            else if (encoding != mobs::to_string(v))
              LOG(LM_WARNING, u8"encoding mismatch: " << encoding << " " << mobs::to_string(v));

          }
          ProcessingInstruction(element, a, v);
        }
        if (paused)
          return;
        eat('>');
        parse2LT();
        continue;
      }
      // Parse Element-Beginn
      parse2GT();
      decode(buffer);
      std::string element = to_string(buffer);
      if (element.empty())
        THROW("missing begin tag");
      tags.emplace(element, currentXmlns());
      if (xmlEncState == 0 and element == u8"EncryptedData") {
        xmlEncState = 1;
//      LOG(LM_DEBUG, "encrypting element " << xmlEncState);
        encryptedData = EncryptedData();
      } else if (xmlEncState > 0) {
        xmlEncState++;
//      LOG(LM_DEBUG, "III element " << element << " " << xmlEncState);
        if (xmlEncState == 3 and element == "CipherValue") {
          LOG(LM_DEBUG, "START CRYPT " << encryptedData.keyName);
          if (not encryptedData.cryptBufp)
            THROW("no suitable decryption found");
          // Pipe aufbauen: filter base64 | bas64-encrypt
          encryptedData.b64buf = new Base64IstBuf(istr);
          encryptedData.tmpstr = new std::istream(encryptedData.b64buf);
          encryptedData.cBuf = new mobs::CryptIstrBuf(*encryptedData.tmpstr, encryptedData.cryptBufp);
          if (encryptedData.cBuf->bad())
            THROW("decryption failed");
          encryptedData.cBuf->getCbb()->setBase64(true);
          encryptedData.istr = new std::wistream(encryptedData.cBuf);
          xmlEncState = 0;
          tags.pop();
          tags.pop();
          tags.pop();
        }
        if (xmlEncState == 2 and (element != u8"KeyInfo" and element != u8"CipherData" and element != u8"EncryptionMethod"))
          THROW("invalid encryption element");
      }
      else
        StartTag(element);
      for (;;)
      {
        if (peek() == '>')  // Ende eines Starttags
        {
          if (paused)
            return;
          eat();
          parse2LT();
          break;
        }
        else if (peek() == '/') // Leertag
        {
          eat();
          if (xmlEncState > 0)
            xmlEncState--;
          else
            NullTag(element);
          tags.pop();
          if (paused)
            return;
          eat('>');
          parse2LT();
          break;
        }
        eat(' ');
        parse2GT();
        decode(buffer);
        std::string a = to_string(buffer);
        eat('=');
        wchar_t c = peek();
        if (c == '"')
          eat('"');
        else
          eat('\'');
        buffer.clear();
        parse2Char(c);
        decode(buffer);
        // XML-Namespace Tags verarbeiten
        if (a == u8"xmlns" and (buffer.substr(0, 18) == L"http://www.w3.org/" or
                                buffer.substr(0, 19) == L"https://www.w3.org/")) {
          tags.top().xmlns = to_string(buffer);
          LOG(LM_DEBUG, "XML-namespace " << currentXmlns());
        }
        else if (xmlEncState > 0 ) {
          LOG(LM_DEBUG, "XmlEnc Att " << element << " " << xmlEncState);
          if (xmlEncState == 1 and element == u8"EncryptedData" and a == u8"Type" and
              buffer == L"https://www.w3.org/2001/04/xmlenc#Element") {
//          LOG(LM_DEBUG, "encrypting element " << xmlEncState);
          } else if (xmlEncState == 2 and element == u8"EncryptionMethod" and a == u8"Algorithm") {
            std::string t = to_string(buffer);
            std::string ns = currentXmlns();
            if (not ns.empty() and t.substr(0, ns.length()) == ns)
              encryptedData.algorithm = t.substr(ns.length());
            else
              encryptedData.algorithm = t;
          }
        }
        else
          Attribute(element, a, buffer);
        eat(c);
      }
      lastKey = element;
    }
//    if (inParse2LT)
//      return;
    saveValue();
    if (Traits::not_eof(curr))
      THROW(u8"Syntax error");
    // nur noch für check Whitespace bis eof
    saveValue();
    if (not tags.empty())
      THROW(u8" expected tag at EOF: " + tags.top().element);
    endOfFile = true;
  };

private:
  void parse2LT() { buffer.clear(); parse2Char('<'); }
  void parse2GT() {
    buffer.clear();
    if (Traits::not_eof(curr)) {
      buffer += Traits::to_char_type(curr);
      while (Traits::not_eof((curr = get()))) {
//      std::cout << "x " << mobs::to_string(Traits::to_char_type(curr));
        if (std::wstring(L"/ <>=\"'?!").find(Traits::to_char_type(curr)) != std::wstring::npos)
          break;
        buffer += Traits::to_char_type(curr);
      }
      if (not Traits::not_eof(curr))
        THROW("Syntax");
    }
  };
  void parse2Char(Traits::char_type c) {
    if (not Traits::not_eof(curr) or Traits::to_char_type(curr) == c)
      return;
    if (try64)
      base64.put(Traits::to_char_type(curr));
    else
      buffer += Traits::to_char_type(curr);
//    if (maxRead == 0 and c == L'<') {
//      inParse2LT = true;
//      return;
//    }
    while (Traits::not_eof(curr = get())) {
      if (Traits::to_char_type(curr) == c)
        break;
      if (try64)
        base64.put(Traits::to_char_type(curr));
      else
        buffer += Traits::to_char_type(curr);
    }
  };
  void parse2Com() {
    for (;;) {
      parse2Char('-');
      if (peek() == '-') {
        eat();
        if (peek() == '-') {
          for (;;) {
            eat();
            if (peek() == '>')
              return;
            if (peek() != '-')
              break;
          }
        }
      }
    }
  };
  void parse2CD() {
    for (;;) {
      base64Start();
      parse2Char(']');
      if (peek() == ']') {
        eat();
        if (peek() == ']') {
          for (;;) {
            eat();
            if (peek() == '>')
              return;
            if (try64)
              THROW("base64 error");
            base64.clear();
            if (peek() != ']')
              break;
          }
        }
      }
      if (try64)
        THROW("base64 error");
      base64.clear();
    }
  };
  void clearValue() { saved.clear(); }; // der Zwischenraum fand Verwendung
/// Verwaltet den Zwischenraum zwischen den <..Tags..>
  void saveValue() {
    // wenn nicht verwendet, darf es nur white space sein
    if (not saved.empty())
    {
      size_t p = saved.find_first_not_of(L" \n\r\t");
      if (p != std::wstring::npos)
        THROW(u8"unexpected char");
    }
    saved = buffer;
  };
  void eat(Traits::char_type c) {
    buffer += Traits::to_char_type(curr);
    if (not Traits::eq_int_type(Traits::to_int_type(c), curr))
      THROW(u8"Expected " << mobs::to_string(c) << " got " << mobs::to_string(Traits::to_char_type(curr)));
    curr = get();
//    pos1++;
  };
  void eat() {
//    pos1++;
    buffer += Traits::to_char_type(curr);
    curr = get();
  };
  Traits::char_type peek() const {
    if (Traits::not_eof(curr))
      return Traits::to_char_type(curr);
    THROW(u8"unexpected EOF");
  };
/// Wandelt einen Teilstring aus Xml von der HTML-Notation in ASCII zurück
  void decode(std::wstring &buf) {
    std::wstring result;
    size_t posS = 0;
    size_t posE = buf.length();
    for (;;) {
      size_t pos = buf.find('&', posS);
      if (pos < posE) // & gefunden
      {
        result += std::wstring(&buf[posS], pos-posS);
        posS = pos +1;
        pos = buf.find(';', posS);
        if (pos < posE and pos < posS + 16) // Token &xxxx; gefunden
        {
          std::wstring tok = std::wstring(&buf[posS], pos-posS);
          //          std::cerr << "TOK " << tok << std::endl;
          wchar_t c = from_html_tag(tok);
          if (c)
          {
            result += c;
            posS = pos +1;
            continue;
          }
        }
        // wenn nichts passt dann einfach übernehmen
        result += '&';
      }
      else if (conFun) // Wandlung bei ISO-Zeichensätzen
      {
        wchar_t (*cf)(wchar_t) = conFun;
        std::transform(buf.cbegin()+posS, buf.cbegin()+posE, std::back_inserter(result),
                       [cf](const wchar_t c) -> wchar_t { return (c <= 127) ? c : cf(c); });
        break;
      }
      else
      {
        result += std::wstring(&buf[posS], posE-posS);
        break;
      }
    }
    buf = result;
  }
  void base64Start() {
    if (not useBase64)
      return;
    base64.clear();
    try64 = true;
  }
  Traits::int_type get() const {
    Traits::int_type c;
//    if (maxRead)
//      maxRead--;
    if (encryptedData.istr) {
      c = encryptedData.istr->get();
//      std::cout << " e" << mobs::to_string(c); // << " " << istr.tellg() << ".";
      if (encryptedData.istr->eof()) {
//      LOG(LM_INFO, "ENC FIN");
        if (encryptedData.cBuf->bad())
          THROW("decryption failed");
        delete encryptedData.istr;
        delete encryptedData.cBuf;
        delete encryptedData.tmpstr;
        delete encryptedData.b64buf;
        encryptedData.istr = nullptr;
        encryptedData.cBuf = nullptr;
        encryptedData.tmpstr = nullptr;
        encryptedData.b64buf = nullptr;
        xmlEncState = 99;
        tags.emplace(u8"EncryptedData", currentXmlns());
        tags.emplace(u8"CipherData", currentXmlns());
        tags.emplace(u8"CipherValue", currentXmlns());

        c = istr.get();
      }
    } else {
      std::wistream::sentry s(istr, true);
      if (not s)
        THROW("bad stream");
      c = istr.get();
//    if (istr.eof())
//      std::cout << "EOF";
//    std::cout << " x" << mobs::to_string(c); // << " " << istr.tellg() << ".";
    }
    return c;
  }
  class Level {
  public:
    Level(std::string e, std::string x) : element(std::move(e)), xmlns(std::move(x)) {}
    std::string element;
    std::string xmlns;
  };
  struct EncryptedData {
    std::string algorithm;
    std::string keyName;
    std::string cipher;
    CryptBufBase *cryptBufp = nullptr;
    mutable mobs::CryptIstrBuf *cBuf = nullptr;
    mutable std::wistream *istr = nullptr;
    mutable std::istream *tmpstr = nullptr;
    mutable mobs::Base64IstBuf *b64buf = nullptr;
  };
  std::wistream &istr;
  std::wstring buffer;
  std::wstring saved;
  std::wstring cdata;
  Traits::int_type curr = 0;
  std::string encoding;
  mutable std::stack<Level> tags;
  std::string lastKey;
  wchar_t (*conFun)(wchar_t) = nullptr;
  std::vector<u_char> base64data;
  Base64Reader base64;
  EncryptedData encryptedData;
  mutable int xmlEncState = 0;
//  mutable size_t maxRead = SIZE_T_MAX;
//  bool inParse2LT = false;
  bool try64 = false;
  bool in64 = false;
  bool useBase64 = false;
  bool running = false;
  bool paused = false;
  bool reedEof = true;
  bool endOfFile = false;

};



}

#endif

