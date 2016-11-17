#ifndef _RPC_DEFINE_H
#define _RPC_DEFINE_H

namespace pink {

#define PINK_PROHIBIT_EVIL_COPY(TypeName) \
	TypeName(TypeName &);                     \
	TypeName& operator=(TypeName &);

#define RPC_MAX_WORKER_NUM 32

/*
 * unit is second
 */
#define RPC_WORKER_CRON_INTERVAl 1
#define RPC_DISPATCH_CRON_INTERVAL 1
#define RPC_SERVER_CRON_INTERVAL 1


#define MAX_TMP_BUFFER_SIZE 256

#define SERVICE_FILE_SUFFIX ".proto"

#define RPC_PB_COMMAND_HEADER_LENGTH 8

}

#endif
