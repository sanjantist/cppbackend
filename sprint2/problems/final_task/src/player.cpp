#include "player.h"

#include <utility>

#include "dog.h"

namespace model {
namespace {
DogPos GenerateRandomPointOnRoad(const Road& road, std::mt19937& gen) {
    const auto a = road.GetStart();
    const auto b = road.GetEnd();

    if (road.IsHorizontal()) {
        std::uniform_real_distribution<double> dist(std::min(a.x, b.x), std::max(a.x, b.x));
        return model::DogPos{dist(gen), static_cast<double>(a.y)};
    } else {
        std::uniform_real_distribution<double> dist(std::min(a.y, b.y), std::max(a.y, b.y));
        return model::DogPos{static_cast<double>(a.x), dist(gen)};
    }
}

const Road& PickRandomRoad(const Map& map, std::mt19937& gen) {
    const auto& roads = map.GetRoads();
    std::uniform_int_distribution<size_t> dist(0, roads.size() - 1);
    return roads[dist(gen)];
}

Dog SpawnDog(const Map& map, std::string name, std::mt19937& gen, bool randomize_dog_spawn) {
    Dog dog{std::move(name)};

    if (randomize_dog_spawn) {
        const Road& road = PickRandomRoad(map, gen);
        dog.SetPosition(GenerateRandomPointOnRoad(road, gen));
        dog.SetDirection(DogDirection::North);
        dog.SetVelocity({0.0, 0.0});
        return dog;
    }

    auto spawn_point = map.GetRoads()[0].GetStart();
    dog.SetPosition({static_cast<double>(spawn_point.x), static_cast<double>(spawn_point.y)});
    return dog;
}

const Map& GetMapOrThrow(const std::weak_ptr<GameSession>& session) {
    auto locked = session.lock();
    if (!locked) {
        throw std::runtime_error("Expired GameSession in Player ctor");
    }
    return locked->GetMap();
}
}  // namespace

Player::Player(Id id, std::string name, std::weak_ptr<GameSession> session,
               bool randomize_dog_spawn)
    : id_(id),
      session_(std::move(session)),
      dog_(SpawnDog(GetMapOrThrow(session_), std::move(name), gen_, randomize_dog_spawn)) {
    std::random_device rd;
    gen_ = std::mt19937(rd());
}

Player::Id Player::GetId() const noexcept { return id_; }

const std::string& Player::GetName() const noexcept { return dog_.GetName(); }

const Dog& Player::GetDog() const noexcept { return dog_; }

Dog& Player::GetDog() noexcept { return dog_; }

std::shared_ptr<GameSession> Player::LockSession() const noexcept { return session_.lock(); }
}  // namespace model