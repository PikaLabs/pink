#ifndef PINK_ITEM_H_
#define PINK_ITEM_H_

#include "status.h"
#include "pink_define.h"

class PinkItem
{
public:
  PinkItem() {};
  PinkItem(int fd);
  ~PinkItem();

  int fd() { return fd_; }

private:

  int fd_;

  /*
   * No copy && assigned allowed
   */
  /*
   * PinkItem(const PinkItem&);
   * void operator=(const PinkItem&);
   */

};

#endif
