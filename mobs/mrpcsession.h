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

/** \file mrpcsession.h
\brief Session-Info für Client-Server Modul mrpc */


#ifndef MOBS_MRPCSESSION_H
#define MOBS_MRPCSESSION_H


#include <string>

namespace mobs {

class MrpcSession {
public:
  MrpcSession() = default;
  MrpcSession(const std::string &hostname) : server(hostname) {};
  std::string server;
  std::vector<u_char> sessionKey;
  u_int sessionId = 0;
  time_t last = 0;
  std::string info;
};

}
#endif //MOBS_MRPCSESSION_H
