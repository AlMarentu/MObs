// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2025 Matthias Lautner
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
\brief Einfacher XML-Parser */


#ifndef MOBS_XMLPARSER_H
#define MOBS_XMLPARSER_H

#include "logging.h"
#include "objtypes.h"
#include "converter.h"
#include "csb.h"

#include <stack>
#include <map>
#include <exception>
#include <iostream>
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








class CryptBufBase;

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
void StartTag(const std::string &element);
void EndTag(const std::string &element);
void ProcessingInstruction(const std::string &element, const std::string &attribut, const std::wstring &value);
};
\endcode
*/

class XmlParserW  {
  typedef std::char_traits<wchar_t> Traits;
public:
  /** Konstruktor der XML-Parser Basisklasse für std::wstring
   
     es kann z.B. ein \c std::wifstream dienen oder ein \c std::wistringstream übergeben werden
          Als Zeichensätze sind UTF-8, UTF-16, ISO8859-1, -9 und -15 erlaubt; Dateien dürfen mit einem BOM beginnen
     Das ENTITY-Konstrukt ist für externe ENTITYs erlaubt: \<!ENTITY greet \"Hallo !!\" \>
     Vordefinierte ENTITYs sind: \&lt; \&gt; \&amp; \&quot; \&apos;
   @param input XML-stream der geparst werden soll   */
  explicit XmlParserW(std::wistream &input) : istr(input), base64(base64data) {
    //istr.exceptions(std::ios::badbit);
  };
  virtual ~XmlParserW() = default;
  /*! \brief Liefert XML-Puffer und aktuelle Position für detaillierte Fehlermeldung
   @param pos Position des Fehlers im Xml-Buffer
   @return zu parsender Text-Buffer
   */
  std::string info(std::streamoff &pos) const {
    pos = istr.tellg();
//    std::cerr << "CUR = " << Traits::to_char_type(curr) << " " << pos << std::endl;
    std::wstring w;
    Traits::int_type c;
    w += Traits::to_char_type(curr);
    for (int i = 0; i < 50; i++) {
      if (Traits::not_eof((c = get())))
        w += Traits::to_char_type(c);
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
  /// liefert aktuellen Namespace
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
  /** \brief Callback-Function: Ein Inhalt eines Tags inkl. CDATA
   @param value Inhalt des Tags
   */
  virtual void Value(const std::wstring &value) = 0;
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
  void readTillEof(bool s) { reedEof = s; }
  /// non blocking parsen
  void readNonBlocking(bool s) { nonblocking = s; }
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
    paused = false;
    if (not running)
    {
      if (nonblocking and not checkAvail(3)) {
        paused = true;
        return;
      }
      std::locale lo1 = std::locale(istr.getloc(), new codec_iso8859_1);
      istr.imbue(lo1);
      eat();  // erstes Zeichen einlesen
      // BOM bearbeiten
      if (curr == 0xff) {
        std::locale lo;
        if ((curr = istr.get()) != 0xfe)
          throw std::runtime_error(u8"Error in BOM");
        lo = std::locale(istr.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>);
        istr.putback(0xfe);
        istr.putback(0xff);
        istr.imbue(lo);
        encoding = u8"UTF-16"; // (LE)
        eat();
        if (curr != 0xFEFF) // BOM
          throw std::runtime_error(u8"Error in Codec");
        eat();
      } else if (curr == 0xfe) {
        std::locale lo;
        if ((curr = istr.get()) != 0xff)
          throw std::runtime_error(u8"Error in BOM");
        lo = std::locale(istr.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::codecvt_mode(0)>);
        istr.putback(0xff);
        istr.putback(0xfe);
        istr.imbue(lo);
        encoding = u8"UTF-16"; // (BE)
        eat();
        if (curr != 0xFEFF) // BOM
          throw std::runtime_error(u8"Error in Codec");
        eat();
      } else if (curr == 0xef) {
        std::locale lo;
        if ((curr = istr.get()) == 0xbb and (curr = istr.get()) == 0xbf)
          lo = std::locale(istr.getloc(), new std::codecvt_utf8<wchar_t, 0x10ffff, std::little_endian>);
        else
          throw std::runtime_error(u8"Error in BOM");
        encoding = u8"UTF-8";
        istr.putback(0xbf);
        istr.putback(0xbb);
        istr.putback(0xef);
        istr.imbue(lo);
        eat();
        if (curr != 0xFEFF) // BOM
          throw std::runtime_error(u8"Error in Codec");
        eat();
      }
      bomCheck = peek() == '<'; // <?xml muß an Dateianfang stehen, Ausnahme BOM
      buffer.clear();
      inParse2Lt = true;
      inParse2LtWork = true;
      running = true;
    }
    // eigentliches Parsing
    for (;;)
    {
      if (inParse2Lt) {
        if (nonblocking and not checkAvail(1))
          paused = true;
        if (paused)
          return;
        if (not inParse2LtWork) {
          eat('>');
          // parsen bis der Arz kommt oder ein '<'
          buffer.clear();
          inParse2LtWork = true;
          continue;
        } else {
          if (Traits::not_eof(curr) and Traits::to_char_type(curr) != '<') {
            if (try64)
              base64.put(Traits::to_char_type(curr));
            else
              buffer += Traits::to_char_type(curr);
            curr = get();
            continue;
          }
          inParse2Lt = false;
        }
      }
      inParse2LtWork = false;
      if (not Traits::eq_int_type(curr, '<'))  // Hauptschleife verlassen
      {
        //LOG(LM_DEBUG, "READY "  << mobs::to_string(buffer));
        saved += buffer;
        if (saved.size() > maxElementSize)
          throw std::runtime_error(u8"Element too large");
        break;
      }
      if (nonblocking and not checkGT())
      {
        paused = true;
        return;
      }
      decode(buffer);
      saved += buffer;
      if (saved.size() > maxElementSize)
        throw std::runtime_error(u8"Element too large");
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
          if (in64)
            saveValueCheckWS(); // nur whitespace prüfen
          if (xmlEncState > 0) {
//            LOG(LM_DEBUG, "XV " << element << " " << xmlEncState);
            if (element == u8"CipherValue") {
              if (xmlEncState == 5) {
                if (not encryptedData.valid())
                  encryptedData.cipher = to_string(saved);
              }
            } else if (element == u8"KeyName" and xmlEncState == 3) {
              if (not encryptedData.valid())
                encryptedData.keyName = to_string(saved);
            }
          } else if (not in64 and xmlEncState <= 0)
            Value(saved);
          clearValue();
          lastKey = "";
          in64 = false;
        }
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
            if (not encryptedData.valid()) {
              CryptBufBase *cbbp{};
              Encrypt(encryptedData.algorithm, encryptedData.keyName, encryptedData.cipher, cbbp);
              encryptedData.setCryptBuf(cbbp);
              // wenn keine encryption, weitersuchen
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
        //else if (tags.empty())
        //  LOG(LM_DEBUG, "READY");
        skipWs();
        inParse2Lt = true;
        continue;
      }
      else if (peek() == '!')
      {
        bomCheck = false;
        eat();
        // Parse CDATA Element
        if (peek() == '[') {
          eat('[');
          eat('C');
          eat('D');
          eat('A');
          eat('T');
          eat('A');
          eat('[');
          buffer.clear();
          parse2CD();
          if (try64) {
            saveValueCheckWS();  // nur whitespace prüfen
            base64.done();
            Base64(base64data);
            clearValue();
            in64 = true;
          }
          else {
            buffer.resize(buffer.length() -2);
            saved += buffer;
            if (saved.size() > maxElementSize)
              throw std::runtime_error(u8"Element too large");
          }
          base64.clear();
          buffer.clear();
          try64 = false;
          //clearValue();
        }
        else if (peek() == 'E') {
          eat('E');
          eat('N');
          eat('T');
          eat('I');
          eat('T');
          eat('Y');
          eatWS();
          buffer.clear();
          parse2Char(' ');
          std::wstring ent = buffer;
          skipWs();
          wchar_t c = peek();
          if (c == '"')
            eat('"');
          else
            eat('\'');
          buffer.clear();
          parse2Char(c);
          decode(buffer, true); // nur CharRef dekodieren, keine EntityRef
          std::wstring val = buffer;
          LOG(LM_DEBUG, "ENTITY " << mobs::to_string(ent) << " " << mobs::to_string(val));
          if (not ent.empty())
            entities[ent] = val;
          eat(c);
          skipWs();
          buffer.clear();
        }
        else {
          // Parse Kommentar
          eat('-');
          eat('-');
          parse2Com();
        }
        inParse2Lt = true;
        continue;
      }
      else if (peek() == '?')
      {
        // Parse Verarbeitungsanweisung
        eat();
        parse2GT();
        decode(buffer);
        std::string element = to_string(buffer);
        if (element == u8"xml" and not bomCheck)
          THROW(u8"Syntax Head");
        bomCheck = false;
        for (;;)
        {
          if (peek() == '?')
          {
            eat();
            ProcessingInstruction(element, "", L"");
            break;
          }
          eatWS();
          parse2GT();
          decode(buffer);
          std::string a = to_string(buffer);
          std::wstring v;
          if (peek() == '=')
          {
            eat('=');
            skipWs();
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
        inParse2Lt = true;
        continue;
      }
      bomCheck = false;
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
          encryptedData.startEncryption(istr);
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
      clearValue();
      for (;;)
      {
        bool hatWs = skipWs();
        if (peek() == '>')  // Ende eines Starttags
        {
          inParse2Lt = true;
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
          inParse2Lt = true;
          break;
        }
        if (not hatWs)
          THROW("missing whitespace");
        parse2GT();
        decode(buffer);
        std::string a = to_string(buffer);
        skipWs();
        eat('=');
        skipWs();
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
    if (Traits::not_eof(curr))
      THROW(u8"Syntax error");
    endOfFile = true;
    if (not tags.empty()) // Am Ende sollten keine offenen Tags übrig bleiben
      THROW(u8" expected tag at EOF: " + tags.top().element);
    // nur noch für check Whitespace bis eof
    saveValueCheckWS();
  };
  std::istream &byteStream(size_t len, CryptBufBase *cbbp = nullptr) {
    auto wbufp = dynamic_cast<CryptIstrBuf*>(istr.rdbuf());
    if (not wbufp)
      throw std::runtime_error("no mobs::CryptIstrBuf");
    istr.clear();
    binaryBuffer = std::unique_ptr<mobs::BinaryIstBuf>(new BinaryIstBuf(*wbufp, len + 1)); // plus delimiter
    if (nonblocking and binaryBuffer->in_avail() <= 0)
      throw std::runtime_error("delimiter missing");
    if (binaryBuffer->sgetc() != mobs::BinaryIstBuf::Traits::to_int_type('\200'))
        THROW("delimiter mismatch");
    binaryBuffer->sbumpc();
    if (cbbp) {
      binaryFiltStream = std::unique_ptr<std::istream>(new std::istream(binaryBuffer.get()));
      cbbp->setIstr(*binaryFiltStream);
      binaryFilt = std::unique_ptr<CryptBufBase>(cbbp);
      binaryStream = std::unique_ptr<std::istream>(new std::istream(binaryFilt.get()));
    } else {
      binaryStream = std::unique_ptr<std::istream>(new std::istream(binaryBuffer.get()));
    }
    return *binaryStream;
  }
  size_t maxElementSize = 256 * 1024 * 1024; ///< Maximale Größe eines Elementes, default: 256MB

private:
  void parse2GT() {
    buffer.clear();
    if (Traits::not_eof(curr)) {
      buffer += Traits::to_char_type(curr);
      while (Traits::not_eof((curr = get()))) {
        // std::cout << "x " << mobs::to_string(Traits::to_char_type(curr));
        switch(Traits::to_char_type(curr)) {
          case '\n':
          case '\r':
          case '\t':
            curr = L' ';
            __attribute__ ((fallthrough));
          case ' ':
          case '<':
          case '>':
          case '=':
          case '/':
          case '?':
          case '!':
          case '"':
          case '\'':
            return;
          default:
            buffer += Traits::to_char_type(curr);
        }
        if (buffer.length() > maxElementSize)
          THROW("Element too large");
        //if (std::wstring(L"/ <>=\"'?!").find(Traits::to_char_type(curr)) != std::wstring::npos)
        //  break;
        //buffer += Traits::to_char_type(curr);
      }
      if (not Traits::not_eof(curr))
        THROW("XmlParseW Syntax");
    }
  };
  void parse2Char(Traits::char_type c) {
    while (Traits::not_eof(curr) and Traits::to_char_type(curr) != c) {
      if (try64) {
        base64.put(Traits::to_char_type(curr));
        if (base64data.size() > maxElementSize)
          THROW("Element too large");
      }  else {
        buffer += Traits::to_char_type(curr);
        if (buffer.length() > maxElementSize)
          THROW("Element too large");
      }
      curr = get();
    }
    if (not Traits::not_eof(curr))
      LOG(LM_DEBUG, "XmlParse::parse2Char EOF");
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
  void saveValueCheckWS() {
    // wenn nicht verwendet, darf es nur white space sein
    if (not saved.empty())
    {
      size_t p = saved.find_first_not_of(L" \n\r\t");
      if (p != std::wstring::npos) {
        LOG(LM_ERROR, "unexpected char in white space WS=" << mobs::to_string(saved));
        THROW(u8"unexpected char in white space");
      }
    }
    saved = buffer;
  };
  void eat(Traits::char_type c) {
    buffer += Traits::to_char_type(curr);
    if (not Traits::eq_int_type(Traits::to_int_type(c), curr))
      THROW(u8"Expected " << mobs::to_string(c) << " got " << mobs::to_string(Traits::to_char_type(curr)));
    curr = get();
  };
  void eat() {
    buffer += Traits::to_char_type(curr);
    curr = get();
  };
  void eatWS() { // mindestens 1 WS entfernen
    for (bool start = true;; start = false) {
      switch (Traits::to_char_type(curr)) {
        case ' ':
        case '\n':
        case '\r':
        case '\t':
        case L'\uFEFF': // ZERO WIDTH NO-BREAK SPACE
          //buffer += Traits::to_char_type(curr);
          curr = get();
          break;
        default:
            if (start)
              THROW(u8"expected WS got " + mobs::to_string(Traits::to_char_type(curr)));
          return;
      }
    }
  };
  bool skipWs() { // WS entfernen, wenn vorhanden
    bool hatWs = false;
    switch (Traits::to_char_type(curr)) {
      case ' ':
      case '\n':
      case '\r':
      case '\t':
      case L'\uFEFF': // ZERO WIDTH NO-BREAK SPACE
        eatWS();
        hatWs = true;
    }
    return hatWs;
  };
  Traits::char_type peek() const {
    if (Traits::not_eof(curr))
      return Traits::to_char_type(curr);
    THROW(u8"unexpected EOF");
  };
  std::wstring fromEntity(const std::wstring &tok) {
    try {
      auto it = entities.find(tok);
      if (it != entities.end())
        return it->second;
      if (tok[0] == L'#' and tok.length() > 2) {
        int c;
        size_t p;
        if (tok[1] == L'x')
          c = std::stoi(tok.substr(2), &p, 16);
        else
          c = std::stoi(tok.substr(1), &p, 10);
        if (p == tok.length() - (tok[1] == L'x' ? 2:1) and
            ((c >= 0 and c <= 0xD7FF) or
             (c >= 0xE000 and c <= 0xFFFD) or (c >= 0x10000 and c <= 0x10FFFF)))
          return std::wstring(1, c);
      }
    } catch (...) {}
    return {};
  };
/// Löst alle Entities auf; wenn entity=true werden nur CharRef dekodiert, keine EntityRef
  void decode(std::wstring &buf, bool entity = false) {
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
          std::wstring s = (entity and tok[0] != L'#') ? L"" : fromEntity(tok);
          if (not s.empty())
          {
            result += s;
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
        std::transform(buf.cbegin()+int(posS), buf.cbegin()+int(posE), std::back_inserter(result),
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
  bool checkGT() const {
    Traits::int_type c;
    do {
      if (not checkAvail(1))
        return false;
      checkGtBuffer.push_back(c = get());
      checkGtBufferEnd++;
      checkGtBufferStart = checkGtBufferEnd; // noch nicht aus Buffer lesen
    } while (Traits::not_eof(c) and not Traits::eq_int_type(Traits::to_int_type('>'), c));
    checkGtBufferStart = 0; // jetzt kann's losgehen
    //LOG(LM_INFO, "XXX " << mobs::to_string(checkGtBuffer) <<  " XXX");
    return true;
  }
  bool checkAvail(std::streamsize n) const {
    if (encryptedData.streamPtr()) {
      auto av = encryptedData.streamPtr()->rdbuf()->in_avail();
      return av >= n or av == -1;
    }
    auto av = istr.rdbuf()->in_avail();
    return av >= n or av == -1;
  }
  Traits::int_type get() const {
    Traits::int_type c;
    if (checkGtBufferStart < checkGtBufferEnd) {
      c = checkGtBuffer[checkGtBufferStart++];
      if (checkGtBufferStart >= checkGtBufferEnd) {
        checkGtBuffer.clear();
        checkGtBufferStart = checkGtBufferEnd = 0;
      }
    } else if (encryptedData.streamPtr()) {
      std::wistream::sentry s(*encryptedData.streamPtr(), true);
      if (not s)
        THROW("bad crypt stream");
      c = encryptedData.streamPtr()->get();
//      std::cout << " e" << mobs::to_string(c); // << " " << istr.tellg() << ".";
      if (encryptedData.streamPtr()->eof()) {
        encryptedData.stopEncryption();
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
      if (not reedEof and istr.eof())
        LOG(LM_DEBUG, "EOF reached");
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
  class EncryptedData {
  public:
    std::string algorithm;
    std::string keyName;
    std::string cipher;

    inline bool valid() { return cryptBufp; } // hat gültige encryption
    inline std::wistream *streamPtr() const { return istr; } // stream pointer bei aktiver encryption
    void setCryptBuf(mobs::CryptBufBase *cbbp) {
      cryptBufp = cbbp;
      if (not valid() ) {
        keyName = "";
        cipher = "";
      }
    }
    void startEncryption(std::wistream &inStr) {
      LOG(LM_DEBUG, "START CRYPT " << keyName);
      if (not cryptBufp)
        THROW("no suitable decryption found");
      // Pipe aufbauen: filter base64 | bas64-encrypt
      b64buf = new mobs::Base64IstBuf(inStr);
      tmpstr = new std::istream(b64buf);
      //tmpstr->exceptions(std::ios::badbit | std::ios::failbit);
      cBuf = new mobs::CryptIstrBuf(*tmpstr, cryptBufp);
      if (cBuf->bad())
        THROW("decryption failed");
      cBuf->getCbb()->setBase64(true);
      istr = new std::wistream(cBuf);
      //istr->exceptions(std::ios::badbit | std::ios::failbit);
      istr->imbue(inStr.getloc());
    }
    void stopEncryption() const {
      LOG(LM_DEBUG, "STOP CRYPT");
      if (cBuf->bad())
        THROW("decryption failed");
      delete istr;
      delete cBuf;
      delete tmpstr;
      delete b64buf;
      istr = nullptr;
      cBuf = nullptr;
      tmpstr = nullptr;
      b64buf = nullptr;
    }
  private:
    CryptBufBase *cryptBufp = nullptr;
    mutable std::wistream *istr = nullptr;
    mutable mobs::CryptIstrBuf *cBuf = nullptr;
    mutable std::istream *tmpstr = nullptr;
    mutable mobs::Base64IstBuf *b64buf = nullptr;
  };
  std::wistream &istr;
  std::wstring buffer;
  std::wstring saved;
  mutable std::vector<Traits::int_type> checkGtBuffer;
  mutable size_t checkGtBufferStart = 0;
  mutable size_t checkGtBufferEnd = 0;
  Traits::int_type curr = 0;
  std::string encoding;
  mutable std::stack<Level> tags;
  std::string lastKey;
  wchar_t (*conFun)(wchar_t) = nullptr;
  std::vector<u_char> base64data;
  Base64Reader base64;
  EncryptedData encryptedData;
  mutable int xmlEncState = 0;
  std::unique_ptr<mobs::BinaryIstBuf> binaryBuffer;
  std::unique_ptr<CryptBufBase> binaryFilt;
  std::unique_ptr<std::istream> binaryFiltStream;
  std::unique_ptr<std::istream> binaryStream;
  bool inParse2Lt = false;
  bool inParse2LtWork = false;
  bool bomCheck = false;
  bool try64 = false;
  bool in64 = false;
  bool useBase64 = false;
  bool running = false;
  bool paused = false;
  bool reedEof = true;
  bool endOfFile = false;
  bool nonblocking = false;
  std::map<std::wstring, std::wstring> entities = {
      {L"lt", L"<"},
      {L"gt", L">"},
      {L"amp", L"&"},
      {L"quot", L"\""},
      {L"apos", L"\'"}
  };
};



}

#endif

