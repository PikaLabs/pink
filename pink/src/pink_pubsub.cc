// Copyright (c) 2015-present, Qihoo, Inc.  All rights reserved.
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <vector>

#include "pink/src/worker_thread.h"

#include "pink/include/pink_conn.h"
#include "pink/src/pink_item.h"
#include "pink/src/pink_epoll.h"

namespace pink {


PubSubThread::PubSubThread(ConnFactory *conn_factory,
                           ServerThread* server_thread,
                           int cron_interval)
      : server_thread_(server_thread),
        conn_factory_(conn_factory),
        cron_interval_(cron_interval),
        keepalive_timeout_(kDefaultKeepAliveTime) {
  /*
   * install the protobuf handler here
   */
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

int PubSubThread::conn_num() const {
  slash::ReadLock l(&rwlock_);
  return conns_.size();
}

std::vector<ServerThread::ConnInfo> PubSubThread::conns_info() const {
  std::vector<ServerThread::ConnInfo> result;
  slash::ReadLock l(&rwlock_);
  for (auto& conn : conns_) {
    result.push_back({
                      conn.first,
                      conn.second->ip_port(),
                      conn.second->last_interaction()
                     });
  }
  return result;
}

PinkConn* PubSubThread::MoveConnOut(int fd) {
  slash::WriteLock l(&rwlock_);
  PinkConn* conn = nullptr;
  auto iter = conns_.find(fd);
  if (iter != conns_.end()) {
    int fd = iter->first;
    conn = iter->second;
    pink_epoll_->PinkDelEvent(fd);
    conns_.erase(iter);
  }
  return conn;
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

void PubSubThread::RemoveConn(int fd) {
  RedisConn * conn_ptr = nullptr;
  for (auto it = pubsub_pattern_.begin(); it != pubsub_pattern_.end(); ++it) {
    for (auto conn = it->second.begin(); conn != it->second.end(); ++conn) {
      if ((*conn)->fd() == fd) {
        conn_ptr = *conn;
        it->second.erase(conn);
      } 
    }
  }
  for (auto it = pubsub_channel_.begin(); it != pubsub_channel_.end(); ++it) {
    for (auto conn = it->second.begin(); conn != it->second.end(); ++conn) {
      if ((*conn)->fd() == fd) {
        conn_ptr = *conn;
        it->second.erase(conn); 
      } 
    }
  }
  //CloseFd(conn_ptr); 
  delete(conn_ptr);
}

void PubSubThread::Subscribe(RedisConn *conn, const std::vector<std::string> channels, bool pattern, const std::string& resp) {
  for(size_t i = 0; i < channels.size(); i++) {
    if (pattern) {
      slash::MutexLock l(&pattern_mutex_);
      if (pubsub_pattern_.find(channels[i]) != pubsub_pattern_.end()) {
        pubsub_pattern_[channels[i]].push_back(conn); 
      } else {  // first
        std::vector<RedisConn *> conns = {conn};
        pubsub_pattern_[channels[i]] = conns;
      }
    } else {
      slash::MutexLock l(&channel_mutex_);
      if (pubsub_channel_.find(channels[i]) != pubsub_channel_.end()) {
        pubsub_channel_[channels[i]].push_back(conn); 
      } else {  // first
        std::vector<RedisConn *> conns = {conn};
        pubsub_channel_[channels[i]] = conns;
      }
    }
  }
  conns_[conn->fd] = fd;
  SendResponse(conn->fd(), resp);  
  pink_epoll_->PinkAddEvent(conn->fd(), EPOLLIN | EPOLLHUP);
}

void PubSubThread::UnSubscribe(RedisConn *conn_ptr, const std::vector<std::string> channels, bool pattern, const std::string& resp) {
  for(size_t i = 0; i < channels.size(); i++) {
    if (pattern) {
      slash::MutexLock l(&pattern_mutex_);
      auto find_ptr = pubsub_pattern_.find(channels[i]);
      if (find_ptr != pubsub_pattern_.end()) {
        for (auto conn = find_ptr->second.begin(); conn != find_ptr->second.end(); ++conn) {
          if ((*conn) == conn_ptr) {
            (*find_ptr).second.erase(conn); 
          }
        }
      } 
    } else {
      slash::MutexLock l(&channel_mutex_);
      auto find_ptr = pubsub_pattern_.find(channels[i]);
      if (find_ptr != pubsub_pattern_.end()) {
        for (auto conn = find_ptr->second.begin(); conn != find_ptr->second.end(); ++conn) {
          if ((*conn) == conn_ptr) {
            (*find_ptr).second.erase(conn); 
          }
        }
      } 
    }
    pink_epoll_->PinkDelEvent(conn_ptr->fd());
  }
  SendResponse(conn_ptr->fd(), resp);  
}

void *PubSubThread::ThreadMain() {
  pink_epoll_ = new PinkEpoll();
  int nfds, flag;
  PinkFiredEvent *pfe;
  slash::Status s;

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
          int nread = read(pfe->fd, triger, 1);
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
                  it->second[i]->BufferWriter(msg);
                  WriteStatus write_status = it->second[i]->SendReply();
                  if (write_status == kWriteHalf) {
                    pink_epoll_->PinkModEvent(it->second[i]->fd(), 0, EPOLLOUT); 
                  } else if (write_status == kWriteError) {
                    pink_epoll_->PinkDelEvent(it->second[i]->fd());
                    RemoveConn(pfe->fd);
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
                  (*it).second[i]->BufferWriter(msg);
                  WriteStatus write_status = it->second[i]->SendReply();
                  if (write_status == kWriteHalf) {
                    pink_epoll_->PinkModEvent(it->second[i]->fd(), 0, EPOLLOUT); 
                  } else if (write_status == kWriteError) {
                    pink_epoll_->PinkDelEvent(it->second[i]->fd());
                    RemoveConn(pfe->fd);
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
          in_conn->set_last_interaction(now);
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
          in_conn->set_last_interaction(now);
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

void PubSubThread::DoCronTask() {
  struct timeval now;
  gettimeofday(&now, NULL);
  slash::WriteLock l(&rwlock_);

  // Check whether close all connection
  slash::MutexLock kl(&killer_mutex_);
  if (deleting_conn_ipport_.count(kKillAllConnsTask)) {
    for (auto& conn : conns_) {
      CloseFd(conn.second);
      delete conn.second;
    }
    conns_.clear();
    deleting_conn_ipport_.clear();
    return;
  }

  std::map<int, PinkConn*>::iterator iter = conns_.begin();
  while (iter != conns_.end()) {
    // Check connection should be closed
    if (deleting_conn_ipport_.count(iter->second->ip_port())) {
      CloseFd(iter->second);
      deleting_conn_ipport_.erase(iter->second->ip_port());
      delete iter->second;
      iter = conns_.erase(iter);
      continue;
    }

    // Check keepalive timeout connection
    if (keepalive_timeout_ > 0 &&
        (now.tv_sec - iter->second->last_interaction().tv_sec >
         keepalive_timeout_)) {
      CloseFd(iter->second);
      server_thread_->handle_->FdTimeoutHandle(
          iter->first, iter->second->ip_port());
      delete iter->second;
      iter = conns_.erase(iter);
      continue;
    }
    ++iter;
  }
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
  server_thread_->handle_->FdClosedHandle(conn->fd(), conn->ip_port());
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
