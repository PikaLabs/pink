#ifndef PINK_ITEM_H_
#define PINK_ITEM_H_

#include "status.h"
#include "pink_define.h"

class PinkItem
{
public:
  PinkItem() {};
  PinkItem(int fd, std::string ip_port);
  ~PinkItem();

  int fd() { return fd_; }
  std::string ip_port() { return ip_port_; }

private:

  int fd_;
  std::string ip_port_;

  /*
   * No copy && assigned allowed
   */
  /*
   * PinkItem(const PinkItem&);
   * void operator=(const PinkItem&);
   */

};

#endif
