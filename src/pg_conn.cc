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
  write_offset_(0),
  parse_error_(false) {
  rbuf_ = reinterpret_cast<char *>(malloc(sizeof(char) * rbuf_size_));
  wbuf_ = reinterpret_cast<char *>(malloc(sizeof(char) * wbuf_size_));
}

PGConn::~PGConn() {
  free(rbuf_);
  free(wbuf_);
}

ReadStatus PGConn::Recv(size_t count) {
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
    } else if (strcmp(key, "client_encoding") == 0) {
      log_info("got param: %s=%s", key, val);
      client_encoding_ = val;
    } else {
      log_warn("unsupported startup param: %s=%s", key, val);
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
      set_is_reply(true);
      if (!CheckUser(username_)) {
        log_info("CheckUser failed");
        Glog("auth failed, no entry for user \"" + username_ + "\"");
        char buf[256];
        snprintf (buf, 256, "no entry for user \"%s\"", username_.c_str());
        AppendFatalResponse(buf);
        return false;
      }
      log_info("HandleStartupParamesters ok");
      AppendAuthRequest(AUTH_PLAIN);

      break;
    case PKT_SSLREQ:
      AppendObuf("N", 1);
      set_is_reply(true);
      return true;
    case 'p': {    //PasswordMessage
      set_is_reply(true);
      passwd_ = std::string(rbuf_ + parse_offset_, packet_header_.len - parse_offset_);
      parse_offset_ = packet_header_.len;
      if (passwd_.back() == '\0') {
        passwd_.pop_back();
      }
      log_info("HandlePasswordMessage passwd is (%s), size is %u", passwd_.data(), passwd_.size());
      if (CheckPasswd(passwd_)) {
        AppendAuthRequest(AUTH_OK);
        conn_status_ = PGStatus::kPGActive;
        log_info("HandlePasswordMessage ok, change to kPGActive");
        break;
      } else {
        Glog("wrong password for user \"" + username_ + "\" with passwd \"" + passwd_ + "\"");
        char buf[256];
        snprintf (buf, 256, "password authentication failed for user \"%s\"", username_.c_str());
        AppendFatalResponse(buf);
        return false;
      }
    }
    case PKT_STARTUP_V2:
    case PKT_CANCEL:
    default:
      log_warn("not supported or bad packet type=%d\n", packet_header_.type);
      return false;
  }
  return true;
}

ReadStatus PGConn::HandleNormal() {
  log_info("Handle packet type: %d/0x%x/%c", packet_header_.type, packet_header_.type, packet_header_.type);
  switch (packet_header_.type) {
    // one-packet queries
    case 'Q':	{ // Query
      parse_error_ = false;
      set_is_reply(true);
      //statement_ = std::string(rbuf_ + parse_offset_, rbuf_offset_ - parse_offset_);
      statement_ = std::string(rbuf_ + parse_offset_,  packet_header_.len - parse_offset_);
      parse_offset_ = packet_header_.len;
      parser_.Init(statement_, packet_header_.type);
      std::string error_info;
      if (parser_.Parse(error_info)) {
        DealMessage();
        AppendCommandComplete();
        return kReadAll;
      } else {
        Glog("syntax error for Query statement\"" + statement_ + "\"");
        Glog("syntax error for Parse statement error_info: " + error_info);
        //AppendErrorResponse();
        return kParseError;
      }
    }

    case 'F':		// FunctionCall
    case 'H':		// Flush
    case 'S': {	// Sync
      set_is_reply(true);
      parse_offset_ = packet_header_.len;
      if (!parse_error_) {
        return kReadAll;
      } else {
        return kParseError;
      }
    }

    // copy end markers
    case 'c':		// CopyDone(F/B)
    case 'f':		// CopyFail(F/B)
      break;

    //
    // extended protocol allows server (and thus pooler)
    // to buffer packets until sync or flush is sent by client
    //	
    case 'P':	{	// Parse
      parse_error_ = false;
      statement_ = std::string(rbuf_ + parse_offset_,  packet_header_.len - parse_offset_);
      parse_offset_ = packet_header_.len;
      parser_.Init(statement_, packet_header_.type);
      std::string error_info;
      if (parser_.Parse(error_info)) {
        AppendSingleResponse('1'); // ParseComplete
      } else {
        Glog("syntax error for Parse statement\"" + statement_ + "\"");
        Glog("syntax error for Parse statement error_info: " + error_info);
        parse_error_ = true;
        AppendErrorResponse(ERROR_MSG_PARSE);
      }
      return kReadHalf;
    }
    case 'B':	{	// Bind
      parse_offset_ = packet_header_.len;
      if (!parse_error_) {
        AppendSingleResponse('2'); // BindComplete
      }
      return kReadHalf;
    }
    case 'D':	{	// Describe
      parse_offset_ = packet_header_.len;
      if (!parse_error_) {
        AppendSingleResponse('n'); // NoData
      }
      return kReadHalf;
    }
    case 'E':	{	// Execute
      parse_offset_ = packet_header_.len;
      if (!parse_error_) {
        DealMessage();
        AppendCommandComplete();
      }
      return kReadHalf;
    }
    case 'C':		// CommandComplete
    case 'd':		// CopyData(F/B)
      break;

    case 'X': // Terminate
      log_warn("client close request");
      //disconnect_client(client, false, "client close request");
      return kReadClose;
      // client wants to go away
    default:
      parse_offset_ = packet_header_.len;
      log_warn("unknown packet type: %d/0x%x", packet_header_.type, packet_header_.type);
      //disconnect_client(client, true, "unknown pkt");
      return kParseError;
  }
  return kParseError;
}

// trival implementation
Status PGConn::AppendWelcome() {
  PacketBuf* packet = NewWelcomeMsg();

  // TODO maybe add cancel key
  //packet.WriteParameterStatus("is_superuser", "on");

  return AppendObuf(packet->buf, packet->write_pos);
}

