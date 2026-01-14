#include "audio.h"

#include <boost/asio.hpp>
#include <iostream>

using namespace std::literals;
namespace net = boost::asio;
using net::ip::udp;

void StartServer(uint16_t port) {
  Player player(ma_format_u8, 1);
  static const int max_buffer_size = 65507;

  try {
    boost::asio::io_context io_context;

    udp::socket socket(io_context, udp::endpoint(udp::v4(), port));
    for (;;) {
      std::array<char, max_buffer_size> recv_buf;
      udp::endpoint remote_endpoint;

      auto size = socket.receive_from(net::buffer(recv_buf), remote_endpoint);
      const size_t frame_count = size / player.GetFrameSize();

      player.PlayBuffer(recv_buf.data(), frame_count, 1.5s);

      boost::system::error_code ignored_error;
    }
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
}

void StartClient(uint16_t port) {
  std::cout << "Enter server IP to send a message"sv << std::endl;
  std::string ip;
  std::cin >> ip;

  static const int max_buffer_size = 65507;

  try {
    net::io_context io_context;

    udp::socket socket(io_context, udp::v4());

    boost::system::error_code ec;
    auto endpoint = udp::endpoint(net::ip::make_address(ip, ec), port);

    Recorder recorder(ma_format_u8, 1);
    auto rec_result = recorder.Record(65000, 1.5s);

    socket.send_to(net::buffer(rec_result.data,
                               rec_result.frames * recorder.GetFrameSize()),
                   endpoint);
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
}

int main(int argc, char **argv) {
  if (argc != 3) {
    std::cout << "Usage: "sv << argv[0] << " <mode> "sv << "<port>"sv
              << std::endl;
    return 1;
  }

  static const uint16_t port = atoi(argv[2]);

  if (argv[1] == "server"s) {
    StartServer(port);
  } else if (argv[1] == "client"s) {
    StartClient(port);
  } else {
    std::cout << "Unknown mode"sv << std::endl;
  }

  return 0;
}