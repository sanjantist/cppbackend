#include "session.h"

#include "model.h"

namespace model {
GameSession::GameSession(const Map* map) : map_(map) {}
const Map& GameSession::GetMap() const noexcept { return *map_; }
}  // namespace model