#include "player.h"

#include <utility>

#include "dog.h"

namespace model {
namespace {
DogPos GenerateRandomPointOnRoad(const Road& road, std::mt19937& gen) {
    const auto a = road.GetStart();
    const auto b = road.GetEnd();

    if (road.IsHorizontal()) {
        std::uniform_int_distribution<int> dist(std::min(a.x, b.x), std::max(a.x, b.x));
        return model::DogPos{static_cast<double>(dist(gen)), static_cast<double>(a.y)};
    } else {
        std::uniform_int_distribution<int> dist(std::min(a.y, b.y), std::max(a.y, b.y));
        return model::DogPos{static_cast<double>(a.x), static_cast<double>(dist(gen))};
    }
}

const Road& PickRandomRoad(const Map& map, std::mt19937& gen) {
    const auto& roads = map.GetRoads();
    std::uniform_int_distribution<size_t> dist(0, roads.size() - 1);
    return roads[dist(gen)];
}

Dog SpawnDog(const Map& map, std::string name, std::mt19937& gen) {
    Dog dog{std::move(name)};

    const Road& road = PickRandomRoad(map, gen);
    dog.SetPosition(GenerateRandomPointOnRoad(road, gen));
    dog.SetDirection(DogDirection::North);
    dog.SetVelocity({0.0, 0.0});

    return dog;
}
}  // namespace

Player::Player(Id id, std::string name, std::weak_ptr<GameSession> session)
    : id_(id),
      dog_(SpawnDog(session.lock()->GetMap(), std::move(name), gen_)),
      session_(std::move(session)) {}

Player::Id Player::GetId() const noexcept { return id_; }

const std::string& Player::GetName() const noexcept { return dog_.GetName(); }

const Dog& Player::GetDog() const noexcept { return dog_; }

std::shared_ptr<GameSession> Player::LockSession() const noexcept { return session_.lock(); }
}  // namespace model