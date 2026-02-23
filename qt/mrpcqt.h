// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2026 Matthias Lautner
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


#ifndef MOBS_MRPCQT_H
#define MOBS_MRPCQT_H

#include "ui_mainwindow.h"
#include <string>

#include "mrpcsession.h"

namespace mobs {
class ObjectBase;
}

class MainWindow: public QMainWindow {
  Q_OBJECT
  public:

public Q_SLOTS:
  void starte();
  void fileRead();
  void fileSend();
  void queryResult(std::unique_ptr<mobs::ObjectBase> &);

Q_SIGNALS:
  void progress(const QObject *, size_t, size_t);


public:
  MainWindow();

private:
  Ui_MainWindow ui;
  mobs::MrpcSession clientSession;
  int error = 0;
  int ok = 0;
};

#endif //MOBS_MRPCQT_H