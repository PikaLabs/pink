// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include "pink/include/redis_conn.h"

#include <stdlib.h>
#include <limits.h>

#include <string>
#include <sstream>

#include "slash/include/xdebug.h"
#include "slash/include/slash_string.h"

namespace pink {

static bool IsHexDigit(char ch) {
  return (ch>='0' && ch<='9') || (ch>='a' && ch<='f') || (ch>='A' && ch<'F');
}

static int HexDigitToInt32(char ch) {
  if (ch <= '9' && ch >= '0') {
    return ch-'0';
  } else if (ch <= 'F' && ch >= 'A') {
    return ch-'A';
  } else if (ch <= 'f' && ch >= 'a') {
    return ch-'a';
  } else {
    return 0;
  }
}

static int split2args(const std::string& req_buf, RedisCmdArgsType& argv) {
  const char *p = req_buf.data();
  std::string arg;

  while (1) {
    // skip blanks
    while (*p && isspace(*p)) p++;
    if (*p) {
      // get a token
      int inq = 0;  // set to 1 if we are in "quotes"
      int insq = 0;  // set to 1 if we are in 'single quotes'
      int done = 0;

      arg.clear();
      while (!done) {
        if (inq) {
          if (*p == '\\' && *(p+1) == 'x' &&
              IsHexDigit(*(p+2)) &&
              IsHexDigit(*(p+3))) {
            unsigned char byte = HexDigitToInt32(*(p+2))*16 + HexDigitToInt32(*(p+3));
            arg.append(1, byte);
            p += 3;
          } else if (*p == '\\' && *(p + 1)) {
            char c;

            p++;
            switch (*p) {
              case 'n': c = '\n'; break;
              case 'r': c = '\r'; break;
              case 't': c = '\t'; break;
              case 'b': c = '\b'; break;
              case 'a': c = '\a'; break;
              default: c = *p; break;
            }
            arg.append(1, c);
          } else if (*p == '"') {
            /* closing quote must be followed by a space or
            * nothing at all. */
            if (*(p+1) && !isspace(*(p+1))) {
              argv.clear();
              return -1;
            }
            done = 1;
          } else if (!*p) {
            // unterminated quotes
            argv.clear();
            return -1;
          } else {
            arg.append(1, *p);
          }
        } else if (insq) {
          if (*p == '\\' && *(p+1) == '\'') {
            p++;
            arg.append(1, '\'');
          } else if (*p == '\'') {
            /* closing quote must be followed by a space or
            * nothing at all. */
            if (*(p+1) && !isspace(*(p+1))) {
              argv.clear();
              return -1;
            }
            done = 1;
          } else if (!*p) {
            // unterminated quotes
            argv.clear();
            return -1;
          } else {
            arg.append(1, *p);
          }
        } else {
          switch (*p) {
            case ' ':
            case '\n':
            case '\r':
            case '\t':
            case '\0':
              done = 1;
            break;
            case '"':
              inq = 1;
            break;
            case '\'':
              insq = 1;
            break;
            default:
              // current = sdscatlen(current,p,1);
              arg.append(1, *p);
              break;
          }
        }
        if (*p) p++;
      }
      argv.push_back(arg);
    } else {
      return 0;
    }
  }
}

RedisConn::RedisConn(const int fd,
                     const std::string &ip_port,
                     ServerThread *thread)
    : PinkConn(fd, ip_port, thread),
      rbuf_(nullptr),
      rbuf_len_(0),
      last_read_pos_(-1),
      next_parse_pos_(0),
      req_type_(0),
      multibulk_len_(0),
      bulk_len_(-1),
      wbuf_len_(0),
      wbuf_pos_(0) {
}

RedisConn::~RedisConn() {
  free(rbuf_);
}

ReadStatus RedisConn::ProcessInlineBuffer() {
  int pos, ret;
  pos = FindNextSeparators();
  if (pos == -1) {
    return rbuf_len_ > REDIS_INLINE_MAXLEN ? kFullError : kReadHalf;
  }
  assert(pos > next_parse_pos_);
  std::string req_buf(rbuf_ + next_parse_pos_, pos - next_parse_pos_ - 1);

  argv_.clear();
  ret = split2args(req_buf, argv_);
  next_parse_pos_ = pos + 1;

  return ret == -1 ? kParseError : kReadAll;
}

ReadStatus RedisConn::ProcessMultibulkBuffer() {
  int pos = 0;
  if (multibulk_len_ == 0) {
    /* The client should have been reset */
    pos = FindNextSeparators();
    if (pos != -1) {
      if (GetNextNum(pos, &multibulk_len_) != 0) {
        // Protocol error: invalid multibulk length
        return kParseError;
      }
      next_parse_pos_ = pos + 1;
      argv_.clear();
      if (next_parse_pos_ > last_read_pos_) {
        return kReadHalf;
      }
    } else {
      return kReadHalf;  // HALF
    }
  }
  while (multibulk_len_) {
    if (bulk_len_ == -1) {
      pos = FindNextSeparators();
      if (pos != -1) {
        if (rbuf_[next_parse_pos_] != '$') {
           return kParseError;  // PARSE_ERROR
        }

        if (GetNextNum(pos, &bulk_len_) != 0) {
            // Protocol error: invalid bulk length
            return kParseError;
        }
        next_parse_pos_ = pos + 1;
      }
      if (pos == -1 || next_parse_pos_ > last_read_pos_) {
        return kReadHalf;
      }
    }
    if (last_read_pos_ - next_parse_pos_ + 1 < bulk_len_ + 2) {
      // Data not enough
      break;
    } else {
      argv_.emplace_back(rbuf_ + next_parse_pos_, bulk_len_);
      next_parse_pos_ = next_parse_pos_ + bulk_len_ + 2;
      bulk_len_ = -1;
      multibulk_len_--;
    }
  }

  if (multibulk_len_ == 0) {
    return kReadAll;  // OK
  } else {
    return kReadHalf;  // HALF
  }
}

ReadStatus RedisConn::ProcessInputBuffer() {
  ReadStatus ret;
  while (next_parse_pos_ <= last_read_pos_) {
    if (!req_type_) {
      if (rbuf_[next_parse_pos_] == '*') {
        req_type_ = REDIS_REQ_MULTIBULK;
      } else {
        req_type_ = REDIS_REQ_INLINE;
      }
    }

    if (req_type_ == REDIS_REQ_INLINE) {
      ret = ProcessInlineBuffer();
      if (ret != kReadAll) {
        return ret;
      }
    } else if (req_type_ == REDIS_REQ_MULTIBULK) {
      ret = ProcessMultibulkBuffer();
      if (ret != kReadAll) { // FULL_ERROR || HALF || PARSE_ERROR
        return ret;
      }
    } else {
      // Unknown requeset type;
      return kParseError;
    }

    if (!argv_.empty()) {
      DealMessage(argv_, &response_);
      wbuf_len_ = response_.size();
      if (wbuf_len_ > 0) {
        set_is_reply(true);
      }
    }
    // ResetClient
    argv_.clear();
    req_type_ = 0;
    multibulk_len_ = 0;
    bulk_len_ = -1;
  }

  return kReadAll; // OK
}

ReadStatus RedisConn::GetRequest() {
  ssize_t nread = 0;
  int next_read_pos = last_read_pos_ + 1;

  int remain = rbuf_len_ - next_read_pos;  // Remain buffer size
  if (remain == 0) {
    size_t new_size = rbuf_len_ + REDIS_IOBUF_LEN;
    if (new_size > REDIS_MAX_MESSAGE) {
      return kFullError;
    }
    rbuf_ = static_cast<char*>(realloc(rbuf_, new_size));
    if (rbuf_ == nullptr) {
      return kFullError;
    }
    rbuf_len_ = new_size;
    remain += REDIS_IOBUF_LEN;
  }

  nread = read(fd(), rbuf_ + next_read_pos, remain);
  if (nread == -1) {
    if (errno == EAGAIN) {
      nread = 0;
      return kReadHalf; // HALF
    } else {
      // error happened, close client
      return kReadError;
    }
  } else if (nread == 0) {
    // client closed, close client
    return kReadClose;
  }

  last_read_pos_ += nread;

  ReadStatus ret = ProcessInputBuffer();
  if (ret == kReadAll) {
    free(rbuf_);
    rbuf_ = nullptr; // Must be assigned to nullptr here for realloc(rbuf_, ...)
    rbuf_len_ = 0;
    next_parse_pos_ = 0;
    last_read_pos_ = -1;
  }

  return ret; // OK || HALF || FULL_ERROR || PARSE_ERROR
}

WriteStatus RedisConn::SendReply() {
  ssize_t nwritten = 0;
  while (wbuf_len_ > 0) {
    nwritten = write(fd(), response_.data() + wbuf_pos_, wbuf_len_ - wbuf_pos_);
    if (nwritten <= 0) {
      break;
    }
    wbuf_pos_ += nwritten;
    if (wbuf_pos_ == wbuf_len_) {
      // Have sended all response data
      wbuf_len_ = 0;
      wbuf_pos_ = 0;

      std::string buf;
      buf.reserve(DEFAULT_WBUF_SIZE);
      response_.swap(buf);
    }
  }
  if (nwritten == -1) {
    if (errno == EAGAIN) {
      return kWriteHalf;
    } else {
      // Here we should close the connection
      return kWriteError;
    }
  }
  if (wbuf_len_ == 0) {
    return kWriteAll;
  } else {
    return kWriteHalf;
  }
}

int RedisConn::ConstructPublishResp(const std::string& subscribe_channel,
                           const std::string& publish_channel,
                           const std::string& msg,
                           const bool pattern) {
  std::stringstream resp;
  std::string common_msg = "message";
  std::string pattern_msg = "pmessage";
  if (pattern) {
    resp << "*4\r\n" << "$" << pattern_msg.length()       << "\r\n" << pattern_msg       << "\r\n" <<
                        "$" << subscribe_channel.length() << "\r\n" << subscribe_channel << "\r\n" <<
                        "$" << publish_channel.length()   << "\r\n" << publish_channel   << "\r\n" <<
                        "$" << msg.length()               << "\r\n" << msg               << "\r\n";
  } else {
    resp << "*3\r\n" << "$" << common_msg.length()        << "\r\n" << common_msg        << "\r\n" <<
                        "$" << publish_channel.length()   << "\r\n" << publish_channel   << "\r\n" <<
                        "$" << msg.length()               << "\r\n" << msg               << "\r\n";
  }
  std::string str_resp = resp.str();
  wbuf_len_ += str_resp.size();
  response_.append(str_resp);

  // if ((wbuf_size_ - wbuf_len_ < str_resp.size())) {
  //   if (!ExpandWbufTo(wbuf_len_ + str_resp.size())) {
  //     memcpy(wbuf_, "-ERR expand writer buffer failed\r\n", 34);
  //     wbuf_len_ = 34;
  //     set_is_reply(true);
  //     return 0;
  //   }
  // }
  // memcpy(wbuf_ + wbuf_len_, str_resp.data(), str_resp.size());
  // wbuf_len_ += str_resp.size();

  set_is_reply(true);
  return 0;
}

std::string RedisConn::ConstructPubSubResp(
                                const std::string& cmd,
                                const std::vector<std::pair<std::string, int>>& result) {
  std::stringstream resp;
  if (result.size() == 0) {
    resp << "*3\r\n" << "$" << cmd.length() << "\r\n" << cmd << "\r\n" <<
                        "$" << -1           << "\r\n" << ":" << 0      << "\r\n";
  }
  for (auto it = result.begin(); it != result.end(); it++) {
    resp << "*3\r\n" << "$" << cmd.length()       << "\r\n" << cmd       << "\r\n" <<
                        "$" << it->first.length() << "\r\n" << it->first << "\r\n" <<
                        ":" << it->second         << "\r\n";
  }
  return resp.str();
}


int RedisConn::FindNextSeparators() {
  int pos = next_parse_pos_ <= last_read_pos_ ? next_parse_pos_ : 0;
  while (pos <= last_read_pos_) {
    if (rbuf_[pos] == '\n') {
      return pos;
    }
    pos++;
  }
  return -1;
}

int RedisConn::GetNextNum(int pos, long* value) {
  std::string tmp;
  assert(pos > next_parse_pos_);
  // next_parse_pos_    pos
  //      |    ----------|
  //      |    |
  //      *3\r\n
  // [next_parse_pos_ + 1, pos - next_parse_pos_- 2]

  if (slash::string2l(rbuf_ + next_parse_pos_ + 1,
                            pos - next_parse_pos_ - 2,
                            value)) {
    return 0; // Success
  }
  return -1; // Failed
}

}  // namespace pink
