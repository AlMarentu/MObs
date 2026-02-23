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

#include <iomanip>
#include <ios>
#include <string>
#include <QtNetwork/QTcpSocket>

#include "mobs/logging.h"
#include "mobs/xmlread.h"
#include "mobs/mrpcsession.h"
#include "mobs/mrpcec.h"
#include "mrpcEventLoop.h"

#include "crypt.h"

#define ECC

#if 0
inline std::basic_ios<char> &operator<<(std::basic_ios<char> &s, QString q) {
  s << q.toStdString();
  return s;
}
#endif

namespace {

// Wandelt die QT-Socket-Slots in std::istream um
class BlockIstBuf : public std::basic_streambuf<char> {
public:
  using Base = std::basic_streambuf<char>;
  using char_type = typename Base::char_type;
  using Traits = std::char_traits<char_type>;
  using int_type = typename Base::int_type;

  BlockIstBuf() : Base() {}
  void addBuff(QByteArray &&move) {
    if (move.isEmpty())
      return;
    //LOG(LM_DEBUG, "BLOCK READ " << std::string(move.data(), move.size()));
    buffers.emplace_back(qMove(move));
    LOG(LM_DEBUG, "Buffers: " << buffers.size());
  }

  int_type underflow() override;

  inline std::streamsize avail() {
    return showmanyc();
  }

protected:
  std::streamsize showmanyc() override {
    std::streamsize res = 0;
    bool first = true;
    for (auto const &b:buffers)
    {
      if (not first or not initialized)
        res += b.size();
      else
        res += std::distance(Base::gptr(), Base::egptr());
      first = false;
    }
    return res;
  }



private:
  std::list<QByteArray> buffers;
  bool initialized = false;
};

BlockIstBuf::int_type BlockIstBuf::underflow() {
  LOG(LM_INFO, "BlockIstBuf::underflow called " << egptr() - gptr());
  if (initialized and not buffers.empty())
    buffers.erase(buffers.begin());
  if (buffers.empty())
  {
    LOG(LM_INFO, "BlockIstBuf: buffer underflow");
    return Traits::eof();
  }
  initialized = true;
  setg(buffers.front().data(), buffers.front().data(), buffers.front().data() + buffers.front().size());
  LOG(LM_DEBUG, "new Buffer " << egptr() - gptr());
  return Traits::to_int_type(*gptr());
}

// Wandelt einen std::ostream in asyncrone Socket-Calls um
class BlockOstBuf : public std::basic_streambuf<char> {
public:
  using Base = std::basic_streambuf<char>; ///< Basis-Typ
  using char_type = typename Base::char_type; ///< Element-Typ
  using Traits = std::char_traits<char_type>; ///< Traits-Typ
  using int_type = typename Base::int_type; ///< zugehöriger int-Typ

  /// default Konstruktor
  BlockOstBuf(QTcpSocket *sock) : Base(), socket(sock) {
    Base::setp(wrBuf.begin(), wrBuf.end());
  }


  /// \private
  int_type overflow(int_type ch) override;

protected:
  /// \private
  std::basic_streambuf<char>::pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which) override;
  /// \private
  int sync() override {
    LOG(LM_INFO, "BlockOstBuf::sync");
    overflow(Traits::eof());
    return socket ? 0 : -1;
  }

private:
  std::array<BlockOstBuf::char_type, 8192> wrBuf;
  std::streamsize wrPos = 0;
  QTcpSocket *socket = nullptr;
};

BlockOstBuf::int_type BlockOstBuf::overflow(BlockOstBuf::int_type ch) {
  if (not socket)
    return Traits::eof();
  auto sz = socket->write(Base::pbase(), std::distance(Base::pbase(), Base::pptr()));
  if (sz != std::distance(Base::pbase(), Base::pptr())) {
    LOG(LM_ERROR, "PART " << sz << " of " << std::distance(Base::pbase(), Base::pptr()));
    socket = nullptr;
    return Traits::eof();
  }
  Base::setp(wrBuf.begin(), wrBuf.end()); // buffer zurücksetzen
  if (not Traits::eq_int_type(ch, Traits::eof()))
    Base::sputc(Traits::to_char_type(ch));
  return ch;
}

