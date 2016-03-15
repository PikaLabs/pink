#include "redis_cli.h"

#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>

#include "pink_define.h"
#include "pink_cli_socket.h"
#include "xdebug.h"


namespace pink {

enum REDIS_STATUS {
  REDIS_ERR = -1,
  REDIS_OK,
  REDIS_REPLY_STRING = 1,
  REDIS_REPLY_ARRAY,
  REDIS_REPLY_INTEGER,
  REDIS_REPLY_NIL,
  REDIS_REPLY_STATUS,
  REDIS_REPLY_ERROR
};

RedisCli::RedisCli()
    : rbuf_len_(0),
    rbuf_pos_(0) {
  rbuf_ = (char *)malloc(sizeof(char) * REDIS_MAX_MESSAGE);
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

Status RedisCli::Recv(void *result) {
  log_info("The Recv function");

  if (GetReply() == REDIS_ERR) {
    return Status::IOError(std::string(strerror(errno)));
  }

  std::string *storage = reinterpret_cast<std::string *>(result);
  storage->assign(rbuf_);

  return Status::OK();
}

static ssize_t Read(int fd, char *buf, size_t count) {
  ssize_t nread;

  while (true) {
    nread = read(fd, buf, count);

    if (nread == -1) {
      if (errno == EINTR) {
        continue;
      } else {
        log_info("read error, %s", strerror(errno));
        return -1;
      }
    } else if (nread == 0) {    // read null
      continue;
    }

    return nread;
  }
}

int RedisCli::ProcessLineItem() {
  assert(rbuf_pos_ > 0);

  ssize_t nread;

  while (true) {
    if ((nread = Read(fd(), rbuf_ + rbuf_pos_, 1)) < 0) {
      return REDIS_ERR;
    }
    
		if (rbuf_[rbuf_pos_] == '\n') {
			rbuf_pos_--;
			rbuf_[rbuf_pos_] = '\0';
			break;
		}
		rbuf_pos_++;
  }
  return rbuf_pos_;
}

// We simply read one byte a time;
int RedisCli::GetReply() {
  int type;
  ssize_t nread;
  rbuf_pos_ = 0;
  
  if ((nread = Read(fd(), rbuf_ + rbuf_pos_, 1)) < 0) {
    return REDIS_ERR;
  }

  // Check reply type 
  switch (rbuf_[rbuf_pos_]) {
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
  rbuf_pos_++;

  switch(type) {
    case REDIS_REPLY_ERROR:
    case REDIS_REPLY_STATUS:
    case REDIS_REPLY_INTEGER:
      return ProcessLineItem();
    case REDIS_REPLY_STRING:
      // need processBulkItem(r);
    case REDIS_REPLY_ARRAY:
      // need processMultiBulkItem(r);
    default:
      return REDIS_ERR; /* Avoid warning. */
  }
}

};  // namespace pink
