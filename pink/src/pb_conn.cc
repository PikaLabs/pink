// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include "pink/include/pb_conn.h"

#include <arpa/inet.h>
#include <string>

#include "slash/include/xdebug.h"
#include "pink/include/pink_define.h"

namespace pink {

PbConn::PbConn(const int fd, const std::string &ip_port, ServerThread *thread) :
  PinkConn(fd, ip_port, thread),
  header_len_(-1),
  cur_pos_(0),
  rbuf_len_(0),
  remain_packet_len_(0),
  connStatus_(kHeader),
  wbuf_len_(0),
  wbuf_pos_(0) {
  rbuf_ = reinterpret_cast<char *>(malloc(sizeof(char) * kProtoMaxMessage));
  wbuf_ = reinterpret_cast<char *>(malloc(sizeof(char) * kProtoMaxMessage));
}

PbConn::~PbConn() {
  free(rbuf_);
  free(wbuf_);
}

// Msg is [ length(COMMAND_HEADER_LENGTH) | body(length bytes) ]
//   step 1. kHeader, we read COMMAND_HEADER_LENGTH bytes;
//   step 2. kPacket, we read header_len bytes;
ReadStatus PbConn::GetRequest() {
  while (true) {
    switch (connStatus_) {
      case kHeader: {
        ssize_t nread = read(
            fd(), rbuf_ + rbuf_len_, COMMAND_HEADER_LENGTH - rbuf_len_);
        if (nread == -1) {
          if (errno == EAGAIN) {
            return kReadHalf;
          } else {
            return kReadError;
          }
        } else if (nread == 0) {
          return kReadClose;
        } else {
          rbuf_len_ += nread;
          if (rbuf_len_ - cur_pos_ == COMMAND_HEADER_LENGTH) {
            uint32_t integer = 0;
            memcpy(reinterpret_cast<char*>(&integer),
                   rbuf_ + cur_pos_, sizeof(uint32_t));
            header_len_ = ntohl(integer);
            remain_packet_len_ = header_len_;
            cur_pos_ += COMMAND_HEADER_LENGTH;
            connStatus_ = kPacket;
            continue;
          }
          return kReadHalf;
        }
      }
      case kPacket: {
        if (header_len_ >= kProtoMaxMessage - COMMAND_HEADER_LENGTH) {
          return kFullError;
        } else {
          // read msg body
          ssize_t nread = read(fd(), rbuf_ + rbuf_len_, remain_packet_len_);
          if (nread == -1) {
            if (errno == EAGAIN) {
              return kReadHalf;
            } else {
              return kReadError;
            }
          } else if (nread == 0) {
            return kReadClose;
          }

          rbuf_len_ += nread;
          remain_packet_len_ -= nread;
          if (remain_packet_len_ == 0) {
            cur_pos_ = rbuf_len_;
            connStatus_ = kComplete;
            continue;
          }
          return kReadHalf;
        }
      }
      case kComplete: {
        if (DealMessage() != 0) {
          return kDealError;
        }
        connStatus_ = kHeader;
        cur_pos_ = 0;
        rbuf_len_ = 0;
        return kReadAll;
      }
      // Add this switch case just for delete compile warning
      case kBuildObuf:
        break;

      case kWriteObuf:
        break;
    }
  }

  return kReadHalf;
}

WriteStatus PbConn::SendReply() {
  if (!BuildObuf().ok()) {
    return kWriteError;
  }
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

Status PbConn::BuildObuf() {
  wbuf_len_ = res_->ByteSize();
  if (wbuf_len_ > kProtoMaxMessage - 4
      || !(res_->SerializeToArray(wbuf_ + 4, wbuf_len_))) {
    return Status::Corruption("Serialize to buffer failed");
  }
  uint32_t u;
  u = htonl(wbuf_len_);
  memcpy(wbuf_, &u, sizeof(uint32_t));
  wbuf_len_ += COMMAND_HEADER_LENGTH;

  return Status::OK();
}

}  // namespace pink
