#ifndef PINK_CLI_H_
#define PINK_CLI_H_

#include <string>

#include "status.h"

namespace pink {

class CliSocket;

class PinkCli {

public:
  PinkCli(const std::string &peer_ip, const int peer_port);
  ~PinkCli();

  Status Connect(const std::string &peer_ip, const int peer_port);

  Status Write(const std::string wbuf);
  Status Recv(const std::string *rbuf);

private:

  std::string peer_ip_;
  int peer_port_;
  CliSocket *cli_socket_;


};

};

#endif
