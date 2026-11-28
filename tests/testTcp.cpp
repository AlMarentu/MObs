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


#include "tcpstream.h"
#include "logging.h"
#include <stdio.h>
#include <sstream>
#include <chrono>
#include <thread>
#include <gtest/gtest.h>


using namespace std;




namespace {
void server(const string &service)
{
  cerr << "starting receiver" << endl;
  try {
    mobs::TcpAccept tcpAccept;
    if (tcpAccept.initService(service) < 0)
      THROW("Service not started");
    cerr << "WAITING" << endl;
    mobs::tcpstream xstream(tcpAccept);
    xstream.exceptions(std::iostream::failbit | std::iostream::badbit);
    cerr <<  "Remote: " << xstream.getRemoteHost() << " " << xstream.getRemoteIp() << endl;
    string received;
    while (true) {
      getline(xstream, received);
      cerr << "RCV: " << received << endl;
      if (received == "Ping")
        xstream << "Pong" << endl;
    }

  }
  catch(...) {
    std::cerr << "Exception in tcpAccept service" << std::endl;
  }
}

class TcpTest : public ::testing::Test {
protected:
  string service = "12345";
  std::unique_ptr<std::thread> t1;

  void SetUp() override {
    t1 = std::unique_ptr<std::thread>(new std::thread(server, std::ref(service)));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));    // Kurze Synchronisation, bis der Kindprozess bereit ist
  }

  void TearDown() override {
    if (t1)
      t1->join();
  }
};
}

TEST_F(TcpTest, SendAndReceiveMessage) {

  mobs::tcpstream stream("localhost", "12345");
  ASSERT_FALSE(stream.eof());
  ASSERT_FALSE(stream.bad());
  stream << "Ping" << endl; // immer endl oder flush da gepufferter stream
  string received;
  getline(stream, received);
  EXPECT_EQ("Pong", received);
  EXPECT_NO_THROW(stream.shutdown());


}





