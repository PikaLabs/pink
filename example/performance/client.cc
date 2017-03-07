#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "message.pb.h"
#include "include/pink_cli.h"
#include "include/pink_define.h"

using namespace pink;

int main(int argc, char* argv[]) {
  if (argc < 3) { 
    printf ("Usage: ./client ip port\n");
    exit(0);
  }

  std::string ip(argv[1]);
  int port = atoi(argv[2]);
  
  PinkCli* cli = NewPbCli();

  Status s = cli->Connect(ip, port);
  for (int i = 0; i < 100000000; i++) {
    if (!s.ok()) {
      printf ("Connect (%s:%d) failed, %s\n", ip.c_str(), port, s.ToString().c_str());
    }
    printf ("Connect (%s:%d) ok, fd is %d\n", ip.c_str(), port, cli->fd());

    Ping msg;
    msg.set_ping("ping");

    s = cli->Send((void *)&msg);
    if (!s.ok()) {
      printf ("Send failed %s\n", s.ToString().c_str());
      break;
    }

    Pong req;
    s = cli->Recv((void *)&req);
    if (!s.ok()) {
      printf ("Recv failed %s\n", s.ToString().c_str());
      break;
    }
    printf ("Recv (%s)\n", req.pong().c_str());

  }
  cli->Close();

  delete cli;
  return 0;
}
