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

/** \file xmlwriter.h
\brief Klasse zur Ausgabe von XML in Dateien oder als String in verschiedenen Zeichensätzen */


#ifndef MOBS_XMLWRITER_H
#define MOBS_XMLWRITER_H

#include "logging.h"
#include "objtypes.h"
#include "csb.h"

#include<stack>
#include<exception>
#include<iostream>
#include <codecvt>


namespace mobs {

class XmlWriterData;

/*! \class XmlWriter
 \brief Einfacher XML-Writer.
 \code
 \endcode
 */
class XmlWriter {
public:
  /// Verfügbare Charsets für die Ausgabe
  enum charset { CS_utf8, CS_utf8_bom, CS_utf16_le, CS_utf16_be, CS_iso8859_1, CS_iso8859_9, CS_iso8859_15 };
  /// \brief Konstruktor bei Verwendung mit std::wosteram
  ///
  /// Wird ein frisch geöffneter \c std::wofstream übergeben, so wird bei CS_utf16 sowie CS_utf8_bom ein BOM erzeugt,
  /// ansonsten kann auch \c std::wstringstream übergeben werden, dann sind aber nur CS_utf8 oder CS_iso* erlaubt
  /// \see writeHead
  /// @param str zu beschreibender stream
  /// @param c Charset für die Ausgabe
  /// @param indent zum Abschalten von Einrückung und whitespace
  explicit XmlWriter(std::wostream &str, charset c = CS_utf8_bom, bool indent = true);
  /// Konstruktor bei Verwendung mit internen Buffer
  ///
  /// als Charsets sind nur UTF-8 ohne BOM und ISO* erlaubt
  /// \see getString
  /// \see clearString
  explicit XmlWriter(charset c = CS_utf8, bool indent = true);
  ~XmlWriter();
  /// Schreibe XML-Header, bei Files auch BOM
  void writeHead();
  /// Schreibe eine Start-Tag
  void writeTagBegin(const std::wstring &tag);
  /// Schreibe ein Attribut/Werre-Paar
  void writeAttribute(const std::wstring &attribute, const std::wstring &value);
  /// Schreibe einen Wert
  void writeValue(const std::wstring &value);
  /// Schreibe ein CDATA-Element
  void writeCdata(const std::wstring &value);
  /// Schreibe ein CDATA-Element mit Base64
  void writeBase64(const std::vector<u_char> &value);
  /// Schreibe eine Ende-Tag
  void writeTagEnd(bool forceNoNulltag = false);
  /// Schreibe einen Kommentar
  void writeComment(const std::wstring &comment, bool inNewLine = true);
  /// Anzeige der aktuellen Ebene
  int level() const;
  /// Anzeige der Ebene ab der Verschlüsselung aktiv ist, sonst 0
  int cryptingLevel() const;
  /// an dieser Stelle darf ein Attribute verwendet werden
  bool attributeAllowed() const;
  /// lesen des XML-Ergebnisses im gewählten Charset, nur bei Verwendung des internen Buffers
  std::string getString() const;
  /// löschen des internen Buffers
  void clearString();
  /// setzte ein XML-Prefix
  void setPrefix(const std::wstring &pf);

  void startEncrypt(CryptBufBase *cbbp);
  void stopEncrypt();

  std::wstring valueToken; ///< Wenn nicht leer, dann als Attributname für Values verwenden
  std::wstring version = L"1.0"; ///< Version für Header
  bool standalone = true; ///< Angabe für Header
  bool escapeControl = false; ///< ersetze Zeichen wie "\n" durch "&#xa;"

private:
  XmlWriterData *data;
};




}

#endif
