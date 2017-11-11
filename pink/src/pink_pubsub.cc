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

// remove 'unused parameter' warning
#define UNUSED(expr) do { (void)(expr); } while (0)

/*
  * FdClosedHandle(...) will be invoked before connection closed.
*/
void FdClosedHandle(int fd, const std::string& ip_port) {
  UNUSED(fd);
  UNUSED(ip_port);
}

PubSubThread::PubSubThread()
      :
        receiver_rsignal_(&receiver_mutex_) {
  pink_epoll_ = new PinkEpoll();
  if (pipe(msg_pfd_)) {
    exit(-1);
  }
  pink_epoll_->PinkAddEvent(msg_pfd_[0], EPOLLIN | EPOLLERR | EPOLLHUP);

  if (pipe(notify_pfd_)) {
    exit(-1); 
  }
  pink_epoll_->PinkAddEvent(notify_pfd_[0], EPOLLIN | EPOLLERR | EPOLLHUP);
}

PubSubThread::~PubSubThread() {
  delete(pink_epoll_);
}

void PubSubThread::RemoveConn(PinkConn* conn) {
  {
  slash::MutexLock l(&pattern_mutex_);
  for (auto it = pubsub_pattern_.begin(); it != pubsub_pattern_.end(); ++it) {
    for (auto conn_ptr = it->second.begin(); conn_ptr != it->second.end(); ++conn_ptr) {
      if ((*conn_ptr) == conn) {
        conn_ptr = it->second.erase(conn_ptr);
        break;
      }
    }
  }
  }

  {
  slash::MutexLock l(&channel_mutex_);
  for (auto it = pubsub_channel_.begin(); it != pubsub_channel_.end(); ++it) {
    for (auto conn_ptr = it->second.begin(); conn_ptr != it->second.end(); ++conn_ptr) {
      if ((*conn_ptr) == conn) {
        conn_ptr = it->second.erase(conn_ptr);
        break;
      }
    }
  }
  }
  
  {
    slash::WriteLock l(&rwlock_);
    pink_epoll_->PinkDelEvent(conn->fd());
  }
  {
    slash::MutexLock l(&mutex_);
    conns_.erase(conn->fd());
  }
}

void PubSubThread::PubSub(std::map<std::string, std::vector<PinkConn* >>& pubsub_channel,
                          std::map<std::string, std::vector<PinkConn* >>& pubsub_pattern) {
  {
    slash::MutexLock channel_lock(&channel_mutex_);
    pubsub_channel = pubsub_channel_;
  }
  {
    slash::MutexLock pattern_lock(&pattern_mutex_);
    pubsub_pattern = pubsub_pattern_;
  }
  return;
}

int PubSubThread::Publish(const std::string& channel, const std::string &msg) {
  int receivers;
  pub_mutex_.Lock();

  channel_ = channel;
  message_ = msg;
  // Send signal to ThreadMain()
  write(msg_pfd_[1], "", 1);
  receiver_mutex_.Lock();
  while (receivers_ == -1) {
    receiver_rsignal_.Wait();
  }
  receivers = receivers_;
  receivers_ = -1;
  receiver_mutex_.Unlock();

  pub_mutex_.Unlock();

  return receivers;
}

int PubSubThread::ClientChannelSize(PinkConn* conn) {
  int subscribed = 0;
  {
    slash::MutexLock l(&channel_mutex_);
    for (auto channel_ptr = pubsub_channel_.begin();
              channel_ptr != pubsub_channel_.end();
              ++channel_ptr) {
      auto conn_ptr = std::find(channel_ptr->second.begin(),
                                channel_ptr->second.end(),
                                conn);
      if (conn_ptr != channel_ptr->second.end()) {
        subscribed++;
      }
    }
  }
  {
    slash::MutexLock l(&pattern_mutex_);
    for (auto channel_ptr = pubsub_pattern_.begin();
              channel_ptr != pubsub_pattern_.end();
              ++channel_ptr) {
      auto conn_ptr = std::find(channel_ptr->second.begin(),
                                channel_ptr->second.end(),
                                conn);
      if (conn_ptr != channel_ptr->second.end()) {
        subscribed++;
      }
    }
  }
    return subscribed;
}

