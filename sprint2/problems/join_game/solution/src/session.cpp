#include "session.h"

#include "model.h"

namespace model {
GameSession::GameSession(const Map* map) : map_(map) {}
const Map& GameSession::GetMap() const noexcept { return *map_; }

Dog::Dog(std::string name) : name_(name) {}
const std::string& Dog::GetName() const noexcept { return name_; }

Player::Player(Id id, std::string name, std::weak_ptr<GameSession> session)
    : id_(id), dog_(std::move(name)), session_(std::move(session)) {}

Player::Id Player::GetId() const noexcept { return id_; }
const std::string& Player::GetName() const noexcept { return dog_.GetName(); }

std::shared_ptr<GameSession> Player::LockSession() const noexcept { return session_.lock(); }
}  // namespace model