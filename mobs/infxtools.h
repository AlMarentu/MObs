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

int infx_connect(const char *db, const char *user, const char *pwd);

void infx_set_connection(int n);
void infx_disconnect(int n);

int infx_execute(const char *stmt);
int infx_count(const char *stmt, long *cnt);
int infx_exec_desc(const char *stmt, struct sqlda *sqlda_ptr);


int infx_query(const char *stmt, const char *cursname, const char *prepname, struct sqlda **sqlda_ptr);
int infx_open_curs(const char *cursname);
int infx_fetch(const char *cursname, struct sqlda *sqlda_ptr);
void infx_remove_curs(const char *cursname, const char *prepname);

int infx_processed_rows();
const char *infx_error_msg2();

#ifdef __cplusplus
}
#endif

#endif //MOBS_INFXTOOLS_H
