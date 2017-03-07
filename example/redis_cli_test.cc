#include "include/pink_cli.h"
#include "include/redis_cli.h"

#include <stdio.h>
#include <errno.h>
#include "xdebug.h"

using namespace pink;

int main(int argc, char* argv[]) {
  if (argc < 3) {
    printf ("Usage: ./redis_cli ip port\n");
    exit(0);
  }

  std::string ip(argv[1]);
  int port = atoi(argv[2]);

  std::string str;
  int i = 5;

  printf ("\nTest Serialize\n");
  int ret = SerializeCommand(&str, "HSET %s %d", "key", i);
  printf ("   1. Serialize by va return %d, (%s)\n", ret, str.c_str());

  RedisCmdArgsType vec;
  vec.push_back("hset");
  vec.push_back("key");
  vec.push_back(std::to_string(5));

  ret = SerializeCommand(vec, &str);
  printf ("   2. Serialize by vec return %d, (%s)\n", ret, str.c_str());

  PinkCli *rcli = NewRedisCli();
  rcli->set_connect_timeout(3000);

  // redis v3.2+ protect mode will block other ip
  //printf ("  Connect with bind_ip(101.199.114.205)\n");
  //Status s = rcli->Connect(ip, port, "101.199.114.205");

  Status s = rcli->Connect(ip, port, "101.199.114.205");
  // Test connect timeout with a non-routable IP
  //Status s = rcli->Connect("10.255.255.1", 9824);

  printf(" RedisCli Connect(%s:%d) return %s\n", ip.c_str(), port, s.ToString().c_str());
  if (!s.ok()) {
      printf ("Connect failed, %s\n", s.ToString().c_str());
      exit(-1);
  }

  ret = rcli->set_send_timeout(100);
  printf("set send timeout 100 ms, return %d\n", ret);

  ret = rcli->set_recv_timeout(100);
  printf("set recv timeout 100 ms, return %d\n", ret);

  /*
  char ch;
  scanf ("%c", &ch);
  */

  printf ("\nTest Send and Recv Ping\n");
  std::string ping = "*1\r\n$4\r\nping\r\n";
  for (int i = 0; i < 1; i++) {
    s = rcli->Send(&ping);
    printf("Send %d: %s\n", i, s.ToString().c_str());

    s = rcli->Recv(NULL);
    printf("Recv %d: return %s\n", i, s.ToString().c_str());
    if (rcli->argv_.size() > 0) {
      printf("  argv[0]  is (%s)\n", rcli->argv_[0].c_str());
    }
  }

  printf ("\nTest Send and Recv Mutli\n");
  SerializeCommand(&str, "MSET a 1 b 2 c 3 d 4");
  printf ("Send mset parse (%s)\n", str.c_str());
  s = rcli->Send(&str);
  printf ("Send mset return %s\n", s.ToString().c_str());

  s = rcli->Recv(NULL);
  printf("Recv mset return %s with %lu elements\n", s.ToString().c_str(), rcli->argv_.size());
  for (size_t i = 0; i < rcli->argv_.size(); i++) {
    printf("  argv[%lu] = (%s)", i, rcli->argv_[i].c_str());
  }

  printf ("\n\nTest Mget case 1: send 1 time, and recv 1 time\n");
  SerializeCommand(&str, "MGET a  b  c  d ");
  printf ("Send mget parse (%s)\n", str.c_str());

  for (int si = 0; si < 2; si++) {
    s = rcli->Send(&str);
    printf ("Send mget case 1: i=%d, return %s\n", si, s.ToString().c_str());

    s = rcli->Recv(NULL);
    printf ("Recv mget case 1: i=%d, return %s with %lu elements\n", si, s.ToString().c_str(), rcli->argv_.size());
    for (size_t i = 0; i < rcli->argv_.size(); i++) {
      printf("  argv[%lu] = (%s)\n", i, rcli->argv_[i].c_str());
    }
  }

  printf ("\nTest Mget case 2: send 2 times, then recv 2 times\n");
  SerializeCommand(&str, "MGET a  b  c  d ");
  printf ("\nSend mget parse (%s)\n", str.c_str());

  for (int si = 0; si < 2; si++) {
    s = rcli->Send(&str);
    printf ("Send mget case 2: i=%d, return %s\n", si, s.ToString().c_str());
  }

  for (int si = 0; si < 2; si++) {
    s = rcli->Recv(NULL);
    printf ("Recv mget case 1: i=%d, return %s with %lu elements\n", si, s.ToString().c_str(), rcli->argv_.size());
    for (size_t i = 0; i < rcli->argv_.size(); i++) {
      printf ("  argv[%lu] = (%s)\n", i, rcli->argv_[i].c_str());
    }
  }

  char ch;
  scanf ("%c", &ch);

  return 0;
}
