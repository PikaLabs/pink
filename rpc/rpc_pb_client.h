#ifndef _RPC_PB_CLIENT_H
#define _RPC_PB_CLIENT_H

#include "pb_cli.h"

namespace pink {

/*
 * The data format to send is:
 * | method_name len |    msg len   | method full name |     msg   |
 * |
 * |    4 bytes      |    4 bytes    | method_name len  |  msg len  |              
 *
 *
 * the data fromat to receive is:
 * |  data len  |    data     |
 * |   4 bytes  |  data len   |
 */

class RpcPbCli : public PbCli {
public:
	Status Send(void *msg, const std::string &method_fname);
private:
	void BuildWbuf() override;

	std::string method_fname_;
};

}

#endif
