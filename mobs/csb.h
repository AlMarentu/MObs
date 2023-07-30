// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2020,2021 Matthias Lautner
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

/** \file csb.h
 *
 *  \brief Erweiterung für Streams mit Verschlüsselung und Base64
 */

#ifndef MOBS_CSB_H
#define MOBS_CSB_H

#include <iostream>
#include <array>
#include <memory>

namespace mobs {

class CryptBufBaseData;
class CryptOstrBuf;
class CryptIstrBuf;

/** \brief Stream-Buffer Basisklasse als Plugin für mobs::CryptIstrBuf oder mobs::CryptOstrBuf
 *
 * Die Basisklasse unterstützt die Umwandlung des Datenstreomes nach Base64
 */
class CryptBufBase : public std::basic_streambuf<char> {
  friend class CryptIstrBufData;
  friend class CryptOstrBufData;
  friend class BinaryIstBuf;
public:
  using Base = std::basic_streambuf<char>; ///< Basis-Typ
  using char_type = typename Base::char_type;  ///< Element-Typ
  using Traits = std::char_traits<char_type>; ///< Traits-Typ
  using int_type = typename Base::int_type; ///< zugehöriger int-Typ

  CryptBufBase();
  /// \private
  CryptBufBase(const CryptBufBase &) = delete;
  CryptBufBase &operator=(const CryptBufBase &) = delete;

  ~CryptBufBase() override;

  /// Bezeichnung des Algorithmus der Verschlüsselung
  virtual std::string name() const { return ""; }
  /** \brief Anzahl der Empfänger für diese Verschlüsselung
   *
   * @return Anzahl der vorhandenen Empfängereinträge
   */
  virtual size_t recipients() const { return 0; }
  /** \brief Abfrage der Id des Empfängers
   *
   * @param pos Nr. des Empfänger-Eintrages 0..(recipients() -1)
   * @return Id
   */
  virtual std::string getRecipientId(size_t pos) const {return ""; }
  /** \brief Key zum Entschlüsseln der Nachricht eines Empfängers
   *
   * @param pos Nr. des Empfänger-Eintrages 0..(recipients() -1)
   * @return Schlüssel in base64, falls vorhanden
   */
  virtual std::string getRecipientKeyBase64(size_t pos) const { return ""; }

  /// \private
  int_type overflow(int_type ch) override;
  /// \private
  int_type underflow() override;

  /// Ausgabe abschließen
  virtual void finalize();

  /** \brief (de-)aktiviert den Base64-Modus
   *
   * @param on Base64 einschalten
   */
  void setBase64(bool on);

  /// Abfrage des Status
  bool bad() const;

  /// Übergeordneten Ausgabe-stream setzen
  void setOstr(std::ostream &ostr);

  /// Übergeordneten Eingabe-stream setzen
  void setIstr(std::istream &istr);

  /** \brief Anzahl der zu lesenden Bytes begrenzen
   *
   * Nachdem die Anzahl bytes die aus dem übergeordneten Buffer gelesen
   * wurde, geht der Stream in den end-of-ile-Zustand
   * @param bytes Anzahl oder -1 für unbegrenzt
   */
  void setReadLimit(std::streamsize bytes = -1);

  /** \brief setze Trennzeichen
   *
   * Über ein Trennzeichen, kann das Leseverhalten des Buffers gesteuert werden, so dass ein read_avail() nur bis zum
   * Trenner liest. Der Delimiter wird immer zu Beginn der Folgeblocks zurückgeliefert
   * @param delim unsigned char des Trenners oder Traits::eof() für aus
   */
  void setReadDelimiter(char_type delim);

  ///  Delimiter-Funktion abschalten
  void setReadDelimiter();

  /// Anzahl der noch zu lesenden Bytes, falls begrenzt
  std::streamsize getLimitRemain() const;

  /// \private
  class base64 {
    template <typename T>
    friend std::basic_ostream<T> &operator<< (std::basic_ostream<T> &s, const CryptBufBase::base64 &);
//    template <typename T>
//    friend std::basic_istream<T> &operator>> (std::basic_istream<T> &s, CryptBufBase::base64 &b);
  public:
    void set(mobs::CryptIstrBuf *rdp) const;
    explicit base64(bool on) : b(on) {}
  protected:
    void set(mobs::CryptOstrBuf *rdp) const;
    bool b;
  };

  class final { };

protected:
  /// \private
  std::streamsize showmanyc() override;
  /// \private
  int sync() override;
  /// \private
  std::streamsize xsputn( const char_type* s, std::streamsize count ) override;
  /// \private
  void doWrite( const char_type* s, std::streamsize count);
  /// \private
  std::streamsize doRead(char *s, std::streamsize count);
  /// \private
  std::streamsize canRead() const;
  /// \private
  bool isGood() const;
  /// \private
  void setBad();
  /// \private
  std::unique_ptr<CryptBufBaseData> data;
};

/** \brief Stream-Buffer zur Basisklasse CryptBufBase als null-device
 *
 * Ignoriert jegliche Ausgabe, liefert leere Eingabe. Kann verwendet werden um eine verschlüsseltes Element zu ignorieren.
 */
class CryptBufNull : public CryptBufBase {
public:
  using Base = std::basic_streambuf<char>; ///< Basis-Typ
  using char_type = typename Base::char_type;  ///< Element-Typ
  using Traits = std::char_traits<char_type>; ///< Traits-Typ
  using int_type = typename Base::int_type; ///< zugehöriger int-Typ

