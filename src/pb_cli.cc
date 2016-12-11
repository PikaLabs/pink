// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/pb_cli.h"
#include "include/pink_define.h"
#include "include/xdebug.h"

namespace pink {

enum PB_STATUS {
  PB_ETIMEOUT = -2,
  PB_ERR = -1,
  PB_OK = 0,
};

PbCli::PbCli() :
  packet_len_(0),
  rbuf_len_(0),
  rbuf_pos_(0),
  wbuf_len_(0),
  wbuf_pos_(0) {
  rbuf_ = reinterpret_cast<char *>(malloc(sizeof(char) * PB_MAX_MESSAGE));
  wbuf_ = reinterpret_cast<char *>(malloc(sizeof(char) * PB_MAX_MESSAGE));
  scratch_ = reinterpret_cast<char *>(malloc(sizeof(char) * PB_MAX_MESSAGE));
}

PbCli::~PbCli() {
  free(scratch_);
  free(wbuf_);
  free(rbuf_);
}


void PbCli::BuildWbuf() {
  wbuf_len_ = msg_->ByteSize();
  msg_->SerializeToArray(wbuf_ + COMMAND_HEADER_LENGTH, wbuf_len_);
  uint32_t len;
  len = htonl(wbuf_len_);
  memcpy(wbuf_, &len, sizeof(uint32_t));
  wbuf_len_ += COMMAND_HEADER_LENGTH;

}

Status PbCli::Send(void *msg) {
  if (!Available()) {
    return Status::IOError("unavailable connection");
  }
  log_info("The Send function");
  msg_ = reinterpret_cast<google::protobuf::Message *>(msg);

  BuildWbuf();

  // TODO(chenzongzhi) already serialize 
  msg_->SerializeToArray(wbuf_ + sizeof(uint32_t), sizeof(wbuf_));

  Status s;

  /*
   * because the fd in PbCli is block so we just use rio_written
   */
  wbuf_pos_ = 0;
  log_info("wbuf_len_ %d", wbuf_len_);
  size_t nleft = wbuf_len_;
  ssize_t nwritten;
  while (nleft > 0) {
    if ((nwritten = write(fd(), wbuf_ + wbuf_pos_, nleft)) <= 0) {
      if (errno == EINTR) {
        nwritten = 0;
        continue;
      } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        s = Status::Timeout("Send timeout");
      } else {
        s = Status::IOError("write error " + std::string(strerror(errno)));
      }
      return s;
    }
    nleft -= nwritten;
    wbuf_pos_ += nwritten;
  }

  return s;
}

Status PbCli::Recv(void *msg_res) {
  if (!Available()) {
    return Status::IOError("unavailable connection");
  }
  log_info("The Recv function");
  msg_res_ = reinterpret_cast<google::protobuf::Message *>(msg_res);
  int ret = ReadHeader();
  if (ret != PB_OK) {
    if (ret == PB_ETIMEOUT) {
      return Status::Timeout("");
    } else {
      return Status::Corruption("read header error");
    }
  }
  log_info("packet_len_ %d", packet_len_);
  ret = ReadPacket();
  if (ret != PB_OK) {
    if (ret == PB_ETIMEOUT) {
      return Status::Timeout("");
    } else {
      return Status::Corruption("read packet error");
    }
  }
  msg_res_->ParseFromArray(rbuf_, packet_len_);

  return Status::OK();
}

 
int PbCli::ReadHeader() {
  int nread = 0;
  rbuf_pos_ = 0;
  size_t nleft = COMMAND_HEADER_LENGTH;
  log_info("nleft %d", nleft);
  while (nleft > 0) {
    nread = read(fd(), rbuf_ + rbuf_pos_, nleft);
    log_info("nread %d", nread);
    if (nread == -1) {
      log_err("nread %d", nread);
      if (errno == EINTR) {
        continue;
      } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return PB_ETIMEOUT;
      } else {
        log_info ("ReadHeader nread is -1 and error isnot EINTR, nleft=%d\n", nleft);
        return PB_ERR;
      }
    } else if (nread == 0) {
      /*
       * we read end of file here, but in our protocol we need more data
       * so this must be an incomplete packet
       */
      log_info ("ReadHeader nread is 0, nleft=%d\n", nleft);
      return PB_ERR;
    }
    nleft -= nread;
    rbuf_pos_ += nread;
  }

  uint32_t integer;
  memcpy((char *)(&integer), rbuf_, sizeof(uint32_t));
  packet_len_ = ntohl(integer);
  return PB_OK;
}

int PbCli::ReadPacket() {
  int nread = 0;
  rbuf_pos_ = 0;
  size_t nleft = packet_len_;
  while (nleft > 0) {
    nread = read(fd(), rbuf_ + rbuf_pos_, nleft);
    if (nread == -1) {
      if (errno == EINTR) {
        continue;
      } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return PB_ETIMEOUT;
      } else {
        log_info ("ReadPacket nread is -1 and error isnot EINTR, nleft=%d\n", nleft);
        return PB_ERR;
      }
    } else if (nread == 0) {
      /*
       * we read end of file here, but in our protocol we need more data
       * so this must be an incomplete packet, so we return an error
       */
      return PB_ERR;
    }
    nleft -= nread;
    rbuf_pos_ += nread;
  }

  return packet_len_;
}
};
