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
  std::string content;

  HttpResponse():
    content() {
  }
  void Clear();
  uint32_t SerializeToArray(void* data, const int size) const;
};

class HttpConn: public PinkConn {
 public:
  HttpConn(const int fd, const std::string &ip_port);
  virtual ~HttpConn();

  virtual ReadStatus GetRequest() override;
  virtual WriteStatus SendReply() override;
  bool BuildResponseBuf();

  virtual void DealMessage(const HttpRequest* req,
      HttpResponse* res) = 0;

 private:
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
