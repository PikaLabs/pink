#ifndef PINK_PB_CONN_H_
#define PINK_PB_CONN_H_

#include "status.h"
#include "csapp.h"
#include "pink_define.h"

template<typename T>
class PbConn
{
public:
    PbConn(int fd);
    PbConn();
    ~PbConn();
    /*
     * Set the fd to nonblock && set the flag_ the the fd flag
     */
    bool SetNonblock();
    void InitPara();
    Status PbReadBuf();
    int PbGetRequest();
    int PbSendReply();
    void set_fd(int fd) { fd_ = fd; };
    int flags() { return flags_; };

private:


    int fd_;
    int flags_;

    /*
     * These functions parse the message from client
     */
    Status PbReadHeader(rio_t *rio);
    Status PbReadCode(rio_t *rio);
    Status PbReadPacket(rio_t *rio);


    Status BuildObuf(int32_t opcode, const int packet_len);
    /*
     * The Variable need by read the buf,
     * We allocate the memory when we start the server
     */
    int header_len_;
    int32_t r_opcode_;
    char* rbuf_;
    int32_t cur_pos_;
    int32_t rbuf_len_;

    ConnStatus connStatus_;

    char* wbuf_;
    int32_t wbuf_len_;
    int32_t wbuf_pos_;
};

#endif