std::basic_streambuf<char>::pos_type
BlockOstBuf::seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which) {
  if (which & std::ios_base::out) {
    if ((which & std::ios_base::in) or dir != std::ios_base::cur or off != 0)
      return pos_type(off_type(-1));
    return pos_type(wrPos + off_type(std::distance(Base::pbase(), Base::pptr())));
  }
  return {off_type(-1)};
}

}

// ===========================================================================

class MrpcEventLoopData {
  public:
    struct  MrpcInfo {
      std::shared_ptr<MrpcClient> mrpc;
      std::string host;
      size_t current = SIZE_T_MAX;
      size_t last{};
      bool active = true;
    };
    std::map<const QObject*, MrpcInfo> connections;
    //std::map<std::string, std::pair<int, int>> istMaxCnt;

};

// ===========================================================================

class MrpcClientData {
public:
  enum ConState {Offline, Connecting, Online, WaitingCon, Waiting, Result, Ready, SessionClosed, Error };
  MrpcClientData(mobs::MrpcSession &clientsession) : befserverSession(clientsession), socket(new QTcpSocket),
                      iBlkStr(), iBstr(&iBlkStr), oBlkStr(socket), oBstr(&oBlkStr),
                      xr(iBstr, oBstr, &befserverSession, true) { //, mobs::Mrpc::keep
    QString server = QString::fromStdString(befserverSession.host());
    u_int port = QString::fromStdString(befserverSession.port()).toUInt();
    LOG(LM_INFO, "MRPC  using server " << server.toStdString() << ":" << port);
    if (not server.isEmpty() and port > 0)
    {
      socket->setProxy(QNetworkProxy::NoProxy);
      socket->connectToHost(server, port);
      state = MrpcClientData::Connecting;
    }  else
      THROW("invalid server " << befserverSession.server);
  }


  ~MrpcClientData() {
    socket->deleteLater();
  }

  void flush() {
    xr.flush();
  }

  void close() {
    if (xr.isConnected() and socket->isValid()) {
      LOG(LM_INFO, "Close Server " << xr.session->server);
      xr.closeServer();
      state = Offline;
      socket->waitForBytesWritten(300);
      socket->close();
    }
  }

  void sendQuery() {
    if (queryObj)
    {
      xr.sendSingle(*queryObj, outFileSz);
      queryObj = nullptr;
      if (outFile) {
        serverStream = &xr.outByteStream();
        serverStream->flush();
        oBstr.flush(); // Syncen, damit 8K-Buffer laufen können
        doStream();
      }
      flush();
      queryObj = nullptr;
      LOG(LM_INFO, "Sending Query");
    }
  }

  void doStream() {
    LOG(LM_INFO, "doStream");
    if (outFile and serverStream and socket->bytesToWrite() < 8192) {
      char buf[8192+1];
      auto s = outFile->rdbuf()->sgetn(buf, sizeof(buf)-1);
      LOG(LM_INFO, "doStream got " << s);
      if (s > 0) {
        serverStream->write(buf, s);
        serverStream->flush();
      }
      else {
        s = xr.closeOutByteStream();
        serverStream = nullptr;
        outFile = nullptr;
        outFileSz = 0;
        LOG(LM_INFO, "Closing OutFile Stream");
      }
      oBstr.flush();
      LOG(LM_INFO, "doStream flush ");
    }
  }

  mobs::MrpcSession &befserverSession;
  QTcpSocket *socket;
  BlockIstBuf iBlkStr;
  std::istream iBstr;
  BlockOstBuf oBlkStr;
  std::ostream oBstr;
  mobs::MrpcEc xr;
  std::string errorMsg;

  const mobs::ObjectBase *queryObj = nullptr;

  ConState state = Offline;
  //std::function<size_t(const mobs::ObjectBase *obj)> attachmentSize = nullptr;
  //QTimer readTimer; // Timer für readyRead, da der socket nicht immer readyRead meldet
  std::istream *outFile = nullptr;
  ssize_t outFileSz = 0;
  std::ostream *serverStream = nullptr;

