#include "pink_cli.h"
#include "status.h"
#include "pink_cli_socket.h"

namespace pink {

PinkCli::PinkCli() {

  cli_socket_ = new CliSocket();
}

PinkCli::~PinkCli() {
  delete cli_socket_;
}

int PinkCli::fd() {
  return cli_socket_->sockfd();
}

Status PinkCli::Close(){
  cli_socket_->Close();
  return Status::OK();
}

Status PinkCli::Connect(const std::string &peer_ip, const int peer_port, const std::string &bind_ip)
{
  return cli_socket_->Connect(peer_ip, peer_port, bind_ip);
}

};
