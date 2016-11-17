#ifndef _RPC_DEFINE_H
#define _RPC_DEFINE_H

namespace pink {

#define MAX_TMP_BUFFER_SIZE                  256

#define SERVICE_FILE_SUFFIX                  ".proto"

#define RPC_PB_COMMAND_HEADER_LENGTH         8

#define MACRO_TO_STR_IMPL(x)                 #x
#define MACRO_TO_STR(x)                      MACRO_TO_STR_IMPL(x)
}

#endif