  ssize_t attachmentSz = 0;
  std::istream *attachmentIstr = nullptr;
  int reParseCnt = 0;

};



MrpcClient::MrpcClient(mobs::MrpcSession &clientsession)
{
  LOG(LM_INFO, "MrpcClient");
  elapsed.start();

  clientsession.keyName = MrpcClient::keyId;
  data = std::unique_ptr<MrpcClientData>(new MrpcClientData(clientsession));
  //data->loop = mrpcLoop;
  //data->readTimer.setSingleShot(true);
  //connect(&data->readTimer, &QTimer::timeout, this, &MrpcClient::readyRead);
  connect(data->socket, &QTcpSocket::connected, this, &MrpcClient::connected);
  connect(data->socket, &QTcpSocket::disconnected, this, &MrpcClient::disconnected);
  connect(data->socket, &QTcpSocket::readyRead, this, &MrpcClient::readyRead);
  connect(this, &MrpcClient::dataAvail, this, &MrpcClient::doParse);
  connect(data->socket, &QTcpSocket::bytesWritten, this, &MrpcClient::bytesWritten);
  connect(data->socket, &QAbstractSocket::errorOccurred, this, &MrpcClient::errorOccurred);
  //connect(mrpcLoop, &MrpcEventLoop::mrpcKill, this, &MrpcClient::cancel);
  //connect(this, &MrpcClient::requestDone, mrpcLoop, &MrpcEventLoop::requestDone);
}

MrpcClient::~MrpcClient() {
  if (data->socket->isOpen())
    data->socket->abort();
  //disconnect(&data->readTimer, &QTimer::timeout, this, &MrpcClient::readyRead);
  disconnect(data->socket, &QTcpSocket::connected, this, &MrpcClient::connected);
  disconnect(data->socket, &QTcpSocket::disconnected, this, &MrpcClient::disconnected);
  disconnect(data->socket, &QTcpSocket::readyRead, this, &MrpcClient::readyRead);
  disconnect(this, &MrpcClient::dataAvail, this, &MrpcClient::doParse);
  disconnect(data->socket, &QTcpSocket::bytesWritten, this, &MrpcClient::bytesWritten);
  disconnect(data->socket, &QAbstractSocket::errorOccurred, this, &MrpcClient::errorOccurred);
  //disconnect(data->loop, &MrpcEventLoop::mrpcKill, this, &MrpcClient::cancel);
  //disconnect(this, &MrpcClient::requestDone, data->loop, &MrpcEventLoop::requestDone);
};


std::string MrpcClient::privateKey; // mit tmpPass verschlüsselter privat Key
std::string MrpcClient::keyId;      // keyId ui privat key
std::string MrpcClient::software;
std::string MrpcClient::tmpPass;    // temporäre UUID zum Verschlüsseln in memory

std::streamsize MrpcClient::in_avail() {
  return data->iBlkStr.avail();
}

void MrpcClient::cancel()
{
  LOG(LM_INFO, "cancel, Avail=" << data->socket->bytesAvailable());
  data->state = MrpcClientData::Error;
  data->oBstr.exceptions(std::iostream::goodbit);
  data->iBstr.exceptions(std::iostream::goodbit);

#if 0
  // Error-Objekt senden
  mrpc::ReturnError obj;
  obj.error("CANCEL");
  data->xr.xmlOut(obj);
  data->xr.writer.sync();
#endif
  data->flush();
  data->socket->waitForBytesWritten(200);
  data->socket->abort();
  data->xr.session->sessionId = 0;
  data->xr.resultObj = nullptr;

  data->serverStream = nullptr;
  data->outFile = nullptr;
  data->outFileSz = 0;
  data->attachmentIstr = nullptr;
  data->attachmentSz = 0;

  emit result(nullptr);
}

void MrpcClient::kill()
{
  LOG(LM_INFO, "Kill,  Avail=" << data->socket->bytesAvailable());
  data->state = MrpcClientData::Error;

  data->socket->abort();
  data->xr.session->sessionId = 0;
  data->xr.resultObj = nullptr;

  data->serverStream = nullptr;
  data->outFile = nullptr;
  data->outFileSz = 0;
  data->attachmentIstr = nullptr;
  data->attachmentSz = 0;
  emit result(nullptr);
}


