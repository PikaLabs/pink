#include "cli_test.h"
#include "pink.pb.h"
#include "xdebug.h"

#include <stdio.h>

using namespace pink;

Pcli::Pcli()
{
}

Status Pcli::Ping(const std::string &address)
{
  Status s;
  log_info("come in the Pcli::Ping");
  pink::Ping ping;
  ping.set_address(address);
  ping.set_port(111);

  s = Send(&ping);

  pink::PingRes ping_res;

  log_info("Send ping success");

  s = Recv(&ping_res);

  log_info("ping_res %d\n", ping_res.res());

  return s;
}

int main()
{

  Pcli *pcli = new Pcli();
  log_info("new Pcli");
  pcli->Connect("127.0.0.1", 9211);
  log_info("new Pcli Connect");
  int i = 1000000;
  while (i--) {
    pcli->Ping("heihei");
  }

  log_info("new Pcli Ping");

  return 0;

}
