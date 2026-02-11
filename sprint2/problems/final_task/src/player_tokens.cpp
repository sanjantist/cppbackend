#include "player_tokens.h"

namespace app {
Token PlayerTokens::Issue(model::Player::Id player_id) {
    Token token = Generate_();
    token_to_player_.emplace(*token, player_id);
    return token;
}

std::optional<model::Player::Id> PlayerTokens::FindPlayerId(std::string_view token) const {
    auto it = token_to_player_.find(std::string(token));
    if (it == token_to_player_.end()) return std::nullopt;
    return it->second;
}
Token PlayerTokens::Generate_() {
    const uint64_t a = generator1_();
    const uint64_t b = generator2_();
    return Token(ToHex16_(a) + ToHex16_(b));
}
static std::string ToHex16_(uint64_t x) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string s(16, '0');
    for (int i = 15; i >= 0; --i) {
        s[i] = kHex[x & 0xF];
        x >>= 4;
    }
    return s;
}
}  // namespace app