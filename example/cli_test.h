#ifndef __PINK_CLI_TEST_H
#define __PINK_CLI_TEST_H

#include "status.h"

class Pcli: public PinkCli {
public:
  Pcli();

  Status Ping();

private:

};

#endif