  CryptBufNull() = default;
  ~CryptBufNull() override = default;;
  /// Bezeichnung des Algorithmus der Verschlüsselung
  std::string name() const override { return u8"null"; }

  /// \private
  int_type overflow(int_type ch) override { return ch; };
  /// \private
  int_type underflow() override { return Traits::eof(); };
};


class CryptIstrBufData;

/** \brief Stream-Buffer Wrapper-Klasse für std::istream Input-Streams des Typs wchar_t
 *
 * Der CryptIstrBuf liest seine Daten aus einen beliebigen std::istream (wie z.B.: std::ifstream oder std::stringstream)
 * Die Umwandlung des Datenstromes von Base64 ist standardmäßig,
 * für Verschlüsselte Daten werden Plugins benötigt.
 *
 * Wird mit imbue die locale geändert, wenn bereits der Lese-Buffer gefüllt ist,
 * so muss der Buffer nach neuer Kodierung immer noch ausreichend sein. Auch darf zuvor kein Zeichensatz
 * verwendet worden sein, der Multibyte-Character verwendet.
 * ISO nach beliebig geht, UTF8 bzw UTF16 darf nicht mehr geändert werden
 */
class CryptIstrBuf : public std::basic_streambuf<wchar_t> {
  friend class BinaryIstBuf;
public:
  using Base = std::basic_streambuf<wchar_t>; ///< Basis-Typ
  using char_type = typename Base::char_type; ///< Element-Typ
  using Traits = std::char_traits<char_type>; ///< Traits-Typ
  using int_type = typename Base::int_type; ///< zugehöriger int-Typ

  /** \brief Konstruktor
   *
   * der Verschlüsselungs-Plugin muss mit new erzeugt werden, die Freigabe erfolgt automatisch
   * @param istr std::istream aus dem die Daten gelesen werden
   * @param cbbp Plugin für Verschlüsselung, bei Fehlen wird CryptBufBase verwendet
   */
  explicit CryptIstrBuf(std::istream &istr, CryptBufBase *cbbp = nullptr);
  /// \private
  CryptIstrBuf(const CryptIstrBuf &) = delete;
  CryptIstrBuf &operator=(const CryptIstrBuf &) = delete;
  ~CryptIstrBuf() override;

  /// \private
  int_type underflow() override;

  /// Referenz auf internen istream
  std::istream &getIstream();

  /// Zugriff auf den Stream-Buffer des Plugins (ist immer gültig)
  CryptBufBase *getCbb();

  /// Tauscht den verwendeten Crypt-Buffer aus
  void swapBuffer(std::unique_ptr<CryptBufBase> &newBuffer);

  /// Abfrage des Status
  bool bad() const;

protected:
  std::streamsize showmanyc() override;

protected:
//  pos_type seekpos(pos_type pos, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override;
  /// \private
  pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which) override;
  /// \private
  void imbue(const std::locale& loc) override;

private:
  std::unique_ptr<CryptIstrBufData> data;
};


/** \brief istream-buffer der einen wistream einliest, bis ein für Base64 ungültiges Zeichen kommt
 *
 * wird für das Auswerten verschlüsselter Elemente im XML-Streams verwendet.
 */
class Base64IstBuf : public std::basic_streambuf<char> {
public:
  using Base = std::basic_streambuf<char>; ///< Basis-Typ
  using char_type = typename Base::char_type;  ///< Element-Typ
  using Traits = std::char_traits<char_type>; ///< Traits-Typ
  using int_type = typename Base::int_type; ///< zugehöriger int-Typ

  /// Konstruktor mit Übergabe des zu verarbeitenden wistream
  explicit Base64IstBuf(std::wistream &istr);

  /// \private
  int_type underflow() override;

protected:
  std::streamsize showmanyc() override;

private:
  std::wistream &inStb;
  char_type ch{};
  bool atEof = false;
};



class BinaryIstrBufData;

/** \brief istream-buffer der von CryptIstrBuf abgeleitet wird um einen Block binärer Daten zu extrahieren
 *
 * wird für das Auswerten binärer Elemente in (CryptIstrBuf) XML-Streams verwendet. Diese müssen in einer UTF-8 locale vorliegen.
 * Durch einen Delimiter 0x80 wird der UTF-8-Stream beendet, danach kann von diesem Stream der BinaryIstBuf abgeleitet werden,
 * der dann eine Fixe Anzahl Bytes (inklusive dem 0x80) liest. Im Anschluss  muss der stream mit clear() zurückgesetzt werden
 * und kann wieder normal weiterarbeiten.
 * Der BinaryIstBuf darf nur von CryptIstrBuf abgeleitet werden, wenn diese eim eof-state sind. Ansonsten werden die bereits im Buffer
 * befindlichen Zeichen übersprungen.
 * Solange der BinaryIstBuf aktiv ist darf der zugehörige CryptIstrBuf nicht vernichtet werden.
 */
