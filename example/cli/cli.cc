#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include <fcntl.h>
#include <event.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include "pink.pb.h"
#include <iostream>
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>


using namespace google::protobuf::io;

using namespace std;
int main(int argv, char** argc){

  /* Coded output stram */

  pink::Ping ping;

  // ping.set_t(pink::PING);
  ping.set_address("127.0.0.1");
  ping.set_port(9888);

  cout<<"size after serilizing is "<<ping.ByteSize()<<endl;
  uint32_t siz = ping.ByteSize()+4;
  uint32_t u = htonl(siz - 4);
  char *pkt = new char [siz];
  memcpy(pkt, &u, sizeof(uint32_t));
  ping.SerializeToArray(pkt + sizeof(int32_t), 10010);

  int host_port= 9211;
  char* host_name="127.0.0.1";

  struct sockaddr_in my_addr;

  char buffer[1024];
  int bytecount;
  int buffer_len=0;

  int hsock;
  int * p_int;
  int err;

  hsock = socket(AF_INET, SOCK_STREAM, 0);
  if(hsock == -1){
    printf("Error initializing socket %d\n",errno);
    return 0;
  }

  p_int = (int*)malloc(sizeof(int));
  *p_int = 1;

  if( (setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1 )||
      (setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1 ) ){
    printf("Error setting options %d\n",errno);
    free(p_int);
    return 0;
  }
  free(p_int);

  my_addr.sin_family = AF_INET ;
  my_addr.sin_port = htons(host_port);

  memset(&(my_addr.sin_zero), 0, 8);
  my_addr.sin_addr.s_addr = inet_addr(host_name);
  if( connect( hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1 ){
    if((err = errno) != EINPROGRESS){
      fprintf(stderr, "Error connecting socket %d\n", errno);
      return 0;
    }
  }



  pink::PingRes ping_res;

  char buf[1024];
  uint32_t integer = 0;
  uint32_t header_len;

  for (int i =0;i<1000;i++){
    for (int j = 0 ;j<100000;j++) {

      if( (bytecount=send(hsock, (void *) pkt,siz,0))== -1 ) {
        fprintf(stderr, "Error sending data %d\n", errno);
        return 0;
      }

      read(hsock, buf, 4);
      memcpy((char *)(&integer), buf, sizeof(int32_t));
      header_len = ntohl(integer);

      read(hsock, buf, header_len);

      ping_res.ParseFromArray(buf, header_len);

      printf("ping_res %d\n", ping_res.res());

      printf("Sent bytes %d\n", bytecount);
      usleep(1);
    }
  }
  delete pkt;

FINISH:
  close(hsock);

}
