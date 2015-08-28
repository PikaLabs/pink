#ifndef PINK_DEFINE_H__
#define PINK_DEFINE_H__

#define PINK_MAX_CLIENTS 10240
#define PINK_MAX_MESSAGE 1024
#define PINK_NAME_LEN 1024

#define PB_MAX_MESSAGE 10240

/*
 * The pb head and code length
 */
#define COMMAND_HEADER_LENGTH 4
#define COMMAND_CODE_LENGTH 4

#include <functional>
#include <iostream>
#include <map>

/*
 * The socket block type
 */
enum BlockType {
    kBlock = 0,
    kNonBlock = 1,
};

enum EventStatus {
    kNone = 0,
    kReadable = 1,
    kWriteable = 2,
};

enum ConnStatus {
    kHeader = 0,
    kPacket = 1,
    kComplete = 2,
    kBuildObuf = 3,
    kWriteObuf = 4,
};

/*
 * define the macro in PINK_conf
 */

#define PINK_WORD_SIZE 1024
#define PINK_LINE_SIZE 1024
#define PINK_CONF_MAX_NUM 1024


/*
 * define common character
 */
#define SPACE ' '
#define COLON ':'
#define SHARP '#'


#endif
