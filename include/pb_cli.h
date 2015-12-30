#ifndef PINK_PB_CLI_H_
#define PINK_PB_CLI_H_

#include <string>

class PbCli
{
public:
  PbCli(std::string host, int port);
  ~PbCli();

private:
  PbCli *cli_conn_(int fd);

  std::string host_;
  int port_;

  PbCli(const PbCli&);
  void operator=(const PbCli&);
};

#endif