mobs::ObjectBase *MrpcClient::lastResult() const
{
  return data->xr.resultObj.get();
}

std::unique_ptr<mobs::ObjectBase> MrpcClient::getLastResult()
{
  return std::move(data->xr.resultObj);
}

void MrpcClient::releaseResult()
{
  data->xr.resultObj.release();
}




void MrpcClient::errorOccurred(QAbstractSocket::SocketError e)
{
  bool stop = false;
  std::string server = data->xr.session->server;
  if (e == QAbstractSocket::HostNotFoundError) {
    LOG(LM_INFO, "MrpcClient HostNotFoundError " << server);
    data->errorMsg = "host not found";
    data->state = MrpcClientData::Error;
  } else if (e == QAbstractSocket::RemoteHostClosedError) {
    LOG(LM_INFO, "MrpcClient Remote Host Closed, state = " << int(data->state) << " " << server);
    if (data->state > MrpcClientData::Offline and data->state < MrpcClientData::SessionClosed) {
      if (data->state > MrpcClientData::Online and in_avail() > 0)
        LOG(LM_ERROR, "MrpcClient error Remote Host Closed  buf=" << in_avail());
      else
        LOG(LM_ERROR, "MrpcClient error Remote Host Closed");
      stop = true;
      data->xr.session->sessionId = 0;
      data->state = MrpcClientData::Error;
      data->errorMsg = "remote host closed";
      //      data->xr.sessionId = 0;
      //      eventLoop->exit(0);
    }
  } else {
    LOG(LM_INFO, "MrpcClient error "  << server << " " << int(e) /*<< " " << socket->errorString()*/);
    data->errorMsg = "unknown error";
    data->state = MrpcClientData::Error;
  }
  if (stop)
  {
    data->xr.resultObj = nullptr;
    emit result(nullptr);
  }
}


void MrpcClient::setPrivateKey(const std::string &softwareName, const std::string &id, const std::string &priv, const std::string &passwd) {
  keyId = id;
  software = softwareName;
  tmpPass = mobs::gen_uuid_v4_p(); // temporäres Password für in memory Verschlüsselung
  privateKey = mobs::readPrivateKey(priv, passwd, tmpPass);
}

void MrpcClient::connected()
{
  LOG(LM_INFO, "connected " << data->befserverSession.server);
#ifdef ECC
  try {
    data->xr.startSession(data->befserverSession.keyName, MrpcClient::software, MrpcClient::privateKey, MrpcClient::tmpPass, data->befserverSession.publicServerKey);
    // wait4connect aktivieren
    data->xr.stopEncrypt();
    data->xr.flush();
    data->state = MrpcClientData::WaitingCon;
  } catch (std::exception &e) {
    LOG(LM_ERROR, "Exception: " << e.what());
  }
#else
  /*bool readyRead =*/ data->parse();
  if (data->state == MrpcClientData::WaitingCon)
  {
    data->state = MrpcClientData::Waiting;
    emit requestDone(&data->xr.session->server);
  }
#endif
}

void MrpcClient::disconnected()
{
  LOG(LM_INFO, "disconnected " << data->befserverSession.server);
  if (data->state < MrpcClientData::Ready)
    emit requestDone(data->xr.session->server);
  if (data->state != MrpcClientData::Error)
    data->state = MrpcClientData::SessionClosed;

}

void MrpcClient::bytesWritten(qint64 bytes) {
  LOG(LM_INFO, "BytesWritten " << bytes << " rest " << data->socket->bytesToWrite());
  if (data->serverStream) {
    try {
      auto wr = data->serverStream->tellp();
      LOG(LM_INFO, "STREAM WRITE " << wr );
      emit fileProgress(this, wr, data->outFileSz);

      data->doStream();
    } catch (std::exception &e) {
      LOG(LM_ERROR, "File transfer failed " << e.what());
      kill();
    }
  }
}


