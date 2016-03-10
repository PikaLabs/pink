#include "pink_cli.h"
#include "status.h"
#include "pink_cli_socket.h"

namespace pink {

PinkCli::PinkCli(const std::string &peer_ip, const int peer_port) :
  peer_ip_(peer_ip),
  peer_port_(peer_port) {

  cli_socket_ = new CliSocket();

}

PinkCli::~PinkCli() {
  delete cli_conn_;
  delete cli_socket_;
}


Status PinkCli::Connect(const std::string &peer_ip, const int peer_port)
{
  return cli_socket_->Connect(peer_ip, peer_port);
}


Status PinkCli::Write(const std::string wbuf)
{
  WriteBuilder(wbuf);
}

Status PinkCli::Recv(const std::string *rbuf)
{

}

};