class BinaryIstBuf : public std::basic_streambuf<char> {
public:
  using Base = std::basic_streambuf<char>; ///< Basis-Typ
  using char_type = typename Base::char_type;  ///< Element-Typ
  using Traits = std::char_traits<char_type>; ///< Traits-Typ
  using int_type = typename Base::int_type; ///< zugehöriger int-Typ

  /// Konstruktor eines istream buffers, der aus einem CryptIstrBuf len (> 0) Bytes binäre Daten extrahiert
  BinaryIstBuf(CryptIstrBuf &ci, size_t len);

  ~BinaryIstBuf() override;

  /// \private
  int_type underflow() override;

protected:
  std::streamsize showmanyc() override;

private:
  std::unique_ptr<BinaryIstrBufData> data;
};




class CryptOstrBufData;

/** \brief Stream-Buffer Wrapper Klasse für std::ostream Output-Streams des Typs wchar_t
 *
 * Der CryptOstrBuf schreibt seine Daten in einen beliebigen std::ostream (wie z.B.: std::ofstream oder std::stringstream)
 * Die Umwandlung des Datenstromes nach Base64 ist standardmäßig,
 * für Verschlüsselung werden Plugins benötigt.
 *
 */
class CryptOstrBuf : public std::basic_streambuf<wchar_t> {
public:
  using Base = std::basic_streambuf<wchar_t>; ///< Basis-Typ
  using char_type = typename Base::char_type; ///< Element-Typ
  using Traits = std::char_traits<char_type>; ///< Traits-Typ
  using int_type = typename Base::int_type; ///< zugehöriger int-Typ

  /** \brief Konstruktor
   *
   * @param ostr std::ostream in den die Daten geschrieben werden
   * @param cbbp Plugin für Verschlüsselung, bei Fehlen wird CryptBufBase verwendet
   */
  explicit CryptOstrBuf(std::ostream &ostr, CryptBufBase *cbbp = nullptr);
  /// \private
  CryptOstrBuf(const CryptOstrBuf &) = delete;
  CryptOstrBuf &operator=(const CryptOstrBuf &) = delete;
  ~CryptOstrBuf() override;
  /// \private
  int_type overflow(int_type ch) override;

  /// Zugriff auf internen ostream
  std::ostream &getOstream();

  /** \brief Schließe des Stream ab.
   *
   */
  void finalize();

  /// Zugriff auf den Stream-Buffer des Plugins (ist immer gültig)
  CryptBufBase *getCbb();

  /// Tauscht den verwendeten Crypt-Buffer aus
  void swapBuffer(std::unique_ptr<CryptBufBase> &newBuffer);

protected:
  /// \private
  int sync() override;
//  pos_type seekpos(pos_type pos, std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) override;
  /// \private
  pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which) override;
  /// \private
  void imbue(const std::locale& loc) override;

private:
  std::unique_ptr<CryptOstrBufData> data;
};


template <typename T>
std::basic_ostream<T> &operator<< (std::basic_ostream<T> &s, const CryptBufBase::base64 &b) {
  s.flush();
//  auto r = dynamic_cast<mobs::CryptOstrBuf *>(s.rdbuf());
//  if (r and s.tellp() > 0) r->finalize();
  b.set(dynamic_cast<mobs::CryptOstrBuf *>(s.rdbuf()));
  return s;
}

template <typename T>
std::basic_istream<T> &operator>> (std::basic_istream<T> &s, CryptBufBase::base64 &b) {
  b.set(dynamic_cast<mobs::CryptIstrBuf *>(s.rdbuf()));
  return s;
}

template <typename T>
std::basic_ostream<T> &operator<< (std::basic_ostream<T> &&s, const CryptBufBase::final &f) {
  auto r = dynamic_cast<mobs::CryptOstrBuf *>(s.rdbuf());
  if (r) r->finalize();
  return s;
}

#if 0
/** \brief Stream-Buffer zur Basisklasse CryptBufBase als forward-device
 *
 * Eingabe und Ausgebe werden 1:1 durchgereicht. Kann verwendet werden um ein unverschlüsseltes Element zu erzeugen.
 */
class CryptBufNone2 : public CryptBufBase {
public:
  using Base = std::basic_streambuf<char>; ///< Basis-Typ
  using char_type = typename Base::char_type;  ///< Element-Typ
  using Traits = std::char_traits<char_type>; ///< Traits-Typ
  using int_type = typename Base::int_type; ///< zugehöriger int-Typ

  CryptBufNone2() = default;
  ~CryptBufNone2() override = default;;
  /// Bezeichnung des Algorithmus der Verschlüsselung
  std::string name() const override { return u8"none"; }

  /// \private
  int_type overflow(int_type ch) override;
  /// \private
  int_type underflow() override;

protected:
  std::streamsize showmanyc() override;

private:
  char_type ch{};
};
#endif

}


#endif //MOBS_CSB_H
