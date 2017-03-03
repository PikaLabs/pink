#ifndef PINK_INCLUDE_PINK_CLI_H_
#define PINK_INCLUDE_PINK_CLI_H_

#include "include/slash_status.h"

using slash::Status;

namespace pink {

class PinkCli {
 public:
  PinkCli();
  virtual ~PinkCli();

  Status Connect(const std::string &peer_ip, const int peer_port, 
      const std::string& bind_ip = "");
  // Compress and write the message
  virtual Status Send(void *msg) = 0;

  // Read, parse and store the reply
  virtual Status Recv(void *result = NULL) = 0;

  void Close();

  // TODO(baotiao): delete after redis_cli use RecvRaw
  int fd() const;

  // Set timeout in miliseconds, default send and recv timeout is 0,
  // default connect timeout is 1000ms
  int set_send_timeout(int send_timeout);
  int set_recv_timeout(int recv_timeout);
  void set_connect_timeout(int connect_timeout);

 protected:
  Status SendRaw(void* buf, size_t len);
  Status RecvRaw(void* buf, size_t* len);

 private:
  struct Rep;
  Rep* rep_;
  PinkCli(const PinkCli&);
  void operator=(const PinkCli&);
};

extern PinkCli *NewPbCli();

extern PinkCli *NewRedisCli();

}  // namespace pink

#endif  // PINK_INCLUDE_PINK_CLI_H_
