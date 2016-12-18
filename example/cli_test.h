#ifndef __PINK_CLI_TEST_H
#define __PINK_CLI_TEST_H

#include "status.h"
#include "pb_cli.h"
#include "pink.pb.h"

#include <google/protobuf/message.h>

using namespace pink;

class Pcli: public PbCli
{
public:
  Pcli();

  Status Ping(const std::string &address);

};

#endif
