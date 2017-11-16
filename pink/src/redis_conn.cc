// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <stdlib.h>
#include <limits.h>

#include <string>

#include "slash/include/xdebug.h"
#include "pink/include/pink_define.h"
#include "pink/include/redis_conn.h"

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
      wbuf_size_(DEFAULT_WBUF_SIZE),
      wbuf_len_(0),
      last_read_pos_(-1),
      next_parse_pos_(0),
      req_type_(0),
      multibulk_len_(0),
      bulk_len_(-1),
      is_find_sep_(true),
      is_overtake_(false),
      wbuf_pos_(0) {
  rbuf_ = reinterpret_cast<char*>(malloc(sizeof(char) * REDIS_MAX_MESSAGE));
  wbuf_ = reinterpret_cast<char*>(malloc(sizeof(char) * DEFAULT_WBUF_SIZE));
}

RedisConn::~RedisConn() {
  free(wbuf_);
  free(rbuf_);
}

ReadStatus RedisConn::ProcessInlineBuffer() {
  int32_t pos, ret;
  pos = FindNextSeparators();
  if (pos == -1 && (last_read_pos_ + 1) % REDIS_MAX_MESSAGE == next_parse_pos_) {
    return kFullError;
  }
  if (pos == -1 && (last_read_pos_ + 1) % REDIS_MAX_MESSAGE != next_parse_pos_) {
    return kReadHalf;
  }
  std::string req_buf;
  if (pos > next_parse_pos_) {
    req_buf = std::string(rbuf_ + next_parse_pos_, pos - next_parse_pos_-1);
  } else if (pos == 0) {
    req_buf = std::string(rbuf_ + next_parse_pos_, REDIS_MAX_MESSAGE - next_parse_pos_ - 1);
  } else {
    req_buf = std::string(rbuf_ + next_parse_pos_, REDIS_MAX_MESSAGE - next_parse_pos_) + std::string(rbuf_, pos - 1);
  }
  argv_.clear();
  ret = split2args(req_buf, argv_);
  next_parse_pos_ = (pos+1)%REDIS_MAX_MESSAGE;
  if (ret == -1) {
    return kParseError;
  }

  if ((last_read_pos_+1)%REDIS_MAX_MESSAGE == next_parse_pos_) {
    is_overtake_ = true;
  }
  return kReadAll;
}

ReadStatus RedisConn::ProcessMultibulkBuffer() {
  int32_t pos;
  if (multibulk_len_ == 0) {
    /* The client should have been reset */
    pos = FindNextSeparators();
    if (pos != -1) {
      if (GetNextNum(pos, &multibulk_len_) != 0) {
        // Protocol error: invalid multibulk length
        return kParseError;
      }
      next_parse_pos_ = (pos + 1) % REDIS_MAX_MESSAGE;
      argv_.clear();
      if ((last_read_pos_ + 1) % REDIS_MAX_MESSAGE == next_parse_pos_) {
        return kReadHalf;
      }
    } else {
      if ((last_read_pos_ + 1) % REDIS_MAX_MESSAGE == next_parse_pos_) {
        return kFullError;  // FULL_ERROR
      } else {
        return kReadHalf;  // HALF
      }
    }
  }
  std::string tmp;
  while (!is_overtake_ && multibulk_len_) {
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
        next_parse_pos_ = (pos + 1) % REDIS_MAX_MESSAGE;
        if ((last_read_pos_ + 1) % REDIS_MAX_MESSAGE == next_parse_pos_) {
          return kReadHalf;
        }
      } else {
        if ((last_read_pos_ + 1) % REDIS_MAX_MESSAGE == next_parse_pos_) {
          return kFullError;   // FULL_ERROR
        } else {
          return kReadHalf;   // HALF
        }
      }
    }
    if (next_parse_pos_ <= last_read_pos_) {
      if (last_read_pos_ - next_parse_pos_ + 1 < bulk_len_ + 2) {
        break;
      } else {
        argv_.push_back(std::string(rbuf_ + next_parse_pos_, bulk_len_));
        next_parse_pos_ = (next_parse_pos_ + (bulk_len_ + 2)) % REDIS_MAX_MESSAGE;
        bulk_len_ = -1;
        multibulk_len_--;
        if ((last_read_pos_ + 1) % REDIS_MAX_MESSAGE == next_parse_pos_) {
          is_overtake_ = true;
        }
      }
    } else {
      if (REDIS_MAX_MESSAGE - next_parse_pos_  + last_read_pos_ + 1 < bulk_len_ + 2) {
        if ((last_read_pos_ + 1) % REDIS_MAX_MESSAGE == next_parse_pos_) {
          return kFullError;
        }
        break;
      } else {
        if (REDIS_MAX_MESSAGE - next_parse_pos_ >= bulk_len_) {
          argv_.push_back(std::string(rbuf_ + next_parse_pos_, bulk_len_));
        } else {
          tmp = std::string(rbuf_ + next_parse_pos_,
                            REDIS_MAX_MESSAGE - next_parse_pos_) +
            std::string(rbuf_,
                        bulk_len_ - (REDIS_MAX_MESSAGE - next_parse_pos_));
          argv_.push_back(tmp);
        }
        next_parse_pos_ = bulk_len_ - (REDIS_MAX_MESSAGE - next_parse_pos_) + 2;
        bulk_len_ = -1;
        multibulk_len_--;
        if ((last_read_pos_ + 1) % REDIS_MAX_MESSAGE == next_parse_pos_) {
          is_overtake_ = true;
        }
      }
    }
  }

  if (multibulk_len_ == 0) {
    return kReadAll;  // OK
  } else {
    return kReadHalf;  // HALF
  }
}

