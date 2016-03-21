#ifndef PINK_CLI_H_
#define PINK_CLI_H_

#include <string>

#include "pink_cli_socket.h"
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

  virtual int set_send_timeout(int send_timeout) {
    return cli_socket_->set_send_timeout(send_timeout);
  }

  virtual int set_recv_timeout(int recv_timeout) {
    return cli_socket_->set_recv_timeout(recv_timeout);
  }

  virtual void set_connect_timeout(int connect_timeout) {
    cli_socket_->set_connect_timeout(connect_timeout);
  }

private:

  std::string peer_ip_;
  int peer_port_;

  CliSocket *cli_socket_;

};

};

#endif
