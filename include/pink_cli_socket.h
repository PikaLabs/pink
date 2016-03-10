#ifndef PINK_CLI_SOCKET_H_
#define PINK_CLI_SOCKET_H_

#include "status.h"

namespace pink {

class CliSocket {

public:
  CliSocket();
  Status Connect(const std::string &ip, const int port);


  void sockfd() {
    return sockfd_;
  }

  /*
   * in the client socket, we support three type of timeout
   * 1. connect timeout
   * 2. send timeout
   * 3. recv timeout
   */
  int set_send_timeout(int send_timeout);

  int CliSocketset_recv_timeout(int recv_timeout);

  void set_connect_timeout(int connect_timeout) {
    connect_timeout_ = connect_timeout;
  }
  
private:
  std::string ip_;
  int port_;

  /*
   * all the time is in ms
   * The default send and recv timeout is 0
   */
  int send_timeout_;
  int recv_timeout_;
  /*
   * The default connect timeout is 1000ms
   */
  int connect_timeout_;

  bool keep_alive_;
  bool is_block_;

  std::string peer_ip_;
  int peer_port_;
  
  struct sockaddr_in servaddr_;
  int sockfd_;

};

};

#endif
