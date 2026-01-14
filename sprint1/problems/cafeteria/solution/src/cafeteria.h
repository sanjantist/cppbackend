#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <memory>

#include "hotdog.h"
#include "result.h"

namespace net = boost::asio;

using namespace std::literals;

// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;

// Класс "Кафетерий". Готовит хот-доги
class Cafeteria {
public:
  explicit Cafeteria(net::io_context &io) : io_{io} {}

  void OrderHotDog(HotDogHandler handler) {
    struct OrderState : std::enable_shared_from_this<OrderState> {
      explicit OrderState(net::any_io_executor ex,
                          std::shared_ptr<HotDogHandler> h)
          : bread_timer(ex), sausage_timer(ex), handler(std::move(h)) {}

      std::shared_ptr<Bread> bread;
      std::shared_ptr<Sausage> sausage;

      net::steady_timer bread_timer;
      net::steady_timer sausage_timer;

      bool bread_done = false;
      bool sausage_done = false;
      bool finished = false;

      std::shared_ptr<HotDogHandler> handler;

      void Complete(Result<HotDog> r) {
        if (finished)
          return;
        finished = true;

        boost::system::error_code ignored;
        bread_timer.cancel(ignored);
        sausage_timer.cancel(ignored);

        (*handler)(std::move(r));
      }
    };

    auto handler_ptr = std::make_shared<HotDogHandler>(std::move(handler));

    net::post(strand_, [this, handler_ptr] {
      auto state = std::make_shared<OrderState>(strand_.get_inner_executor(),
                                                handler_ptr);

      try {
        state->bread = store_.GetBread();
        state->sausage = store_.GetSausage();

        state->bread->StartBake(*gas_cooker_, [this, state] {
          net::dispatch(strand_, [this, state] {
            state->bread_timer.expires_after(1s);
            state->bread_timer.async_wait(net::bind_executor(
                strand_, [this, state](const boost::system::error_code &ec) {
                  if (ec)
                    return;

                  try {
                    state->bread->StopBaking();
                    state->bread_done = true;
                  } catch (...) {
                    state->Complete(Result<HotDog>::FromCurrentException());
                  }

                  if (!state->finished && state->bread_done &&
                      state->sausage_done) {
                    try {
                      HotDog hd{++next_hotdog_id_, state->sausage,
                                state->bread};
                      state->Complete(Result<HotDog>(std::move(hd)));
                    } catch (...) {
                      state->Complete(Result<HotDog>::FromCurrentException());
                    }
                  }
                }));
          });
        });

        state->sausage->StartFry(*gas_cooker_, [this, state] {
          net::dispatch(strand_, [this, state] {
            using namespace std::chrono_literals;

            state->sausage_timer.expires_after(1500ms);
            state->sausage_timer.async_wait(net::bind_executor(
                strand_, [this, state](const boost::system::error_code &ec) {
                  if (ec)
                    return;

                  try {
                    state->sausage->StopFry();
                    state->sausage_done = true;
                  } catch (...) {
                    state->Complete(Result<HotDog>::FromCurrentException());
                    return;
                  }

                  if (!state->finished && state->bread_done &&
                      state->sausage_done) {
                    try {
                      HotDog hd{++next_hotdog_id_, state->sausage,
                                state->bread};
                      state->Complete(Result<HotDog>(std::move(hd)));
                    } catch (...) {
                      state->Complete(Result<HotDog>::FromCurrentException());
                    }
                  }
                }));
          });
        });
      } catch (...) {
        state->Complete(Result<HotDog>::FromCurrentException());
      }
    });
  }

private:
  net::io_context &io_;
  net::strand<net::io_context::executor_type> strand_ = net::make_strand(io_);

  Store store_;
  std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
  int next_hotdog_id_ = 0;
};