void PubSubThread::Subscribe(PinkConn *conn,
                             const std::vector<std::string>& channels,
                             const bool pattern,
                             std::vector<std::pair<std::string, int>>* result) {
  int subscribed = ClientChannelSize(conn);

  for (size_t i = 0; i < channels.size(); i++) {
    if (pattern) {  // if pattern mode, register channel to data structure
      {
        slash::MutexLock channel_lock(&pattern_mutex_);
        if (pubsub_pattern_.find(channels[i]) != pubsub_pattern_.end()) {
          auto conn_ptr = std::find(pubsub_pattern_[channels[i]].begin(),
                                    pubsub_pattern_[channels[i]].end(),
                                    conn);
          if (conn_ptr == pubsub_pattern_[channels[i]].end()) {
            pubsub_pattern_[channels[i]].push_back(conn);
            ++subscribed;
          }
        } else {    // at first
          std::vector<PinkConn *> conns = {conn};
          pubsub_pattern_[channels[i]] = conns;
          ++subscribed;
        }
        result->push_back(std::make_pair(channels[i], subscribed));
      }
    } else {  // if general mode, reigster channel to data structure
      {
        slash::MutexLock channel_lock(&pattern_mutex_);
        if (pubsub_channel_.find(channels[i]) != pubsub_channel_.end()) {
          auto conn_ptr = std::find(pubsub_channel_[channels[i]].begin(),
                                    pubsub_channel_[channels[i]].end(),
                                    conn);
          if (conn_ptr == pubsub_channel_[channels[i]].end()) {
            pubsub_channel_[channels[i]].push_back(conn);
            ++subscribed;
          }
        } else {  // at first
          std::vector<PinkConn *> conns = {conn};
          pubsub_channel_[channels[i]] = conns;
          ++subscribed;
        }
      }
      result->push_back(std::make_pair(channels[i], subscribed));
    }
  }
 
  {
    slash::WriteLock l(&rwlock_);
    conns_[conn->fd()] = conn;
  } 
  {
    slash::MutexLock l(&mutex_);
    fd_queue_.push(conn->fd());
    write(notify_send_fd(), "", 1);
  }
}

int PubSubThread::UnSubscribe(PinkConn *conn, 
                              const std::vector<std::string>& channels,
                              const bool pattern,
                              std::vector<std::pair<std::string, int>>* result) {
  int subscribed = ClientChannelSize(conn);
  bool exist = true;
  if (subscribed == 0) {
    exist = false; 
  }
  if (channels.size() == 0) {       // if client want to unsubscribe all of channels
    if (pattern) {                  // if pattern mode
      {
        slash::MutexLock l(&pattern_mutex_);
        for (auto channel_ptr = pubsub_pattern_.begin();
                  channel_ptr != pubsub_pattern_.end();
                  ++channel_ptr) {
          auto conn_ptr = std::find(channel_ptr->second.begin(),
                                channel_ptr->second.end(),
                                conn);
            if (conn_ptr != channel_ptr->second.end()) {
              result->push_back(std::make_pair(channel_ptr->first, --subscribed));
            }
          }
        }
      }
      {
        slash::MutexLock l(&channel_mutex_);
        for (auto channel_ptr = pubsub_channel_.begin();
                  channel_ptr != pubsub_channel_.end();
                  ++channel_ptr) {
          auto conn_ptr = std::find(channel_ptr->second.begin(),
                                channel_ptr->second.end(),
                                conn);
          if (conn_ptr != channel_ptr->second.end()) {
            result->push_back(std::make_pair(channel_ptr->first, --subscribed));
          }
        }
      }
    if (exist) {
      RemoveConn(conn);
    }
    return 0;
  }

  for (size_t i = 0; i < channels.size(); i++) {
    if (pattern) {      // if pattern mode, unsubscribe the channels of specified
      {
        slash::MutexLock l(&pattern_mutex_);
        auto channel_ptr = pubsub_pattern_.find(channels[i]);
        if (channel_ptr != pubsub_pattern_.end()) {
          auto it = std::find(channel_ptr->second.begin(),
                              channel_ptr->second.end(),
                              conn);
          if (it != channel_ptr->second.end()) {
            channel_ptr->second.erase(std::remove(
                                                  channel_ptr->second.begin(),
                                                  channel_ptr->second.end(),
                                                  conn),
                                      channel_ptr->second.end());
            result->push_back(std::make_pair(channels[i], --subscribed));
          } else {
            result->push_back(std::make_pair(channels[i], subscribed));
          }
        } else {
          result->push_back(std::make_pair(channels[i], 0));
        }
      }
    } else {            // if general mode, unsubscribe the channels of specified
      {
        slash::MutexLock l(&channel_mutex_);
        auto channel_ptr = pubsub_channel_.find(channels[i]);
        if (channel_ptr != pubsub_channel_.end()) {
          auto it = std::find(channel_ptr->second.begin(),
                              channel_ptr->second.end(),
                              conn);
          if (it != channel_ptr->second.end()) {
            channel_ptr->second.erase(std::remove(
                                  channel_ptr->second.begin(),
                                  channel_ptr->second.end(),
                                  conn),
                                channel_ptr->second.end());
            result->push_back(std::make_pair(channels[i], --subscribed));
          } else {
            result->push_back(std::make_pair(channels[i], subscribed));
          }
        } else {
          result->push_back(std::make_pair(channels[i], 0));
        }
      }
    }
  }
  // The number of channels this client currently subscibred
  // include general mode and pattern mode
  subscribed = ClientChannelSize(conn);
  if (subscribed == 0 && exist) {
    RemoveConn(conn);
  }
  return subscribed;
}

