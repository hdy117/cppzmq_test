#include "core/core.h"
#include "glog/logging.h"
#include "req_rep.pb.h"
#include <iomanip>
#include <zmq.hpp>

int main(int argc, char *argv[]) {
  const std::string server_addr = sim::IPHelper::BuildBindAddress();
  LOG(INFO) << "server_addr: " << server_addr;

  zmq::context_t context(1);
  zmq::socket_t socket(context, ZMQ_REP);

  socket.bind(server_addr);

  // wait for request
  zmq::message_t request;
  auto recv_result = socket.recv(request, zmq::recv_flags::none);
  assert(recv_result.has_value());
  std::string request_str(static_cast<char *>(request.data()), request.size());
  sim_msg::Request req_msg;
  req_msg.ParseFromString(request_str);
  LOG(INFO) << "server | got request id: " << req_msg.id()
            << ", content: " << req_msg.content()
            << ", timestamp_s: " << std::setprecision(12)
            << req_msg.timestamp_s();

  // send reply
  sim_msg::Response response;
  response.set_id(sim::MouduleName::MODULE_RESPONSE);
  response.set_content("this is response from server");
  response.set_timestamp_s(0.123456789);
  std::string payload;
  response.SerializeToString(&payload);

  zmq::message_t reply(payload.size());
  memcpy(reply.data(), payload.data(), payload.size());

  socket.send(reply, zmq::send_flags::none);
  LOG(INFO) << "server | send response with payload size: " << payload.size();

  return 0;
}