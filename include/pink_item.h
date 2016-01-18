#ifndef PINK_ITEM_H_
#define PINK_ITEM_H_

#include "status.h"
#include "pink_define.h"

namespace pink {
  
class PinkItem
{
public:
  PinkItem() {};
  PinkItem(int fd, std::string ip_port);
  ~PinkItem();

  int fd() const {
    return fd_;
  }
  std::string ip_port() const {
    return ip_port_;
  }

private:

  int fd_;
  std::string ip_port_;

  /*
   * Here we should allow the copy and copy assign operator
   */
  // PinkItem(const PinkItem&);
  // void operator=(const PinkItem&);

};

}

#endif
