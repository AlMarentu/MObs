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


#include "infxtools.h"
#include <stdio.h>

int infx_connect(db, user, pwd)
EXEC SQL BEGIN DECLARE SECTION;
PARAMETER const char *db;
PARAMETER const char *user;
PARAMETER const char *pwd;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
  char conn[16];
EXEC SQL END DECLARE SECTION;
  static int n = 0;
  sprintf(conn, "con%d", ++n);
  EXEC SQL connect to :db as :conn user :user using :pwd with concurrent transaction;
  int e = SQLCODE;
  if (e < 0)
    return e;
  if (e != 0)
    return 0;
  return n;
}

//ifx_getcur_conn_name()
void infx_set_connection(int n)
{
  EXEC SQL BEGIN DECLARE SECTION;
  char conn[16];
  EXEC SQL END DECLARE SECTION;
  sprintf(conn, "con%d", ++n);
  EXEC SQL set connection :conn;
}

void infx_disconnect(int n)
{
  EXEC SQL BEGIN DECLARE SECTION;
  char conn[16];
  EXEC SQL END DECLARE SECTION;
  sprintf(conn, "con%d", n);
  EXEC SQL disconnect :conn;
}


int infx_execute(stmt)
EXEC SQL BEGIN DECLARE SECTION;
PARAMETER const char *stmt;
EXEC SQL END DECLARE SECTION;
{
  int e = 0;
  EXEC SQL prepare d_id from :stmt;
  if ((e = SQLCODE))
    return e;
  EXEC SQL execute d_id;
  if ((e = SQLCODE))
    return e;
  EXEC SQL free d_id;
  return SQLCODE;
}

int infx_count(stmt, cnt)
EXEC SQL BEGIN DECLARE SECTION;
PARAMETER const char *stmt;
PARAMETER long *cnt;
EXEC SQL END DECLARE SECTION;
{
  int e = 0;
  EXEC SQL prepare c_id from :stmt;
  if ((e = SQLCODE))
    return e;
  EXEC SQL execute c_id into :*cnt;
  if ((e = SQLCODE))
    return e;
  EXEC SQL free c_id;
  return SQLCODE;
}


int infx_exec_desc(stmt, sqlda_ptr)
EXEC SQL BEGIN DECLARE SECTION;
PARAMETER const char *stmt;
PARAMETER struct sqlda *sqlda_ptr;
EXEC SQL END DECLARE SECTION;
{
  int e = 0;
  EXEC SQL prepare p_id from :stmt;
  if ((e = SQLCODE))
    return e;
  EXEC SQL execute p_id using descriptor sqlda_ptr;

  return SQLCODE;
}




int infx_query(stmt, cursname, prepname, desc_ptr)
EXEC SQL BEGIN DECLARE SECTION;
PARAMETER const char *stmt;
PARAMETER const char *cursname;
PARAMETER const char *prepname;
PARAMETER struct sqlda **desc_ptr;
EXEC SQL END DECLARE SECTION;
{
EXEC SQL BEGIN DECLARE SECTION;
struct sqlda *sqlda_ptr;
EXEC SQL END DECLARE SECTION;

 int e = 0;
  EXEC SQL prepare :prepname from :stmt;
  if ((e = SQLCODE))
    return e;
  EXEC SQL declare :cursname cursor for :prepname;
  if ((e = SQLCODE))
    return e;
  EXEC SQL describe :prepname into sqlda_ptr;
  *desc_ptr = sqlda_ptr;
  return SQLCODE;
}

int infx_fetch(cursname, sqlda_ptr)
EXEC SQL BEGIN DECLARE SECTION;
PARAMETER const char *cursname;
PARAMETER struct sqlda *sqlda_ptr;
EXEC SQL END DECLARE SECTION;
{
  EXEC SQL fetch :cursname using descriptor sqlda_ptr;
  return SQLCODE;
}

int infx_open_curs(cursname)
EXEC SQL BEGIN DECLARE SECTION;
PARAMETER const char *cursname;
EXEC SQL END DECLARE SECTION;
{
  EXEC SQL open :cursname;
  return SQLCODE;
}

void infx_remove_curs(cursname, prepname)
EXEC SQL BEGIN DECLARE SECTION;
PARAMETER const char *cursname;
PARAMETER const char *prepname;
EXEC SQL END DECLARE SECTION;
{
  EXEC SQL close :cursname;
  EXEC SQL free :cursname;
  EXEC SQL free :prepname;
}

/* 0 - estimated number of rows returned */
/* 1 - serial value after insert or  ISAM error code */
/* 2 - number of rows processed */
/* 3 - estimated cost */
/* 4 - offset of the error into the SQL statement */
/* 5 - rowid after insert  */
int infx_processed_rows()
{
  return sqlca.sqlerrd[2];
}

int infx_isam_or_serial()
{
  return sqlca.sqlerrd[1];
}

const char *infx_error_msg2()
{
  return sqlca.sqlerrm;
}

int infx_check_warn1()
{
  return sqlca.sqlwarn.sqlwarn0 == 'W' && sqlca.sqlwarn.sqlwarn1 == 'W';
}