void *PubSubThread::ThreadMain() {
  int nfds;
  PinkFiredEvent *pfe;
  slash::Status s;
  PinkConn *in_conn = NULL;
  char triger[1];

  while (!should_stop()) {
    nfds = pink_epoll_->PinkPoll(-1);
    for (int i = 0; i < nfds; i++) {
      pfe = (pink_epoll_->firedevent()) + i;
      if (pfe->fd == notify_pfd_[0]) {        // New Connection
        if (pfe->mask & EPOLLIN) {
          read(notify_pfd_[0], triger, 1);
          {
            slash::MutexLock l(&mutex_);
            int new_fd = fd_queue_.front();
            fd_queue_.pop();
            pink_epoll_->PinkAddEvent(new_fd, EPOLLIN);
          } 
        } 
        continue;
      }
      if (pfe->fd == msg_pfd_[0]) {           // Publish
        if (pfe->mask & EPOLLIN) {
          read(msg_pfd_[0], triger, 1);
          std::string channel, msg;
          int32_t receivers = 0;
          if (channel_ != "" && message_ != "") {
            channel = channel_;
            msg = message_;
            channel_ = "";
            message_ = "";
          } else {
            continue;
          }
          // Send msg to clients
          channel_mutex_.Lock();
          for (auto it = pubsub_channel_.begin(); it != pubsub_channel_.end(); it++) {
            if (channel == it->first) {
              for (size_t i = 0; i < it->second.size(); i++) {
                it->second[i]->ConstructPublishResp(it->first, channel, msg, false);
                WriteStatus write_status = it->second[i]->SendReply();
                if (write_status == kWriteHalf) {
                  pink_epoll_->PinkModEvent(it->second[i]->fd(),
                                            EPOLLIN, EPOLLOUT);
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

          // Send msg to clients
          pattern_mutex_.Lock();
          for (auto it = pubsub_pattern_.begin(); it != pubsub_pattern_.end(); it++) {
            if (slash::stringmatchlen(it->first.c_str(), it->first.size(),
                                      channel.c_str(), channel.size(), 0)) {
              for (size_t i = 0; i < it->second.size(); i++) {
                it->second[i]->ConstructPublishResp(it->first, channel, msg, true);
                WriteStatus write_status = it->second[i]->SendReply();
                if (write_status == kWriteHalf) {
                  pink_epoll_->PinkModEvent(it->second[i]->fd(),
                                            EPOLLIN, EPOLLOUT);
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
          receivers_ = receivers;
          receiver_rsignal_.Signal();
          receiver_mutex_.Unlock();
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
            continue;  //  send all write buffer,
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
            RemoveConn(in_conn);
            CloseFd(in_conn);
            delete(in_conn);
            in_conn = NULL;
          }
        }
      }
    }
  }
  Cleanup();
  return NULL;
}

void PubSubThread::CloseFd(PinkConn* conn) {
  close(conn->fd());
  FdClosedHandle(conn->fd(), conn->ip_port());
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
