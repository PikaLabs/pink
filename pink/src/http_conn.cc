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

#include "slash/include/xdebug.h"
#include "slash/include/slash_string.h"
#include "pink/include/pink_define.h"
#include "pink/src/worker_thread.h"

namespace pink {

static const uint32_t kHttpMaxMessage = 1024 * 1024 * 8;
static const uint32_t kHttpMaxHeader = 1024 * 1024 * 8;

static const std::map<int, std::string> http_status_map = {
  {100, "Continue"},
  {101, "Switching Protocols"},
  {102, "Processing"},

  {200, "OK"},
  {201, "Created"},
  {202, "Accepted"},
  {203, "Non-Authoritative Information"},
  {204, "No Content"},
  {205, "Reset Content"},
  {206, "Partial Content"},
  {207, "Multi-Status"},

  {400, "Bad Request"},
  {401, "Unauthorized"},
  {402, ""},  // reserve
  {403, "Forbidden"},
  {404, "Not Found"},
  {405, "Method Not Allowed"},
  {406, "Not Acceptable"},
  {407, "Proxy Authentication Required"},
  {408, "Request Timeout"},
  {409, "Conflict"},
  {416, "Requested Range not satisfiable"},

  {500, "Internal Server Error"},
  {501, "Not Implemented"},
  {502, "Bad Gateway"},
  {503, "Service Unavailable"},
  {504, "Gateway Timeout"},
  {505, "HTTP Version Not Supported"},
  {506, "Variant Also Negotiates"},
  {507, "Insufficient Storage"},
  {508, "Bandwidth Limit Exceeded"},
  {509, "Not Extended"},
};

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
  // Format path
  if (path.find(headers["host"]) != std::string::npos &&
      path.size() > (7 + headers["host"].size())) {
    // http://www.xxx.xxx/path/to
    path.assign(path.substr(7 + headers["host"].size()));
  }
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

// Parse query parameter from GET url or POST application/x-www-form-urlencoded
// format: key1=value1&key2=value2&key3=value3
bool HttpRequest::ParseParameters(const std::string data,
                                  size_t line_start, bool from_url) {
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
      if (from_url) {
        query_params[data.substr(pre, end - pre)]
          = std::string();
      } else {
        post_params[data.substr(pre, end - pre)]
          = std::string();
      }
      pre = end + 1;
    } else {
      if (from_url) {
        query_params[data.substr(pre, mid - pre)]
          = data.substr(mid + 1, end - mid - 1);
      } else {
        post_params[data.substr(pre, mid - pre)]
          = data.substr(mid + 1, end - mid - 1);
      }
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
    line_start = ++line_end;
  }
  
  // Parse query parameter from url
  if (!ParseGetUrl()) {
    return false;
  }
  return true;
}

// bool HttpRequest::ParseBodyFromArray(const char* data, const int size) {
//   content.append(data, size);
//   if (method == "POST" &&
//       headers["content-type"] == "application/x-www-form-urlencoded") {
//     return ParseParameters(content, 0, false);
//   }
//   return true;
// }

void HttpRequest::Clear() {
  version.clear();
  path.clear();
  method.clear();
  query_params.clear();
  post_params.clear();
  headers.clear();
}

void HttpResponse::Clear() {
  status_code_ = 0;
  reason_phrase_.clear();
  headers_.clear();
  content_length = 0;
}

// Return bytes actual be writen, should be less than size
int HttpResponse::SerializeHeaderToArray(char* data, size_t size) {
  int serial_size = 0;
  int ret;

  // Serialize statues line
  ret = snprintf(data, size, "HTTP/1.1 %d %s\r\n",
                 status_code_, reason_phrase_.c_str());
  serial_size += ret;
  if (ret < 0 || ret == static_cast<int>(size)) {
    return -1;
  }

  // Serialize header
  if (headers_.count("Content-Length")) {
    content_length = std::stoi(headers_.at("Content-Length"));
  } else {
    content_length = 0;
  }

  for (auto &line : headers_) {
    ret = snprintf(data + serial_size, size - serial_size, "%s: %s\r\n",
                   line.first.c_str(), line.second.c_str());
    serial_size += ret;
    if (ret < 0 || serial_size == static_cast<int>(size)) {
      return -1;
    }
  }

  ret = snprintf(data + serial_size, size - serial_size, "\r\n");
  serial_size += ret;
  if (ret < 0 || serial_size == static_cast<int>(size)) {
    return -1;
  }

  return serial_size;;
}

void HttpResponse::SetStatusCode(int code) {
  assert((code >= 100 && code <= 102) ||
         (code >= 200 && code <= 207) ||
         (code >= 400 && code <= 409) ||
         (code == 416) ||
         (code >= 500 && code <= 509));
  status_code_ = code;
  reason_phrase_.assign(http_status_map.at(code));
}

