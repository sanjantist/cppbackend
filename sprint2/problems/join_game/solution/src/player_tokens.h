#pragma once
#include <optional>
#include <random>
#include <unordered_map>

#include "session.h"
#include "tagged.h"

namespace detail {
struct TokenTag {};
}  // namespace detail

using Token = util::Tagged<std::string, detail::TokenTag>;

namespace app {

class PlayerTokens {
   public:
    Token Issue(model::Player::Id player_id);

    std::optional<model::Player::Id> FindPlayerId(std::string_view token) const;

   private:
    Token Generate_();

    static std::string ToHex16_(uint64_t x) {
        static constexpr char kHex[] = "0123456789abcdef";
        std::string s(16, '0');
        for (int i = 15; i >= 0; --i) {
            s[i] = kHex[x & 0xF];
            x >>= 4;
        }
        return s;
    }

   private:
    std::unordered_map<std::string, model::Player::Id> token_to_player_;

    std::random_device random_device_;
    std::mt19937_64 generator1_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    std::mt19937_64 generator2_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
};

}  // namespace app