#include "zmq.hpp"
#include <atomic>
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "core/core.h"
#include "glog/logging.h"

/**
 * @brief 是的，您的这个理解**非常准确**，可以作为一个非常好的实践原则来记忆。

我们来逐条梳理并确认一下，这能帮您把概念理解得更透彻：

---

### 1. `DEALER` <-> `ROUTER` 通信

**您的理解：** 不需要考虑空帧。
**结论：** **正确。**

**原因：** 在纯粹的、点对点的异步通信场景下，`DEALER` 和 `ROUTER`
都不依赖空帧来工作。`ROUTER` 的核心功能是基于 `identity` 帧进行路由。`DEALER`
发送 `[消息体]`，`ROUTER` 收到 `[identity][消息体]`；`ROUTER` 回复时发送
`[identity][回复体]`，`DEALER` 收到
`[回复体]`。整个过程流畅自然，完全不需要空帧的参与。

> **唯一的例外：** 如果你的 `ROUTER` 扮演的是一个代理（Broker）角色，它接收
`DEALER` 的消息，然后又将该消息转发给后端的 `REP` 服务，那么这个 `DEALER`
就必须发送空帧。但请注意，这个要求不是 `ROUTER`
本身提出的，而是为了满足最终接收者 `REP` 的格式要求。

---

### 2. `REQ` <-> `ROUTER` 通信

**您的理解：** 必须考虑空帧。
**结论：** **正确。**

**原因：** 这其实是双向的。

* **`REQ` 发送时：** `REQ` 套接字会自动在消息体前**添加一个空帧**。所以 `ROUTER`
收到的消息实际上是 `[identity][空帧][消息体]`。`ROUTER`
端的应用程序在解析消息时，必须处理掉这个空帧。
* **`ROUTER` 回复时：** `REQ` 套接字期望收到的回复格式也是
`[空帧][回复体]`。因此，`ROUTER` 端的应用程序在发送回复时，必须先发送 `identity`
帧，然后**手动添加一个空帧**，最后再发送回复体。即发送
`[identity][空帧][回复体]`。

所以，在这种模式下，`ROUTER`
端的代码逻辑**必须**要意识到空帧的存在，并进行相应的处理。

---

### 3. `DEALER` -> `REP` 通信

**您的理解：** 必须考虑空帧。
**结论：** **正确。**

**原因：** 这是 `DEALER` 模拟 `REQ` 的经典用法。

* `REP` 套接字是一个严格的“应答者”，它只接受符合 `REQ`/`REP` 规范的请求，也就是
`[可选的identity...][空帧][消息体]`。
* `DEALER` 套接字是一个“裸套接字”，它不会自动添加任何东西。
* 因此，为了让 `REP` 能够正确接收和处理消息，`DEALER`
端的应用程序**必须手动**在消息体前先发送一个空帧。

---

### 总结表格

为了让这个规则更清晰，可以看下这个表格：

| 模式 (Pattern) | 发送方行为 | 接收方期望 | 是否需要手动处理空帧？ |
| :--- | :--- | :--- | :--- |
| **`DEALER` -> `ROUTER`** | 不自动添加空帧 | 不关心空帧 | **否 (No)**
(在纯异步模式下) | | **`DEALER` -> `REP`** | 不自动添加空帧 | **必须有**空帧 |
**是 (Yes)**, `DEALER`端必须手动发送 | | **`REQ` -> `ROUTER`** |
**自动添加**空帧 | — | **是 (Yes)**, `ROUTER`端在**接收和回复**时都需处理 |

所以，您的总结完全正确。这个判断准则可以帮助您在设计 ZeroMQ 应用架构时，快速确定
`DEALER` 的消息构建策略。核心就在于：**`DEALER` 是手动的，`REQ`/`REP`
是自动的，跟谁通信，就要 맞춰 谁的规矩。**
 *
 * @param client_id
 */

std::atomic<bool> stop_server_flag(false);
const bool enable_empty_frame = false; // 是否启用空帧

enum class ErrCode {
  OK = 0,
  ERROR = 1,
};

void dealer_req_msg(zmq::socket_t &socket, bool enable_empty_frame,
                    const std::string &msg, const std::string &identity) {
  // send empty frame if necessary
  if (enable_empty_frame) {
    socket.send(zmq::message_t(), zmq::send_flags::sndmore);
  }

  // send message content
  auto result = socket.send(zmq::buffer(msg), zmq::send_flags::none);
}

void dealer_recv_msg(zmq::socket_t &socket, bool enable_empty_frame,
                     std::string &msg_out) {
  // recv empty frame if necessary
  if (enable_empty_frame) {
    zmq::message_t empty_frame;
    socket.recv(empty_frame, zmq::recv_flags::none);
  }

  // clear msg out
  msg_out.clear();

  // recv message content
  zmq::message_t msg;
  socket.recv(msg, zmq::recv_flags::none);
  msg_out = std::string(static_cast<char *>(msg.data()), msg.size());
}

