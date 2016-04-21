#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "simple_message.pb.h"
#include "pink_define.h"

int NewSocket(std::string ip, uint32_t port) {
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd == -1) {
    return -1;
  }
  int sock_opt = 1;
  if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &sock_fd, sizeof(sock_fd)) == -1 ||
      setsockopt(sock_fd, SOL_SOCKET, SO_KEEPALIVE, &sock_fd, sizeof(sock_fd)) == -1) {
    close(sock_fd);
    return -1;
  }
  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = inet_addr(ip.c_str());
  if (connect(sock_fd, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
    return -1;
  }
  return sock_fd;
}


int main(int argc, char* argv[]) {
  std::string server_host = (argc > 1) ? argv[1] : "127.0.0.1";
  int port = (argc > 2) ? atoi(argv[2]) : 8221;
  int sock_fd;
  if ((sock_fd = NewSocket(server_host, port)) == -1) {
    fprintf(stderr, "create a new socket failed\n");
    return -1;
  }
  char buf[PB_MAX_MESSAGE+4];
  uint32_t host_len, net_len, buf_len;
  int total_bytes_send, total_bytes_recv, num_bytes;
  ::simple_message::SimpleMessage message;
  while (true) {
    fputs("\r>", stderr);
    fgets(buf, sizeof(buf), stdin);
    if((buf_len = strlen(buf)) == 0) {
      continue;
    }
    message.set_name(buf, buf_len);
    host_len = message.ByteSize();
    net_len = htonl(host_len);
    memset(buf, 0, sizeof(buf));
    memcpy(buf, &net_len, sizeof(net_len));
//    fprintf(stderr, "message.ByteSize: %d\n", message.ByteSize());
    message.SerializeToArray(buf+sizeof(net_len), host_len);
    if (send(sock_fd, buf, host_len + sizeof(net_len), 0) == -1) { // this should not be once
      fprintf(stderr, "send error\n");
      return -1;
    }
    memset(buf, 0, sizeof(buf));
    if (recv(sock_fd, buf, sizeof(buf), 0) == -1) {
      fprintf(stderr, "recv error\n");
      return -1;
    }
    memcpy(&net_len, buf, sizeof(net_len));
    host_len = ntohl(net_len);
//    fprintf(stderr, "receive name: %s\n", message.name().c_str());
    message.ParseFromArray(buf + sizeof(net_len), host_len);
    fputs(message.name().c_str(), stderr); 
    fputs("\r", stderr);
  }
}
