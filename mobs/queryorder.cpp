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


#include "queryorder.h"
#include "logging.h"

namespace mobs {

class QuerySortData {
public:
  class SortInfo {
  public:
    uint pos;
    int sort;
  };

  std::map<const mobs::MemberBase *, QuerySortData::SortInfo> info;
  uint cnt = 0;
  bool asc = true;
};


QueryOrder::SortSwitch QueryOrder::ascending{};
QueryOrder::SortSwitch QueryOrder::descending{};


bool QueryOrder::sortInfo(const mobs::MemberBase &mem, uint &pos, int &dir) const {
  auto i = data->info.find(&mem);
  if (i == data->info.end())
    return false;
  QuerySortData::SortInfo &k = i->second;
  pos = k.pos;
  dir = k.sort;

  return true;
}

void QueryOrder::add(const MemberBase &mem) {
  LOG(LM_INFO, "ADD " << mem.name());
  QuerySortData::SortInfo &k = data->info[&mem];
  k.pos = data->cnt++;
  k.sort = data->asc ? 1 : -1;
}

QueryOrder::QueryOrder() {
  data = new QuerySortData;
}

QueryOrder::~QueryOrder() {
  delete data;
}

void QueryOrder::directionAsc(bool asc) {
  data->asc = asc;
}


QueryOrder &operator<<(QueryOrder &k, QueryOrder::SortSwitch &s) {
  if (&s == &QueryOrder::ascending)
    k.directionAsc(true);
  if (&s == &QueryOrder::descending)
    k.directionAsc(false);
  return k;
}




}