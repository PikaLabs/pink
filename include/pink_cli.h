#ifndef PINK_CLI_H_
#define PINK_CLI_H_

#include <string>

#include "status.h"

namespace pink {

class CliSocket;

class PinkCli {

public:
  PinkCli();
  ~PinkCli();

  Status Connect(const std::string &peer_ip, const int peer_port);

  virtual Status Send(void *msg) = 0;
  virtual Status Recv(void *msg_res) = 0;

  int fd();

private:

  std::string peer_ip_;
  int peer_port_;

  CliSocket *cli_socket_;


};

};

#endif
