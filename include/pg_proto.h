// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef INCLUDE_PG_PROTO_H_
#define INCLUDE_PG_PROTO_H_

#include <vector>

#include <slice.h>

namespace pink {

#define PS_VERSION "0.1"

//
// PG related
//
#define PG_MAX_MESSAGE  102400
#define PG_MAX_BUF      10240

// old style V2 packet header: 
//    [ len(4 bytes) | code(4 bytes) ]
//  note: the first 2 bytes are 0;
#define V2_HEADER_LEN   8

// new style V3 packet header len:
//    [ type(1 byte) | len(4 bytes) ]
#define V3_HEADER_LEN   5

// type codes for weird pkts
#define PKT_STARTUP_V2  0x20000
#define PKT_STARTUP     0x30000
#define PKT_CANCEL      80877102
#define PKT_SSLREQ      80877103

// protocol codes
#define AUTH_OK       0
#define AUTH_KRB      2
#define AUTH_PLAIN    3
#define AUTH_CRYPT    4
#define AUTH_MD5      5
#define AUTH_CREDS    6

#define ERROR_MSG_PARSE "syntax error"
//#define ERROR_MSG_AUTH  "password authentication failed for user"

enum PGStatus {
  // Startup phase
  kPGLogin = 0,
  kPGActive = 1,
  // Normal phase
  kPGHeader = 2,
  kPGPacket = 3,
  kPGComplete = 4,
  //kBuildObuf = 5,
  //kWriteObuf = 6,
};


struct PacketHeader;
struct PacketBuf;
class InsertParser;

struct PacketHeader {
  uint32_t type;
  uint32_t len;
  //struct MBuf data;
  //Slice data;

  int version;
  bool ParseFromArray(const char* data, int *size);

  void Dump() {
    printf ("Packet: type=%c len=%d\n", type > 256 ? '!' : (char)type, len);
  }
};

// is packet header completely in buffer
static inline bool IsCompleteHeader(const char* buf, uint32_t offset) {
  if (offset >= V2_HEADER_LEN)
    return true;
  if (offset < V3_HEADER_LEN)
    return false;
  // is it V2 header?
  return buf[0] != 0;
}

//
// Packet Buffer
//
struct PacketBuf {
  char* buf;
  int buf_len;
  int write_pos;
  int len_pos;

  PacketBuf(); 
  ~PacketBuf();

  void MakeRoom(int len);
  void PutChar(char val);
  void PutU16(uint16_t val);
  void PutU32(uint32_t val);
  void PutU64(uint64_t val);
  void PutBytes(const void *data, int len);
  void PutString(const char *str);

  void StartPacket(int type);
  void FinishPacket();
  void WriteGeneric(int type, const char *format, ...);
};


void GetRandomBytes(char *s, const int len);

//
// Shortcuts for actual packets.
//
#define WriteParameterStatus(key, val) \
      WriteGeneric('S', "ss", key, val)

#define WriteAuthenticationOk() \
      WriteGeneric('R', "i", 0)

#define WriteReadyForQuery() \
      WriteGeneric('Z', "c", 'I')

#define WriteCommandComplete(desc) \
      WriteGeneric('C', "s", desc)

#define WriteBackendKeyData(key) \
      WriteGeneric('K', "b", key, 8)

#define WriteCancelRequest(key) \
      WriteGeneric(PKT_CANCEL, "b", key, 8)

#define WriteStartupMessage(user, parms, parms_len) \
      WriteGeneric(PKT_STARTUP, "bsss", parms, parms_len, "user", user, "")

#define WritePasswordMessage(psw) \
      WriteGeneric('p', "s", psw)

#define WriteNotice(msg) \
      WriteGeneric('N', "sscss", "SNOTICE", "C00000", 'M', msg, "");

#define WriteSSLRequest() \
      WriteGeneric(PKT_SSLREQ, "")

#define WriteErrorResponse(msg) \
      WriteGeneric('E', "sscss", "SERROR", "C42601", 'M', msg, "")

#define WriteFatalResponse(msg) \
      WriteGeneric('E', "sscss", "SFATAL", "C28P01", 'M', msg, "")

#define WriteFatalAuthResponse(msg) \
      WriteGeneric('E', "sscss", "SFATAL", "C28000", 'M', msg, "")

PacketBuf* NewWelcomeMsg();

class InsertParser {
 public:
  InsertParser();
  InsertParser(const std::string &str);
  void Init(const std::string &str, const uint32_t type);
  uint32_t query_type_;

  std::string statement_;
  std::string table_;
  std::vector<std::string> rows_;

  int64_t cnt_;
  std::vector<std::string> attributes_;
  std::string header_;

  bool Parse(std::string &error_info);
  bool NextToken(std::string& token);
  std::string EscapeValues(const std::string& str);
  void EscapeAttribute(const std::string& str);

 private:
  size_t parse_pos_;
  size_t len_;
};

}  // namespace pink

#endif  // INCLUDE_PB_CONN_H_
