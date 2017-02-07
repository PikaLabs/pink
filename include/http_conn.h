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
  enum ParseStatus {
    kHeaderMethod,
    kHeaderPath,
    kHeaderVersion,
    kHeaderParamKey,
    kHeaderParamValue,
    kBody
  };

  ParseStatus parseStatus_;

  // attach in header
  enum Method {
    GET = 1,
    POST = 2
  };
  std::string method;
  std::string path;
  std::string version;
  std::map<std::string, std::string> headers;

  // attach in content
  std::map<std::string, std::string> query_params;
  std::string content;

  HttpRequest();
  void Clear();
  bool ParseHeadFromArray(const char* data, const int size);

 private:
  void ParseHeadLine(const char* data, int line_start, int line_end);
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

  virtual ReadStatus GetRequest();

  virtual WriteStatus SendReply();

  ConnStatus connStatus_;

  virtual bool HandleGet(class HttpRequest* req, struct HttpResponse* res) = 0;
  virtual bool HandlePost(class HttpRequest* req, struct HttpResponse* res) = 0;
  virtual bool DealMessage();

 private:
  bool BuildRequestContent();
  bool BuildRequestHeader();
  bool BuildResponseBuf();
  void AddToHeader(ssize_t nread);

  char* rbuf_;
  uint32_t header_len_;
  uint32_t rbuf_len_;  // length we read in
  uint32_t rbuf_size_;  // size we allocate
  uint32_t remain_packet_len_;

  char* wbuf_;
  uint32_t wbuf_len_;  // length we wanna write out
  uint32_t wbuf_size_;  // size we allocate
  uint32_t wbuf_pos_;

  HttpRequest* request_;
  HttpResponse* response_;
};

}  // namespace pink

#endif  // INCLUDE_HTTP_CONN_H_
