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


int PbConn::PbGetRequest()
{
  ssize_t nread = 0;
  nread = read(fd_, rbuf_ + rbuf_len_, PB_MAX_MESSAGE);

  if (nread == -1) {
    if (errno == EAGAIN) {
      nread = 0;
      return 1;
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
            log_info("k Packet cur_pos_ %d rbuf_len_ %d", cur_pos_, rbuf_len_);
            connStatus_ = kComplete;
          } else {
            flag = false;
          }
          break;
        case kComplete:
          pbThread_->DealMessage(rbuf_ + COMMAND_HEADER_LENGTH, header_len_, res_);

          BuildObuf();
          connStatus_ = kHeader;
          if (cur_pos_ == rbuf_len_) {
            cur_pos_ = 0;
            rbuf_len_ = 0;
          }
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
  log_info("wbuf_len_ is %d", wbuf_len_);
  ssize_t nwritten = 0;
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
  }
  if (nwritten == -1) {
    if (errno == EAGAIN) {
      nwritten = 0;
    } else {
      // Here we should close the connection
      return -1;
    }
  }
  if (wbuf_len_ == 0) {
    return 0;
  } else {
    return -1;
  }
}


Status PbConn::BuildObuf()
{
  wbuf_len_ = res_->ByteSize();
  res_->SerializeToArray(wbuf_ + 4, wbuf_len_);
  uint32_t u;
  u = htonl(wbuf_len_);
  memcpy(wbuf_, &u, sizeof(uint32_t));
  wbuf_len_ += COMMAND_HEADER_LENGTH;

  return Status::OK();
}
