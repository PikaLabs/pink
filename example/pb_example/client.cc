#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "message.pb.h"
#include "pb_cli.h"
#include "pink_define.h"

using namespace pink;

int main(int argc, char* argv[]) {
  if (argc < 3) { 
    printf ("Usage: ./client ip port\n");
    exit(0);
  }

  std::string ip(argv[1]);
  int port = atoi(argv[2]);
  
  PbCli* cli = new PbCli();

  for (int i = 0; i < 10; i++) {
    Status s = cli->Connect(ip, port);
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

    cli->Close();

    char ch;
    scanf ("%c", &ch);
  }

  delete cli;
  return 0;
}
