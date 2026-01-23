#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/signal_set.hpp>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "application.h"
#include "json_loader.h"
#include "logging.h"
#include "request_handler.h"
#include "logging_request_handler.h"
#include "sdk.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;

namespace {

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn &fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

int main(int argc, const char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: game_server <game-config-json> <static-root>"sv << std::endl;
        return EXIT_FAILURE;
    }

    app_logging::InitLogging();

    try {
        model::Game game = json_loader::LoadGame(argv[1]);

        const unsigned num_threads = std::thread::hardware_concurrency();
        net::io_context ioc(num_threads);

        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code &ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                ioc.stop();
            }
        });

        std::error_code ec;
        std::filesystem::path doc_root = std::filesystem::canonical(argv[2], ec);
        if (ec || !std::filesystem::exists(doc_root) || !std::filesystem::is_directory(doc_root)) {
            std::cerr << "Invalid doc_root: " << argv[2] << "\n";
            return EXIT_FAILURE;
        }

        app::Application application(std::move(game));
        http_handler::RequestHandler handler{ioc, application, doc_root};
        http_handler::LoggingRequestHandler<http_handler::RequestHandler> logging_handler{handler};


        const int port = 8080;
        const auto address = net::ip::make_address("0.0.0.0");

        boost::json::object data;
        data["port"] = port;
        data["address"] = address.to_string();

        {
            boost::json::object data;
            data["port"] = port;
            data["address"] = address.to_string();

            BOOST_LOG_TRIVIAL(info)
                << boost::log::add_value(app_logging::additional_data, boost::json::value(std::move(data)))
                << "server started"sv;
        }

        http_server::ServeHttp(ioc, {address, port}, [&logging_handler](auto &&req, auto &&send) {
            logging_handler(std::forward<decltype(req)>(req), std::forward<decltype(send)>(send));
        });

        RunWorkers(std::max(1u, num_threads), [&ioc] { ioc.run(); });

        {
            boost::json::object data;
            data["code"] = 0;

            BOOST_LOG_TRIVIAL(info)
                << boost::log::add_value(app_logging::additional_data, boost::json::value(std::move(data)))
                << "server exited";
        }
        
    } catch (const std::exception &ex) {
        {
            boost::json::object data;
            data["code"] = static_cast<std::int64_t>(EXIT_FAILURE);
            data["exception"] = ex.what();

            BOOST_LOG_TRIVIAL(info)
                << boost::log::add_value(app_logging::additional_data, boost::json::value(std::move(data)))
                << "server exited";
        }

        return EXIT_FAILURE;
    }
}
