#include "redis_cli.h"

#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>

#include "pink_define.h"
#include "pink_cli_socket.h"
#include "xdebug.h"


namespace pink {

enum REDIS_STATUS {
  REDIS_ERR = -1,
  REDIS_OK = 0,
  REDIS_HALF,
  REDIS_REPLY_STRING,
  REDIS_REPLY_ARRAY,
  REDIS_REPLY_INTEGER,
  REDIS_REPLY_NIL,
  REDIS_REPLY_STATUS,
  REDIS_REPLY_ERROR
};

RedisCli::RedisCli()
  : rbuf_size_(REDIS_MAX_MESSAGE),
    rbuf_pos_(0),
    rbuf_offset_(0),
    err_(REDIS_OK) {
      rbuf_ = (char *)malloc(sizeof(char) * rbuf_size_);
    }

RedisCli::~RedisCli() {
  free(rbuf_);
}

#if 0
void RedisCli::Init(int send_timeout, int recv_timeout, int connect_timeout) {
  if (send_timeout > 0) {
    cli_socket_->set_send_timeout(send_timeout);
  }
  if (recv_timeout > 0) {
    cli_socket_->set_recv_timeout(recv_timeout);
  }
  if (connect_timeout > 0) {
    cli_socket_->set_connect_timeout(connect_timeout);
  }
}
#endif

// We use passed-in send buffer here
Status RedisCli::Send(void *msg) {
  log_info("The Send function");
  Status s;

  std::string* storage = reinterpret_cast<std::string *>(msg);
  const char *wbuf = storage->data();
  size_t nleft = storage->size();
  int wbuf_pos = 0;

  ssize_t nwritten;

  while (nleft > 0) {
    if ((nwritten = write(fd(), wbuf + wbuf_pos, nleft)) <= 0) {
      if (errno == EINTR) {
        nwritten = 0;
        continue;
        // block will EAGAIN ?
      } else {
        s = Status::IOError(wbuf, "write context error");
        return s;
      }
    }

    nleft -= nwritten;
    wbuf_pos += nwritten;
  }

  return s;
}

// The result is useless
Status RedisCli::Recv(void *result) {
  log_info("The Recv function");

  if (GetReply() == REDIS_ERR) {
    return Status::IOError(std::string(strerror(errno)));
  }

  return Status::OK();
}

ssize_t RedisCli::BufferRead() {
  // memmove the remain chars to rbuf begin
  if (rbuf_pos_ > 0) {
    memmove(rbuf_, rbuf_ + rbuf_pos_, rbuf_offset_);
    rbuf_pos_ = 0;
  }

  ssize_t nread;

  while (true) {
    nread = read(fd(), rbuf_ + rbuf_offset_, rbuf_size_ - rbuf_offset_);

    if (nread == -1) {
      if (errno == EINTR) {
        continue;
      } else {
        log_info("read error, %s", strerror(errno));
        return -1;
      }
    } else if (nread == 0) {    // we consider read null an error 
      err_ = REDIS_ERR;
      return -1;
    }

    rbuf_offset_ += nread;
    return nread;
  }
}

/* Find pointer to \r\n. */
static char *seekNewline(char *s, size_t len) {
  int pos = 0;
  int _len = len-1;

  /* Position should be < len-1 because the character at "pos" should be
   * followed by a \n. Note that strchr cannot be used because it doesn't
   * allow to search a limited length and the buffer that is being searched
   * might not have a trailing NULL character. */
  while (pos < _len) {
    while (pos < _len && s[pos] != '\r') pos++;
    if (s[pos] != '\r') {
      /* Not found. */
      return NULL;
    } else {
      if (s[pos+1] == '\n') {
        /* Found. */
        return s+pos;
      } else {
        /* Continue searching. */
        pos++;
      }
    }
  }
  return NULL;
}

int RedisCli::ProcessLineItem() {
  char *p;
  int len;

  if ((p = ReadLine(&len)) == NULL) {
    return REDIS_HALF;
  }

  std::string arg(p, len);
  argv_.push_back(arg);

  return REDIS_OK;
}

int RedisCli::GetReply() {
  int result = REDIS_OK;

  while (true) {
    // Should read again
    if (rbuf_offset_ == 0 || result == REDIS_HALF) {
      if ((result = BufferRead()) < 0) {
        return REDIS_ERR;
      }
    }

    // REDIS_HALF will read and parse again
    if ((result = GetReplyFromReader()) != REDIS_HALF)
      return result;
  }
}

char* RedisCli::ReadBytes(unsigned int bytes) {
  char *p = NULL;
  if (rbuf_offset_ >= bytes) {
    p = rbuf_ + rbuf_pos_;
    rbuf_pos_ += bytes;
    rbuf_offset_ -= bytes;
  }
  return p;
}

char *RedisCli::ReadLine(int *_len) {
  char *p, *s;
  int len;

  p = rbuf_ + rbuf_pos_;
  s = seekNewline(rbuf_ + rbuf_pos_, rbuf_offset_);
  if (s != NULL) {
    len = s - (rbuf_ + rbuf_pos_); 
    rbuf_pos_ += len + 2; /* skip \r\n */
    rbuf_offset_ -= len + 2;
    if (_len) *_len = len;
    return p;
  }
  return NULL;
}

int RedisCli::GetReplyFromReader() {
  if (err_) {
    return REDIS_ERR;
  }

  if (rbuf_offset_ == 0) {
    return REDIS_HALF;
  }

  char *p;
  if ((p = ReadBytes(1)) == NULL) {
    return REDIS_HALF;
  }

  int type;
  // Check reply type 
  switch (*p) {
    case '-':
      type = REDIS_REPLY_ERROR;
      break;
    case '+':
      type = REDIS_REPLY_STATUS;
      break;
    case ':':
      type = REDIS_REPLY_INTEGER;
      break;
    case '$':
      type = REDIS_REPLY_STRING;
      break;
    case '*':
      type = REDIS_REPLY_ARRAY;
      break;
    default:
      return REDIS_ERR;
  }

  switch(type) {
    case REDIS_REPLY_ERROR:
    case REDIS_REPLY_STATUS:
    case REDIS_REPLY_INTEGER:
      return ProcessLineItem();
    case REDIS_REPLY_STRING:
      // need processBulkItem();
    case REDIS_REPLY_ARRAY:
      // need processMultiBulkItem();
    default:
      return REDIS_ERR; // Avoid warning.
  }
}

// Calculate the number of bytes needed to represent an integer as string.
static int intlen(int i) {
  int len = 0;
  if (i < 0) {
    len++;
    i = -i;
  }
  do {
    len++;
    i /= 10;
  } while(i);
  return len;
}

// Helper that calculates the bulk length given a certain string length.
static size_t bulklen(size_t len) {
  return 1 + intlen(len) + 2 + len + 2;
}

int redisvFormatCommand(std::string *cmd, const char *format, va_list ap) {
  const char *c = format;
  std::string curarg;
  char buf[REDIS_MAX_MESSAGE];
  std::vector<std::string> args;
  int touched = 0; /* was the current argument touched? */
  size_t totlen = 0;

  while (*c != '\0') {
    if (*c != '%' || c[1] == '\0') {
      if (*c == ' ') {
        if (touched) {
          args.push_back(curarg);
          totlen += bulklen(curarg.size());
          curarg.clear();
          touched = 0;
        }
      } else {
        curarg.append(c, 1);
        touched = 1;
      }
    } else {
      char *arg;
      size_t size;

      switch (c[1]) {
        case 's':
          arg = va_arg(ap, char*);
          size = strlen(arg);
          if (size > 0) {
            curarg.append(arg, size);
          }
          break;
        case 'b':
          arg = va_arg(ap, char*);
          size = va_arg(ap, size_t);
          if (size > 0) {
            curarg.append(arg, size);
          }
          break;
        case '%':
          curarg.append(arg, size);
          break;
        default:
          /* Try to detect printf format */
          {
            static const char intfmts[] = "diouxX";
            char _format[16];
            const char *_p = c+1;
            size_t _l = 0;
            va_list _cpy;
            bool fmt_valid = false;

            /* Flags */
            if (*_p != '\0' && *_p == '#') _p++;
            if (*_p != '\0' && *_p == '0') _p++;
            if (*_p != '\0' && *_p == '-') _p++;
            if (*_p != '\0' && *_p == ' ') _p++;
            if (*_p != '\0' && *_p == '+') _p++;

            /* Field width */
            while (*_p != '\0' && isdigit(*_p)) _p++;

            /* Precision */
            if (*_p == '.') {
              _p++;
              while (*_p != '\0' && isdigit(*_p)) _p++;
            }

            /* Copy va_list before consuming with va_arg */
            va_copy(_cpy, ap);

            if (strchr(intfmts, *_p) != NULL) {       /* Integer conversion (without modifiers) */
              va_arg(ap,int);
              fmt_valid = true;
            } else if (strchr("eEfFgGaA",*_p) != NULL) { /* Double conversion (without modifiers) */
              va_arg(ap, double);
              fmt_valid = true;
            } else if (_p[0] == 'h' && _p[1] == 'h') {    /* Size: char */
              _p += 2;
              if (*_p != '\0' && strchr(intfmts,*_p) != NULL) {
                va_arg(ap,int); /* char gets promoted to int */
                fmt_valid = true;
              }
            } else if (_p[0] == 'h') {                /* Size: short */
              _p += 1;
              if (*_p != '\0' && strchr(intfmts,*_p) != NULL) {
                va_arg(ap,int); /* short gets promoted to int */
                fmt_valid = true;
              }
            } else if (_p[0] == 'l' && _p[1] == 'l') { /* Size: long long */
              _p += 2;
              if (*_p != '\0' && strchr(intfmts,*_p) != NULL) {
                va_arg(ap, long long);
                fmt_valid = true;
              }
            } else if (_p[0] == 'l') {           /* Size: long */
              _p += 1;
              if (*_p != '\0' && strchr(intfmts,*_p) != NULL) {
                va_arg(ap,long);
                fmt_valid = true;
              }
            }

            if (!fmt_valid) {
              va_end(_cpy);
              return REDIS_ERR;
            }

            _l = (_p + 1) - c;
            if (_l < sizeof(_format) - 2) {
              memcpy(_format, c, _l);
              _format[_l] = '\0';

              int n = vsnprintf (buf, REDIS_MAX_MESSAGE, _format, _cpy);
              curarg.append(buf, n);

              /* Update current position (note: outer blocks
               * increment c twice so compensate here) */
              c = _p - 1;
            }

            va_end(_cpy);
            break;
          }
      }

      if (curarg.empty()) {
        return REDIS_ERR;
      }

      touched = 1;
      c++;
    }
    c++;
  }

  /* Add the last argument if needed */
  if (touched) {
    args.push_back(curarg);
    totlen += bulklen(curarg.size());
  }

  /* Add bytes needed to hold multi bulk count */
  totlen += 1 + intlen(args.size()) + 2;

  /* Build the command at protocol level */
  cmd->clear();
  cmd->reserve(totlen);

  cmd->append(1, '*');
  cmd->append(std::to_string(args.size()));
  cmd->append("\r\n");
  for (size_t i = 0; i < args.size(); i++) {
    cmd->append(1, '$');
    cmd->append(std::to_string(args[i].size()));
    cmd->append("\r\n");
    cmd->append(args[i]);
    cmd->append("\r\n");
  }
  assert(cmd->size() == totlen);

  return totlen;
}

int redisvAppendCommand(std::string *cmd, const char *format, va_list ap) {
  int len = redisvFormatCommand(cmd, format, ap);
  if (len == -1) {
    return REDIS_ERR;
  }

  return REDIS_OK;
}

int RedisCli::SerializeCommand(std::string *cmd, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  int result = redisvAppendCommand(cmd, format, ap);
  va_end(ap);
  return result;
}

};  // namespace pink
