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


#include <QApplication>
#include "mrpcqt.h"

#include <fstream>

#include "crypt.h"
#include "mrpcEventLoop.h"


#include "mobs/logging.h"
#include "mobs/mrpcsession.h"
#include  <sys/stat.h>

class MrpcPerson : virtual public mobs::ObjectBase {
public:
  ObjInit(MrpcPerson);
  MemVar(std::string, name);
};
ObjRegister(MrpcPerson);

class MrpcPing : virtual public mobs::ObjectBase {
public:
  ObjInit(MrpcPing);
  MemVar(std::string, comment);
};
ObjRegister(MrpcPing);

class Progress : virtual public mobs::ObjectBase
{
public:
  ObjInit(Progress);

  MemVar(int, percent);
  MemVar(std::string, comment);
};
ObjRegister(Progress);

class LangeListe : virtual public mobs::ObjectBase
{
public:
  ObjInit(LangeListe);

  MemVar(std::string, name);
  MemVar(std::string, comment);
};
ObjRegister(LangeListe);

class LoadFile : virtual public mobs::ObjectBase
{
public:
  ObjInit(LoadFile);

  MemVar(long int, length);
  MemVar(std::string, name);
};
ObjRegister(LoadFile);

class BigDat : virtual public mobs::ObjectBase
{
public:
  ObjInit(BigDat);

  MemVar(long int, length);
  MemVar(std::string, name);
};
ObjRegister(BigDat);



MainWindow::MainWindow() {
  ui.setupUi(this);
  connect(this, &QObject::destroyed, this, &MainWindow::deleteLater);
  connect(ui.pushButtonStart, &QPushButton::clicked, this, &MainWindow::starte);
  connect(ui.pushButtonStartFileOut, &QPushButton::clicked, this, &MainWindow::fileSend);
  connect(ui.pushButtonStartFileIn, &QPushButton::clicked, this, &MainWindow::fileRead);

  clientSession = mobs::MrpcSession("localhost:4444");
  clientSession.publicServerKey = "../srv.pub";
}

void MainWindow::queryResult(std::unique_ptr<mobs::ObjectBase> &obj) {
  LOG(LM_INFO, "RCV: " << obj->to_string());
  if (auto p = moveIfTypeMatches<Progress>(obj)) {
    emit progress(sender(), p->percent(), 100);
  }
}

void MainWindow::starte() {
  LOG(LM_INFO, "START");
  MrpcEventLoop mrpcLoop(this);

  LangeListe p1;
  p1.name("Picard");

  try {
    auto mrpc = mrpcLoop.startClient(clientSession, &p1);
    connect(mrpc.get(), &MrpcClient::queryResult, this, &MainWindow::queryResult);
    connect(this, &MainWindow::progress, &mrpcLoop, &MrpcEventLoop::setProgress);

    mrpcLoop.waitForAnswer();
    //auto res = mrpc->getLastResult();
    auto results = mrpcLoop.getResults();
    if (results.size() != 1)
      THROW("Fehler");
    LOG(LM_INFO, "MRPC RECEIVED " << results.begin()->first);
    auto p = moveIfTypeMatches<LangeListe>(results.begin()->second);
    if (p) {
      LOG(LM_INFO, "RESULT RECEIVED " << p->to_string());
      ok++;
    }
    else
      THROW("RESULT RECEIVED " << "NULL");

  } catch (std::exception &e) {
    LOG(LM_ERROR, "ERROR: " << e.what());
    error++;
  }
  ui.lineEditErr->setText(QString::number(error));
  ui.lineEditOk->setText(QString::number(ok));


}

void MainWindow::fileRead() {
  LOG(LM_INFO, "START");
  MrpcEventLoop mrpcLoop(this);

  LoadFile p1;

  try {
    auto mrpc = mrpcLoop.startClient(clientSession, &p1);
    connect(mrpc.get(), &MrpcClient::queryResult, this, &MainWindow::queryResult);
    connect(this, &MainWindow::progress, &mrpcLoop, &MrpcEventLoop::setProgress);

    mrpcLoop.waitForAnswer(20);
    //auto res = mrpc->getLastResult();
    //auto p = moveIfTypeMatches<LoadFile>(res);
    auto p = mrpcLoop.getResult<LoadFile>();
    if (p) {
      LOG(LM_INFO, "RESULT RECEIVED " << p->to_string());
      ok++;
    }
    else
      THROW("RESULT RECEIVED " << "NULL");

  } catch (std::exception &e) {
    LOG(LM_ERROR, "ERROR: " << e.what());
    error++;
  }
  ui.lineEditErr->setText(QString::number(error));
  ui.lineEditOk->setText(QString::number(ok));


}

void MainWindow::fileSend() {
  LOG(LM_INFO, "START");
  try {
    MrpcEventLoop mrpcLoop(this);

    BigDat p1;
    struct stat sbuf;
    if (::stat("log", &sbuf) != 0)
      THROW("stat failed, file log missing");
    p1.length(sbuf.st_size);
    p1.name("log");
    std::ifstream outfile("log");

    auto mrpc = mrpcLoop.startClient(clientSession, &p1, &outfile, p1.length());
    connect(mrpc.get(), &MrpcClient::queryResult, this, &MainWindow::queryResult);
    connect(this, &MainWindow::progress, &mrpcLoop, &MrpcEventLoop::setProgress);


    mrpcLoop.waitForAnswer();
    auto p = mrpcLoop.getResult<BigDat>();
    if (p) {
      LOG(LM_INFO, "RESULT RECEIVED " << p->to_string());
      ok++;
    }
    else
      THROW("RESULT RECEIVED " << "NULL");

  } catch (std::exception &e) {
    LOG(LM_ERROR, "ERROR: " << e.what());
    error++;
  }
  ui.lineEditErr->setText(QString::number(error));
  ui.lineEditOk->setText(QString::number(ok));

}


int main(int argc, char *argv[]) {
  // alle Key-Files liegen im selben Verzeichnis
  // neues Schlüsselpaar verwenden
  mobs::generateCryptoKey(mobs::CryptECprime256v1, "../qttest.priv", "../qttest.pub", "12345");
  MrpcClient::setPrivateKey("qttest", "qttest", "../qttest.priv", "12345");
  QApplication a(argc, argv);
  MainWindow main;
  main.show();
  return QApplication::exec();
}
