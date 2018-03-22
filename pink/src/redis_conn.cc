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
      msg_peak_(0),
      wbuf_pos_(0),
      last_read_pos_(-1),
      next_parse_pos_(0),
      req_type_(0),
      multibulk_len_(0),
      bulk_len_(-1) {
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
  // args \r\n
  std::string req_buf(rbuf_ + next_parse_pos_, pos + 1 - next_parse_pos_);

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
      if (DealMessage(argv_, &response_) != 0) {
        return kDealError;
      }
      if (!response_.empty()) {
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
  int new_size = 0;
  if (remain == 0) {
    new_size = rbuf_len_ + REDIS_IOBUF_LEN;
    remain += REDIS_IOBUF_LEN;
  } else if (remain < bulk_len_) {
    new_size = next_read_pos + bulk_len_;
    remain = bulk_len_;
  }
  if (new_size > rbuf_len_) {
    if (new_size > REDIS_MAX_MESSAGE) {
      return kFullError;
    }
    rbuf_ = static_cast<char*>(realloc(rbuf_, new_size));
    if (rbuf_ == nullptr) {
      return kFullError;
    }
    rbuf_len_ = new_size;
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

  // assert(nread > 0);
  last_read_pos_ += nread;
  msg_peak_ = last_read_pos_;

  ReadStatus ret = ProcessInputBuffer();
  if (ret == kReadAll) {
    next_parse_pos_ = 0;
    last_read_pos_ = -1;
  }

  return ret; // OK || HALF || FULL_ERROR || PARSE_ERROR
}

WriteStatus RedisConn::SendReply() {
  ssize_t nwritten = 0;
  size_t wbuf_len = response_.size();
  while (wbuf_len > 0) {
    nwritten = write(fd(), response_.data() + wbuf_pos_, wbuf_len - wbuf_pos_);
    if (nwritten <= 0) {
      break;
    }
    wbuf_pos_ += nwritten;
    if (wbuf_pos_ == wbuf_len) {
      // Have sended all response data
      if (wbuf_len > DEFAULT_WBUF_SIZE) {
        std::string buf;
        buf.reserve(DEFAULT_WBUF_SIZE);
        response_.swap(buf);
      }
      response_.clear();

      wbuf_len = 0;
      wbuf_pos_ = 0;
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
  if (wbuf_len == 0) {
    return kWriteAll;
  } else {
    return kWriteHalf;
  }
}

void RedisConn::WriteResp(const std::string& resp) {
  response_.append(resp);
  set_is_reply(true);
}

void RedisConn::TryResizeBuffer() {
  log_info("Current buffer size: %d", rbuf_len_);
  struct timeval now;
  gettimeofday(&now, nullptr);
  int idletime = now.tv_sec - last_interaction().tv_sec;
  if (rbuf_len_ > REDIS_MBULK_BIG_ARG &&
      ((rbuf_len_ / (msg_peak_ + 1)) > 2 || idletime > 2)) {
    int new_size =
      ((last_read_pos_ + REDIS_IOBUF_LEN) / REDIS_IOBUF_LEN) * REDIS_IOBUF_LEN;
    if (new_size < rbuf_len_) {
      rbuf_ = static_cast<char*>(realloc(rbuf_, new_size));
      rbuf_len_ = new_size;
      log_info("Resize buffer to %d, last_read_pos_: %d\n",
               rbuf_len_, last_read_pos_);
    }
    msg_peak_ = 0;
  }
}

int RedisConn::FindNextSeparators() {
  if (next_parse_pos_ > last_read_pos_) {
    return -1;
  }
  int pos = next_parse_pos_;
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
