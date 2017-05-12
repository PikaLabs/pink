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
  std::string method_;
  std::string url_;
  std::string path_;
  std::string version_;
  std::map<std::string, std::string> headers_;
  std::map<std::string, std::string> query_params_;

  void Clear();
  bool ParseHeadFromArray(const char* data, const int size);
  void Dump() const;

 private:
  friend class HttpConn;
  HttpRequest() {}

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
  void Clear();
  int SerializeHeaderToArray(char* data, size_t size);

  void SetStatusCode(int code);

  void SetHeaders(const std::string& key, const std::string& value) {
    headers_[key] = value;
  }

  void SetHeaders(const std::string& key, const size_t value) {
    headers_[key] = std::to_string(value);
  }

  void SetContentLength(size_t size) {
    SetHeaders("Content-Length", size);
  }

  size_t content_length() {
    if (headers_.count("Content-Length"))
      return std::stoul(headers_["Content-Length"]);
    return 0;
  }

 private:
  friend class HttpConn;

  HttpResponse() {}

  int status_code_;
  std::string reason_phrase_;
  std::map<std::string, std::string> headers_;
};

class HttpHandles {
 public:
  // You need implement these handles.
  /*
   * We have parsed HTTP request for now,
   * then ReqHeadersHandle(...) will be called.
   * Return true if reply needed, and then handle response header and body
   * by functions below, otherwise false.
   */
  virtual bool ReqHeadersHandle(const HttpRequest* req) = 0;
  /*
   * ReqBodyPartHandle(...) will be called if there are data follow up,
   * We deliver data just once.
   */
  virtual void ReqBodyPartHandle(const char* data, size_t data_size) = 0;

  /*
   * Fill response headers in this handle.
   * You MUST set Content-Length by means of calling resp->SetContentLength(num).
   * Besides, resp->SetStatusCode(code) should be called either.
   */
  virtual void RespHeaderHandle(HttpResponse* resp) = 0;
  /*
   * Fill write buffer 'buf' in this handle, and should not exceed 'max_size'.
   * Return actual size filled.
   */
  virtual int RespBodyPartHandle(char* buf, size_t max_size) = 0;

  // Close handle
  virtual void ConnClosedHandle() {
  }

  HttpHandles() {
  }
  virtual ~HttpHandles() {
  }

 protected:
  // Assigned in constructor of HttpConn
  Thread* thread_ptr_;

 private:
  friend class HttpConn;

  /*
   * No allowed copy and copy assign
   */
  HttpHandles(const HttpHandles&);
  void operator=(const HttpHandles&);
};

class HttpConn: public PinkConn {
 public:

  HttpConn(const int fd, const std::string &ip_port,
           Thread *thread, HttpHandles* handles = nullptr);
  virtual ~HttpConn();

  virtual ReadStatus GetRequest() override;
  virtual WriteStatus SendReply() override;

 private:

  bool BuildRequestHeader();

  // Receive related
  ConnStatus recv_status_;
  char* rbuf_;
  uint32_t rbuf_pos_;
  uint32_t header_len_;
  uint64_t remain_recv_len_;

  // Send related
  ConnStatus send_status_;
  char* wbuf_;
  uint32_t wbuf_pos_;
  uint64_t remain_unfetch_len_;
  uint64_t remain_send_len_;
  size_t wbuf_len_;

  HttpRequest* request_;
  HttpResponse* response_;

  HttpHandles* handles_;
};

}  // namespace pink

#endif  // INCLUDE_HTTP_CONN_H_
