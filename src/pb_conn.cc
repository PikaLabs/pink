#include "pb_conn.h"
#include "pink_define.h"
#include "pink_util.h"
#include "pb_thread.h"
#include "xdebug.h"

#include <string>

void PbConn::InitPara()
{
  rbuf_ = (char *)malloc(sizeof(char) * PB_MAX_MESSAGE);
  header_len_ = -1;
  cur_pos_ = 0;
  rbuf_len_ = 0;

  wbuf_ = (char *)malloc(sizeof(char) * PB_MAX_MESSAGE);
}

PbConn::PbConn(int fd) :
  fd_(fd)
{
	InitPara();
}

PbConn::PbConn(int fd, PbThread *phThread) :
  fd_(fd),
  pbThread_(phThread)
{
    log_info("PbConn construct pbHandler");
    InitPara();
}

PbConn::PbConn()
{
  InitPara();
}

PbConn::~PbConn()
{
  free(rbuf_);
  free(wbuf_);
}

bool PbConn::SetNonblock()
{
  flags_ = Setnonblocking(fd_);
  if (flags_ == -1) {
    return false;
  }
  return true;
}


Status PbConn::PbReadBuf()
{
  Status s;
  rio_t rio;
  rio_readinitb(&rio, fd_);

  char buffer[4];
  int packetLen;
  while (1) {
    //Peek into the socket and get the packet size
    if((packetLen = recv(fd_, buffer, 4, MSG_PEEK))== -1){

      log_info("Error receiving data %d\n", errno);
    } else if (packetLen == 0) {
      break;
    }
    log_info("packetLen is %d\n", packetLen);

  }
  s = PbReadPacket(&rio);
  return s;
}

int PbConn::PbGetRequest()
{
  log_info("In the pbgetrequest");
  ssize_t nread = 0;
  nread = read(fd_, rbuf_ + rbuf_len_, PB_MAX_MESSAGE);

  if (nread == -1) {
    if (errno == EAGAIN) {
      nread = 0;
    } else {
      return -1;
    }
  } else if (nread == 0) {
    return -1;
  }

  int32_t integer = 0;
  bool flag = true;


  if (nread) {
    rbuf_len_ += nread;
    while (flag) {
      switch (connStatus_) {
        case kHeader:
          if (rbuf_len_ - cur_pos_ >= COMMAND_HEADER_LENGTH) {
            memcpy((char *)(&integer), rbuf_ + cur_pos_, sizeof(int32_t));
            log_info("integer is %d", integer);
            header_len_ = ntohl(integer);
            log_info("Header_len %d", header_len_);
            connStatus_ = kPacket;
            cur_pos_ += COMMAND_HEADER_LENGTH;
          } else {
            flag = false;
          }
          break;
        case kPacket:
          if (rbuf_len_ >= header_len_) {
            cur_pos_ += header_len_;
            connStatus_ = kComplete;
          } else {
            flag = false;
          }
          break;
        case kComplete:
          log_info("header_len %d rbuf_ %s complete", header_len_, rbuf_ + COMMAND_HEADER_LENGTH);
          pbThread_->DealMessage(rbuf_ + COMMAND_HEADER_LENGTH, header_len_);

          return 0;
          break;

          /*
           * Add this switch case just for delete compile warning
           */
        case kBuildObuf:
          break;

        case kWriteObuf:
          break;
      }
    }
  }
  return -1;
}

int PbConn::PbSendReply()
{
  ssize_t nwritten = 0;
  log_info("wbuf_len %d", wbuf_len_);
  while (wbuf_len_ > 0) {
    nwritten = write(fd_, wbuf_ + wbuf_pos_, wbuf_len_ - wbuf_pos_);
    if (nwritten <= 0) {
      break;
    }
    wbuf_pos_ += nwritten;
    if (wbuf_pos_ == wbuf_len_) {
      wbuf_len_ = 0;
      wbuf_pos_ = 0;
    }

    if (nwritten == -1) {
      if (errno == EAGAIN) {
        nwritten = 0;
      } else {
        /*
         * Here we clear this connection
         */
        return 0;
      }
    }
  }
  if (wbuf_len_ == 0) {
    return 0;
  } else {
    return -1;
  }
}

Status PbConn::PbReadPacket(rio_t *rio)
{
  Status s;
  int nread = 0;
  if (header_len_ < 4) {
    return Status::Corruption("The packet no integrity");
  }
  while (1) {
    nread = rio_readnb(rio, (void *)(rbuf_ + COMMAND_HEADER_LENGTH), header_len_ - 4);
    if (nread == -1) {
      if ((errno == EAGAIN && (flags_ & O_NONBLOCK)) || (errno == EINTR)) {
        continue;
      } else {
        s = Status::IOError("Read data error");
        return s;
      }
    } else if (nread == 0) {
      return Status::Corruption("Connect has interrupt");
    } else {
      break;
    }
  }
  rbuf_len_ = nread;
  log_info("rbuf len %d", rbuf_len_);
  return Status::OK();
}

Status PbConn::BuildObuf(int32_t opcode, const int packet_len)
{
  uint32_t code_len = COMMAND_CODE_LENGTH + packet_len;
  uint32_t u;

  u = htonl(code_len);
  memcpy(wbuf_, &u, sizeof(uint32_t));
  u = htonl(opcode);
  memcpy(wbuf_ + COMMAND_CODE_LENGTH, &u, sizeof(uint32_t));

  wbuf_len_ = COMMAND_HEADER_LENGTH + COMMAND_CODE_LENGTH + packet_len;

  return Status::OK();
}
