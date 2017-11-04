// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <vector>
#include <algorithm>

#include "pink/src/worker_thread.h"

#include "pink/include/pink_conn.h"
#include "pink/src/pink_item.h"
#include "pink/src/pink_epoll.h"
#include "pink/include/pink_pubsub.h"

namespace pink {


PubSubThread::PubSubThread()
      :
        msg_rsignal_(&msg_mutex_),
        receiver_rsignal_(&receiver_mutex_) {
      
  pink_epoll_ = new PinkEpoll();
  int fds[2];
  if (pipe(fds)) {
    exit(-1);
  }
  notify_receive_fd_ = fds[0];
  notify_send_fd_ = fds[1];
  pink_epoll_->PinkAddEvent(notify_receive_fd_, EPOLLIN | EPOLLERR | EPOLLHUP);
}

PubSubThread::~PubSubThread() {
  delete(pink_epoll_);
}

pink::WriteStatus PubSubThread::SendResponse(int32_t fd, const std::string& message) {
  ssize_t nwritten = 0, message_len_sended = 0, message_len_left = message.size();
  while (message_len_left > 0) {
    nwritten = write(fd, message.data() + message_len_sended, message_len_left);
    if (nwritten == -1 && errno == EAGAIN) {
      continue;
    } else if (nwritten == -1) {
      return pink::kWriteError;
    }
    message_len_sended += nwritten;
    message_len_left -= nwritten;
  }
  return pink::kWriteAll;
}

void PubSubThread::PubSub(std::map<std::string, std::vector<PinkConn* >>& pubsub_channel, std::map<std::string, std::vector<PinkConn* >>& pubsub_pattern) {
  pubsub_channel = pubsub_channel_;
  pubsub_pattern = pubsub_pattern_; 
}

int PubSubThread::Publish(int fd, const std::string& channel, const std::string &msg) {
  std::string triger = "t";
  int receivers;
  msg_mutex_.Lock();
  std::map<std::string, std::string> message;
  message[channel] = msg;
  msgs_[fd] = message;
  msg_rsignal_.Signal();
  msg_mutex_.Unlock();

  SendResponse(msg_pfd_[1], triger);
  receiver_mutex_.Lock();
  while(receivers_.find(fd) == receivers_.end()) {
    receiver_rsignal_.Wait();
  }
  receivers = receivers_[fd];
  receivers_.erase(receivers_.find(fd));
  receiver_mutex_.Unlock();
  return receivers;
}

void PubSubThread::RemoveConn(PinkConn* conn) {
  for (auto it = pubsub_pattern_.begin(); it != pubsub_pattern_.end(); ++it) {
    for (auto conn_ptr = it->second.begin(); conn_ptr != it->second.end(); ) {
      if ((*conn_ptr) == conn) {
        conn_ptr = it->second.erase(conn_ptr);
      } else {
        conn_ptr++; 
      }
    }
  }
  for (auto it = pubsub_channel_.begin(); it != pubsub_channel_.end(); ++it) {
    for (auto conn_ptr = it->second.begin(); conn_ptr != it->second.end(); ) {
      if ((*conn_ptr) == conn) {
        conn_ptr = it->second.erase(conn_ptr); 
      } else {
        conn_ptr++;
      }
    }
  }
  
  slash::WriteLock l(&rwlock_);
  pink_epoll_->PinkDelEvent(conn->fd());
  conns_.erase(conn->fd());
} 
  
void PubSubThread::Subscribe(PinkConn *conn, const std::vector<std::string> channels, bool pattern, std::vector<std::pair<std::string, int>>& result) { 
  for(size_t i = 0; i < channels.size(); i++) { 
    if (pattern) {
      slash::MutexLock l(&pattern_mutex_);
      if (pubsub_pattern_.find(channels[i]) != pubsub_pattern_.end()) {
        auto conn_ptr = std::find(pubsub_pattern_[channels[i]].begin(), pubsub_pattern_[channels[i]].end(), conn);
        if (conn_ptr == pubsub_pattern_[channels[i]].end()) {
          pubsub_pattern_[channels[i]].push_back(conn); 
        }
      } else {  // first
        std::vector<PinkConn *> conns = {conn};
        pubsub_pattern_[channels[i]] = conns;
      }
    } else {
      slash::MutexLock l(&channel_mutex_);
      if (pubsub_channel_.find(channels[i]) != pubsub_channel_.end()) {
        auto conn_ptr = std::find(pubsub_channel_[channels[i]].begin(), pubsub_channel_[channels[i]].end(), conn);
        if (conn_ptr == pubsub_channel_[channels[i]].end()) {
          pubsub_channel_[channels[i]].push_back(conn); 
        }
      } else {  // first
        std::vector<PinkConn *> conns = {conn};
        pubsub_channel_[channels[i]] = conns;
      }
    }
  }
  conns_[conn->fd()] = conn;
  pink_epoll_->PinkAddEvent(conn->fd(), EPOLLIN | EPOLLHUP);

  // The number of channels this client this currently subscribed to
  for (size_t i = 0; i < channels.size(); i++) {
    if (pattern) {
      int channel_subscribed = client_channel_[conn].size();
      auto conn_ptr = client_pattern_.find(conn);
      if (conn_ptr != client_pattern_.end()) {
        auto it = std::find(conn_ptr->second.begin(), conn_ptr->second.end(), channels[i]);
        if (it == conn_ptr->second.end()) {
          conn_ptr->second.push_back(channels[i]);
        }
        result.push_back(std::make_pair(channels[i], conn_ptr->second.size() + channel_subscribed));
      } else {
        std::vector<std::string> client_channels = {channels[i]};
        client_pattern_[conn] = client_channels;
        result.push_back(std::make_pair(channels[i], 1));
      }
    } else {
      int pattern_subscribed = client_pattern_[conn].size();
      auto conn_ptr = client_channel_.find(conn);
      if (conn_ptr != client_channel_.end()) {
        auto it = std::find(conn_ptr->second.begin(), conn_ptr->second.end(), channels[i]);
        if (it == conn_ptr->second.end()) {
          conn_ptr->second.push_back(channels[i]);
        }
        result.push_back(std::make_pair(channels[i], conn_ptr->second.size() + pattern_subscribed));
      } else {
        std::vector<std::string> client_channels = {channels[i]};
        client_channel_[conn] = client_channels;
        result.push_back(std::make_pair(channels[i], 1));
      }
    }
  }
}

int PubSubThread::UnSubscribe(PinkConn *conn_ptr, const std::vector<std::string> channels, bool pattern, std::vector<std::pair<std::string, int>>& result) {
  // Unsubscibre channels
  if (channels.size() == 0) {
    // TODO
    if (pattern) {
      for (auto it = client_pattern_.begin(); it != client_pattern_.end(); ) {
        if (it->first == conn_ptr) {
          int subscribed = it->second.size();
          for (auto channels = it->second.begin(); channels != it->second.end(); channels++) {
            result.push_back(std::make_pair(*channels, --subscribed));
          }
          client_pattern_.erase(it++); 
        } else{
          it++; 
        }
      } 
      for (auto it = pubsub_pattern_.begin(); it != pubsub_pattern_.end(); it++) {
        for (auto conn = it->second.begin(); conn != it->second.end(); ) {
          if ((*conn) == conn_ptr) {
            conn = it->second.erase(conn); 
          } else {
            conn++; 
          }
        } 
      }
    } else {
      for (auto it = client_channel_.begin(); it != client_channel_.end(); ) {
        if (it->first == conn_ptr) {
          int subscribed = it->second.size();
          for (auto channels = it->second.begin(); channels != it->second.end(); channels++) {
            result.push_back(std::make_pair(*channels, --subscribed));
          }
          client_channel_.erase(it++); 
        } else{
          it++; 
        }
      }  
      for (auto it = pubsub_channel_.begin(); it != pubsub_channel_.end(); it++) {
        for (auto conn = it->second.begin(); conn != it->second.end(); ) {
          if ((*conn) == conn_ptr) {
            conn = it->second.erase(conn); 
          } else {
            conn++; 
          }
        } 
      }
    }
    RemoveConn(conn_ptr);
    return 0;
  }

  for(size_t i = 0; i < channels.size(); i++) {
    if (pattern) {
      slash::MutexLock l(&pattern_mutex_);
      auto find_ptr = pubsub_pattern_.find(channels[i]);
      if (find_ptr != pubsub_pattern_.end()) {
        auto it = std::find(find_ptr->second.begin(), find_ptr->second.end(), conn_ptr);
        if (it != find_ptr->second.end()) {
          find_ptr->second.erase(std::remove(find_ptr->second.begin(), find_ptr->second.end(), conn_ptr), find_ptr->second.end());
        }
      } 
    } else {
      slash::MutexLock l(&channel_mutex_);
      auto find_ptr = pubsub_channel_.find(channels[i]);
      if (find_ptr != pubsub_channel_.end()) {
        auto it = std::find(find_ptr->second.begin(), find_ptr->second.end(), conn_ptr);
        if (it != find_ptr->second.end()) {
          find_ptr->second.erase(std::remove(find_ptr->second.begin(), find_ptr->second.end(), conn_ptr), find_ptr->second.end());
        }
      } 
    }
  }

  // The number of channels this client this currently subscribed to
  int subscribed = 0; 
  for (size_t i = 0; i < channels.size(); i++) {
    if (pattern) {
      auto conn = client_pattern_.find(conn_ptr);
      if (conn != client_pattern_.end()) {
        subscribed = conn->second.size();
        auto it = std::find(conn->second.begin(), conn->second.end(), channels[i]);
        if (it != conn->second.end()) {
          result.push_back(std::make_pair(channels[i], subscribed - 1));
          conn->second.erase(std::remove(conn->second.begin(), conn->second.end(), channels[i]), conn->second.end());
        } else {
          result.push_back(std::make_pair(channels[i], subscribed));
        }
      } else {
        result.push_back(std::make_pair(channels[i], 0));
      }
    } else {
      auto conn = client_channel_.find(conn_ptr);
      if (conn != client_channel_.end()) {
        subscribed = conn->second.size();
        auto it = std::find(conn->second.begin(), conn->second.end(), channels[i]);
        if (it != conn->second.end()) {
          result.push_back(std::make_pair(channels[i], subscribed - 1));
          conn->second.erase(std::remove(conn->second.begin(), conn->second.end(), channels[i]), conn->second.end());
        } else {
          result.push_back(std::make_pair(channels[i], subscribed));
        }
      } else {
        result.push_back(std::make_pair(channels[i], 0));
      }
    }
  }
  // TODO
  subscribed = client_channel_[conn_ptr].size() + client_pattern_[conn_ptr].size();
  if (subscribed == 0) {
    RemoveConn(conn_ptr);
  }
  return subscribed;
}

void *PubSubThread::ThreadMain() {
  pink_epoll_ = new PinkEpoll();
  int nfds, flag;
  PinkFiredEvent *pfe;
  slash::Status s;
  PinkConn *in_conn = NULL;

  if (pipe(msg_pfd_) == -1) {
    return nullptr; 
  }

  flag = fcntl(msg_pfd_[0], F_GETFL);
  fcntl(msg_pfd_[0], F_SETFL, flag | O_NONBLOCK);
  flag = fcntl(msg_pfd_[1], F_GETFL);
  fcntl(msg_pfd_[1], F_SETFL, flag | O_NONBLOCK);

  pink_epoll_->PinkAddEvent(msg_pfd_[0], EPOLLIN | EPOLLHUP);
  while (!should_stop()) {
    nfds = pink_epoll_->PinkPoll(-1);
    for(int i = 0; i < nfds; i++) {
      pfe = (pink_epoll_->firedevent()) + i;
      // Pubilsh
      if (pfe->fd == msg_pfd_[0]) { 
        if (pfe->mask & EPOLLIN) {
          char* triger = new char;
          read(pfe->fd, triger, 1);
          if (*triger != 't') {
            continue;
          }
          while (MessageNum() != 0) {
            int32_t receivers = 0;
            std::string channel, msg;
            int publish_fd;
            msg_mutex_.Lock();
            if (msgs_.size() != 0) {
              publish_fd = msgs_.begin()->first;
              channel = msgs_.begin()->second.begin()->first; 
              msg = msgs_.begin()->second.begin()->second; 
              msgs_.erase(msgs_.begin());
            }
            msg_mutex_.Unlock();

            channel_mutex_.Lock(); 
            for(auto it = pubsub_channel_.begin(); it != pubsub_channel_.end(); it++) {
              if (channel == it->first) {
                for(size_t i = 0; i < it->second.size(); i++) {
                  it->second[i]->ConstructPublishResp(it->first, channel, msg, false);
                  WriteStatus write_status = it->second[i]->SendReply();
                  if (write_status == kWriteHalf) {
                    pink_epoll_->PinkModEvent(it->second[i]->fd(), 0, EPOLLOUT); 
                  } else if (write_status == kWriteError) {
                    pink_epoll_->PinkDelEvent(it->second[i]->fd());
                    CloseFd(it->second[i]);
                    delete(it->second[i]);
                    conns_.erase(it->second[i]->fd());
                  } else if (write_status == kWriteAll) {
                    receivers++; 
                  }
                } 
              }   
            }
            channel_mutex_.Unlock();

            pattern_mutex_.Lock();
            for(auto it = pubsub_pattern_.begin(); it != pubsub_pattern_.end(); it++) {
              if (slash::stringmatchlen(it->first.c_str(), it->first.size(), channel.c_str(), channel.size(), 0)) {
                for(size_t i = 0; i < it->second.size(); i++) {
                  it->second[i]->ConstructPublishResp(it->first, channel, msg, true);
                  WriteStatus write_status = it->second[i]->SendReply();
                  if (write_status == kWriteHalf) {
                    pink_epoll_->PinkModEvent(it->second[i]->fd(), 0, EPOLLOUT); 
                  } else if (write_status == kWriteError) {
                    pink_epoll_->PinkDelEvent(it->second[i]->fd());
                    CloseFd(it->second[i]);
                    delete(it->second[i]);
                    conns_.erase(it->second[i]->fd());
                  } else if (write_status == kWriteAll) {
                    receivers++; 
                  }
                }
              }
            }
            pattern_mutex_.Unlock();
          
            receiver_mutex_.Lock();
            receivers_[publish_fd] = receivers;
            receiver_rsignal_.SignalAll();
            receiver_mutex_.Unlock();
          }
        } else {
          continue;
        }
      } else {
        in_conn = NULL;
        int should_close = 0;
        std::map<int, PinkConn *>::iterator iter = conns_.find(pfe->fd);
        if (iter == conns_.end()) {
          pink_epoll_->PinkDelEvent(pfe->fd);
          continue;
        }

        in_conn = iter->second;
        
        // Send reply 
        if (pfe->mask & EPOLLOUT && in_conn->is_reply()) {
          pink_epoll_->PinkModEvent(pfe->fd, 0, EPOLLIN);  // Remove EPOLLOUT
          WriteStatus write_status = in_conn->SendReply();
          if (write_status == kWriteAll) {
            in_conn->set_is_reply(false);
          } else if (write_status == kWriteHalf) {
            pink_epoll_->PinkModEvent(pfe->fd, EPOLLIN, EPOLLOUT);
            continue; //  send all write buffer,
                      //  in case of next GetRequest()
                      //  pollute the write buffer
          } else if (write_status == kWriteError) {
            should_close = 1;
          }
        }

        // Client request again
        if (!should_close && pfe->mask & EPOLLIN) {
          ReadStatus getRes = in_conn->GetRequest();
          if (getRes != kReadAll && getRes != kReadHalf) {
            // kReadError kReadClose kFullError kParseError kDealError
            should_close = 1;
          } else if (in_conn->is_reply()) {
            WriteStatus write_status = in_conn->SendReply();
            if (write_status == kWriteAll) {
              in_conn->set_is_reply(false);
            } else if (write_status == kWriteHalf) {
              pink_epoll_->PinkModEvent(pfe->fd, EPOLLIN, EPOLLOUT);
            } else if (write_status == kWriteError) {
              should_close = 1;
            }
          } else {
            continue;
          }
        }
        
        // Error
        if ((pfe->mask & EPOLLERR) || (pfe->mask & EPOLLHUP) || should_close) {
          {
            slash::WriteLock l(&rwlock_);
            RemoveConn(in_conn);
            pink_epoll_->PinkDelEvent(pfe->fd);
            CloseFd(in_conn);
            delete(in_conn);
            in_conn = NULL;
            conns_.erase(pfe->fd);
          }
        }
      }
    }
  }
  return nullptr;

}

bool PubSubThread::TryKillConn(const std::string& ip_port) {
  bool find = false;
  if (ip_port != kKillAllConnsTask) {
    slash::ReadLock l(&rwlock_);
    for (auto& iter : conns_) {
      if (iter.second->ip_port() == ip_port) {
        find = true;
        break;
      }
    }
  }
  if (find || ip_port == kKillAllConnsTask) {
    slash::MutexLock l(&killer_mutex_);
    deleting_conn_ipport_.insert(ip_port);
    return true;
  }
  return false;
}

void PubSubThread::CloseFd(PinkConn* conn) {
  close(conn->fd());
  //handle_->FdClosedHandle(conn->fd(), conn->ip_port());
}

void PubSubThread::Cleanup() {
  slash::WriteLock l(&rwlock_);
  for (auto& iter : conns_) {
    CloseFd(iter.second);
    delete iter.second;
  }
  conns_.clear();
}

};  // namespace pink
