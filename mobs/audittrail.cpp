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


#include "audittrail.h"

namespace mobs {


void AuditObjects::unsplit() {
  if (changes.size() <= 1)
    return;
  size_t j = 0;
  for (size_t i = 1; i < changes.size(); i++) {
    std::string s = changes[j].value();
    if (not s.empty() and s[s.length() - 1] == '\\' and changes[i].field() == changes[j].field()) {
      s.replace(s.length() - 1, 1, changes[i].value());
      changes[j].value(s);
    } else {
      j++;
      if (i != j)
        changes[j] = changes[i];
    }
  }
  changes.resize(j + 1);
}


}