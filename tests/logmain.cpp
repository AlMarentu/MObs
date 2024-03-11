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

#include "logging.h"
#include "mchrono.h"

#include <stdio.h>
#include <sstream>
#include <iostream>
#include <getopt.h>


using namespace std;
using namespace logging;
using namespace mobs;




int main(int argc, char **argv) {
  int anzahl = 1;
  char c = getopt(argc, argv, "hn:");
  switch (c) {
    case 'h':
      cout << "Usage: testlogging [-n <number of seconds>]" << endl;
      return 0;
    case 'n':
      anzahl = atoi(optarg);
      break;
  }


  cerr << "START " << endl;
  try {
    LogMultiStream log("test.log");
    for (int i = 0; i < anzahl; i++) {
      cerr << "LOG " << i << endl;
      MTime t = MTimeNow();
      log << to_string_ansi(t, MF3) + string(" Test ") << getpid() << endl;
#ifdef _WIN32
      Sleep(1000);
#else
      ::sleep(1);
#endif
    }
    MTime t = MTimeNow();
    log << to_string_ansi(t, MF3) + string(" Test ") << getpid() << endl;
  } catch (exception &e)
  {
    cerr << "EXCEPTION " << e.what() << endl;
    return 1;
  }
  return 0;
}

