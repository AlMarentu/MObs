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
#include "base64.h"

#include<stack>
#include<exception>
#include<iostream>
#include <codecvt>


namespace mobs {

/*! \class XmlParser
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
 void NullTag(const std::string &element);
 void Attribute(const std::string &element, const std::string &attribut, const std::string &value);
 void Value(const std::string &value);
 void Cdata(const char *value, size_t len);
 void StartTag(const std::string &element);
 void EndTag(const std::string &element);
 void ProcessingInstruction(const std::string &element, const std::string &attribut, const std::string &value);
 };
 \endcode
 */




/// Einfacher XML-Parser - Basisklasse
class XmlParser  {
public:
  /*! Konstruktor der XML-Parser Basisklasse
   @param input XML-Text der geparst werden soll   */
  XmlParser(const std::string &input) : Xml(input) {
    pos1 = pos2 = posS = posE = 0;
  };
  virtual ~XmlParser() { };
  
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
   @param value Inhalz der Verarbeitungsanweisung
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
                                      /// Verwaltet den Zischenraum zwischen den <..Tags..>
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
  /// @param posS StartPosition
  /// @param posE EndePosition
  std::string decode(size_t posS, size_t posE) {
    // Da in XML Die Zeicxhe & und < immer escaped (in HTML) werden müssen, kann ein Rückwandlung
    // immer erfolgen, da ein '&' ansonsten nicht vorkommen sollte
    std::string result;
    for (;;)
    {
      size_t pos = Xml.find('&', posS);
      if (pos < posE) // & gefunden
      {
        result += std::string(&Xml[posS], pos-posS);
        posS = pos +1;
        pos = Xml.find(';', posS);
        if (pos < posE and pos < posS + 16) // Token &xxxx; gefunden
        {
          std::string tok = std::string(&Xml[posS], pos-posS);
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
            posS = pos +1;
            continue;
          }
        }
        // wenn nichts passt dann einfach übernehmen
        result += '&';
      }
      else
      {
        result += std::string(&Xml[posS], posE-posS);
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







/*! \class XmlParserW
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



class XmlParserW  {
public:
  /*! Konstruktor der XML-Parser Basisklasse für std::wstring
   
     es kann z.B. ein \c std::wifstream dienen oder ein \c std::wistringstream übergeben werden
          Als Zeichensätze sind UTF-8, UTF-16, ISO8859-1, -9 und -15 erlaubt; Dateien dürfen mit einem BOM beginnen
   @param input XML-stream der geparst werden soll   */
  XmlParserW(std::wistream &input) : istr(input) { };
  virtual ~XmlParserW() { };
  /*! \brief Liefert XML-Puffer und aktuelle Position für detaillierte Fehlermeldung
   @param pos Position des Fehlers im Xml-Buffer
   @return zu parsender Text-Buffer
   */
  std::string info(size_t &pos) const {
    pos = istr.tellg();
    std::cerr << "CUR = " << curr << " " << pos << std::endl;
    std::wstring w;
    wchar_t c;
    w += curr;
    for (int i = 0; i < 50; i++) {
      if ((c = istr.get()) <= 0)
        break;
      else
        w += c;
    }
    std::cerr << mobs::to_string(w) << std::endl;
    return to_string(w);
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
  virtual void Attribute(const std::string &element, const std::string &attribut, const std::wstring &value) = 0;
  /** \brief Callback-Function: Ein Inhalt eines Tags
   @param value Inhalt des Tags
   */
  virtual void Value(const std::wstring &value) = 0;
  /** \brief Callback-Function: Ein CDATA-Elemet (optional) ansonsten wird auf \c Value abgebildet
   @param value Inhalt des Tags
   */
  virtual void Cdata(const std::wstring &value) { Value(value); }
  /** \brief Callback-Function: Ein CDATA-Elemet mit base64 codiertem Inhalt
   
      nur, wenn setBase64(true) gesetzt wurde
   @param base64 Inhalt des base64 codierden Wertes
   */
  virtual void Base64(const std::vector<u_char> &base64) { }
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
   @param value Inhalz der Verarbeitungsanweisung
   */
  virtual void ProcessingInstruction(const std::string &element, const std::string &attribut, const std::wstring &value) = 0;
  
  /// ist beim Parsen das Ende erreicht
  bool eof() const { return endOfFile; }
  /// verlasse bein nächsten End-Tag den parser
  void stop() { running = false; }
  /// Aktiviere automatische base64 erkennung
  /// \see Base64
  void setBase64(bool b) { useBase64 = b; }
  /// Starte den Parser
  void parse() {
    TRACE("");
    if (not running)
    {
      eat();  // erstes Zeichen einlesen
              /// BOM bearbeiten
      if (curr == 0xff)
      {
        std::locale lo;
        if ((curr = istr.get()) == 0xfe)
        {
          lo = std::locale(istr.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>);
          istr.imbue(lo);
          encoding = u8"UTF-16"; // (LE)
        }
        else
          throw std::runtime_error(u8"Error in BOM");
        eat();
      }
      else if (curr == 0xfe)
      {
        std::locale lo;
        if ((curr = istr.get()) == 0xff)
          lo = std::locale(istr.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::codecvt_mode(0)>);
        else
          throw std::runtime_error(u8"Error in BOM");
        istr.imbue(lo);
        encoding = u8"UTF-16"; // (BE)
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
      if (curr != '<')
        throw std::runtime_error(u8"Syntax Head");
      // BOM überlesen
      if (not buffer.empty() and buffer != L"\0xEF\0xBB\0xBF" and buffer != L"\ufeff")
      {
        for (auto c:buffer) std::cerr << '#' <<  int(c) << std::endl;
        throw std::runtime_error("invalid begin of File");
      }
      buffer.clear();
      running = true;
    }
    // eigentliches Parsing
    while (curr == '<')
    {
      if (not running)
      {
        running = true;
        return;
      }
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
          throw std::runtime_error("missing end tag");
        if (lastKey == element)
        {
          decode(saved);
          Value(saved);
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
          saveValue();  // nur whitespace prüfen
          parse2CD();
          if (try64) {
            if (b64Cnt > 0 and b64Cnt < 4)
              base64Check('=');
            Base64(base64);
          }
          else
            Cdata(buffer.substr(2));
          base64.clear();
          try64 = false;
          clearValue();
          lastKey = "";
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
            char c = peek();
            if (c == '"')
              eat('"');
            else
              eat('\'');
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
                conFun = from_iso_8859_15;
              else if (encoding == u8"ISO-8859-9")
                conFun = from_iso_8859_9;
              else if (encoding != u8"ISO-8859-1")
                LOG(LM_WARNING, u8"encoding mismatch: " << encoding << " using ISO-8859-1");
            }
            else if (encoding != mobs::to_string(v))
              LOG(LM_WARNING, u8"encoding mismatch: " << encoding << " " << mobs::to_string(v));

          }
          ProcessingInstruction(element, a, v);
        }
        eat('>');
        parse2LT();
        continue;
      }
      // Parse Element-Beginn
      parse2GT();
      decode(buffer);
      std::string element = to_string(buffer);
      if (element.empty())
        throw std::runtime_error("missing begin tag ");
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
        decode(buffer);
        std::string a = to_string(buffer);
        eat('=');
        wchar_t c = peek();
        if (c == '"')
          eat('"');
        else
          eat('\'');
        parse2Char(c);
        decode(buffer);
        Attribute(element, a, buffer);
        eat(c);
      }
      lastKey = element;
    }
    saveValue();
//    std::cerr << "LAST BUFFER " <<  mobs::to_string(buffer) << "|" << curr << std::endl;
//    for (auto i:buffer) std::cerr << "#" << int(i) << std::endl;
    if (curr != -1)
      throw std::runtime_error(u8"Syntax error");
    // nur noch für check Whitespace bis eof
    saveValue();
    if (not tags.empty())
      throw std::runtime_error(u8" expected tag at EOF: " + tags.top());
    endOfFile = true;
  };
  
private:
  void parse2LT() { parse2Char('<'); }
  void parse2GT() {
    buffer.clear();
    if (curr <= 0)
      return;
    buffer += curr;
    while ((curr = istr.get()) > 0) {
      if (std::wstring(L"/ <>=\"'?!").find(curr) != std::wstring::npos )
        break;
//      std::cout << mobs::to_string(curr);
      buffer += curr;
    }
    if (curr < 0)
      throw std::runtime_error("Syntax");
  };
  void parse2Char(wchar_t c) {
    buffer.clear();
    if (curr == c or curr <= 0)
      return;
    if (try64)
      base64Check(curr);
    else
      buffer += curr;
    while ((curr = istr.get()) > 0) {
      if (curr == c)
        break;
      if (try64)
        base64Check(curr);
      else
        buffer += curr;
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
      if (peek() < 0)
        throw std::runtime_error("Syntax");
    }
  };
  void parse2CD() {
//    pos2 = Xml.find(L"]]>", pos1);
    buffer.clear();
    for (;;) {
      std::wstring tmp = buffer;
      base64Start();
      parse2Char(']');
      buffer = tmp + buffer;
      if (peek() == ']') {
        eat();
        if (peek() == ']') {
          for (;;) {
            eat();
            if (peek() == '>')
              return;
            if (try64)
              throw std::runtime_error("base64 error");
            base64.clear();
            if (peek() != ']')
              break;
          }
        }
      }
      if (try64)
        throw std::runtime_error("base64 error");
      base64.clear();
      if (peek() < 0)
        throw std::runtime_error("Syntax");
    }
  };
  void clearValue() { saved.clear(); }; // der Zwischenraum fand Verwendung
                                      /// Verwaltet den Zischenraum zwischen den <..Tags..>
  void saveValue() {
    // wenn nicht verwendet, darf es nur white space sein
    if (not saved.empty())
    {
      size_t p = saved.find_first_not_of(L" \n\r\t");
      if (p != std::wstring::npos)
        throw std::runtime_error(u8"unexpected char");
    }
    saved = buffer;
  };
  void eat(wchar_t c) {
    buffer += curr;
    if (curr != c)
      throw std::runtime_error(u8"Expected " + mobs::to_string(c) + " got " + mobs::to_string(curr));
    curr = istr.get();
//    pos1++;
  };
  void eat() {
//    pos1++;
    buffer += curr;
    curr = istr.get();
  };
  wchar_t peek() {
    if (curr < 0)
      throw std::runtime_error(u8"unexpected EOF");
    //cerr << "Peek " << Xml[pos1] << " " << pos1 << endl;
    return curr;
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
  void base64Check(wchar_t c) {
    int v = from_base64(c);
    if (v < 0) {
      if (c == '=') { // padding
        v = 0;
        switch (b64Cnt) {
          case 3:
            base64.push_back(b64Value >> 10);
            base64.push_back((b64Value >> 2) & 0xff);
            // fall into
          case 100:
            b64Cnt = 999; // Wenn noch ein = kommt -> fehler
            break;
          case 2:
            base64.push_back(b64Value >> 4);
            b64Cnt = 100; // es darf noch 1 = kommen
            break;
          case 1: throw std::runtime_error("base64 unexpecrted end");
          default: throw std::runtime_error("base64 unexpecrted padding");
        }
      }
      else
        throw std::runtime_error("base64 padding");
    }
    else if (v < 64) {
      if (b64Cnt > 3)
        throw std::runtime_error("base64 invalid");
      b64Value = (b64Value << 6) + v;
      if (++b64Cnt == 4) {
        base64.push_back(b64Value >> 16);
        base64.push_back((b64Value >> 8) & 0xff);
        base64.push_back(b64Value & 0xff);
        b64Cnt = 0;
        b64Value = 0;
      }
    }
  }
  void base64Start() {
    if (not useBase64)
      return;
    base64.clear();
    b64Cnt = 0;
    b64Value = 0;
    try64 = true;
  }
  
  std::wistream &istr;
  std::wstring buffer;
  std::wstring saved;
  wchar_t curr = 0;
  std::string encoding;
  std::stack<std::string> tags;
  std::string lastKey;
  wchar_t (*conFun)(wchar_t) = nullptr;
  std::vector<u_char> base64;
  int b64Value = 0;
  int b64Cnt = 0;
  bool try64 = false;
  bool useBase64 = false;
  bool running = false;
  bool endOfFile = false;
  
};



}

#endif
