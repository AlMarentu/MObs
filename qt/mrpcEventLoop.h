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

/** \file mrpcEventLoop.h
\brief  Hilfsfunktionen für QT-MRPC-Client */

#ifndef MOBS_MRPCEVENTLOOP_H
#define MOBS_MRPCEVENTLOOP_H

#include "mobs/objgen.h"
#include "mobs/mrpcsession.h"

#include <QtCore/QObject>
#include <QtNetwork/QtNetwork>
#include <QtCore/QEventLoop>
#include <QtWidgets/QProgressDialog>

template<typename T>
std::unique_ptr<T> moveIfTypeMatches(std::unique_ptr<mobs::ObjectBase> &obj) {
  if (auto p = dynamic_cast<T *>(obj.get())) {
    auto tmp = std::unique_ptr<T>(p);
    obj.release();
    return std::move(tmp);
  }
  return {};
}




class MrpcClientData;

/** \brief MRPC-Client auf Basis des Signal-SLot-Mechanismusses von Qt
 *
 */
class MrpcClient : public QObject {
  Q_OBJECT
  friend class MrpcEventLoop;
public:
  MrpcClient(mobs::MrpcSession &clientsession);
  ~MrpcClient();

  std::streamsize in_avail();
  void kill();
  void close();
  void query(const mobs::ObjectBase *queryObj);

  std::string server();

  mobs::ObjectBase *lastResult() const;
  std::unique_ptr<mobs::ObjectBase> getLastResult();
  void releaseResult();
  std::ostream &outByteStream();
  std::streamsize closeOutByteStream();

  QElapsedTimer elapsed;
  static std::string privateKey;
  static std::string software;
  static std::string tmpPass;
  static std::string keyId;
  static void setPrivateKey(const std::string &softwareName, const std::string &id, const std::string &priv, const std::string &passwd);


public Q_SLOTS:
  void connected();
  void disconnected();
  void readyRead();
  void doParse();
  void bytesWritten(qint64 bytes);
  void errorOccurred(QAbstractSocket::SocketError);
  void cancel();

  Q_SIGNALS:
    void result(const mobs::ObjectBase *);
    void queryResult(std::unique_ptr<mobs::ObjectBase> &);
    void dataAvail();
    void fileProgress(const QObject *, size_t, size_t);
    void requestDone(const std::string &server);

private:
  std::unique_ptr<MrpcClientData> data;
  void error();
};

class MrpcEventLoopData;
/** \brief Qt modaler Event-Loop der eine MRPC-Kommunikation kapselt und eine Progress-Bar anzeigt
 *
 */
class MrpcEventLoop : public QEventLoop {
  Q_OBJECT
  public Q_SLOTS:
    void result(const mobs::ObjectBase *);
    //void queryResult(const mobs::ObjectBase *);
    //void dataAvail(); == quit()
    void cancel();
    void requestDone(const std::string &server);
    void setProgress(const QObject *, size_t, size_t);

  Q_SIGNALS:
    void mrpcKill();

public:
  explicit MrpcEventLoop(QWidget *parent = nullptr, int step = 100);
  ~MrpcEventLoop() override;
  // TODO wenn Client (session) vorhanden und idle, dann neues Kommando schicken
  std::shared_ptr<MrpcClient> startClient(mobs::MrpcSession &clientSession, const mobs::ObjectBase *obj = nullptr,
    std::istream *outfile = nullptr, std::streamsize ofSize = 0);

  /// entfernt die gespeicherten MrpcClients
  void clear();

  void startProgress(QWidget *parentWindow);
  void endProgress();

  std::map<std::string, std::unique_ptr<mobs::ObjectBase>> getResults();

  template<typename T>
  /// liefert das Objekt vom gewünschten Typ oder eine Exception
  std::unique_ptr<T> getResult();

  void waitForAnswer(int maxPercent = 0);
  int sequence = -1;
  bool abbruch = false;
  std::string error;
  int stepMaxPercent = 100;
  int currentMaxPercent = 0;
  int maxPercent = 99;
protected:
  bool waitReqDone = false;
private:
  static int loopCounter;
  QProgressDialog *progress = nullptr;
  MrpcEventLoopData *data = nullptr;
  QTcpSocket *socket;
};

template<typename T>
std::unique_ptr<T> MrpcEventLoop::getResult() {
  auto results = getResults();
  if (results.size() != 1)
    THROW("result size != 1");
  auto p = moveIfTypeMatches<T>(results.begin()->second);
  if (not p) {
    if (results.begin()->second)
      THROW("result was " << results.begin()->second->getObjectName());
    THROW("result not found");
  }
  return std::move(p);
}


#endif //MOBS_MRPCEVENTLOOP_H
