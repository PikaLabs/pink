#include "pb_conn.h"
#include "pink_define.h"
#include "pink_util.h"
#include "worker_thread.h"
#include "xdebug.h"

#include <string>


PbConn::PbConn(int fd) :
  PinkConn(fd)
{
  rbuf_ = (char *)malloc(sizeof(char) * PB_MAX_MESSAGE);
  header_len_ = -1;
  cur_pos_ = 0;
  rbuf_len_ = 0;

  wbuf_ = (char *)malloc(sizeof(char) * PB_MAX_MESSAGE);
}

PbConn::~PbConn()
{
  free(rbuf_);
  free(wbuf_);
}

bool PbConn::SetNonblock()
{
  flags_ = Setnonblocking(fd());
  if (flags_ == -1) {
    return false;
  }
  return true;
}


ReadStatus PbConn::GetRequest()
{
  ssize_t nread = 0;
  nread = read(fd(), rbuf_ + rbuf_len_, PB_MAX_MESSAGE);

  if (nread == -1) {
    if (errno == EAGAIN) {
      return kReadHalf;
    } else {
      return kReadError;
    }
  } else if (nread == 0) {
    return kReadClose;
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
        DealMessage();

        BuildObuf();
        connStatus_ = kHeader;
        if (cur_pos_ == rbuf_len_) {
          cur_pos_ = 0;
          rbuf_len_ = 0;
        }
        return kReadAll;
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
  return kReadHalf;
}

WriteStatus PbConn::SendReply()
{
  log_info("wbuf_len_ is %d", wbuf_len_);
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
