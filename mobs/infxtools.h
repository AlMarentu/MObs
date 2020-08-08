/// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
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

/** \file infxtools.h
 \brief Hilfsfunktionen für Zugriff auf IBM Informix

 IBM Informix is a registered trademark of IBM Corp.
 \see www.ibm.com
 */
#ifndef MOBS_INFXTOOLS_H
#define MOBS_INFXTOOLS_H

#ifdef __cplusplus
extern "C" {
#endif

/** \brief Aufbau einer Informix Datenbankverbindung
 *
 * @param db DB-Name aus SQL-Hosts
 * @param user Benutzer
 * @param pwd Passwd
 * @return Connection Nr, falls >= 0, Informix-Error wenn < 0
 */
int infx_connect(const char *db, const char *user, const char *pwd);

/** \brief setzen einer aktiven Verbindung
 *
 * @param n Connection-Nr.
 */
void infx_set_connection(int n);

/** \brief Schließen einer Datenbankverbindung
 *
 * @param n Connection-Nr.
 */
void infx_disconnect(int n);

/** \brief Ausführen eines SQL-Statements
 *
 * @param stmt Statement
 * @return SQLCODE
 */
int infx_execute(const char *stmt);

/** \brief Ausführen eines count-SQL-Statements
 *
 * @param stmt Statement
 * @param cnt Count-Result
 * @return SQLCODE
 */
int infx_count(const char *stmt, long *cnt);

/** \brief Ausführen eines SQL-Statements mit Descriptor
 *
 * @param stmt Statement
 * @param sqlda_ptr Descriptor-Struktur
 * @return SQLCODE
 */
int infx_exec_desc(const char *stmt, struct sqlda *sqlda_ptr);

/** \brief Ausführen eines SQL-Statements mit Cursor
 *
 * @param stmt Statement
 * @param cursname Name des Cursors
 * @param prepname Name der Prepare-Id
 * @param sqlda_ptr Descriptor-Struktur
 * @return SQLCODE
 */
int infx_query(const char *stmt, const char *cursname, const char *prepname, struct sqlda **sqlda_ptr);

/** \brief Ausführen eines Cursors
 *
 * @param cursname Name des Cursors
 * @return SQLCODE
 */
int infx_open_curs(const char *cursname);

/** \brief Holen des nächsten Elementes
 *
 * @param cursname Name des Cursors
 * @param sqlda_ptr Descriptor-Struktur
 * @return SQLCODE oder 100 == eof()
 */
int infx_fetch(const char *cursname, struct sqlda *sqlda_ptr);

/** \brief Aufräumen des Cursors
 *
 * @param cursname Name des Cursors
 * @param prepname Name der Prepare-Id
 */
void infx_remove_curs(const char *cursname, const char *prepname);

/** \brief Anzahl der bearbeiteten Datensätze
 *
 * @return Anzahl
 */
int infx_processed_rows();

/** \brief Liefert den dynamischen Teil der Fehlermeldung
 *
 * @return Fehler-Detail-Info
 */
const char *infx_error_msg2();

/** \brief Prüft die Warnung1
 *
 * Warnung 1 ist gesetzt, wenn bei einem Fetch ein Feldinhalt abgeschnitten werden muss
 * @return 1 wen Warnung 1 gesetzt, sonst 0
 */
int infx_check_warn1();

#ifdef __cplusplus
}
#endif

#endif //MOBS_INFXTOOLS_H
