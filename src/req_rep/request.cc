#include "core/core.h"
#include "glog/logging.h"
#include "req_rep.pb.h"
#include <iomanip>
#include <zmq.hpp>

int main() {
  const std::string client_addr = sim::IPHelper::BuildConnectAddress();
  LOG(INFO) << "client | client_addr: " << client_addr;

  zmq::context_t context(1);
  zmq::socket_t socket(context, ZMQ_REQ);

  socket.connect(client_addr);

  // prepare request message
  sim_msg::Request request;
  request.set_id(sim::MouduleName::MODULE_REQUEST);
  request.set_content("hi this is request");
  request.set_timestamp_s(1.23456789);
  std::string payload;
  request.SerializeToString(&payload);

  // send request
  zmq::message_t msg(payload.size());
  memcpy(msg.data(), payload.c_str(), payload.size());
  socket.send(msg, zmq::send_flags::none);
  LOG(INFO) << "client | send request with payload size:" << payload.size();

  // wait for reply
  zmq::message_t reply;
  socket.recv(reply, zmq::recv_flags::none);
  std::string reply_payload(static_cast<char *>(reply.data()), reply.size());
  sim_msg::Response response;
  response.ParseFromString(reply_payload);
  LOG(INFO) << "client | got reply: " << response.id() << ", "
            << response.content() << ", " << std::setprecision(12)
            << response.timestamp_s();

  return 0;
}