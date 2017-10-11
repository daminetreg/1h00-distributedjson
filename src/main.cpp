#include <iostream>
#include <fstream>

#include <thread>
#include <nlohmann/json.hpp>


#include <boost/asio.hpp>
#include "boost/bind.hpp"

#include <filewatch.hpp>

namespace jsonsync {

  using json = nlohmann::json;

  const short multicast_port = 30001;
  const int max_message_count = 0;

  class controller {
    public:
    controller(boost::asio::io_service& io_service,
        const boost::asio::ip::address& listen_address,
        const boost::asio::ip::address& multicast_address)
      : socket_(io_service),
        // Create the sender endpoint
        send_endpoint_(multicast_address, multicast_port),
        send_socket_(io_service, send_endpoint_.protocol())
    {
      // Create the socket so that multiple may be bound to the same address.
      boost::asio::ip::udp::endpoint listen_endpoint(
          listen_address, multicast_port);
      socket_.open(listen_endpoint.protocol());
      socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true));
      socket_.bind(listen_endpoint);



      // Join the multicast group.
      socket_.set_option(
          boost::asio::ip::multicast::join_group(multicast_address));

      socket_.async_receive_from(
          boost::asio::buffer(data_, max_length), receiving_endpoint,
          boost::bind(&controller::handle_receive_from, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
    }

    void handle_receive_from(const boost::system::error_code& error,
        size_t bytes_recvd)
    {
      std::cout << "handle_receive_from : " << std::endl;
      if (!error)
      {
        std::cout.write(data_, bytes_recvd);
        std::cout << std::endl;

        auto rfc6902_patch = json::parse(std::string(data_, bytes_recvd));
        std::cout << "Applying patch : " << rfc6902_patch.dump(2) << std::endl;
        the_object = the_object.patch(rfc6902_patch);
  
        std::cout << "the_object is now : " << std::endl;
        std::cout << "====================" << std::endl;
        std::cout << the_object.dump(2);
        std::cout << "====================" << std::endl;

        auto outputstr = the_object.dump(2);
        std::fstream outfile{"athe_object.json", std::ios::trunc | std::ios::in | std::ios::out };
        outfile.write(outputstr.data(), outputstr.size());

        socket_.async_receive_from(
            boost::asio::buffer(data_, max_length), receiving_endpoint,
            boost::bind(&controller::handle_receive_from, this,
              boost::asio::placeholders::error,
              boost::asio::placeholders::bytes_transferred));

      }
    }

    void handle_send_to(const boost::system::error_code& error)
    {
      if (!error) {
        std::cout << "message sent" << message_ << std::endl;
      } else {
        std::cout << "ERROR: cannot send - " << message_ << std::endl;
      }
    }

    void add_changes(const json& j) {
      json patch = json::diff(the_object, j);
     
      message_ = patch.dump();

      std::cout << "Sending : " << message_ << std::endl;

      send_socket_.async_send_to(
          boost::asio::buffer(message_), send_endpoint_,
          boost::bind(&controller::handle_send_to, this,
            boost::asio::placeholders::error));

    }

  private:
    boost::asio::ip::udp::socket socket_;
    boost::asio::ip::udp::endpoint receiving_endpoint;
    enum { max_length = 1024 };
    char data_[max_length];
    json the_object = "{}"_json;

    std::string message_;

    boost::asio::ip::udp::endpoint send_endpoint_;
    boost::asio::ip::udp::socket send_socket_;

  };

}

int main(int argc, char** argv) {

  using json = nlohmann::json;

  try
  {
    if (argc < 2)
    {
      std::cerr << "Usage: " << argv[0] << " <listen_address> <multicast_address>\n";
      std::cerr << "  For IPv4, try:\n";
      std::cerr << "    controller 0.0.0.0 239.255.0.1\n";
      std::cerr << "  For IPv6, try:\n";
      std::cerr << "    controller 0::0 ff31::8000:1234\n";
      return 1;
    }

    using namespace jsonsync;
    
    boost::asio::io_service io_service;
    controller r(io_service,
        boost::asio::ip::address::from_string(argv[1]),
        boost::asio::ip::address::from_string(argv[2]));


    std::thread input_thread{[&](){

      for (;;) {
        if (argc == 4) {
          try { 
            watch_path(argv[3]);
            std::ifstream file{"the_object.json"};
            json j = json::parse(std::string(std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}));
            r.add_changes(j);

          } catch (...) {}

        } else {

          std::cout << "JSON please : " << std::endl;
          std::string line; 
          std::getline(std::cin, line);
          json j = json::parse(line);
          r.add_changes(j);
        }
      }

    }};

    io_service.run();
    input_thread.join();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }



  return 0;
}
