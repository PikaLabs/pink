#include <stdlib.h>
#include <limits.h>
#include "redis_conn.h"
#include "pink_define.h"
#include "pink_util.h"
#include "worker_thread.h"
#include "xdebug.h"

#include <string>


RedisConn::RedisConn(int fd) :
  PinkConn(fd)
{
  rbuf_ = (char *)malloc(sizeof(char) * REDIS_MAX_MESSAGE);
  req_type_ = 0;
  multibulk_len_ = 0;
  bulk_len_ = -1;
  is_find_sep_ = true;
  is_overtake_ = false;
  last_read_pos_ = -1;
  next_parse_pos_ = 0;

  wbuf_ = (char *)malloc(sizeof(char) * REDIS_MAX_MESSAGE);
  wbuf_pos_ = 0;
  wbuf_len_ = 0;
}

RedisConn::~RedisConn()
{
  free(rbuf_);
  free(wbuf_);
}

bool RedisConn::SetNonblock()
{
  flags_ = Setnonblocking(fd());
  if (flags_ == -1) {
    return false;
  }
  return true;
}

ReadStatus RedisConn::ProcessMultibulkBuffer() {
  int32_t pos;
  if (multibulk_len_ == 0) {
    /* The client should have been reset */;
    //

    pos = FindNextSeparators();
    if (pos != -1) {
        if (GetNextNum(pos, &multibulk_len_) != 0) {
            //Protocol error: invalid multibulk length
            return kParseError; 
        }
      next_parse_pos_ = (pos + 1) % REDIS_MAX_MESSAGE;
      if ((last_read_pos_ + 1) % REDIS_MAX_MESSAGE == next_parse_pos_) {
        return kReadHalf;
      }
      argv_.clear();
    } else {
      if ((last_read_pos_ + 1) % REDIS_MAX_MESSAGE == next_parse_pos_) {
        return kFullError; /*FULL_ERROR*/
      } else {
        return kReadHalf; /*HALF*/
      }
    }
  }
  std::string tmp;
  while (!is_overtake_ && multibulk_len_) {
    if (bulk_len_ == -1) {
      pos = FindNextSeparators();
      if (pos != -1) {
        if (rbuf_[next_parse_pos_] != '$') {
           return kParseError;//PARSE_ERROR
        }

        if (GetNextNum(pos, &bulk_len_) != 0) {
            //Protocol error: invalid bulk length
            return kParseError; 
        }
        next_parse_pos_ = (pos + 1) % REDIS_MAX_MESSAGE;
        if ((last_read_pos_ + 1) % REDIS_MAX_MESSAGE == next_parse_pos_) {
          return kReadHalf;
        }
      } else {
        if ((last_read_pos_ + 1) % REDIS_MAX_MESSAGE == next_parse_pos_) {
          return kFullError; /*FULL_ERROR*/
        } else {
          return kReadHalf; /*HALF*/
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
          tmp = std::string(rbuf_ + next_parse_pos_, REDIS_MAX_MESSAGE - next_parse_pos_) + std::string(rbuf_, bulk_len_ - (REDIS_MAX_MESSAGE - next_parse_pos_));
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
    return kReadAll; /*OK*/
  } else {
    return kReadHalf; /*HALF*/
  }
}

void RedisConn::ResetClient() {
    argv_.clear();
    req_type_ = 0;
    multibulk_len_ = 0;
    bulk_len_ = -1;
}

ReadStatus RedisConn::ProcessInputBuffer() {
  ReadStatus ret;
  while (/*(last_read_pos_ + 1) % REDIS_MAX_MESSAGE != next_parse_pos_ && */!is_overtake_) { //is_find_sep_ ?
    if (!req_type_) {
      if (rbuf_[next_parse_pos_] == '*') {
        req_type_ = REDIS_REQ_MULTIBULK;
      } else {
        req_type_ = REDIS_REQ_INLINE;
      }
    }

    if (req_type_ == REDIS_REQ_INLINE) {
      //ProcessInlineBuffer();
      return kReadAll;
    } else if (req_type_ == REDIS_REQ_MULTIBULK) {
      ret = ProcessMultibulkBuffer();
      if (ret != kReadAll/*OK*/) { //FULL_ERROR || HALF || PARSE_ERROR
        return ret;
      }
    } else {
      //Unknown requeset type;
      return kParseError;
    }

    if (argv_.size() == 0) {
      ResetClient();
    } else {
      DealMessage(); 
      set_is_reply(true);
    }
  }
  return kReadAll;/*OK*/
}

ReadStatus RedisConn::GetRequest()
{
  ssize_t nread = 0;
  int32_t next_read_pos = (last_read_pos_ + 1) % REDIS_MAX_MESSAGE;
  int32_t read_len = 0;
  if (next_read_pos == next_parse_pos_ && !is_find_sep_) {
    //too big message, close client;
    //err_msg_ = "-ERR: Protocol error: too big mbulk count string\r\n";  
    return kParseError;
  } else if (next_read_pos >= next_parse_pos_) {
    read_len = REDIS_MAX_MESSAGE - next_read_pos;
  } else if (next_read_pos < next_parse_pos_) {
    read_len = next_parse_pos_ - next_read_pos;
  } 

  nread = read(fd(), rbuf_ + next_read_pos, read_len);
  if (nread == -1) {
    if (errno == EAGAIN) {
      nread = 0;
      return kReadHalf; //HALF
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
  return ret; //OK || HALF || FULL_ERROR || PARSE_ERROR
}

WriteStatus RedisConn::SendReply()
{
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
