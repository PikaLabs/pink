// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef PINK_INCLUDE_HTTP_CONN_H_
#define PINK_INCLUDE_HTTP_CONN_H_

#include <map>
#include <vector>
#include <string>

#include "src/csapp.h"
#include "third/slash/include/slash_status.h"

#include "include/pink_define.h"
#include "include/worker_thread.h"
#include "include/xdebug.h"
#include "src/pink_util.h"
#include "src/pink_conn.h"

namespace pink {

class HttpRequest {
 public:
  // attach in header
  std::string method;
  std::string path;
  std::string version;
  std::map<std::string, std::string> headers;

  // in header for Get, in content for Post Put Delete
  std::map<std::string, std::string> query_params;
  
  // attach in content
  std::string content;

  HttpRequest();
  void Clear();
  bool ParseHeadFromArray(const char* data, const int size);
  bool ParseBodyFromArray(const char* data, const int size);

 private:
  enum ParseStatus {
    kHeaderMethod,
    kHeaderPath,
    kHeaderVersion,
    kHeaderParamKey,
    kHeaderParamValue,
    kBody
  };

  bool ParseGetUrl();
  bool ParseHeadLine(const char* data, int line_start,
    int line_end, ParseStatus* parseStatus);
  bool ParseParameters(const std::string data, size_t line_start = 0);
};

class HttpResponse {
 public:
  HttpResponse():
    status_code_(0) {
  }
  void Clear();
  uint32_t SerializeToArray(char* data, size_t size);

  void SetStatusCode(int code);

  void SetHeaders(const std::string &key, const std::string &value) {
    headers_[key] = value;
  }

  void SetHeaders(const std::string &key, const int value) {
    headers_[key] = std::to_string(value);
  }

  void SetBody(const std::string &body) {
    body_.assign(body);
  }

 private:
  int status_code_;
  std::string reason_phrase_;
  std::map<std::string, std::string> headers_;
  std::string body_;
};

namespace {
const char *message_phrase[] = {
  "Continue",             // 100
  "Switching Protocols",  // 101
  "Processing",           // 102
};

const char *success_phrase[] = {
  "OK",                             // 200
  "Created",                        // 201
  "Accepted",                       // 202
  "Non-Authoritative Information",  // 203
  "No Content",                     // 204
  "Reset Content",                  // 205
  "Partial Content",                // 206
  "Multi-Status",                   // 207
};

const char *request_error[] = {
  "Bad Request",                    // 400
  "Unauthorized",                   // 401
  "",                               // 402 reserve
  "Forbidden",                      // 403
  "Not Found",                      // 404
  "Method Not Allowed",             // 405
  "Not Acceptable",                 // 406
  "Proxy Authentication Required",  // 407
  "Request Timeout",                // 408
  "Conflict",                       // 409
};

const char *server_error[] = {
  "Internal Server Error",        // 500
  "Not Implemented",              // 501
  "Bad Gateway",                  // 502
  "Service Unavailable",          // 503
  "Gateway Timeout",              // 504
  "HTTP Version Not Supported",   // 505
  "Variant Also Negotiates",      // 506
  "Insufficient Storage",         // 507
  "Bandwidth Limit Exceeded",     // 508
  "Not Extended",                 // 509
};
}

class HttpConn: public PinkConn {
 public:
  HttpConn(const int fd, const std::string &ip_port);
  virtual ~HttpConn();

  virtual ReadStatus GetRequest() override;
  virtual WriteStatus SendReply() override;
  bool BuildResponseBuf();


 private:
  virtual void DealMessage(const HttpRequest* req,
      HttpResponse* res) = 0;

  bool BuildRequestHeader();
  bool BuildRequestBody();
  void HandleMessage();

  ConnStatus conn_status_;
  char* rbuf_;
  uint32_t rbuf_pos_;
  char* wbuf_;
  uint32_t wbuf_len_;  // length we wanna write out
  uint32_t wbuf_pos_;
  uint32_t header_len_;
  int64_t remain_packet_len_;

  HttpRequest* request_;
  HttpResponse* response_;
};

}  // namespace pink

#endif  // INCLUDE_HTTP_CONN_H_
