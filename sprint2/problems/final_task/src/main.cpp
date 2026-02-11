#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/program_options.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/value_semantic.hpp>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <thread>

#include "application.h"
#include "http_server.h"
#include "json_loader.h"
#include "logging.h"
#include "logging_request_handler.h"
#include "request_handler.h"
#include "sdk.h"
#include "ticker.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
using net::ip::tcp;

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

struct Args {
    int tick_period = -1;
    std::string config_file;
    std::string www_root;
    bool randomize_spawn_points = false;
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char *argv[]) {
    namespace po = boost::program_options;

    Args args;

    po::options_description desc{"Allowed options"s};
    desc.add_options()("help,h", "produce help message")(
        "tick-period,t", po::value(&args.tick_period)->value_name("milliseconds"),
        "set tick period")("config-file,c", po::value(&args.config_file)->value_name("file"),
                           "set config file path")(
        "www-root,w", po::value(&args.www_root)->value_name("dir"), "set static files root")(
        "randomize-spawn-points", "spawn dogs at random positions");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) {
        std::cerr << desc;
        return std::nullopt;
    }

    if (!vm.contains("config-file"s)) {
        throw std::runtime_error("Config file path is not specified"s);
    }
    if (!vm.contains("www-root"s)) {
        throw std::runtime_error("Static files root is not specified"s);
    }

    if (vm.contains("randomize-spawn-points"s)) {
        args.randomize_spawn_points = true;
    }

    return args;
}

int main(int argc, const char *argv[]) {
    app_logging::InitLogging();

    try {
        if (auto args = ParseCommandLine(argc, argv)) {
            model::Game game = json_loader::LoadGame(args->config_file);

            const unsigned num_threads = std::thread::hardware_concurrency();
            net::io_context ioc(num_threads);

            net::signal_set signals(ioc, SIGINT, SIGTERM);
            signals.async_wait(
                [&ioc](const sys::error_code &ec, [[maybe_unused]] int signal_number) {
                    if (!ec) {
                        ioc.stop();
                    }
                });

            std::error_code ec;
            std::filesystem::path doc_root = std::filesystem::canonical(args->www_root, ec);
            if (ec || !std::filesystem::exists(doc_root) ||
                !std::filesystem::is_directory(doc_root)) {
                std::cerr << "Invalid doc_root: " << args->www_root << "\n";
                return EXIT_FAILURE;
            }

            auto ms = std::chrono::milliseconds{args->tick_period};
            bool autotick = (args->tick_period > 0);
            app::Application application(std::move(game), args->randomize_spawn_points, autotick);
            auto api_starnd = net::make_strand(ioc);
            auto ticker = std::make_shared<Ticker>(
                api_starnd, ms,
                [&application](std::chrono::milliseconds delta) { application.Tick(delta); });
            http_handler::RequestHandler handler{std::move(api_starnd), application, doc_root};
            http_handler::LoggingRequestHandler<http_handler::RequestHandler> logging_handler{
                handler};

            const int port = 8080;
            const auto address = net::ip::make_address("0.0.0.0");
            auto endpoint = net::ip::tcp::endpoint(address, port);
            tcp::acceptor acceptor(net::make_strand(ioc));
            acceptor.open(endpoint.protocol());
            acceptor.set_option(net::socket_base::reuse_address(true));
            acceptor.bind(endpoint);
            acceptor.listen(net::socket_base::max_listen_connections);

            boost::json::object data;
            data["port"] = port;
            data["address"] = address.to_string();

            {
                boost::json::object data;
                data["port"] = port;
                data["address"] = address.to_string();

                BOOST_LOG_TRIVIAL(info)
                    << boost::log::add_value(app_logging::additional_data,
                                             boost::json::value(std::move(data)))
                    << "server started"sv;
            }

            ticker->Start();
            http_server::ServeHttp(
                ioc, std::move(acceptor),
                [&logging_handler](auto &&req, auto &&send, const std::string& client_ip) {
                    logging_handler(std::forward<decltype(req)>(req),
                                    std::forward<decltype(send)>(send), client_ip);
                });

            RunWorkers(std::max(1u, num_threads), [&ioc] { ioc.run(); });

            {
                boost::json::object data;
                data["code"] = 0;

                BOOST_LOG_TRIVIAL(info)
                    << boost::log::add_value(app_logging::additional_data,
                                             boost::json::value(std::move(data)))
                    << "server exited";
            }
        }

        return EXIT_SUCCESS;
    } catch (const std::exception &ex) {
        {
            boost::json::object data;
            data["code"] = static_cast<std::int64_t>(EXIT_FAILURE);
            data["exception"] = ex.what();

            BOOST_LOG_TRIVIAL(info) << boost::log::add_value(app_logging::additional_data,
                                                             boost::json::value(std::move(data)))
                                    << "server exited";
        }

        return EXIT_FAILURE;
    }
}
