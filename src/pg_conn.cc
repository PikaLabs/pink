#include "pg_conn.h"

#include <string>

#include "pink_define.h"
#include "worker_thread.h"
#include "xdebug.h"

// new style V3 packet header len:
//    [ type(1byte) | len(4 bytes) ]
//#define V3_HEADER_LEN   5
//#define PG_MAX_MESSAGE  102400

namespace pink {

PGConn::PGConn(const int fd, const std::string &ip_port) :
  PinkConn(fd, ip_port),
  //header_len_(-1),
  rbuf_size_(PG_MAX_MESSAGE),
  rbuf_offset_(0),
  parse_offset_(0),
  //remain_packet_len_(0),
  conn_status_(PGStatus::kPGLogin),
  wbuf_size_(PG_MAX_MESSAGE),
  wbuf_offset_(0),
  write_offset_(0) {
  rbuf_ = reinterpret_cast<char *>(malloc(sizeof(char) * rbuf_size_));
  wbuf_ = reinterpret_cast<char *>(malloc(sizeof(char) * wbuf_size_));
}

PGConn::~PGConn() {
  free(rbuf_);
  free(wbuf_);
}

ReadStatus PGConn::Recv(size_t count) {
//ReadStatus PGConn::Recv(size_t count, ssize_t *nread) {
  ssize_t nread = read(fd(), rbuf_ + rbuf_offset_, count);

  if (nread == -1) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return kReadHalf;
    } else {
      return kReadError;
    }
  } else if (nread == 0) {
    return kReadClose;
  }

  rbuf_offset_ += nread;
  //if (nread < count) {
  //  log_info("read error, %s", strerror(errno));
  //  return kReadHalf;
  //}

  log_info("Recv nread=%d", nread);
  return kReadAll;
}

const char* PGConn::ReadLine() {
  const void* p = rbuf_ + parse_offset_;
  const char* nul = (const char*)memchr(p, 0, rbuf_offset_ - parse_offset_);
  if (!nul) 
    return NULL;

  parse_offset_ = nul + 1 - rbuf_;
  return (const char*)p;
}

bool PGConn::HandleStartupParameters() {
  const char* key;
  const char* val;

  while (parse_offset_ < rbuf_offset_) {
    if ((key = ReadLine()) == NULL) {
      return false;
    }
    // trailing 0
    if (*key == 0) {
      break;
    }

    if ((val = ReadLine()) == NULL) {
      return false;
    }

    if (strcmp(key, "database") == 0) {
      log_info("got param: %s=%s", key, val);
      dbname_ = val;
    } else if (strcmp(key, "user") == 0) {
      log_info("got param: %s=%s", key, val);
      username_ = val;
    } else if (strcmp(key, "application_name") == 0) {
      log_info("got param: %s=%s", key, val);
      appname_ = val;
    } else {
      log_warn("unsupported startup param: %s=%s", key, val);
      return false;
    }
  }

  if (dbname_.empty()) {
    dbname_ = username_;
  }
  return true;
}

bool PGConn::HandleStartup() {
  switch (packet_header_.type) {
    case PKT_STARTUP:
      // omit SSL
      // omit re-sent Startup Packet
      if (!HandleStartupParameters()) {
        return false;
      }
      
      log_info("HandleStartupParamesters ok, change to kPGActive");
      conn_status_ = PGStatus::kPGActive; 
      break;
    case PKT_SSLREQ:
    case PKT_STARTUP_V2:
    case 'p':    //PasswordMessage
    case PKT_CANCEL:
    default:
      log_warn("not supported or bad packet type=%d\n", packet_header_.type);
      return false;
  }
  return true;
}

