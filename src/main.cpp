#include "glog/logging.h"
#include <zmq.hpp>

int main(int argc, char *argv[]) {
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();

  zmq::context_t context(1);
  zmq::socket_t socket(context, ZMQ_REQ);
  socket.connect("tcp://localhost:5555");

  zmq::message_t request(5);
  memcpy(request.data(), "Hello", 5);
  socket.send(request, zmq::send_flags::none);

  zmq::message_t reply;
  socket.recv(reply, zmq::recv_flags::none);
  LOG(INFO) << "Received reply: "
            << std::string(static_cast<char *>(reply.data()), reply.size());

  return 0;
}