#pragma once
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>

#include "sdk.h"

namespace http_server {

namespace net = boost::asio;
using tcp = net::ip::tcp;
namespace sys = boost::system;
namespace beast = boost::beast;
namespace http = beast::http;

using namespace std::literals;

inline void ReportError(beast::error_code ec, std::string_view what) {
    std::cerr << what << ": "sv << ec.message() << std::endl;
}

class SessionBase {
   public:
    SessionBase(const SessionBase&) = delete;
    SessionBase& operator=(const SessionBase&) = delete;

    void Run();

   protected:
    using HttpRequest = http::request<http::string_body>;

    explicit SessionBase(tcp::socket&& socket) : stream_(std::move(socket)) {}

    template <typename Body, typename Fields>
    void Write(http::response<Body, Fields>&& response) {
        // Запись выполняется асинхронно, поэтому response перемещаем в область кучи
        auto safe_response = std::make_shared<http::response<Body, Fields>>(std::move(response));

        auto self = GetSharedThis();
        http::async_write(stream_, *safe_response,
                          [safe_response, self](beast::error_code ec, std::size_t bytes_written) {
                              self->OnWrite(safe_response->need_eof(), ec, bytes_written);
                          });
    }

    ~SessionBase() = default;

   private:
    void Read();
    void OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read);
    void OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written);
    void Close();

    virtual std::shared_ptr<SessionBase> GetSharedThis() = 0;
    virtual void HandleRequest(HttpRequest&& request) = 0;

   private:
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    HttpRequest request_;
};

template <typename RequestHandler>
class Session : public SessionBase, public std::enable_shared_from_this<Session<RequestHandler>> {
   public:
    template <typename Handler>
    Session(tcp::socket&& socket, Handler&& request_handler, std::string&& client_ip)
        : SessionBase(std::move(socket)),
          request_handler_(std::forward<Handler>(request_handler)),
          client_ip_(std::move(client_ip)) {}

   private:
    std::shared_ptr<SessionBase> GetSharedThis() override { return this->shared_from_this(); }
    void HandleRequest(HttpRequest&& request) override {
        request_handler_(
            std::move(request),
            [self = this->shared_from_this()](auto&& response) {
                self->Write(std::move(response));
            },
            client_ip_);
    }

   private:
    RequestHandler request_handler_;
    const std::string client_ip_;
};

template <typename RequestHandler>
class Listener : public std::enable_shared_from_this<Listener<RequestHandler>> {
   public:
    template <typename Handler>
    Listener(net::io_context& ioc, tcp::acceptor&& acceptor, Handler&& handler)
        : ioc_(ioc),
          acceptor_(std::move(acceptor)),
          request_handler_(std::forward<Handler>(handler)) {}

    void Run() { DoAccept(); }

   private:
    void DoAccept() {
        acceptor_.async_accept(
            net::make_strand(ioc_),
            beast::bind_front_handler(&Listener::OnAccept, this->shared_from_this()));
    }

    void OnAccept(sys::error_code ec, tcp::socket socket) {
        if (ec) {
            return ReportError(ec, "accept"sv);
        }

        beast::error_code ep_ec;
        auto ep = socket.remote_endpoint(ep_ec);

        std::string client_ip;
        if (!ep_ec) {
            client_ip = ep.address().to_string();
        }

        AsyncRunSession(std::move(socket), std::move(client_ip));

        DoAccept();
    }

    void AsyncRunSession(tcp::socket&& socket, std::string&& client_ip) {
        std::make_shared<Session<RequestHandler>>(std::move(socket), request_handler_,
                                                  std::move(client_ip))
            ->Run();
    }

   private:
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    RequestHandler request_handler_;
};

template <typename RequestHandler>
void ServeHttp(net::io_context& ioc, tcp::acceptor&& acceptor, RequestHandler&& handler) {
    using MyListener = Listener<std::decay_t<RequestHandler>>;

    std::make_shared<MyListener>(ioc, std::move(acceptor), std::forward<RequestHandler>(handler))
        ->Run();
}

}  // namespace http_server