bool PGConn::HandleNormal() {
  log_info("Handle packet type: %d/0x%x", packet_header_.type, packet_header_.type);
  switch (packet_header_.type) {
    // one-packet queries
    case 'Q':		// Query
    case 'F':		// FunctionCall
    case 'S':		// Sync
      break;
    case 'H':		// Flush
      break;

    // copy end markers
    case 'c':		// CopyDone(F/B)
    case 'f':		// CopyFail(F/B)
      break;

    //
    // extended protocol allows server (and thus pooler)
    // to buffer packets until sync or flush is sent by client
    //	
    case 'P':		// Parse
    case 'E':		// Execute
    case 'C':		// Close
    case 'B':		// Bind
    case 'D':		// Describe
    case 'd':		// CopyData(F/B)
      break;

    case 'X': // Terminate
      //disconnect_client(client, false, "client close request");
      return false;
      // client wants to go away
    default:
      log_warn("unknown packet type: %d/0x%x", packet_header_.type, packet_header_.type);
      //disconnect_client(client, true, "unknown pkt");
      return false;
  }

  statements_ = std::string(rbuf_ + parse_offset_, rbuf_offset_ - parse_offset_);

  return true;
}

// trival implementation
Status PGConn::AppendWelcome() {
  PacketBuf* packet = NewWelcomeMsg();

  // TODO maybe add cancel key
  //packet.WriteParameterStatus("is_superuser", "on");

  return AppendObuf(packet->buf, packet->write_pos);
}

Status PGConn::AppendReadyForQuery() {
  PacketBuf *packet = new PacketBuf;
  //packet.WriteReadyForQuery();
  packet->WriteGeneric('Z', "c", 'I');
  AppendObuf(packet->buf, packet->write_pos);

  delete packet;

  return Status::OK();
}

Status PGConn::AppendObuf(const char* data, int size) {
  if (wbuf_offset_ + size > wbuf_size_) {
    return Status::Corruption("wbuf overflow");
  }

  memcpy(wbuf_ + wbuf_offset_, data, size);
  wbuf_offset_ += size;

  return Status::OK();
}

ReadStatus PGConn::ClientProto() {
  // we read 8 bytes to store both v2 and v3 header
  ReadStatus read_status = Recv(V2_HEADER_LEN - rbuf_offset_);
  if (read_status != kReadAll) {
    return read_status;
  }

  if (!IsCompleteHeader(rbuf_, rbuf_offset_)) {
    return kReadHalf;
  }

  int len = rbuf_offset_;
  if (!packet_header_.ParseFromArray(rbuf_, &len)) {
    return kParseError;
  }
  parse_offset_ = len;

  log_info ("ClientProto parse header:");
  packet_header_.Dump();

  // we read the len 
  read_status = Recv(packet_header_.len - rbuf_offset_);
  if (read_status != kReadAll) {
    return read_status;
  }
  if (rbuf_offset_ < packet_header_.len) {
    return kReadHalf;
  }
  
  log_info ("ClientProto recv msg ok");
  return kReadAll;
}