void RedisConn::ResetClient() {
    argv_.clear();
    req_type_ = 0;
    multibulk_len_ = 0;
    bulk_len_ = -1;
}

bool RedisConn::ExpandWbufTo(uint32_t new_size) {
  if (new_size <= wbuf_size_) {
    return true;
  }
  void* new_wbuf = realloc(wbuf_, new_size);
  if (new_wbuf != nullptr) {
    wbuf_ = reinterpret_cast<char*>(new_wbuf);
    wbuf_size_ = new_size;
    return true;
  } else {
    wbuf_pos_ = 0;
    return false;
  }
}

ReadStatus RedisConn::ProcessInputBuffer() {
  ReadStatus ret;
  while (!is_overtake_) {
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
      DealMessage();
    }
    ResetClient();
  }
  req_type_ = 0;
  next_parse_pos_ = 0;
  last_read_pos_ = -1;
  return kReadAll; // OK
}

ReadStatus RedisConn::GetRequest() {
  ssize_t nread = 0;
  int32_t next_read_pos = (last_read_pos_ + 1) % REDIS_MAX_MESSAGE;
  int32_t read_len = 0;
  if (next_read_pos == next_parse_pos_ && !is_find_sep_) {
    // too big message, close client;
    // err_msg_ = "-ERR: Protocol error: too big mbulk count string\r\n";
    return kParseError;
  } else if (next_read_pos >= next_parse_pos_) {
    read_len = REDIS_IOBUF_LEN < (REDIS_MAX_MESSAGE - next_read_pos) ? REDIS_IOBUF_LEN : (REDIS_MAX_MESSAGE - next_read_pos);
  } else if (next_read_pos < next_parse_pos_) {
    read_len = next_parse_pos_ - next_read_pos;
  }

  nread = read(fd(), rbuf_ + next_read_pos, read_len);
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

  if (nread) {
    last_read_pos_ = (last_read_pos_ + nread) % REDIS_MAX_MESSAGE;
    is_overtake_ = false;
  }
  ReadStatus ret = ProcessInputBuffer();
  if (ret == kFullError/*FULL_ERROR*/) {
    is_find_sep_ = false;
  }
  return ret; // OK || HALF || FULL_ERROR || PARSE_ERROR
}

WriteStatus RedisConn::SendReply() {
  ssize_t nwritten = 0;
  while (wbuf_len_ > 0) {
    nwritten = write(fd(), wbuf_ + wbuf_pos_, wbuf_len_ - wbuf_pos_);
    if (nwritten <= 0) {
      break;
    }
    wbuf_pos_ += nwritten;
    if (wbuf_pos_ == wbuf_len_) {
      wbuf_len_ = 0;
      wbuf_pos_ = 0;
      if (wbuf_size_ > DEFAULT_WBUF_SIZE) {
        free(wbuf_);
        wbuf_ = reinterpret_cast<char*>(malloc(sizeof(char) *
              DEFAULT_WBUF_SIZE));
        wbuf_size_ = DEFAULT_WBUF_SIZE;
      }
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

int32_t RedisConn::FindNextSeparators() {
  int pos;
  if (next_parse_pos_ <= last_read_pos_) {
    pos = next_parse_pos_;
  } else {
    pos = 0;
  }
  while (pos <= last_read_pos_) {
    if (rbuf_[pos] == '\n') {
      return pos;
    }
    pos++;
  }
  return -1;
}

int32_t RedisConn::GetNextNum(int32_t pos, int32_t *value) {
  std::string tmp;
  if (pos > next_parse_pos_) {
    // [next_parse_pos_ + 1, pos - next_parse_pos_- 2]
    tmp = std::string(rbuf_ + next_parse_pos_ + 1, pos - next_parse_pos_ - 2);
  } else {
    if (pos != 0) {
      tmp = std::string(rbuf_ + ((next_parse_pos_ + 1) % REDIS_MAX_MESSAGE), REDIS_MAX_MESSAGE - next_parse_pos_ - 1) + std::string(rbuf_, pos - 1);
    } else {
      tmp = std::string(rbuf_ + ((next_parse_pos_ + 1) % REDIS_MAX_MESSAGE), REDIS_MAX_MESSAGE - next_parse_pos_ - 1);
    }
    // [next_parse_pos_ + 1, REDIS_MAX_MESSAGE - next_parse_pos_ - 1] + [0, pos - 1]
  }

  char* end;
  errno = 0;
  long num = strtol(tmp.c_str(), &end, 10);
  if ((num == 0 && errno == EINVAL) ||
          ((num == LONG_MAX || num == LONG_MIN) && errno == ERANGE)) {
    return -1;
  }

  if (value) *value = static_cast<int32_t>(num);
  return 0;
}

}  // namespace pink