bool PGConn::CheckUser(const std::string &user) {
  return true;
}

bool PGConn::CheckPasswd(const std::string &passwd) {
  return true;
}
bool PGConn::Glog(const std::string &msg) {
  return true;
}

Status PGConn::AppendCommandComplete() {
  char buf[128];
  snprintf (buf, 128, "INSERT 0 %lu", parser_.rows_.size());

  PacketBuf *packet = new PacketBuf;
  packet->WriteCommandComplete(buf);
  //packet->WriteGeneric('Z', "c", 'I');
  AppendObuf(packet->buf, packet->write_pos);

  delete packet;

  return Status::OK();
}

Status PGConn::AppendReadyForQuery() {
 // PacketBuf *packet = new PacketBuf;
 // packet->WriteGeneric('Z', "c", 'I');
 // AppendObuf(packet->buf, packet->write_pos);
 // delete packet;

  PacketBuf packet;
  packet.WriteGeneric('Z', "c", 'I');
  //packet.WriteReadyForQuery();
  AppendObuf(packet.buf, packet.write_pos);

  return Status::OK();
}

Status PGConn::AppendAuthRequest(int type) {
  PacketBuf packet;
  packet.WriteGeneric('R', "i", type);

  AppendObuf(packet.buf, packet.write_pos);

  return Status::OK();
}

Status PGConn::AppendErrorResponse(const char* msg) {
  //char buf[128];
  //snprintf (buf, 128, "syntax error at or near.");

  //PacketBuf *packet = new PacketBuf;
  //packet->WriteErrorResponse(msg);
  //AppendObuf(packet->buf, packet->write_pos);
  //delete packet;

  PacketBuf packet;
  packet.WriteErrorResponse(msg);
  AppendObuf(packet.buf, packet.write_pos);

  return Status::OK();
}

Status PGConn::AppendFatalResponse(const char* msg) {
  PacketBuf packet;
  packet.WriteFatalResponse(msg);
  AppendObuf(packet.buf, packet.write_pos);

  return Status::OK();
}

Status PGConn::AppendSingleResponse(char type) {
  PacketBuf packet;
  packet.WriteGeneric(type, "");
  //packet->WriteGeneric('Z', "c", 'I');
  AppendObuf(packet.buf, packet.write_pos);

  return Status::OK();
}

Status PGConn::AppendSpecialParameter() {
  PacketBuf packet;
  packet.WriteParameterStatus("session_authorization", username_.c_str());
  packet.WriteParameterStatus("client_encoding", client_encoding_.c_str());
  //packet.WriteParameterStatus("server_encoding", client_encoding_.c_str());
  GetRandomBytes(cancel_key_, 8);
  packet.WriteBackendKeyData(cancel_key_);
  //packet->WriteGeneric('Z', "c", 'I');
  AppendObuf(packet.buf, packet.write_pos);

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

// TODO: fix bug
//   we need to distinguish the header or the body when read partial happens;
ReadStatus PGConn::ClientProto() {
  // we read 8 bytes to store both v2 and v3 header
  if (V2_HEADER_LEN > rbuf_offset_) {
    ReadStatus read_status = Recv(V2_HEADER_LEN - rbuf_offset_);
    if (read_status != kReadAll) {
      return read_status;
    }
  }

  if (!IsCompleteHeader(rbuf_, rbuf_offset_)) {
    return kReadHalf;
  }

  int len = rbuf_offset_;
  if (!packet_header_.ParseFromArray(rbuf_, &len)) {
    log_warn ("ClientProto P1: parse header failed");
    return kParseError;
  }
  parse_offset_ = len;

  log_info ("ClientProto parse header:");
  //packet_header_.Dump();

  // we read the len 
  int remain = packet_header_.len - rbuf_offset_;
  if (remain <= 0) {
    log_warn ("ClientProto P2: remain=%d packet_header_.len=%d rbuf_offset_=%d", remain, packet_header_.len, rbuf_offset_);
    return kReadAll;
  }
  ReadStatus read_status = Recv(remain);
  if (read_status != kReadAll) {
    return read_status;
  }
  if (rbuf_offset_ < packet_header_.len) {
    return kReadHalf;
  }
  
  log_info ("ClientProto recv msg ok");
  return kReadAll;
}

void PGConn::ResetRbuf() {
  if (rbuf_offset_ > parse_offset_) {
    memmove(rbuf_, rbuf_ + parse_offset_, rbuf_offset_ - parse_offset_);
  }
  rbuf_offset_ -= parse_offset_;
  parse_offset_ = 0;
}

// FrontEnd/BackEnd proto has 2 phases: startup and normal;
//   Start-Up:
//      we receive ParameterStatus;
//      request for plain password;
//      check password so that change status to Active;
//      send welcome message and ready for query
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

        if (!HandleStartup()) {
          return kParseError;
        }

        ResetRbuf();

        if (conn_status_ == kPGActive) {
          AppendWelcome();
          AppendSpecialParameter();
          AppendReadyForQuery();

          conn_status_ = kPGHeader;
          set_is_reply(true);

          //ResetRbuf();

          log_info ("kPGActive: send welcome msg(%d)", wbuf_offset_);
        }
        log_info ("kPGLogin end");
        return kReadAll;
        //break;
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

        result = HandleNormal();
        if (result == kParseError) {
          return kParseError;
        } else if (result == kReadHalf) {
          ResetRbuf();
        } else {    // kReadAll
          //set_is_reply(true);
          ResetRbuf();
          AppendReadyForQuery();
          return kReadAll;
        }
        break;
      }
      default:
        log_warn ("unknown conn status");
        return kParseError;
        break;
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
