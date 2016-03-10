#include "cli_test.h"

Status Pcli::Ping(const std::string &address)
{
  pink::Ping ping;

  ping.set_address(address);
  ping.set_port(111);

  Send(ping);

  pink::PingRes ping_res;


  Recv(pink::PingRes &ping_res);

  printf("ping_res %d\n", ping_res.res());

  return Status::OK();
}

int main()
{

  Pcli *pcli = new Pcli();
  pcli->Ping("heihei");

  return 0;

}
