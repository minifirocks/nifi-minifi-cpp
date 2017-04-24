/**
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "io/BaseStream.h"
#include "Site2SitePeer.h"
#include "Site2SiteClientProtocol.h"
#include <uuid/uuid.h>
#include "core/logging/LogAppenders.h"
#include "core/logging/BaseLogger.h"
#include "SiteToSiteHelper.h"
#include <algorithm>
#include <string>
#include <memory>
#include "../TestBase.h"
#define FMT_DEFAULT fmt_lower

using namespace org::apache::nifi::minifi::io;
TEST_CASE("TestSetPortId", "[S2S1]") {

  std::unique_ptr<minifi::Site2SitePeer> peer =
      std::unique_ptr < minifi::Site2SitePeer
          > (new minifi::Site2SitePeer(
              std::unique_ptr < DataStream > (new DataStream()), "fake_host",
              65433));

  minifi::Site2SiteClientProtocol protocol(std::move(peer));

  std::string uuid_str = "c56a4180-65aa-42ec-a945-5fd21dec0538";

  uuid_t fakeUUID;

  uuid_parse(uuid_str.c_str(), fakeUUID);

  protocol.setPortId(fakeUUID);

  REQUIRE(uuid_str == protocol.getPortId());

}

TEST_CASE("TestSetPortIdUppercase", "[S2S2]") {

  std::unique_ptr<minifi::Site2SitePeer> peer =
      std::unique_ptr < minifi::Site2SitePeer
          > (new minifi::Site2SitePeer(
              std::unique_ptr < DataStream > (new DataStream()), "fake_host",
              65433));

  minifi::Site2SiteClientProtocol protocol(std::move(peer));

  std::string uuid_str = "C56A4180-65AA-42EC-A945-5FD21DEC0538";

  uuid_t fakeUUID;

  uuid_parse(uuid_str.c_str(), fakeUUID);

  protocol.setPortId(fakeUUID);

  REQUIRE(uuid_str != protocol.getPortId());

  std::transform(uuid_str.begin(), uuid_str.end(), uuid_str.begin(), ::tolower);

  REQUIRE(uuid_str == protocol.getPortId());

}

TEST_CASE("TestWriteUTF", "[MINIFI193]") {

  DataStream baseStream;

  Serializable ser;

  std::string stringOne = "helo world"; // yes, this has a typo.
  std::string verifyString;
  ser.writeUTF(stringOne, &baseStream, false);

  ser.readUTF(verifyString, &baseStream, false);

  REQUIRE(verifyString == stringOne);

}

TEST_CASE("TestWriteUTF2", "[MINIFI193]") {

  DataStream baseStream;

  Serializable ser;

  std::string stringOne = "hel\xa1o world";
  REQUIRE(11 == stringOne.length());
  std::string verifyString;
  ser.writeUTF(stringOne, &baseStream, false);

  ser.readUTF(verifyString, &baseStream, false);

  REQUIRE(verifyString == stringOne);

}

TEST_CASE("TestWriteUTF3", "[MINIFI193]") {

  DataStream baseStream;

  Serializable ser;

  std::string stringOne = "\xe4\xbd\xa0\xe5\xa5\xbd\xe4\xb8\x96\xe7\x95\x8c";
  REQUIRE(12 == stringOne.length());
  std::string verifyString;
  ser.writeUTF(stringOne, &baseStream, false);

  ser.readUTF(verifyString, &baseStream, false);

  REQUIRE(verifyString == stringOne);

}

void sunny_path_bootstrap(SiteToSiteResponder *collector) {

  char a = 0x14; // RESOURCE_OK
  std::string resp_code;
  resp_code.insert(resp_code.begin(), a);
  collector->push_response(resp_code);

  // Handshake respond code
  resp_code = "R";
  collector->push_response(resp_code);
  resp_code = "C";
  collector->push_response(resp_code);
  char b = 0x1;
  resp_code = b;
  collector->push_response(resp_code);

  // Codec Negotiation
  resp_code = a;
  collector->push_response(resp_code);
}

TEST_CASE("TestSiteToSiteVerifySend", "[S2S2]") {

  SiteToSiteResponder *collector = new SiteToSiteResponder();

  sunny_path_bootstrap(collector);

  std::unique_ptr<minifi::Site2SitePeer> peer = std::unique_ptr
      < minifi::Site2SitePeer
      > (new minifi::Site2SitePeer(
          std::unique_ptr < minifi::io::DataStream > (collector), "fake_host",
          65433));

  minifi::Site2SiteClientProtocol protocol(std::move(peer));

  std::string uuid_str = "C56A4180-65AA-42EC-A945-5FD21DEC0538";

  uuid_t fakeUUID;

  uuid_parse(uuid_str.c_str(), fakeUUID);

  protocol.setPortId(fakeUUID);

  REQUIRE(true == protocol.bootstrap());

  REQUIRE(collector->get_next_client_response() == "NiFi");
  collector->get_next_client_response();
  REQUIRE(collector->get_next_client_response() == "SocketFlowFileProtocol");
  collector->get_next_client_response();
  collector->get_next_client_response();
  collector->get_next_client_response();
  collector->get_next_client_response();
  REQUIRE(collector->get_next_client_response() == "nifi://fake_host:65433");
  collector->get_next_client_response();
  collector->get_next_client_response();
  REQUIRE(collector->get_next_client_response() == "GZIP");
  collector->get_next_client_response();
  REQUIRE(collector->get_next_client_response() == "false");
  collector->get_next_client_response();
  REQUIRE(collector->get_next_client_response() == "PORT_IDENTIFIER");
  collector->get_next_client_response();
  REQUIRE(
      collector->get_next_client_response()
          == "c56a4180-65aa-42ec-a945-5fd21dec0538");
  collector->get_next_client_response();
  REQUIRE(collector->get_next_client_response() == "REQUEST_EXPIRATION_MILLIS");
  collector->get_next_client_response();
  REQUIRE(collector->get_next_client_response() == "30000");
  collector->get_next_client_response();
  REQUIRE(collector->get_next_client_response() == "NEGOTIATE_FLOWFILE_CODEC");
  collector->get_next_client_response();
  REQUIRE(collector->get_next_client_response() == "StandardFlowFileCodec");
  collector->get_next_client_response(); // codec version

  // start to send the stuff
  // Create the transaction
  std::string transactionID;
  std::string payload = "Test MiNiFi payload";
  minifi::Transaction *transaction;
  transaction = protocol.createTransaction(transactionID, minifi::SEND);
  collector->get_next_client_response();
  REQUIRE(collector->get_next_client_response() == "SEND_FLOWFILES");
  std::map < std::string, std::string > attributes;
  minifi::DataPacket packet(&protocol, transaction, attributes, payload);
  REQUIRE(protocol.send(transactionID, &packet, nullptr, nullptr) == true);
  collector->get_next_client_response();
  collector->get_next_client_response();
  std::string rx_payload = collector->get_next_client_response();
  ;
  REQUIRE(payload == rx_payload);

}

TEST_CASE("TestSiteToSiteVerifyNegotiationFail", "[S2S2]") {

  SiteToSiteResponder *collector = new SiteToSiteResponder();

  char a = 0xFF;
  std::string resp_code;
  resp_code.insert(resp_code.begin(), a);
  collector->push_response(resp_code);
  collector->push_response(resp_code);

  std::unique_ptr<minifi::Site2SitePeer> peer = std::unique_ptr
      < minifi::Site2SitePeer
      > (new minifi::Site2SitePeer(
          std::unique_ptr < minifi::io::DataStream > (collector), "fake_host",
          65433));

  minifi::Site2SiteClientProtocol protocol(std::move(peer));

  std::string uuid_str = "C56A4180-65AA-42EC-A945-5FD21DEC0538";

  uuid_t fakeUUID;

  uuid_parse(uuid_str.c_str(), fakeUUID);

  protocol.setPortId(fakeUUID);

  REQUIRE(false == protocol.bootstrap());

}
