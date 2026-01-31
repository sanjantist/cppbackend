#include "dog.h"

#include "model.h"

namespace model {

Dog::Dog(std::string name)
    : name_(name), direction_(DogDirection::North), velocity_(), position_() {}

void Dog::SetPosition(DogPos position) { position_ = position; }

void Dog::SetDirection(DogDirection direction) { direction_ = direction; }

void Dog::SetVelocity(DogVelocity velocity) { velocity_ = velocity; }

const std::string& Dog::GetName() const noexcept { return name_; }

const DogDirection Dog::GetDirection() const noexcept { return direction_; }

const DogPos Dog::GetPosition() const noexcept { return position_; }

const DogVelocity Dog::GetVelocity() const noexcept { return velocity_; }

}  // namespace model