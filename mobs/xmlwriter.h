// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2024 Matthias Lautner
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
#include<codecvt>
#include<memory>

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
  ///
  /// Unter Windows sollte sollte der Stream mit ios::binary geöffnet worden sein, ansonsten werden NL durch CR/LF ersetzt,
  /// was bei UTF16 zu kaputten Dateien führt.
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
  /** \brief Schreibe XML-Header, bei Files auch BOM
   *
   * Wiederholtes Aufrufen setzt den level wieder auf 0 und erzeugt einen neuen Header
   */
  void writeHead();
  /// Schreibe eine Start-Tag
  void writeTagBegin(const std::wstring &tag);
  /// Schreibe ein Attribut/Werte-Paar
  void writeAttribute(const std::wstring &attribute, const std::wstring &value);
  /// Schreibe einen Wert
  void writeValue(const std::wstring &value);
  /// Schreibe ein CDATA-Element
  void writeCdata(const std::wstring &value);
  /// Schreibe ein CDATA-Element mit Base64
  void writeBase64(const std::vector<u_char> &value);
  /// Schreibe ein CDATA-Element mit Base64
  void writeBase64(const u_char *value, uint64_t size);
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
  /// lesen des XML-Ergebnisses im gewählten Charset, nur bei Verwendung des internen Buffers
  std::wstring getWString() const;
  /// löschen des internen Buffers
  void clearString();
  /// setzte ein XML-Prefix
  void setPrefix(const std::wstring &pf);
  /// Encryption aktivieren \see moba::CryptBufBase
  void startEncrypt(CryptBufBase *cbbp);
  /// Encryption wieder beenden
  void stopEncrypt();
  /// darunter liegenden Buffer flushen
  void sync();
  /// ein Zeichen in den Ausgabe-Stream schreiben
  void putc(wchar_t c);
  /// Tag-Stack aktualisieren (nach wiederholtem writeHead())
  void pushTag(const std::wstring &tag);
  /// Schreibe binäre Daten in den output stream, bei bedarf mit Verschlüsselung; der delimiter wird vorangestellt
  std::ostream &byteStream(const char *delimiter = nullptr, CryptBufBase *cbbp = nullptr);
  /// Schließe den binären stream wieder und liefere die Anzahl übertragener Bytes wenn unterstützt, ansonsten -1
  std::streamsize closeByteStream();

  std::wstring valueToken; ///< Wenn nicht leer, dann als Attributname für Values verwenden
  std::wstring version = L"1.0"; ///< Version für Header
  bool standalone = true; ///< Angabe für Header
  /// ersetze Zeichen "\n" und "\r" durch ihre HTML-Sequenz, ebenso das erste sowie letzte führende/folgende Leerzeichen
  bool escapeControl = true;
private:
  std::unique_ptr<XmlWriterData> data;

};




}

#endif
