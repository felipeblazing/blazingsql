#pragma once

#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>
#include "blazingdb/transport/Message.h"

namespace blazingdb {
namespace transport {

class MessageQueue {
public:
  MessageQueue() = default;

  ~MessageQueue() = default;

  MessageQueue(MessageQueue&&) = delete;

  MessageQueue(const MessageQueue&) = delete;

  MessageQueue& operator=(MessageQueue&&) = delete;

  MessageQueue& operator=(const MessageQueue&) = delete;

public:
  std::shared_ptr<GPUMessage> getMessage(const std::string& messageToken);

  void putMessage(std::shared_ptr<GPUMessage>& message);

private:
  std::shared_ptr<GPUMessage> getMessageQueue(const std::string& messageToken);

  void putMessageQueue(std::shared_ptr<GPUMessage>& message);

private:
  std::mutex mutex_;
  std::vector<std::shared_ptr<GPUMessage>> message_queue_;
  std::condition_variable condition_variable_;
};

}  // namespace transport
}  // namespace blazingdb
