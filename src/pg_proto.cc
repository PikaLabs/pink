#include "pg_proto.h"

#include <string>
#include <climits>

#include <stdarg.h>

#include "xdebug.h"

namespace pink {

static inline void ReadByte(const char* data, uint8_t *byte) {
  *byte = data[0];
}

static inline void ReadU32be(const char* data, uint32_t *val) {
  uint32_t a = data[0] & 0x000000ff;
  uint32_t b = data[1] & 0x000000ff;
  uint32_t c = data[2] & 0x000000ff;
  uint32_t d = data[3] & 0x000000ff;

  *val = (a << 24) | (b << 16) | (c << 8) | d;
}

static inline void ReadU16be(const char* data, uint16_t *val) {
  uint32_t a = data[0];
  uint32_t b = data[1];

  *val = (a << 8) | b;
}

void GetRandomBytes(char *s, const int len) {
  static const char alphanum[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";

  for (int i = 0; i < len; ++i) {
    s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
  }

  s[len] = 0;
}

bool PacketHeader::ParseFromArray(const char* data, int *size) {
  if (*size < V3_HEADER_LEN) {
    log_warn("Parse header: less than 5 bytes available");
    return false;
  }

  //int p = 0;
  type = (uint8_t)data[0];
  if (type != 0) {
    ReadU32be(data + 1, &len);
    // TODO: ?
    len++;
    *size = V3_HEADER_LEN;

    version = 3;
    log_info ("Parse header: v3 type=%u/%c, len=%u\n", type, (char)type, len);
  } else { // old v2 header
    if ((uint8_t)data[1] != 0) {
      log_warn("Parse header: unknown special packet");
      return false;
    }
    uint16_t len16;
    ReadU16be(data + 2, &len16);
    len = len16;

    uint32_t code;
    ReadU32be(data + 4, &code);
    if (code == PKT_CANCEL) {
      type = PKT_CANCEL;
    } else if (code == PKT_SSLREQ) {
      type = PKT_SSLREQ;
    } else if ((code >> 16) == 3 && (code & 0xFFFF) < 2) {
      type = PKT_STARTUP;
    } else if (code == PKT_STARTUP_V2) {
      type = PKT_STARTUP_V2;
    } else {
      log_warn("Parse header: unknown special packet: len=%u code=%u", len, code);
      return false;
    }
    *size = V2_HEADER_LEN;
  }

 // if (len < *size || len >= LONG_MAX) {
 //   return false;
 // }

  return true;
}

//
// Write Packet related
//
PacketBuf::PacketBuf()
  : write_pos(0) {
  buf_len = PG_MAX_BUF;
  buf = (char *)malloc(PG_MAX_BUF);
}

PacketBuf::~PacketBuf() {
  free(buf);
}

void PacketBuf::MakeRoom(int len) {
  int need = write_pos + len;

  if (buf_len >= need)
    return;

  while (buf_len < need)
    buf_len = buf_len * 2;

  buf = (char*)realloc(buf, buf_len);
}

void PacketBuf::PutChar(char val) {
  MakeRoom(1);
  buf[write_pos++] = val;
}

void PacketBuf::PutU16(uint16_t val) {
  MakeRoom(4);

  buf[write_pos++] = (val >> 8) & 255;
  buf[write_pos++] = val & 255;
}

void PacketBuf::PutU32(uint32_t val) {
  MakeRoom(4);

  uint8_t* pos = (uint8_t*)buf + write_pos;
  pos[0] = (val >> 24) & 255;
  pos[1] = (val >> 16) & 255;
  pos[2] = (val >> 8) & 255;
  pos[3] = val & 255; 
  write_pos += 4;
}

void PacketBuf::PutU64(uint64_t val) {
  PutU32(val >> 32);
  PutU32((uint32_t)val);
}

void PacketBuf::PutBytes(const void *data, int len) {
  MakeRoom(len);

  memcpy(buf + write_pos, data, len);
  write_pos += len;
}

void PacketBuf::PutString(const char *str) {
  int len = strlen(str);
  PutBytes(str, len + 1);
}

// Write Packet header
void PacketBuf::StartPacket(int type) {
  if (type < 256) {
    // new-style packet
    PutChar(type);
    len_pos = write_pos;
    PutU32(0);
  } else {
    // old-style packet
    len_pos = write_pos;
    PutU32(0);
    PutU32(type);
  }
}

void PacketBuf::FinishPacket() {
  int len = write_pos - len_pos;
  uint8_t* pos = (uint8_t*)buf + len_pos;

  len_pos = 0;

  *pos++ = (len >> 24) & 255;
  *pos++ = (len >> 16) & 255;
  *pos++ = (len >> 8) & 255;
  *pos++ = len & 255;
}

// 
// types:
//  c - char/byte
//  h - uint16
//  i - uint32
//  q - uint64
//  s - Cstring
//  b - bytes
void PacketBuf::WriteGeneric(int type, const char *format, ...) {
  va_list ap;
  const char *p = format;

  StartPacket(type);

  va_start(ap, format);
  while (*p) {
    switch (*p) {
      case 'c':
        PutChar(va_arg(ap, int));
        break;
      case 'h':
        PutU16(va_arg(ap, int));
        break;
      case 'i':
        PutU32(va_arg(ap, int));
        break;
      case 'q':
        PutU64(va_arg(ap, uint64_t));
        break;
      case 's':
        PutString(va_arg(ap, char *));
        break;
      case 'b': {
        uint8_t* bin = va_arg(ap, uint8_t *);
        int len = va_arg(ap, int);
        PutBytes(bin, len);
        break;
      }
      default:
        log_warn("bad packet format: %s", format);
    }
    p++;
  }
	va_end(ap);

	FinishPacket();
}

PacketBuf* NewWelcomeMsg() {
  PacketBuf* packet = new PacketBuf();
  packet->WriteAuthenticationOk();
  packet->WriteParameterStatus("server_version", "pgstall" PS_VERSION);
  //packet->WriteParameterStatus("client_encoding", "UNICODE");
  packet->WriteParameterStatus("server_encoding", "SQL_ASCII");
  packet->WriteParameterStatus("DateStyle", "ISO");
  packet->WriteParameterStatus("TimeZone", "GMT");
  packet->WriteParameterStatus("standard_conforming_strings", "on");
  packet->WriteParameterStatus("is_superuser", "on");

  // TODO maybe need application_name too
  return packet;
}

// 
// InsertParser
//
InsertParser::InsertParser()
  : parse_pos_(0),
    len_(0) {
}

InsertParser::InsertParser(const std::string &str)
  : statement_(str),
    parse_pos_(0),
    len_(str.size()) {
}

void InsertParser::Init(const std::string &str) {
  statement_ = str;
  parse_pos_ = 0;
  len_ = str.size();
  rows_.clear();
}

//static void Tokenize(const std::string& str, std::vector<std::string>& tokens, const char& delimiter = ' ') {
//  size_t prev_pos = str.find_first_not_of(delimiter, 0);
//  size_t pos = str.find(delimiter, prev_pos);
//
//  while (prev_pos != std::string::npos || pos != std::string::npos) {
//    std::string token(str.substr(prev_pos, pos - prev_pos));
//    //printf ("find a token(%s), prev_pos=%u pos=%u\n", token.c_str(), prev_pos, pos);
//    tokens.push_back(token);
//
//    prev_pos = str.find_first_not_of(delimiter, pos);
//    pos = str.find_first_of(delimiter, prev_pos);
//  }
//}

// delimiters are " (),"
//  we consider "()" as one token while ',' not;
//  we handle quotes;
std::string InsertParser::NextToken() {
  ssize_t begin_pos = -1;
  bool bracket = false;

  while (parse_pos_ < len_) {
    if (statement_[parse_pos_] == ' ') {
      if (!bracket && begin_pos >= 0) {
        parse_pos_++;
        return statement_.substr(begin_pos, parse_pos_ - begin_pos - 1);
      }
    } else  if (statement_[parse_pos_] == '(') {
      if (begin_pos >= 0) {
        return statement_.substr(begin_pos, parse_pos_ - begin_pos);
      } else {  // the first '('
        begin_pos = parse_pos_;
        bracket = true;
      }
    } else if (statement_[parse_pos_] == ')') {
      if (bracket) {
        parse_pos_++;
        return statement_.substr(begin_pos, parse_pos_ - begin_pos);
      } else {
        return "";
      }
    } else if (statement_[parse_pos_] == ',') {
      if (!bracket && begin_pos >= 0) {
        parse_pos_++;
        return statement_.substr(begin_pos, parse_pos_ - begin_pos - 2);
      }
    } else if (statement_[parse_pos_] == '\0') {
      // don't handle \0
    } else if (begin_pos < 0) {
      begin_pos = parse_pos_;
    }
    parse_pos_++;
  }
  if (begin_pos < 0) {
    return "";
  }

  return statement_.substr(begin_pos, parse_pos_ - begin_pos - 1);
}

// brute force match Insert statement
//    insert into Table values
//      (attr1, attr...),
//      (attr1, att...);
bool InsertParser::Parse() {
  std::string token;

  // Insert
  token = NextToken();
  log_info ("InsertParse: 1st token=(%s)", token.c_str());
  if (strcasecmp(token.c_str(), "insert") == 0) {
    token = NextToken();
    log_info ("InsertParse: 2nd token=(%s)", token.c_str());
    if (strcasecmp(token.c_str(), "into") == 0) {
      // handle table_name;
      token = NextToken();
      if (isalnum(token[0])) {
        table_ = token;
        log_info ("InsertParse: 3rd token table=%s\n", table_.c_str());

        token = NextToken();
        if (token[0] == '(' && (token.size() > 1 && token[token.size() - 1] == ')')) {
          log_info ("InsertParse: 4rd token attribute=(%s)\n", token.c_str());
          token = NextToken();
        }
        if (strcasecmp(token.c_str(), "values") == 0) {
          log_info ("InsertParse: 4rd token values=(%s)\n", token.c_str());
          while (parse_pos_ < statement_.size()) {
            token = NextToken();
            if (token == ";") {
              break;
            } else if (token[0] == '(' && (token.size() > 1 && token[token.size() - 1] == ')')) {
              rows_.push_back(token.substr(1, token.size() - 2));
            } else if (token.size() > 0) {
              return false;
            }
          }
        } else {
          log_info ("InsertParse failed: 4rd token unsupported (%s)\n", token.c_str());
          return false;
        }
      }
    }
  }
  
  return true;
}

} // namespace pink
