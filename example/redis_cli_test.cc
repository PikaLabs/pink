#include "redis_cli.h"

#include <stdio.h>
#include "xdebug.h"

using namespace pink;

int main()
{

  RedisCli *rcli = new RedisCli();
  Status s = rcli->Connect("127.0.0.1", 9824);
  log_info("new RedisCli Connect %s", s.ToString().c_str());
  if (!s.ok()) {
      printf ("Connect %s\n", s.ToString().c_str());
      exit(-1);
  }

  std::string ping = "*1\r\n$4\r\nping\r\n";

  for (int i = 0; i < 10; i++) {
    s = rcli->Send(&ping);
    log_info("Send %d: %s", i, s.ToString().c_str());

    std::string reply;
    s = rcli->Recv(&reply);
    log_info("Recv %d: return %s, reply is (%s)", i, s.ToString().c_str(), reply.c_str());
  }

  return 0;
}
