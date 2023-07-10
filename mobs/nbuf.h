// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2023 Matthias Lautner
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

/** \file aes.h
 *
 *  \brief Plugins und Funktionen für AES-Verschlüsselung
 */

#ifndef MOBS_NBUF_H
#define MOBS_NBUF_H

#include "csb.h"
#include <vector>
#include <memory>

#ifdef __MINGW32__
typedef unsigned char u_char;
#endif

namespace mobs {

class CryptBufNoneData;

/** \brief Stream-Buffer zur Basisklasse CryptBufBase ohne Verschlüsselung
 *
 * Dient als Plugin für mobs::CryptIstrBuf oder mobs::CryptOstrBuf
 *
 * Nur du Debug-Zwecken
 *
 */
class CryptBufNone : public CryptBufBase {
public:
  using Base = std::basic_streambuf<char>; ///< Basis-Typ
  using char_type = typename Base::char_type;  ///< Element-Typ
  using Traits = std::char_traits<char_type>; ///< Traits-Typ
  using int_type = typename Base::int_type; ///< zugehöriger int-Typ

  /** \brief Konstruktor
   *
   * @param id Id des Empfängers (falls für Export benötigt)
   */
  explicit CryptBufNone(const std::string &id = "");
  ~CryptBufNone() override;
   /// Bezeichnung der Verschlüsselung
  std::string name() const override { return u8"none"; }
  /// Anzahl der Empfänger-Ids ist immer 1
  size_t recipients() const override { return 1; }
  /// liefert bei pos==0 die Id des des Empfängers wie im Konstruktor angegeben
  std::string getRecipientId(size_t pos) const override;

  /// \private
  int_type overflow(int_type ch) override;
  /// \private
  int_type underflow() override;

  /// \private
  void finalize() override;

protected:
  std::streamsize showmanyc() override;

//  virtual int sync() override;
//  virtual std::streamsize xsputn( const char_type* s, std::streamsize count ) override;

private:
  std::unique_ptr<CryptBufNoneData> data;

  int underflowWorker(bool nowait);

};


}


#endif //MOBS_NBUF_H