void MrpcClient::readyRead()
{
  try {
    std::string server = data->xr.session->server;

    LOG(LM_INFO, "readyRead " << server << ": " << data->state);
    while (data->state != MrpcClientData::Offline and not data->socket->atEnd()) {
      QByteArray bytes = data->socket->read(128*1024); // 8192 = 3.5s, 1024 = 25.5s, 64k 1.2s, 256k = 0.7sec auf localhost
      //LOG(LM_DEBUG, "BYTES " << std::string(bytes.data(), bytes.size()));
      LOG(LM_INFO, "READ " << server << ": " << bytes.size());

      data->iBlkStr.addBuff(qMove(bytes));
    }
#if 0
    if (data->state != MrpcClientData::Offline and data->state != MrpcClientData::Error and
        data->state != MrpcClientData::SessionClosed)
    {
      LOG(LM_INFO, "TIMER START SOCKET");
      data->readTimer.start(1000);
    }
#endif

    LOG(LM_INFO, "readyRead avail " << server << ": " << in_avail());
  } catch (std::exception &e) {
    LOG(LM_ERROR, "Exception: " << e.what());
  }
  if (in_avail() > 0)
    emit dataAvail();
}


void MrpcClient::doParse()
{
  LOG(LM_DEBUG, "doParse " << int(data->state));
  try {
    if (data->state >= MrpcClientData::Ready)
       return;
    if (not data->attachmentIstr) {
      bool ready = data->xr.parseClient();
      if (data->state == MrpcClientData::WaitingCon and data->xr.isConnected())
      {
        LOG(LM_INFO, "Connection confirmed " << data->befserverSession.keyValidTime);
        if (data->befserverSession.keyNeedsRefresh()) {
          LOG(LM_INFO, "Refresh Key");
          data->xr.clientRefreshKey(data->befserverSession.publicServerKey);
        }
        data->state = MrpcClientData::Waiting;
        emit requestDone(data->xr.session->server);
      }
      if (data->queryObj and data->xr.isConnected()) {
        data->sendQuery();
        LOG(LM_INFO, "Sending Query 1");
      }

      LOG(LM_INFO, "PARSE FERTIG '" << (data->xr.resultObj ? data->xr.resultObj->getObjectName() : "none") <<
                                    "' Avail=" << data->socket->bytesAvailable() << " + " << in_avail() <<
                                    " Level=" << data->xr.level() << " ready=" << ready);
      if (data->state == MrpcClientData::Result) {
        data->state = MrpcClientData::Ready;
        return;
      }
      if (data->xr.resultObj)
      {
        //data->reParseCnt = 0;
        if (data->xr.level() > 1) {
          emit queryResult(data->xr.resultObj);
        }
        else {
          if (data->xr.getAttachmentLength()) {
            if (not ready) {
              LOG(LM_INFO, "WARTE ATTACHMENT " << data->attachmentSz << " " << data->xr.inByteStreamAvail());
              QTimer::singleShot(1, this, &MrpcClient::doParse);
              return;
            } else {
              data->attachmentSz = data->xr.getAttachmentLength();
              LOG(LM_INFO, "STARTE ATTACHMENT " << data->attachmentSz << " " << data->xr.inByteStreamAvail() << " " << data->iBstr.rdbuf()->in_avail());
              //if (not data->attachmentIstr and data->xr.inByteStreamAvail())
                data->attachmentIstr = &data->xr.inByteStream();
            }
          } else {
            data->state = MrpcClientData::Result;
            emit result(data->xr.resultObj.get());
            QTimer::singleShot(1, this, &MrpcClient::doParse);
          }
        }
        QTimer::singleShot(1, this, &MrpcClient::doParse);
      }
    } else {
      LOG(LM_INFO, "READ ATTACHMENT");
      auto tg = data->attachmentIstr->tellg();
      char buf[8192+1];
      auto s = data->attachmentIstr->readsome(buf, sizeof(buf)-1);
      if (s > 0) {
        buf[s] = '\0';
        auto rd = data->attachmentIstr->tellg();
        //data->reParseCnt = 0;
        LOG(LM_INFO, "STREAM READ " << rd ); //<< " " << buf);
        emit fileProgress(this, rd, data->attachmentSz);
        QTimer::singleShot(1, this, &MrpcClient::doParse);
      }
      else if (data->attachmentIstr->eof()) {
        LOG(LM_INFO, "STREAM FERTIG");
        if (tg != data->attachmentSz)
          THROW("MrpcEventLoop::waitForData EOF aber Datei unvollständig");
        data->state = MrpcClientData::Ready;
        data->attachmentIstr = nullptr;
        //data->reParseCnt = 100;
        data->attachmentSz = 0;
        if (data->xr.resultObj) // jetzt das eigentliche Objekt erst senden
          emit result(data->xr.resultObj.get());
      }
    }
    if (data->state >= MrpcClientData::Ready)
      return;
    LOG(LM_INFO, "Parse Ende " << data->socket->bytesAvailable() << "  " << in_avail()  << " - " << data->socket->bytesAvailable() << " " << data->reParseCnt);
  }
  catch (std::exception &e)
  {
    LOG(LM_ERROR, "EX: " << e.what());
    data->state = MrpcClientData::Error;
    data->socket->abort();
    data->xr.session->sessionId = 0;
    emit result(nullptr);
    data->xr.resultObj = nullptr;
  }
}

