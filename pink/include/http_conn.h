// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef PINK_INCLUDE_HTTP_CONN_H_
#define PINK_INCLUDE_HTTP_CONN_H_
#include <map>
#include <vector>
#include <string>

#include "slash/include/slash_status.h"
#include "slash/include/xdebug.h"

#include "pink/include/pink_conn.h"
#include "pink/include/pink_define.h"
#include "pink/src/csapp.h"
#include "pink/src/worker_thread.h"
#include "pink/src/pink_util.h"

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

  // POST: content-type: application/x-www-form-urlencoded
  std::map<std::string, std::string> post_params;
  
  HttpRequest();
  void Clear();
  bool ParseHeadFromArray(const char* data, const int size);

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
  bool ParseParameters(const std::string data, size_t line_start = 0, bool from_url = true);
};

class HttpResponse {
 public:
  HttpResponse():
    status_code_(0),
    content_length(0) {
  }
  void Clear();
  int SerializeHeaderToArray(char* data, size_t size);

  void SetStatusCode(int code);

  void SetHeaders(const std::string &key, const std::string &value) {
    headers_[key] = value;
  }

  void SetHeaders(const std::string &key, const int value) {
    headers_[key] = std::to_string(value);
  }

  size_t ContentLength() {
    return content_length;
  }

 private:
  int status_code_;
  std::string reason_phrase_;
  std::map<std::string, std::string> headers_;
  size_t content_length;
};

class HttpConn: public PinkConn {
 public:

  class HttpHandles {
   public:
    // Request handles
    virtual bool ReqHeadersHandle(HttpRequest* req) { return false; }
    virtual void ReqBodyPartHandle(const char* data, size_t max_size) {}
    virtual void ReqCompleteHandle(HttpRequest* req, HttpResponse* resp) {}
    virtual void ConnClosedHandle() {}

    // Response handles
    virtual void RespHeaderHandle(HttpResponse* resp) {}
    virtual int RespBodyPartHandle(char* buf, size_t max_size) { return 0; }
  };

  HttpConn(const int fd, const std::string &ip_port,  Thread *thread);
  virtual ~HttpConn();

  virtual ReadStatus GetRequest() override;
  virtual WriteStatus SendReply() override;

 protected:
  HttpHandles* handles_;

 private:

  bool BuildRequestHeader();

  ConnStatus recv_status_;
  ConnStatus send_status_;

  char* rbuf_;
  uint32_t rbuf_pos_;
  char* wbuf_;
  uint32_t wbuf_pos_;

  uint32_t header_len_;
  uint64_t remain_recv_len_;
  uint64_t remain_send_len_;

  HttpRequest* request_;
  HttpResponse* response_;
};

}  // namespace pink

#endif  // INCLUDE_HTTP_CONN_H_
