// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#ifndef PINK_DEFINE_H__
#define PINK_DEFINE_H__

#include <functional>
#include <iostream>
#include <map>

namespace pink {

#define PINK_MAX_CLIENTS 10240
#define PINK_MAX_MESSAGE 1024
#define PINK_NAME_LEN 1024

#define PB_MAX_MESSAGE 102400
const int kProtoMaxMessage = 64 * 1024 * 1024; // 64MB
/*
 * The pb head and code length
 */
#define COMMAND_HEADER_LENGTH 4
#define COMMAND_CODE_LENGTH 4

const int kCommandHeaderLength = 4;

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

enum ReadStatus {
  kReadHalf = 0,
  kReadAll = 1,
  kReadError = 2,
  kReadClose = 3,
  kFullError = 4,
  kParseError = 5,
  kOk = 6,
};

enum WriteStatus {
  kWriteHalf = 0,
  kWriteAll = 1,
  kWriteError = 2,
};

enum RetCode {
  kSuccess = 0,
  kBindError = 1,
  kCreateThreadError = 2,
  kListenError = 3,
  kSetSockOptError = 4,
};

/*
 * define the redis protocol
 */
#define REDIS_MAX_MESSAGE 67108864 // 64MB
#define REDIS_IOBUF_LEN 16384
#define REDIS_REQ_INLINE 1
#define REDIS_REQ_MULTIBULK 2

/*
 * define the pink cron interval (ms)
 */
#define PINK_CRON_INTERVAL 1000

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

}

#endif