void MrpcClient::query(const mobs::ObjectBase *queryObj)
{
  if (data->queryObj)
    THROW("query pending");
  if (data->xr.isConnected())
  {
    LOG(LM_INFO, "already connected : sending Query");
    data->xr.sendSingle(*queryObj);
    LOG(LM_INFO, "Sending Query 2");
    data->state = MrpcClientData::Waiting;
    data->flush();
    emit requestDone(data->xr.session->server);
  }
  else
    data->queryObj = queryObj;
}

std::string MrpcClient::server()
{
  return data->befserverSession.server;
}


std::ostream &MrpcClient::outByteStream()
{
  return data->xr.outByteStream();
}

std::streamsize MrpcClient::closeOutByteStream()
{
  auto sz = data->xr.closeOutByteStream();
  data->flush();
  return sz;
}



void MrpcClient::error() {
  LOG(LM_INFO, "error");
  data->state = MrpcClientData::Error;
  data->socket->abort();
}



void MrpcClient::close()
{
  data->close();
}



std::shared_ptr<MrpcClient> MrpcEventLoop::startClient(mobs::MrpcSession &clientSession, const mobs::ObjectBase *obj,
    std::istream *outfile, std::streamsize ofSize) {
  LOG(LM_INFO, "startClient");
  auto mrpc = new MrpcClient(clientSession);
  mrpc->data->queryObj = obj;
  mrpc->data->outFile = outfile;
  mrpc->data->outFileSz = ofSize;
  auto &client = data->connections[&static_cast<QObject &>(*mrpc)];
  client.mrpc = std::shared_ptr<MrpcClient>(mrpc);
  client.host = clientSession.host();
  connect(mrpc, &MrpcClient::fileProgress, this, &MrpcEventLoop::setProgress);

  return client.mrpc;
}






void MrpcEventLoop::clear() {
  data->connections.clear();
}

void MrpcEventLoop::result(const mobs::ObjectBase *obj)
{
  LOG(LM_INFO, "MRPC FERTIG " << std::boolalpha << bool(obj));
  auto it = data->connections.find(sender());
  if (it != data->connections.end()) {
    LOG(LM_INFO, "Closing " << it->second.host);
    it->second.active = false;
    it->second.current = it->second.last;
  }
  for (auto &i:data->connections)
    if (it->second.active)
      return;
  //endProgress();
  LOG(LM_INFO, "QUIT");
  QTimer::singleShot(1, this, &QEventLoop::quit);
}


