//
//  testParser.cpp
//  mobs
//
//  Created by Matthias Lautner on 19.03.20.
//  Copyright © 2020 Matthias Lautner. All rights reserved.
//

#include "jsonparser.h"
#include "xmlparser.h"

#include <stdio.h>
#include <sstream>
#include <gtest/gtest.h>

using namespace std;


namespace {

class JParser: public mobs::JsonParser {
public:
  JParser(const string &i) : mobs::JsonParser(i) {};
  virtual void Key(const std::string &value) { LOG(LM_INFO, "KEY " << value); };
  virtual void Value(const std::string &value, bool charType) { LOG(LM_INFO, "VALUE " << value); };
  virtual void StartArray() { LOG(LM_INFO, "START ARRAY"); };
  virtual void EndArray() { LOG(LM_INFO, "END ARRAY"); };
  virtual void StartObject() { LOG(LM_INFO, "START OBJECT"); };
  virtual void EndObject() { LOG(LM_INFO, "END OBJECT"); };
};

TEST(parserTest, jsonTypes) {
  string inhalt = R"({Bool:true,Char:"a",Char16_t:"b",Char32_t:"c",Wchar_t:"d",SignedChar:"e",ShortInt:42,Int:-9876543,LongInt:-45454545,LongLongInt:-34343434343434,UnsignedChar:"f",UnsignedShortInt:999,UnsignedInt:88888,UnsignedLongLong:109876543,UnsignedLongLongInt:1234567890,Float:-21.3,Double:1e-05,LongDouble:123.456,String:"Anton",Wstring:"Berti",U16string:"Conni",U32string:"Det"})";

  JParser p(inhalt);
  ASSERT_NO_THROW(p.parse());
}

TEST(parserTest, jsonParser) {

  string jjj =
  // aus https://de.wikipedia.org/wiki/JavaScript_Object_Notation
  R"(
  {
  "Herausgeber": "Xema",
  "Nummer": "1234-5678-9012-3456",
  "Deckung": 2e+6,
  "Waehrung": "EURO",
  "Inhaber":
  {
  "Name": "Mustermann",
  "Vorname": "Max",
  "maennlich": true,
  "Hobbys": ["Reiten", "Golfen", "Lesen"],
  "Alter": 42,
  "Kinder": [],
  "Partner": null
  }
  })";
  
 
  JParser p(jjj);
  ASSERT_NO_THROW(p.parse());   
}

void parse(string s) { JParser p(s); p.parse(); };

TEST(parserTest, jsonStruct1) {
  
  EXPECT_NO_THROW(parse(u8"{}"));
  EXPECT_NO_THROW(parse(u8""));  // Obacht
  EXPECT_NO_THROW(parse(u8"[{}]"));
  EXPECT_NO_THROW(parse(u8"{ \"a\" : 1 }"));
  EXPECT_ANY_THROW(parse(u8"\"a\":1"));// { fehlt  JParser p5();
  EXPECT_ANY_THROW(parse(u8"{[]}"));
  EXPECT_NO_THROW(parse(u8"{a:[]}"));
  EXPECT_NO_THROW(parse(u8"[]"));
  EXPECT_ANY_THROW(parse(u8"{a:b,[]}"));
  EXPECT_ANY_THROW(parse(u8"{a:b:c}"));
  EXPECT_ANY_THROW(parse(u8"{a,b}"));
  EXPECT_ANY_THROW(parse(u8"{a:[a:b]}"));
  EXPECT_NO_THROW(parse(u8"{a:[a,b,]}")); // TODO ???

}




}

