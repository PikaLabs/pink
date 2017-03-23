// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.
#include "pink/include/http_conn.h"

#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

#include <string>
#include <algorithm>

#include "slash/include/slash_string.h"
#include "slash/include/xdebug.h"

#include "pink/include/pink_define.h"
#include "pink_util.h"

namespace pink {

static const uint32_t kHttpMaxMessage = 1024 * 1024 * 8;
static const uint32_t kHttpMaxHeader = 1024 * 64;

HttpRequest::HttpRequest():
  method("GET"),
  path("/index") {
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

bool HttpRequest::ParseHeadLine(const char* data, int line_start,
    int line_end, ParseStatus* parseStatus) {
  std::string param_key;
  std::string param_value;
  for (int i = line_start; i <= line_end; i++) {
    switch (*parseStatus) {
      case kHeaderMethod:
        if (data[i] != ' ') {
          method.push_back(data[i]);
        } else {
          *parseStatus = kHeaderPath;
        }
        break;
      case kHeaderPath:
        if (data[i] != ' ') {
          path.push_back(data[i]);
        } else {
          *parseStatus = kHeaderVersion;
        }
        break;
      case kHeaderVersion:
        if (data[i] != '\r' && data[i] != '\n') {
          version.push_back(data[i]);
        } else if (data[i] == '\n') {
          *parseStatus = kHeaderParamKey;
        }
        break;
      case kHeaderParamKey:
        if (data[i] != ':' && data[i] != ' ') {
          param_key.push_back(data[i]);
        } else if (data[i] == ' ') {
          *parseStatus = kHeaderParamValue;
        }
        break;
      case kHeaderParamValue:
        if (data[i] != '\r' && data[i] != '\n') {
          param_value.push_back(data[i]);
        } else if (data[i] == '\r') {
          headers[slash::StringToLower(param_key)] = param_value;
          *parseStatus = kHeaderParamKey;
        }
        break;

      default:
        return false;
    }
  }
  return true;
}

bool HttpRequest::ParseGetUrl() {
  size_t n = path.find('?');
  if (n == std::string::npos) {
    return true; // no parameter
  }
  if (!ParseParameters(path, n + 1)) {
    return false;
  }
  path.resize(n);
  return true;
}

// Parse query parameter from GET url or POST body
// format: key1=value1&key2=value2&key3=value3
bool HttpRequest::ParseParameters(const std::string data,
    size_t line_start) {
  size_t pre = line_start, mid, end;
  while (pre < data.size()) {
    mid = data.find('=', pre);
    if (mid == std::string::npos) {
      mid = data.size();
    }
    end = data.find('&', pre);
    if (end == std::string::npos) {
      end = data.size();
    }
    if (end <= mid) {
      // empty value
      query_params[data.substr(pre, end - pre)]
        = std::string();
      pre = end + 1;
    } else {
      query_params[data.substr(pre, mid - pre)]
        = data.substr(mid + 1, end - mid - 1);
      pre = end + 1;
    }
  }
  return true;
}

bool HttpRequest::ParseHeadFromArray(const char* data, const int size) {
  int remain_size = size;
  if (remain_size <= 5) {
    return false;
  }

  // Parse header line
  int line_start = 0;
  int line_end = 0;
  ParseStatus parseStatus = kHeaderMethod;
  while (remain_size > 4) {
    line_end += find_lf(data + line_start, remain_size);
    if (line_end < line_start) {
      return false;
    }
    if (!ParseHeadLine(data, line_start, line_end, &parseStatus)) {
      return false;
    }
    remain_size -= (line_end - line_start + 1);
		line_end++;
    line_start = line_end;
  }
  
  // Parse query parameter from GET url
  if (method == "GET" && !ParseGetUrl()) {
    return false;
  }
  return true;
}

bool HttpRequest::ParseBodyFromArray(const char* data, const int size) {
  content.append(data, size);
  if (method == "POST") {
    return ParseParameters(content);
  }
  return true;
}

void HttpRequest::Clear() {
  version.clear();
  path.clear();
  method.clear();
  query_params.clear();
  headers.clear();
  content.clear();
}

void HttpResponse::Clear() {
  status_code_ = 0;
  reason_phrase_.clear();
  headers_.clear();
  body_.clear();
}

// Return bytes actual be writen, should be less than size
int HttpResponse::SerializeHeaderToArray(char* data, size_t size) {
  int serial_size = 0;
  int ret;

  // Serialize statues line
  ret = snprintf(data, size, "HTTP/1.1 %d %s\r\n",
                 status_code_, reason_phrase_.c_str());
  if (ret < 0 || ret == static_cast<int>(size)) {
    return ret;
  }
  serial_size += ret;

  // Serialize header
  SetHeaders("Content-Length", body_.size());
  for (auto &line : headers_) {
    ret = snprintf(data + serial_size, size - serial_size, "%s: %s\r\n",
                   line.first.c_str(), line.second.c_str());
    if (ret < 0) {
      return ret;
    }
    serial_size += ret;
    if (serial_size == static_cast<int>(size)) {
      return serial_size;
    }
  }

  ret = snprintf(data + serial_size, size - serial_size, "\r\n");
  serial_size += ret;
  return serial_size;
}

// Serialize body begin from 'pos', return the new pos
int HttpResponse::SerializeBodyToArray(char* data, size_t size, int* pos) {
  // Serialize body
  int actual = size;
  if (body_.size() - *pos < size) {
    actual = body_.size() - *pos;
  }
  memcpy(data, body_.data() + *pos, actual);
  *pos += actual;
  return actual;
}

void HttpResponse::SetStatusCode(int code) {
  status_code_ = code;
  if (code < 200) {
    assert(code >= 100 && code <= 102);
    reason_phrase_.assign(message_phrase[code - 100]);
  } else if (code < 300) {
    assert(code >= 200 && code <= 207);
    reason_phrase_.assign(success_phrase[code - 200]);
  } else if (code < 500) {
    assert(code >= 400&& code <= 409);
    reason_phrase_.assign(request_error[code - 400]);
  } else {
    assert(code >= 500 && code <= 509);
    reason_phrase_.assign(server_error[code - 500]);
  }
}

HttpConn::HttpConn(const int fd, const std::string &ip_port, Thread *thread) :
  PinkConn(fd, ip_port, thread),
  conn_status_(kHeader),
  rbuf_pos_(0),
  wbuf_len_(0),
  wbuf_pos_(0),
  header_len_(0),
  remain_packet_len_(0),
  response_pos_(-1) {
  rbuf_ = (char *)malloc(sizeof(char) * kHttpMaxMessage);
  wbuf_ = (char *)malloc(sizeof(char) * kHttpMaxMessage);
  request_ = new HttpRequest();
  response_ = new HttpResponse();
}

HttpConn::~HttpConn() {
  free(rbuf_);
  free(wbuf_);
  delete request_;
  delete response_;
}

/*
 * Build request_
 */
bool HttpConn::BuildRequestHeader() {
  request_->Clear();
  if (!request_->ParseHeadFromArray(rbuf_, header_len_)) {
    return false;  
  }
  auto iter = request_->headers.begin();
  iter = request_->headers.find("content-length");
  if (iter == request_->headers.end()) {
    remain_packet_len_ = 0;
  } else {
    int64_t tmp = 0;
    slash::string2l(iter->second.data(), iter->second.size(),
        static_cast<long*>(&tmp));
    remain_packet_len_ = tmp;
  }

  if (rbuf_pos_ > header_len_) {
    remain_packet_len_ -= rbuf_pos_ - header_len_;
  }
  return true;
}

bool HttpConn::AppendRequestBody() {
  return request_->ParseBodyFromArray(rbuf_ + header_len_,
      rbuf_pos_  - header_len_);
}

void HttpConn::HandleMessage() {
  response_->Clear();
  DealMessage(request_, response_);
  set_is_reply(true);
}

ReadStatus HttpConn::GetRequest() {
  ssize_t nread = 0;
  while (true) {
    switch (conn_status_) {
      case kHeader: {
        nread = read(fd(), rbuf_ + rbuf_pos_, kHttpMaxHeader - rbuf_pos_);
        if (nread == -1 && errno == EAGAIN) {
          return kReadHalf;
        } else if (nread <= 0) {
          return kReadClose;
        } else {
          rbuf_pos_ += nread;
          rbuf_[rbuf_pos_] = '\0'; // So that strstr will not parse the expire char
          char *sep_pos = strstr(rbuf_, "\r\n\r\n");
          if (!sep_pos) {
            break;
          }
          header_len_ = sep_pos - rbuf_ + 4;
          if (!BuildRequestHeader()) {
            return kReadError;
          }
          conn_status_ = kPacket;
        }
        break;
      }
      case kPacket: {
        if (remain_packet_len_ > 0) {
          nread = read(fd(), rbuf_ + rbuf_pos_,
              (kHttpMaxMessage - rbuf_pos_ > remain_packet_len_)
              ? remain_packet_len_ : kHttpMaxMessage - rbuf_pos_);
          if (nread == -1 && errno == EAGAIN) {
            return kReadHalf;
          } else if (nread <= 0) {
            return kReadClose;
          } else {
            rbuf_pos_ += nread;
            remain_packet_len_ -= nread;
          }
        }
        if (remain_packet_len_ == 0 || // no more content
            rbuf_pos_ == kHttpMaxMessage) { // buffer full
          AppendRequestBody();
          if (remain_packet_len_ == 0) {
            conn_status_ = kComplete;
          } else {
            rbuf_pos_ = header_len_ = 0; // read more packet content from begin
          }
        }
        break;
      }
      case kComplete: {
        HandleMessage();
        conn_status_ = kHeader;
        rbuf_pos_ = 0;
        return kReadAll;
      }
      default: {
        return kReadError;
      }
    }
    // else continue
  }
}

bool HttpConn::FillResponseBuf() {
  if (response_pos_ < 0) {
    // Not ever serialize response header
    int actual = response_->SerializeHeaderToArray(wbuf_ + wbuf_len_,
        kHttpMaxMessage - wbuf_len_);
    if (actual < 0) {
      return false;
    }
    wbuf_len_ += actual;
    response_pos_ = 0; // Serialize body next time
  }
  while (response_->HasMoreBody(response_pos_)
      && wbuf_len_ < kHttpMaxMessage) {
    // Has more body and more space in wbuf_
    wbuf_len_ += response_->SerializeBodyToArray(wbuf_ + wbuf_len_,
        kHttpMaxMessage - wbuf_len_, &response_pos_);
  }
  return true;
}

WriteStatus HttpConn::SendReply() {
  // Fill as more as content into the buf
  if (!FillResponseBuf()) {
    return kWriteError;
  }
  
  ssize_t nwritten = 0;
  while (wbuf_len_ > 0) {
    nwritten = write(fd(), wbuf_ + wbuf_pos_, wbuf_len_ - wbuf_pos_);
    if (nwritten == -1 && errno == EAGAIN) {
      return kWriteHalf;
    } else if (nwritten <= 0) {
      return kWriteError;
    }
    wbuf_pos_ += nwritten;
    if (wbuf_pos_ == wbuf_len_) {
      // Send all in wbuf_ and Try to fill more
      wbuf_len_ = 0;
      wbuf_pos_ = 0;
      if (!FillResponseBuf()) {
        return kWriteError;
      }
    }
  }
  response_pos_ = -1; // fill header first next time
  
  return kWriteAll;
}

}  // namespace pink