void MrpcEventLoop::setProgress(const QObject *mrpc, size_t pos, size_t last) {
  LOG(LM_INFO, "PROGRESSINFO " << pos << " / " << last);
  auto it = data->connections.find(mrpc);
  if (it != data->connections.end()) {
    LOG(LM_INFO, "progress " << it->second.host);
    it->second.current = pos;
    it->second.last = last;
  }

  int cnt = 0;
  int max = 0;
  std::string warten;
  for (auto &i:data->connections)
  {
    if (i.second.current == SIZE_T_MAX)
      continue;
    cnt += i.second.current;
    max += i.second.last;
    if (i.second.active and cnt < max and warten.empty())
      warten = i.second.host;
  }
  if (progress)
  {
    if (currentMaxPercent < 5)
      currentMaxPercent = 5;
    if (currentMaxPercent > 50)
      currentMaxPercent = 50;
    progress->setLabelText(tr("warten auf %1").arg(warten.c_str()));
    int d = currentMaxPercent + (98 - currentMaxPercent) * cnt / max;
    progress->setValue(d);
    LOG(LM_DEBUG, "PROGRESS " << d);
  }
}

#if 0
void MrpcEventLoop::queryResult(const mobs::ObjectBase *obj)
{
  bool doProgress = false;
  std::string server;
   LOG(LM_INFO, "MRPC EMPFANGEN " << server << " " << std::boolalpha << bool(obj));
  if (obj)
    LOG(LM_ERROR, "Unbekanntes Objekt " << obj->getObjectName());

  if (not obj)
  {
    LOG(LM_INFO, "Kein Objekt ABBRUCH=" << std::boolalpha << abbruch);
    exit(abbruch ? 2 : 1);
  }
}
#endif

void MrpcEventLoop::startProgress(QWidget *parentWindow)
{
  progress = new QProgressDialog("Operation in arbeit.", "Cancel", 0, 100, parentWindow);
  progress->setWindowModality(Qt::WindowModal);
  progress->setMinimumDuration(200);
  progress->setAutoClose(true);
  progress->setValue(0);
  LOG(LM_DEBUG, "PROGRESS " << 0);
  progress->setLabelText(tr("Bitte warten"));
  //progEnd = (maxPercent <= data->progress->maximum() and maxPercent > 1) ? maxPercent : data->progress->maximum();
  connect(progress, &QProgressDialog::canceled, this, &MrpcEventLoop::cancel);

}

void MrpcEventLoop::cancel()
{
  LOG(LM_INFO, "LOOP ABBRUCH gedrückt ");
  abbruch = true;
  emit mrpcKill(); // Serververbindung abbrechen
  endProgress();
  quit();
}

void MrpcEventLoop::endProgress()
{
  if (progress)
  {
    progress->setValue(progress->maximum());
    LOG(LM_DEBUG, "PROGRESS " << progress->value() << " DEL");
    progress->deleteLater();
  }
  progress = nullptr;
}



std::map<std::string, std::unique_ptr<mobs::ObjectBase>> MrpcEventLoop::getResults()
{
  std::map<std::string, std::unique_ptr<mobs::ObjectBase>> result;
  for (auto &i:data->connections) {
    auto &mrpc = *i.second.mrpc.get();
    if (mrpc.lastResult()) {
      result.emplace(mrpc.server(), std::move(mrpc.getLastResult()));

    }
  }
  return result;
}

void MrpcEventLoop::waitForAnswer(int mp)
{
  abbruch = false;
  waitReqDone = false;
  if (mp > currentMaxPercent and mp < 100)
    maxPercent = mp;
  for (auto &i:data->connections) {
    auto &mrpc = *i.second.mrpc.get();
    mrpc.releaseResult();
    connect(&mrpc, &MrpcClient::result, this, &MrpcEventLoop::result);
    connect(this, &MrpcEventLoop::mrpcKill, &mrpc, &MrpcClient::cancel);
    connect(&mrpc, &MrpcClient::requestDone, this, &MrpcEventLoop::requestDone);
    QTimer::singleShot(1, &mrpc, &MrpcClient::doParse);
  }
  LOG(LM_DEBUG, "MRPC-LOOP start");
  exec();
  LOG(LM_DEBUG, "MRPC-LOOP ende");
  for (auto &i:data->connections) {
    auto &mrpc = *i.second.mrpc.get();
    disconnect(&mrpc, &MrpcClient::result, this, &MrpcEventLoop::result);
    disconnect(this, &MrpcEventLoop::mrpcKill, &mrpc, &MrpcClient::cancel);
    disconnect(&mrpc, &MrpcClient::requestDone, this, &MrpcEventLoop::requestDone);
  }
#if 0
  if (auto sess = dynamic_cast<CommandError *>(mrpc.lastResult()))
  {
    std::string err = sess->msg(); // TODO
    LOG(LM_ERROR, "Error received: " << err);
    mrpc.kill();
    throw std::runtime_error(err);
  }
  else
#endif
  if (abbruch)
    THROW("MRPC Abbruch");
}

