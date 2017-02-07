// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
#include "http_conn.h"
#include <stdlib.h>
#include <limits.h>

#include <string>
#include<iostream>
#include <algorithm>

#include "pink_define.h"
#include "worker_thread.h"
#include "xdebug.h"

namespace pink {

const int HTTP_MAX_MESSAGE = 1024*100;
const int HTTP_MAX_HEADER = 1024;

HttpRequest::HttpRequest():
  method("GET"),
  path("/index"),
  headers(),
  query_params(),
  content() {
}

inline int find_lf(const char* data, int size) {
  const char* c = data;
  int count = 0;
  while (count < size) {
    if (*c == '\n') {
      break;
    }
    c++;
    count++;
  }
  return count;
}

void HttpRequest::ParseHeadLine(const char* data, int line_start,
    int line_end) {
  std::string param_key;
  std::string param_value;
  for (int i = line_start; i <= line_end; i++) {
    switch (parseStatus_) {
      case kHeaderMethod:
        if (data[i] != ' ') {
          method.push_back(data[i]);
        } else {
          parseStatus_ = kHeaderPath;
        }
        break;
      case kHeaderPath:
        if (data[i] != ' ') {
          path.push_back(data[i]);
        } else {
          parseStatus_ = kHeaderVersion;
        }
        break;
      case kHeaderVersion:
        if (data[i] != '\r' && data[i] != '\n') {
          version.push_back(data[i]);
        } else if (data[i] == '\n') {
          parseStatus_ = kHeaderParamKey;
        }
        break;
      case kHeaderParamKey:
        if (data[i] != ':' && data[i] != ' ') {
          param_key.push_back(data[i]);
        } else if (data[i] == ' ') {
          parseStatus_ = kHeaderParamValue;
        }
        break;
      case kHeaderParamValue:
        if (data[i] != '\r' && data[i] != '\n') {
          param_value.push_back(data[i]);
        } else if (data[i] == '\r') {
          parseStatus_ = kHeaderParamKey;
          headers[param_key] = param_value;
          parseStatus_ = kHeaderParamKey;
        }
        break;

      default:
        std::cout<< "wrong state parsing header";
    }
  }
}

bool HttpRequest::ParseHeadFromArray(const char* data, const int size) {
  parseStatus_ = kHeaderMethod;
  int remain_size = size;
  if (remain_size <= 5) {
    return false;
  }
  int line_start = 0;
  int line_end = 0;
  while (remain_size > 4) {
    line_end += find_lf(data + line_start, remain_size);
    if (line_end < line_start) {
      return false;
    }
    ParseHeadLine(data, line_start, line_end);
    remain_size -= line_end - line_start;
    line_start = line_end + 1;
  }
  return true;
}

void HttpRequest::Clear() {
  method.clear();
  path.clear();
  version.clear();
  headers.clear();
}

void HttpResponse::Clear() {
  content.clear();
}

uint32_t HttpResponse::SerializeToArray(void* data, const int size) const {
  memcpy(data, content.c_str(), std::min((int)content.size(), size));
  return std::min((int)content.size(), size);
}

HttpConn::HttpConn(const int fd, const std::string &ip_port) :
  PinkConn(fd, ip_port),
  header_len_(0),
  rbuf_len_(0),
  rbuf_size_(0),
  remain_packet_len_(0),
  wbuf_len_(0),
  wbuf_size_(102400),
  wbuf_pos_(0) {
  rbuf_ = (char *)malloc(sizeof(char) * HTTP_MAX_MESSAGE);
  wbuf_ = (char *)malloc(sizeof(char) * HTTP_MAX_MESSAGE);
  request_ = new HttpRequest();
  response_ = new HttpResponse();
}

HttpConn::~HttpConn() {
  free(rbuf_);
  free(wbuf_);
  delete request_;
  delete response_;
}

bool HttpConn::BuildResponseBuf() {
  wbuf_len_ = response_->SerializeToArray(wbuf_, wbuf_size_);
  return true;
}

/*
 * This func change connStatus to kParcket and build request_
 * kPacket means we begin read contents of the http message.
 *     rbuf_size_ : total_message_length we read
 *     header_pos_ : end of header part
 *     remain_packet_len_ : content we haven't read
 */

bool HttpConn::BuildRequestHeader() {
  connStatus_ = kPacket;
  request_->Clear();
  request_->ParseHeadFromArray(rbuf_, header_len_);
  auto iter = request_->headers.begin();
  iter = request_->headers.find("Content-Length");
  if (iter == request_->headers.end()) {
    remain_packet_len_ = 0;
  } else {
    remain_packet_len_ = atoi(request_->headers["content-length"].c_str());
  }

  return true;
}

bool HttpConn::BuildRequestContent() {
  request_->content.assign(rbuf_ + header_len_, rbuf_len_ - header_len_);
  return true;
}


/*
 * Bytes we read may include both header and content.This func
 * detach the end of header.
 */
void HttpConn::AddToHeader(ssize_t nread) {
  bool receive_full_header;
  char* sep_pos = strstr(rbuf_, "\r\n\r\n");
  if (sep_pos) {
    receive_full_header = true;
    header_len_ = sep_pos - rbuf_ + 4;
  }
  if (receive_full_header) {
    BuildRequestHeader();
  }
}

bool HttpConn::DealMessage() {
  response_->Clear();
  if (request_->method == std::string("GET")) {
    HandleGet(request_, response_);
  } else if (request_->method == std::string("POST")) {
    HandlePost(request_, response_);
  }
  set_is_reply(true);
  return true;
}

ReadStatus HttpConn::GetRequest() {
  ssize_t nread = 0;
  while (true) {
    switch (connStatus_) {
      case kHeader: {
        nread = read(fd(), rbuf_ + header_len_, HTTP_MAX_HEADER - header_len_);
        if (nread > 0) {
          rbuf_len_ += nread;
          AddToHeader(nread);
        }
        break;
      }
      case kPacket: {
        if (remain_packet_len_ <= 0) {
          connStatus_ = kComplete;
          break;
        }
        nread = read(fd(), rbuf_ + rbuf_len_, remain_packet_len_);
        if (nread > 0) {
          rbuf_len_ += nread;
          remain_packet_len_ -= nread;
          if (remain_packet_len_ <= 0) {
            connStatus_ = kComplete;
            BuildRequestContent();
          }
        }
        break;
      }
      case kComplete: {
        DealMessage();
        connStatus_ = kHeader;
        rbuf_len_ = 0;
        return kReadAll;
      }
      default: {
        return kReadError;
      }
    }
    if (nread == -1) {
      if (errno == EAGAIN) {
        return kReadHalf;
      }
      return kReadError;
    }
    if (nread == 0) {
      return kReadClose;
    }
  }
}

WriteStatus HttpConn::SendReply() {
  BuildResponseBuf();
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
      return kWriteError;
    }
  }
  if (wbuf_len_ == 0) {
    return kWriteAll;
  } else {
    return kWriteHalf;
  }
}

}  // namespace pink
