#include "pg_proto.h"

#include <string>
#include <climits>

#include <stdarg.h>
#include <sys/time.h>

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
  //packet->WriteAuthenticationOk();
  packet->WriteParameterStatus("server_version", "gpstall" PS_VERSION);
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
  : cnt_(0),
    parse_pos_(0),
    len_(0) {
}

InsertParser::InsertParser(const std::string &str)
  : statement_(str),
    parse_pos_(0),
    len_(str.size()) {
}

void InsertParser::Init(const std::string &str, const uint32_t type) {
  query_type_ = type;
  statement_ = str;
  parse_pos_ = 0;
  len_ = str.size();
  rows_.clear();
  attributes_.clear();
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
bool InsertParser::NextToken(std::string& token) {
  token.clear();
  ssize_t begin_pos = -1;
  bool bracket = false;
  bool single_quote = false;
  bool double_quote = false;

  // timeout 1s
  struct timeval before;
  gettimeofday(&before, NULL);

  // we handle quotes simply;
  while (parse_pos_ < len_) {
    // timeout 1s
    struct timeval now;
    gettimeofday(&now, NULL);
    if (now.tv_sec > before.tv_sec + 1) {
      return false;
    }

    if (statement_[parse_pos_] == ' ' 
        || statement_[parse_pos_] == '\0') {
      if (!single_quote && !double_quote && !bracket && begin_pos >= 0) {
        parse_pos_++;
        token = statement_.substr(begin_pos, parse_pos_ - begin_pos - 1);
        return true;
      }
    } else if (statement_[parse_pos_] == '(') {
      if (!single_quote && !double_quote) {
        if (begin_pos >= 0) {
          token = statement_.substr(begin_pos, parse_pos_ - begin_pos);
          return true;
        } else {  // the first '('
          begin_pos = parse_pos_;
          bracket = true;
        }
      }
    } else if (statement_[parse_pos_] == ')') {
      if (!single_quote && !double_quote) {
        if (bracket) {
          parse_pos_++;
          token = statement_.substr(begin_pos, parse_pos_ - begin_pos);
          return true;
        } else {
          return false;
        }
      }
    } else if (statement_[parse_pos_] == ',') {
      if (!single_quote && !double_quote && !bracket && begin_pos >= 0) {
        parse_pos_++;
        token = statement_.substr(begin_pos, parse_pos_ - begin_pos - 2);
        return true;
      }
    } else if (begin_pos < 0) {
      begin_pos = parse_pos_;
      if (statement_[parse_pos_] == '\'') { // single quote
        single_quote = !single_quote;
      } else if (statement_[parse_pos_] == '"') {
        double_quote = !double_quote;
      }
    } else if (statement_[parse_pos_] == '"') {
      if (double_quote) {
        parse_pos_++;
        token = statement_.substr(begin_pos, parse_pos_ - begin_pos);
        return true;
      }
    }

    parse_pos_++;
  }

  if (begin_pos < 0) {
    return false;
  }

  token = statement_.substr(begin_pos, parse_pos_ - begin_pos - 1);
  return true;
}

// similar with NextToken
// we pass parse_pos by reference
std::string NextAttribute(const std::string& str, int &start_pos) {
  int len = str.size();
  int i = start_pos;
  ssize_t begin_pos = -1;
  ssize_t end_pos = -1;
  bool single_quote = false;
  bool double_quote = false;

  while (i < len) {
    if (str[i] == ' ') {
      if (!single_quote && !double_quote && begin_pos >= 0 && end_pos < 0) {
        end_pos = i;
      }
    } else if (str[i] == ',') {
      if (!single_quote && !double_quote) {
        if (end_pos < 0) {
          end_pos = i;
        }
        break;
      }
    } else {
      if (begin_pos < 0) {
        begin_pos = i;
      }
      if (!double_quote && str[i] == '\'') { // single quote
        single_quote = !single_quote;
      }

      // single quote will escape double quote
      if (!single_quote && str[i] == '"') {
        double_quote = !double_quote;
      }
    }

    i++;
  }

  start_pos = i + 1;

  if (end_pos < 0) {
    end_pos = i;
  }

  if (begin_pos < 0 || end_pos == begin_pos) {
    return "";
  }

  return str.substr(begin_pos, end_pos - begin_pos);
}

// similar to EscapteAttributes
// str is like ("asdf  ", 123 , ) or ('asdf ')  without the parenthese;
std::string InsertParser::EscapeValues(const std::string& str) {
  int len = str.size();
  std::string res;
  res.reserve(2 * len);

  int token_cnt = 0;
  int start_pos = 0;
  while (start_pos < len) {
    std::string token = NextAttribute(str, start_pos);
    if (token_cnt > 0) {
      res.append(1, ',');
    }
    token_cnt++;

    if (token.size() == 0) {
      res.append("\"\"");
    } else if (token.size() == 1) {
      res.append("\"");
      res.append(token);
      res.append("\"");
    } else {
      size_t i = 0;
      size_t len = token.size();

      if ((token[0] == '\'' && token.back() == '\'')
          || (token[0] == '"' && token.back() == '"')) {
        res.append(1, '"');
        i = 1;
        len--;
      }

      for (; i < len; i++) {
        if (token[i] == '"') {
          res.append(1, '"');
        }
        res.append(1, token[i]);
      }
      if ((token[0] == '\'' && token.back() == '\'')
          || (token[0] == '"' && token.back() == '"')) {
        res.append(1, '"');
      }
    }
  }

  log_info ("After escape values: [%s] size=%u", res.data(), res.size());
  return res;
}

// attribute name can only be double qutoted;
void InsertParser::EscapeAttribute(const std::string& str) {
  int len = str.size();
  header_.reserve(2 * len);
  //res.reserve(2 * len);

  int token_cnt = 0;
  int start_pos = 0;
  while (start_pos < len) {
    std::string token = NextAttribute(str, start_pos);
    if (token_cnt > 0) {
      header_.append(1, ',');
    }
    token_cnt++;

    std::string res;

    if (token.size() == 0) {
      res.append("\"\"");
    } else if (token.size() == 1) {
      res.append("\"");
      res.append(token);
      res.append("\"");
    } else {
      size_t i = 0;
      size_t len = token.size();

      if ((token[0] == '\'' && token.back() == '\'')
          || (token[0] == '"' && token.back() == '"')) {
        res.append(1, '"');
        i = 1;
        len--;
      }

      for (; i < len; i++) {
        if (token[i] == '"') {
          res.append(1, '"');
        }
        res.append(1, token[i]);
      }
      if ((token[0] == '\'' && token.back() == '\'')
          || (token[0] == '"' && token.back() == '"')) {
        res.append(1, '"');
      }
    }

    attributes_.push_back(res);
    header_.append(res);
    //res.clear();
  }

  log_info ("After escape attribute[%s] size=%u", header_.data(), header_.size());
  header_.append(1, '\n');
}

// brute force match Insert statement
//    insert into Table values
//      (attr1, attr...),
//      (attr1, att...);
bool InsertParser::Parse(std::string &error_info) {
  std::string token;
  bool need_parse_header = true;

  // Insert
  if (NextToken(token)) {
    // PDO will use parametered prepared statements and deallocate statement;
    if (strcasecmp(token.c_str(), "insert") != 0) {
      log_info ("InsertParse: first token=(%s) not insert", token.c_str());
      error_info = "InsertParse: first token=" + token + " not insert";
      if (!NextToken(token)) {
        return false;
      }
      if (strcasecmp(token.c_str(), "deallocate") == 0) {
        return true;
      }
      if (strcasecmp(token.c_str(), "insert") != 0) {
        log_info ("InsertParse: first 2 token=(%s) not insert", token.c_str());
        error_info = "InsertParse: first token=" + token + " not insert";
        return false;
      }
    }

    log_info ("InsertParse: 1st token=(%s)", token.c_str());
    if (NextToken(token) && strcasecmp(token.c_str(), "into") == 0) {
      log_info ("InsertParse: 2nd token=(%s)", token.c_str());
      // table_name;
      if (NextToken(token) && (isalnum(token[0]) || token[0] == '"')) {
        if (token[0] == '"') {
          // Trim quote in table string
          token = token.substr(1, token.size() - 2);
        }
        // if clinet insert different table, need parse header again.
        if (token != table_)
          need_parse_header = true;
        table_ = token;
        log_info ("InsertParse: 3rd token table=%s\n", table_.c_str());

        if (NextToken(token)) {
          if (token[0] == '(' && (token.size() > 1 && token[token.size() - 1] == ')')) {
            log_info ("InsertParse: 4rd token attribute=(%s)\n", token.c_str());

            // We parse header when one connection insert different table
            if (header_.empty() || need_parse_header) {
              header_.clear();
              EscapeAttribute(token.substr(1, token.size() - 2));
            }
            NextToken(token);
          }

          if (strcasecmp(token.c_str(), "values") == 0) {
            log_info ("InsertParse: 4rd token values=(%s)\n", token.c_str());

            // timeout 5s
            struct timeval before;
            gettimeofday(&before, NULL);

            while (parse_pos_ < statement_.size()) {
              // We trick here
              // jdbc maybe end without semicolon
              NextToken(token);

              // timeout 5s
              struct timeval now;
              gettimeofday(&now, NULL);
              if (now.tv_sec > before.tv_sec + 5) {
                error_info = "timeout statement: " + statement_;
                error_info += "\ntimeout values: " + token;
                return false;
              }

              if (token == ";") {
                break;
              } else if (token[0] == '(' && (token.size() > 1 && token[token.size() - 1] == ')')) {
                rows_.push_back(EscapeValues(token.substr(1, token.size() - 2)));
              } else if (token.size() > 0) {
                error_info = "unknow values: " + token;
                break;
              }
            }
          } else {
            log_info ("InsertParse failed: 4rd token unsupported (%s)\n", token.c_str());
            return false;
          }
        }
      }
    }
  }
  
  return true;
}

namespace {
class ParamBinder {
 public:
  ParamBinder(std::vector<std::string> *rows, int attribute_size)
      : rows_(rows),
        param_count_(0),
        row_index_(0),
        attr_size_(attribute_size) {
  }

  // Replace '$1, $2, $3, ...' with param
  // Test in lavavel with pdo driver only.
  bool Apply(std::string &param) {
    ++param_count_;
    if (param_count_ > attr_size_ * rows_->size())
      return false;
    std::string placeholder = "$" + std::to_string(param_count_);

    std::string &row = (*rows_)[row_index_];
    if (param_count_ % attr_size_ == 0)
      ++row_index_;

    size_t pos = row.find(placeholder);
    row.replace(pos, placeholder.size(), param);

    return true;
  }
 private:
  std::vector<std::string> *rows_;
  int param_count_;
  int row_index_;
  int attr_size_;
};
}

// On Error parse_error_ = true;
bool InsertParser::BindParam(std::string &statement) {
  size_t str_count = 0;
  size_t start_pos = 0;

  // Parse two String
  for (; start_pos < statement.size(); start_pos++) {
    if (statement[start_pos] == 0) {
      ++str_count;
      if (str_count == 2)
        break;
    }
  }
  ++start_pos; // Skip '0'

  if (start_pos > statement.size())
    return false;
  // int16 and int16[c], number of parameter format codes, useless for now
  size_t a, b, c;
  a = (unsigned char)statement[start_pos];
  b = (unsigned char)statement[start_pos + 1];
  c = (a << 8) | b;
  start_pos += (c + 1) * 2;

  if (start_pos > statement.size())
    return false;
  // int16, number of parameter values
  a = (unsigned char)statement[start_pos];
  b = (unsigned char)statement[start_pos + 1];
  c = (a << 8) | b;
  if (attributes_.size() > 0 &&
      c / attributes_.size() != rows_.size()) {
    return false;
  }
  start_pos += 2;

  if (start_pos > statement.size())
    return false;
  // int32 and Byte n
  ParamBinder binder(&rows_, attributes_.size());
  size_t n, w, x, y, z;
  std::string param;
  for (size_t i = 0; i < c; i++) {
    n = 0;
    w = (unsigned char)statement[start_pos];
    x = (unsigned char)statement[start_pos + 1];
    y = (unsigned char)statement[start_pos + 2];
    z = (unsigned char)statement[start_pos + 3];
    n = (w << 24) | (x << 16) | (y << 8) | z;
    param.assign(statement.substr(start_pos + 4, n));
    binder.Apply(param);
    start_pos += 4 + n;
  }

  return true;
}

} // namespace pink
