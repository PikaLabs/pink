#ifndef PINK_SOCKET_H_
#define PINK_SOCKET_H_

class ServerSocket
{
public:
  explicit ServerSocket(int port);

  ServerSocket(int port, bool is_block);
  ~ServerSocket();

  void Listen();

  /*
   * The get and set functions
   */
  void set_port(int port) {
    port_ = port;
  }

  int port() {
    return port_;
  }
  
  void set_keep_alive(bool keep_alive) {
    keep_alive_ = keep_alive;
  }
  bool keep_alive() {
    return keep_alive_;
  }

  void set_send_timeout(int send_timeout) {
    send_timeout_ = send_timeout;
  }
  int send_timeout() {
    return send_timeout_;
  }

  void set_recv_timeout(int recv_timeout) {
    recv_timeout_ = recv_timeout;
  }
  int recv_timeout() {
    return recv_timeout_;
  }

  int sockfd() {
    return sockfd_;
  }
  void set_sockfd(int sockfd) {
    sockfd_ = sockfd;
  }

private:

  int SetNonBlock();

  void Close();
  /*
   * The tcp server port and address
   */
  int port_;
  int flags_;
  int send_timeout_;
  int recv_timeout_;
  int accept_timeout_;
  int accept_backlog_;
  int tcp_send_buffer_;
  int tcp_recv_buffer_;
  bool keep_alive_;
  bool listening_;
  bool is_block_;
  
  struct sockaddr_in servaddr_;
  int sockfd_;
};

#endif