bool router_recv_msg(zmq::socket_t &socket, bool enable_empty_frame,
                     std::string &client_identity, std::string &request_msg) {
  // recv client identity
  zmq::message_t identity;
  auto result = socket.recv(identity, zmq::recv_flags::dontwait);

  if (result.has_value() && result.value() > 0) {
    // identity of dealer client
    client_identity =
        std::string(static_cast<char *>(identity.data()), identity.size());

    // recv empty frame
    if (enable_empty_frame) {
      zmq::message_t empty_frame;
      socket.recv(empty_frame, zmq::recv_flags::none);
    }

    // recv request message
    zmq::message_t request;
    socket.recv(request, zmq::recv_flags::none);
    request_msg =
        std::string(static_cast<char *>(request.data()), request.size());
    return true;
  }

  return false;
}

void router_reply_msg(zmq::socket_t &socket, bool enable_empty_frame,
                      const std::string &client_identity,
                      const std::string &response_msg) {
  socket.send(zmq::buffer(client_identity), zmq::send_flags::sndmore);
  if (enable_empty_frame) {
    socket.send(zmq::message_t(), zmq::send_flags::sndmore);
  }
  socket.send(zmq::buffer(response_msg), zmq::send_flags::none);
}

// client线程函数：使用DEALER套接字
void dealer_client(zmq::context_t &context, int client_id) {
  try {
    // 创建DEALER套接字
    zmq::socket_t client_socket(context, zmq::socket_type::dealer);

    // 设置client唯一标识，方便服务器识别
    std::string identity = "client-" + std::to_string(client_id);
    client_socket.setsockopt(ZMQ_IDENTITY, identity.data(), identity.size());

    // 连接到ROUTER服务器
    auto connect_address = sim::IPHelper::BuildConnectAddress();
    LOG(INFO) << "connect_address: " << connect_address;
    client_socket.connect(connect_address.c_str());
    LOG(INFO) << "client connect to server id:" << client_id;

    // 发送1个请求
    // msg content
    std::string request = "this is client id: " + std::to_string(client_id);
    dealer_req_msg(client_socket, enable_empty_frame, request, identity);
    LOG(INFO) << "client id: " << identity << " sent message: " << request;

    // recv from server
    // recv poll on socket
    zmq::pollitem_t items[] = {{client_socket, 0, ZMQ_POLLIN, 0}};

    // poll on the socket every 10ms, up to 10 times
    int counter = 0;
    while (counter < 10) {
      int rc = zmq::poll(items, 1, 10);
      if (rc < 0) {
        throw std::runtime_error("zmq poll error");
      }
      if (items[0].revents & ZMQ_POLLIN) {
        std::string response;
        dealer_recv_msg(client_socket, enable_empty_frame, response);
        LOG(INFO) << "client id: " << client_id
                  << " got response: " << response;
      }
      counter++;
    }

    // 关闭client套接字
    LOG(INFO) << "client id: " << client_id << " closed";
    client_socket.close();
  } catch (const std::exception &e) {
    std::cerr << "client " << client_id << " error: " << e.what() << std::endl;
  }
}

// 服务器线程函数：使用ROUTER套接字
void router_server(zmq::context_t &context) {
  try {
    // 创建ROUTER套接字
    zmq::socket_t server_socket(context, zmq::socket_type::router);

    // 绑定到端口
    auto bind_address = sim::IPHelper::BuildBindAddress();
    LOG(INFO) << "bind_address: " << bind_address;
    server_socket.bind(bind_address.c_str());
    LOG(INFO) << "ROUTER listenning 5555..." << std::endl;

    // 服务器持续运行
    while (!stop_server_flag.load()) {
      // ROUTER接收消息格式：[client标识][空帧][消息内容]
      std::string client_identity, request_msg;
      auto success = router_recv_msg(server_socket, enable_empty_frame,
                                     client_identity, request_msg);
      if (success) {
        // print received request message
        LOG(INFO) << "server got request from client id: " << client_identity
                  << " content: " << request_msg << std::endl;

        // 发送响应内容，多次回复
        for (auto i = 0; i < 3; ++i) {
          std::string response =
              "server processed " + request_msg + " " + std::to_string(i);
          router_reply_msg(server_socket, enable_empty_frame, client_identity,
                           response);
        }
      }

      // 模拟处理请求
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    server_socket.close();
  } catch (const std::exception &e) {
    std::cerr << "server error: " << e.what() << std::endl;
  }

  LOG(INFO) << "server closed.";
}

int main() {
  // zmq context
  zmq::context_t context(2);

  // 启动ROUTER服务器线程
  std::thread server(router_server, std::ref(context));

  // 等待服务器启动完成
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // 启动多个DEALERclient
  const int client_count = 3;
  std::vector<std::thread> clients;

  for (int i = 0; i < client_count; ++i) {
    clients.emplace_back(dealer_client, std::ref(context), i);
  }

  // 等待所有client完成
  for (auto &client : clients) {
    client.join();
  }

  // 优雅关闭服务器（实际应用中可使用信号处理）
  LOG(INFO) << "all client shutdown, closing server..." << std::endl;
  stop_server_flag.store(true);
  server.join();

  // 关闭上下文
  LOG(INFO) << "program terminated";

  return 0;
}
