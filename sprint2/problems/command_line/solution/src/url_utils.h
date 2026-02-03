#pragma once

#include <optional>
#include <string>
#include <string_view>

using namespace std::literals;

namespace http_handler {
struct Endpoint {
    Endpoint() = delete;
    constexpr static std::string_view API_PREFIX = "/api/"sv;
    constexpr static std::string_view V1_PREFIX = "/api/v1/"sv;

    constexpr static std::string_view MAPS = "/api/v1/maps"sv;
    constexpr static std::string_view JOIN = "/api/v1/game/join"sv;
    constexpr static std::string_view PLAYERS = "/api/v1/game/players"sv;
    constexpr static std::string_view STATE = "/api/v1/game/state"sv;
    constexpr static std::string_view ACTION = "/api/v1/game/player/action"sv;
    constexpr static std::string_view TICK = "/api/v1/game/tick"sv;
};

namespace url {
inline bool IsApi(std::string_view target) { return target.starts_with(Endpoint::API_PREFIX); }

inline std::optional<std::string_view> ExtractLastSegment(std::string_view target) {
    const auto pos = target.find_last_of('/');
    if (pos == std::string_view::npos || pos + 1 >= target.size()) {
        return std::nullopt;
    }
    return target.substr(pos + 1);
}

inline std::optional<std::string_view> ExtractMapId(std::string_view target) {
    if (!target.starts_with(Endpoint::MAPS)) {
        return std::nullopt;
    }
    if (target.size() == Endpoint::MAPS.size()) {
        return std::nullopt;
    }
    if (target[Endpoint::MAPS.size()] != '/') {
        return std::nullopt;
    }
    return target.substr(Endpoint::MAPS.size() + 1);
}

inline int HexVal(unsigned char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

inline bool DecodeSymbol(std::string_view sym, char &out) {
    if (sym.size() != 3 || sym[0] != '%') return false;

    int hi = HexVal(static_cast<unsigned char>(sym[1]));
    int lo = HexVal(static_cast<unsigned char>(sym[2]));
    if (hi < 0 || lo < 0) return false;

    out = static_cast<char>((hi << 4) | lo);
    return true;
}

inline std::string DecodeURI(std::string_view uri) {
    std::string result;
    result.reserve(uri.size());

    std::size_t prev_pos = 0;
    std::size_t current_pos = 0;

    while ((current_pos = uri.find('%', current_pos)) != std::string_view::npos) {
        result.append(uri.substr(prev_pos, current_pos - prev_pos));

        char decoded = 0;
        if (DecodeSymbol(uri.substr(current_pos, 3), decoded)) {
            result += decoded;
            current_pos += 3;
            prev_pos = current_pos;
        } else {
            result += '%';
            current_pos += 1;
            prev_pos = current_pos;
        }
    }

    result.append(uri.substr(prev_pos));
    return result;
}
}  // namespace url
}  // namespace http_handler