// FrontEnd/BackEnd proto has 2 phases: startup and normal;
//   During Startup
//
ReadStatus PGConn::GetRequest() {
  while (true) {
    switch (conn_status_) {
      case kPGLogin: {
        ReadStatus result = ClientProto();
        if (result != kReadAll) {
          return result;
        }
        if (rbuf_offset_ < packet_header_.len) {
          return kReadHalf;
        }

        /*
        // we read 8 bytes to store both v2 and v3 header
        ReadStatus read_status = Recv(V2_HEADER_LEN - rbuf_offset_);
        if (read_status != kReadAll) {
          return read_status;
        }

        if (!IsCompleteHeader(rbuf_, rbuf_offset_)) {
          return kReadHalf;
        }

        int len = rbuf_offset_;
        if (!packet_header_.ParseFromArray(rbuf_, &len)) {
          return kParseError;
        }
        parse_offset_ = len;

        packet_header_.Dump();

        // we read the len 
        read_status = Recv(packet_header_.len - rbuf_offset_);
        if (read_status != kReadAll) {
          return read_status;
        }
        if (rbuf_offset_ < packet_header_.len) {
          return kReadHalf;
        }
        */

        if (!HandleStartup()) {
          return kParseError;
        }

        log_info ("kPGLogin ok");
        break;
      }
      case kPGActive: {
        AppendWelcome();
        AppendReadyForQuery();

        conn_status_ = kPGHeader;
        set_is_reply(true);
        
        // clean rbuf_
        rbuf_offset_ = 0;
        parse_offset_ = 0;

        log_info ("kPGActive: send welcome msg(%d)", wbuf_offset_);
        return ReadStatus::kReadAll;
      }
      case kPGHeader: {
        log_info ("kPGHeader start");
        ReadStatus result = ClientProto();
        if (result != kReadAll) {
          return result;
        }
        if (rbuf_offset_ < packet_header_.len) {
          return kReadHalf;
        }

        if (!HandleNormal()) {
          return kParseError;
        }

        // clean rbuf_
        rbuf_offset_ = 0;
        parse_offset_ = 0;

        DealMessage();

        return ReadStatus::kReadAll;
    //    ssize_t nread = read(fd(), rbuf_ + rbuf_len_, V3_HEADER_LENGTH - rbuf_len_);
    //    if (nread == -1) {
    //      if (errno == EAGAIN) {
    //        return kReadHalf;
    //      } else {
    //        return kReadError;
    //      }
    //    } else if (nread == 0) {
    //      return kReadClose;
    //    } else {
    //      rbuf_len_ += nread;
    //      if (rbuf_len_ - cur_pos_ == COMMAND_HEADER_LENGTH) {
    //        uint32_t integer = 0;
    //        memcpy((char *)(&integer), rbuf_ + cur_pos_, sizeof(uint32_t));
    //        header_len_ = ntohl(integer);
    //        remain_packet_len_ = header_len_;
    //        cur_pos_ += COMMAND_HEADER_LENGTH;
    //        connStatus_ = kPacket;
    //      }
    //      log_info("GetRequest kHeader header_len=%u cur_pos=%u rbuf_len=%u remain_packet_len_=%d nread=%d\n", header_len_, cur_pos_, rbuf_len_, remain_packet_len_, nread);
    //    }
        break;
      }
    //  case kPGPacket: {
    //    if (header_len_ >= PB_MAX_MESSAGE - COMMAND_HEADER_LENGTH) {
    //      return kFullError;
    //    } else {
    //      // read msg body
    //      ssize_t nread = read(fd(), rbuf_ + rbuf_len_, remain_packet_len_);
    //      if (nread == -1) {
    //        if (errno == EAGAIN) {
    //          return kReadHalf;
    //        } else {
    //          return kReadError;
    //        }
    //      } else if (nread == 0) {
    //        return kReadClose;
    //      }

    //      rbuf_len_ += nread;
    //      remain_packet_len_ -= nread;
    //      if (remain_packet_len_ == 0) {
    //        cur_pos_ = rbuf_len_;
    //        connStatus_ = kComplete;
    //      }
    //      log_info("GetRequest kPacket header_len=%u cur_pos=%u rbuf_len=%u remain_packet_len_=%d nread=%d\n", header_len_, cur_pos_, rbuf_len_, remain_packet_len_, nread);
    //    }
    //    break;
    //  }
    //  case kComplete: {
    //    DealMessage();
    //    connStatus_ = kHeader;

    //    cur_pos_ = 0;
    //    rbuf_len_ = 0;
    //    return kReadAll;
    //  }
    //  // Add this switch case just for delete compile warning
    //  case kBuildObuf:
    //    break;

    //  case kWriteObuf:
    //    break;
    }
  }

  return kReadHalf;
}

WriteStatus PGConn::SendReply() {
  ssize_t nwritten = 0;
  while (wbuf_offset_ > 0) {
    nwritten = write(fd(), wbuf_ + write_offset_, wbuf_offset_ - write_offset_);
    if (nwritten <= 0) {
      break;
    }
    write_offset_ += nwritten;
    if (write_offset_ == wbuf_offset_) {
      wbuf_offset_ = 0;
      write_offset_ = 0;
    }
  }
  if (nwritten == -1) {
    if (errno == EAGAIN) {
      return kWriteHalf;
    } else {
      // Here we should close the connection
      return kWriteError;
    }
  }
  if (wbuf_offset_ == 0) {
    return kWriteAll;
  } else {
    return kWriteHalf;
  }
}

} // namespace pink
