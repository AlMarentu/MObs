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

/** \file audittrail.h
 \brief  Objektdefinitionen für Audit Trail */


#ifndef MOBS_AUDITTRAIL_H
#define MOBS_AUDITTRAIL_H

#include <string>
#include "objgen.h"
#include "mchrono.h"


namespace mobs {


/// Datenbank-Objekt für Audit-Trail
class AuditChanges : public mobs::ObjectBase {
public:
  ObjInit(AuditChanges);
  MemVar(std::string, field, LENGTH(100));
  MemVar(std::string, value, LENGTH(200));
  MemVar(bool,        nullVal);

};

/// Datenbank-Objekt für Audit-Trail
class AuditObjects : public mobs::ObjectBase {
public:
  ObjInit(AuditObjects);

  MemVar(int, initialVersion); // wenn 0, dann Startwerte, sonst alte Werte
  MemVar(bool, destroy);  // wenn true, letzter Wert
  MemVar(std::string, objectName, LENGTH(20));
  MemVar(std::string, objectKey, LENGTH(80));
  MemVector(AuditChanges, changes, COLNAME(auditChanges));

  /// fügt gesplittete changes.values wieder zusammen
  void unsplit();
};

/// Datenbank-Objekt für Audit-Trail
class AuditActivity : public mobs::ObjectBase {
public:
  ObjInit(AuditActivity);

  MemVar(MTime, time, KEYELEMENT1, DBCOMPACT);
  MemVar(int, userId, KEYELEMENT2);
  MemVar(std::string, jobId, LENGTH(36));
  MemVar(std::string, comment, USENULL, LENGTH(200));
  MemVector(AuditObjects, objects, COLNAME(auditObjects));

protected:
  // Callback
  void loaded() override { for (auto &i:objects) i.unsplit(); }


};



}

#endif //MOBS_AUDITTRAIL_H