HttpConn::HttpConn(const int fd, const std::string &ip_port,  Thread *thread) :
  PinkConn(fd, ip_port, thread),
  recv_status_(kHeader),
  send_status_(kHeader),
  rbuf_pos_(0),
  wbuf_pos_(0),
  header_len_(0),
  remain_recv_len_(0) {
  rbuf_ = (char *)malloc(sizeof(char) * kHttpMaxMessage + sizeof(char));
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
    remain_recv_len_ = 0;
  } else {
    int64_t tmp = 0;
    slash::string2l(iter->second.data(), iter->second.size(),
        static_cast<long*>(&tmp));
    remain_recv_len_ = tmp;
  }

  if (rbuf_pos_ > header_len_) {
    remain_recv_len_ -= rbuf_pos_ - header_len_;
  }
  return true;
}

ReadStatus HttpConn::GetRequest() {
  ssize_t nread = 0;
  bool need_reply;
  while (true) {
    switch (recv_status_) {
      case kHeader: {
        nread = read(fd(), rbuf_ + rbuf_pos_, kHttpMaxHeader - rbuf_pos_);
        if (nread == -1 && errno == EAGAIN) {
          return kReadHalf;
        } else if (nread <= 0) {
          handles_->ConnClosedHandle();
          return kReadClose;
        } else {
          rbuf_pos_ += nread;
          rbuf_[rbuf_pos_] = '\0'; // So that strstr will not parse the expire char
          char *sep_pos = strstr(rbuf_, "\r\n\r\n");
          if (!sep_pos) {
            break;
          }
          header_len_ = sep_pos - rbuf_ + 4;
          if (header_len_ > kHttpMaxHeader || !BuildRequestHeader()) {
            return kReadError;
          }

          need_reply = handles_->ReqHeadersHandle(request_);

          if (remain_recv_len_ > 0) {
            recv_status_ = kPacket;
          } else {
            recv_status_ = kComplete;
          }

          if (need_reply) {
            set_is_reply(true);
            return kReadHalf;
          }
        }
        break;
      }
      case kPacket: {
        if (remain_recv_len_ > 0) {
          size_t remain_buf_size = kHttpMaxMessage - rbuf_pos_;
          nread = read(fd(), rbuf_ + rbuf_pos_,
                       std::min(remain_buf_size, remain_recv_len_));
          if (nread == -1 && errno == EAGAIN) {
            return kReadHalf;
          } else if (nread <= 0) {
            handles_->ConnClosedHandle();
            return kReadClose;
          } else {
            rbuf_pos_ += nread;
            remain_recv_len_ -= nread;
          }
        }
        if (remain_recv_len_ == 0 || // no more content
            rbuf_pos_ == kHttpMaxMessage) { // buffer full
          handles_->ReqBodyPartHandle(rbuf_ + header_len_, rbuf_pos_ - header_len_);
          if (remain_recv_len_ == 0) {
            recv_status_ = kComplete;
          } else {
            rbuf_pos_ = header_len_ = 0; // read more packet content from begin
          }
        }
        break;
      }
      case kComplete: {
        response_->Clear();
        handles_->ReqCompleteHandle(request_, response_);
        set_is_reply(true);
        recv_status_ = kHeader;
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

WriteStatus HttpConn::SendReply() {
  ssize_t nwritten = 0;
  size_t remain_buf_size = 0;
  size_t wbuf_len = 0;
  int header_len = 0;
  while (true) {
    switch (send_status_) {
      case kHeader:
        handles_->RespHeaderHandle(response_);
        header_len = response_->SerializeHeaderToArray(wbuf_, kHttpMaxHeader);
        remain_send_len_ = response_->ContentLength();
        if (header_len < 0 || static_cast<uint32_t>(header_len) > kHttpMaxHeader) {
          handles_->ConnClosedHandle();
          return kWriteError;
        }

        nwritten = write(fd(), wbuf_ + wbuf_pos_, header_len);
        if (nwritten == -1 && errno == EAGAIN) {
          return kWriteHalf;
        } else if (nwritten <= 0) {
          handles_->ConnClosedHandle();
          return kWriteError;
        } else {
          wbuf_pos_ += nwritten;
        }

        if (remain_send_len_ == 0) {
          return kWriteAll;
        }
        if (wbuf_pos_ == static_cast<uint32_t>(header_len)) {
          send_status_ = kPacket;
        }
        if (wbuf_pos_ == kHttpMaxHeader) {
          wbuf_pos_ = 0;
        }
      case kPacket:
        remain_buf_size = kHttpMaxMessage - wbuf_pos_;
        wbuf_len = handles_->RespBodyPartHandle(wbuf_ + wbuf_pos_,
                                std::min(remain_buf_size, remain_send_len_));
        nwritten = write(fd(), wbuf_ + wbuf_pos_, wbuf_len);
        if (nwritten == -1 && errno == EAGAIN) {
          return kWriteHalf;
        } else if (nwritten <= 0) {
          handles_->ConnClosedHandle();
          return kWriteError;
        } else {
          wbuf_pos_ += nwritten;
          remain_send_len_ -= nwritten;
        }
        if (remain_send_len_ == 0) {
          send_status_ = kComplete;
        }
        if (wbuf_pos_ == kHttpMaxMessage) {
          wbuf_pos_ = 0;
        }
        break;
      case kComplete:
        wbuf_pos_ = 0;
        send_status_ = kHeader;
        return kWriteAll;
        break;
      default:
        return kWriteError;
    } // switch
  } // while (true)
}

}  // namespace pink
