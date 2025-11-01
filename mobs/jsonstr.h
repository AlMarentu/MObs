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
//ifi
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

/** \file xmlwriter.h
\brief Klasse zur Ausgabe von JSON in Dateien oder als String */


#ifndef MOBS_JSONSTR_H
#define MOBS_JSONSTR_H

#include "objtypes.h"
#include "mchrono.h"

namespace mobs {


class JsonStr_Data;
class ObjectBase;
class MemberBase;

/** \brief Output-Stream für JSON-Ausgabe in einen std::ostream
 *
 * Beispiel:
 * \code
 *   std::stringstream str;
 *   using JS = mobs::JsonStream;
 *   JS js(str);
 *   js << JS::ObjectBegin
 *      << JS::Tag("Zahl") << 1
 *      << JS::Tag("bool") << true
 *      << JS::Tag("nix") << nullptr
 *      << JS::Tag("Name") << "mobs"
 *      << JS::ObjectEnd;
 *   if (not js.isRoot())
 *     ... Fehler
 *   cout << str.str;
 * \endcode
 */
class JsonStream {
public:
  /** \brief Klasse die einen Names-Tag für JSON fasst
   *
   */
  class Tag {
    friend JsonStream;
  public:
    /** \brief Namen-Tag für JSON
     *
     * @param tag Name des Elements/des Objektes
     */
    Tag(const std::string &tag) noexcept : t(tag) {}; // NOLINT(*-explicit-constructor)
  private:
    const std::string &t;
  };
  /** \brief Operatoren für die JSON-Ausgabe, die Objekte sowie Array beginnen oder beenden
   *
   */
  enum Operator { ArrayBegin, ArrayEnd, ObjectBegin, ObjectEnd };
   /** \brief Konstruktor der JSON-Ausgabeklasse
    *
    * @param s Rekerenz auf einen Ausgabestream
    * @param cthn Modifier für die Ausgabe
    * \throws std::runtime_error wenn ein Struktur-Fehler auftritt. Die Fehlerüberprüfung ist nicht vollständig
    */
  JsonStream(std::ostream &s, // NOLINT(*-explicit-constructor)
             const mobs::ConvObjToString &cthn = mobs::ConvObjToString().exportJson().doIndent());
  ~JsonStream();

  /// Streamoperator
  JsonStream &operator<<(const std::wstring &t);
  /// Streamoperator
  JsonStream &operator<<(const std::string &t);
  /// Streamoperator
  JsonStream &operator<<(const MDate &t);
  /// Streamoperator
  JsonStream &operator<<(const MTime &t);
  /// Streamoperator
  JsonStream &operator<<(const char *t);
  /// Streamoperator
  JsonStream &operator<<(const wchar_t *t);
  /// Streamoperator
  JsonStream &operator<<(char t);
  /// Streamoperator
  JsonStream &operator<<(wchar_t t);
  /// Streamoperator
  JsonStream &operator<<(int t);
  /// Streamoperator
  JsonStream &operator<<(u_int64_t t);
  /// Streamoperator
  JsonStream &operator<<(int64_t t);
  /// Streamoperator
  JsonStream &operator<<(bool t);
  /// Streamoperator
  JsonStream &operator<<(nullptr_t t);
  /// Streamoperator
  JsonStream &operator<<(const mobs::ObjectBase &t);
  /// Streamoperator
  JsonStream &operator<<(const mobs::MemberBase &t);

  /// Streamoperator
  JsonStream &operator<<(const Tag &tag);
  /// Streamoperator
  JsonStream &operator<<(Operator o);

  /** \brief Neues Objekt beginnen.
   * Alternativ kann der Operator  << ObjectBegin verwendet werden
   */
  void objectBegin();

  /** \brief Neues Objekt beginnen.
 * Alternativ kann der Operator  << ObjectBegin verwendet werden
 */
  void objectEnd();

  /** \brief Neues Array beenden.
 * Alternativ kann der Operator  << ArrayBegin verwendet werden
 */
  void arrayBegin();

  /** \brief Neues Array beenden.
 * Alternativ kann der Operator  << ArrayEnd verwendet werden
 */
  void arrayEnd();

  /** \brief Ausgabeformat das im Konstruktor gesetzt wurde abfragen
   *
   * @return  Ausgabeformat
   */
  const ConvObjToString &cts() const;

  /** \brief Zeigt an, ob die Klammerebene bei Null ist.
   *
   * Die ist bei leerem Dokument der Fall u nd sollte am Ende ebenfalls vorliegen.
   * @return true, wenn Klammerebene null ist
   */
  bool isRoot() const;

private:
  JsonStr_Data *data;
};

}

#endif // MOBS_JSONSTR_H