#if 0
void MrpcEventLoop::waitForDataSend(MrpcClient &mrpc, int mp)
{
  //if (mrpc.data->xr.isConnected() and mrpc.data->queryObj)

  abbruch = false;
  //data = mrpc.data.get();
  if (mp > currentMaxPercent and mp < 100)
    maxPercent = mp;
  //connect(&mrpc, &MrpcClient::result, this, &MrpcEventLoop::result);
  connect(this, &MrpcEventLoop::mrpcKill, &mrpc, &MrpcClient::cancel);
  connect(&mrpc, &MrpcClient::requestDone, this, &MrpcEventLoop::requestDone);
  waitReqDone = true;
  LOG(LM_DEBUG, "MRPC-LOOP start");
  exec();
  LOG(LM_DEBUG, "MRPC-LOOP ende");
  disconnect(this, &MrpcEventLoop::mrpcKill, &mrpc, &MrpcClient::cancel);
  disconnect(&mrpc, &MrpcClient::requestDone, this, &MrpcEventLoop::requestDone);
}

void MrpcEventLoop::waitForClosed(MrpcClient &mrpc)
{
  mrpc.close();
  abbruch = false;
  //data = mrpc.data.get();
  maxPercent = 99;
  //connect(&mrpc, &MrpcClient::result, this, &MrpcEventLoop::result);
  connect(this, &MrpcEventLoop::mrpcKill, &mrpc, &MrpcClient::cancel);
  connect(&mrpc, &MrpcClient::requestDone, this, &MrpcEventLoop::requestDone);
  waitReqDone = true;
  LOG(LM_DEBUG, "MRPC-LOOP (END) start");
  exec();
  LOG(LM_DEBUG, "MRPC-LOOP (END) ende");
  disconnect(this, &MrpcEventLoop::mrpcKill, &mrpc, &MrpcClient::cancel);
  disconnect(&mrpc, &MrpcClient::requestDone, this, &MrpcEventLoop::requestDone);
}
#endif

void MrpcEventLoop::requestDone(const std::string &server)
{
  LOG(LM_INFO, "MrpcEventLoop::requestDone");
  if (progress and maxPercent > currentMaxPercent and maxPercent <= 100)
  {
    int a = (maxPercent - currentMaxPercent) * 5 / 100 + currentMaxPercent;
    if (a <= currentMaxPercent)
      a = currentMaxPercent + 1;
    if (currentMaxPercent < 100)
    {
      LOG(LM_DEBUG, "PROGRESS " << a << " CON");
      progress->setValue(a);
    }
  }
  if (waitReqDone) {
    waitReqDone = false;
    quit();
  }
}

MrpcEventLoop::MrpcEventLoop(QWidget *parent, int step) : QEventLoop(parent), stepMaxPercent(step)
{
  data = new MrpcEventLoopData;
  LOG(LM_INFO, "MrpcEventLoop::MrpcEventLoop " << loopCounter);

  loopCounter++;
  if (loopCounter != 1)
    LOG(LM_ERROR, "MrpcEventLoop: Verschachtelter Aufruf " << loopCounter);
  if (step > 99)
    step = 99;
  if (/*step > 0 and*/ parent)
  {
    startProgress(parent);
    maxPercent = step;
  }
}

int MrpcEventLoop::loopCounter = 0;

MrpcEventLoop::~MrpcEventLoop()
{
  loopCounter--;
  LOG(LM_INFO, "MrpcEventLoop::~MrpcEventLoop " << loopCounter);
  endProgress();
  for (auto &i:data->connections) {
    auto &mrpc = *i.second.mrpc.get();
    LOG(LM_INFO, "Closing Server " << i.second.host);
    mrpc.close();
  }
  delete data;
}




