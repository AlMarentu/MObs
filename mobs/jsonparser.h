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

/** \file jsonparser.h
\brief EInfacher JSON-Parser */

#ifndef MOBS_JSONPARSER_H
#define MOBS_JSONPARSER_H

#include "logging.h"
#include<stack>
#include<exception>
#include<iostream>

namespace mobs {

/*! \class JsonParser
    \brief Einfacher JSON-Parser.
    Virtuelle Basisklasse mit Callback-Funktionen.
\code
concept JsonParser {
    JsonParser(const std::string &input);
 
    // Starte Parser
    void parse();

    // Callback Funktionen
    void Key(const std::string &value);
    void Value(const std::string &value, bool charType);
    void StartArray();
    void EndArray();
    void StartObject();
    void EndObject();
 };
\endcode
*/




/// Einfacher JSON-Parser - Basisklasse
class JsonParser  {
public:
  /*! Generiere Parser-Objekt
   @param input JSON Text zum Parsen
   */
  JsonParser(const std::string &input) : buffer(input) {
    pos1 = pos2 = 0;
  };
  virtual ~JsonParser() { };
  
  /** \brief Liefert JSON-Puffer und aktuelle Position für detaillierte Fehlermeldung
   @param pos Position des Fehlers im Json-Buffer
   @return Buffer des zu parsenden Textes
   */
  const std::string &info(size_t &pos) const {
    pos = pos1;
    return buffer;
  };

  /** \brief Callback funktion für gelesenes Key-Element
   @param value Name des Schlüssels
  */
  virtual void Key(const std::string &value) = 0;
  /** \brief Call backfunktion für gelesenes Wert-Elementt
   @param value Name des Wertes
   @param charType true, wenn Wert in Quotes eingeschlossen war
  */
  virtual void Value(const std::string &value, bool charType) = 0;
  /** \brief Call backfunktion für Start eines Arrays
  */
  virtual void StartArray() = 0;
  /** \brief Call backfunktion für Ende eines Arrays
  */
  virtual void EndArray() = 0;
  /** \brief Call backfunktion für Start eines Objektes
  */
  virtual void StartObject() = 0;
  /** \brief Call backfunktion für Ende eiines Objektes
   */
  virtual void EndObject() = 0;
  /// \brief Starte den Parser
  /// \throw runtime_error     Im Fehlerfall werden exceptions geworfen.
  void parse() {
    TRACE("");
    std::string element;
    bool expectKey = true;
    bool expectEnd = false;
    char expectDelimiter = ' '; // ' '..Key-Elemet
    for (;pos1 < buffer.length();)
    {
      switch (peek())
      {
        case ' ':
        case '\n':
        case '\t':
        case '\r':
          eat();
          continue;
        case '[':
          eat();
          if (expectDelimiter != ' ' or ( not tags.empty() and expectKey))
            throw std::runtime_error(u8"unexpected '{'");
          tags.push('[');
          StartArray();
          expectKey = false;
          expectEnd = true;
          expectDelimiter = ' ';
          break;
        case '{':
          eat();
          if (expectDelimiter != ' ' or ( not tags.empty() and expectKey))
            throw std::runtime_error(u8"unexpected '{'");
          tags.push('{');
          StartObject();
          expectKey = true;
          expectEnd = true;
          expectDelimiter = ' ';
          break;
        case ']':
          eat();
          if (tags.empty() or tags.top() != '[')
            throw std::runtime_error(u8"unexpectet ']'");
          if (not expectEnd)
            throw std::runtime_error(u8"missing element");
          tags.pop();
          EndArray();
          expectKey = (tags.empty() or tags.top() == '{');
          expectDelimiter = ',';
          expectEnd = true;
          break;
        case '}':
          eat();
          if (tags.empty() or tags.top() != '{' or expectDelimiter == ':')
            throw std::runtime_error(u8"unexpectet '}'");
          if (not expectEnd)
            throw std::runtime_error(u8"missing element");
          tags.pop();
          EndObject();
          expectKey = (tags.empty() or tags.top() == '{');
          expectEnd = true;
          expectDelimiter = ',';
          break;
        case '"':
          if (not element.empty())
          element = "";
          for (;;)
          {
            eat();
            parse2QUOT();
            element += getValue();
            if (peek() == '"')
              break;
            if (peek() == '\\')
            {
              eat();
              element += peek();
            }
          }
          eat();
          if (expectKey)
          {
            Key(element);
            expectKey = false;
            expectEnd = false;
            expectDelimiter = ':';
          }
          else
          {
            Value(element, true);
            expectEnd = true;
            expectDelimiter = ',';
          }
          break;
        case ',':
          if (expectDelimiter != ',')
            throw std::runtime_error(u8"unexpected ','");
          expectKey = (tags.empty() or tags.top() == '{');
          expectEnd = false;
          expectDelimiter = ' ';
          eat();
          break;
        case ':':
          if (expectDelimiter != ':')
            throw std::runtime_error(u8"unexpected ':'");
          expectKey = false;
          expectEnd = false;
          expectDelimiter = ' ';
          eat();
          break;
        case '+':
        case '-':
        case '.':
        case '_':
        case 'A' ... 'Z':
        case '0' ... '9':
        case 'a' ... 'z':
          element = "";
          for (;pos2 != std::string::npos;)
          {
            switch (peek())
            {
              case '+':
              case '-':
              case '.':
              case '_':
              case 'A' ... 'Z':
              case '0' ... '9':
              case 'a' ... 'z':
                element += peek();
                eat();
                continue;
            }
            break;
          }
          if (expectKey)
          {
            Key(element);
            expectEnd = false;
            expectDelimiter = ':';
          }
          else
          {
            Value(element, false);
            expectEnd = true;
            expectDelimiter = ',';
          }
          break;
        default:
          throw std::runtime_error(u8"unmatching char " + std::to_string(peek()));
      }
      // TODO whitespace bis EOF
    }
    if (not tags.empty())
      throw std::runtime_error(u8"unexpected EOF: missing " + std::string((tags.top() == '[' ? "]":"}")));
    if (not expectDelimiter)
      throw std::runtime_error(u8"unexpected EOF: missing " + std::string((tags.top() == '[' ? "]":"}")));
    if (not expectEnd)
      throw std::runtime_error(u8"missing element");
  };
  
  private:

  void parse2QUOT() {
    pos2 = buffer.find_first_of("\\\"", pos1);
//    cerr << "PGT " << pos2 << " " << pos1 << endl;
    if (pos2 == std::string::npos)
      throw std::runtime_error("Syntax end of quote missing");
  };
  std::string getValue() {
      if (pos2 == std::string::npos)
        throw std::runtime_error(u8"unexpected EOF");
    size_t p = pos1;
    pos1 = pos2;
    return std::string(&buffer[p], pos2-p);
  };
  /// Verwaltet den Zischenraum zwischen den <..Tags..>
  void eat(char c) {
    if (buffer[pos1] != c)
      throw std::runtime_error(u8"Expected " + std::to_string(c) + " got " + std::to_string(buffer[pos1]));
    pos1++;
  };
  void eat() { pos1++; };
  char peek() {
    if (pos1 >= buffer.length())
      throw std::runtime_error(u8"unexpected EOF");
    //cerr << "Peek " << buffer[pos1] << " " << pos1 << endl;
    return buffer[pos1];
  };
  const std::string &buffer;
  size_t pos1, pos2;  // current / search pointer for parsing
  std::stack<char> tags;
//  std::string lastKey;
  
};
}
  
#endif
