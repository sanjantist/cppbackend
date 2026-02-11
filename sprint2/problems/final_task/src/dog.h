#pragma once

#include <string>

#include "model.h"

namespace model {
class Dog {
   public:
    explicit Dog(std::string name);

    void SetPosition(DogPos position);
    void SetDirection(DogDirection direction);
    void SetVelocity(DogVelocity velocity);

    const std::string& GetName() const noexcept;
    const DogDirection GetDirection() const noexcept;
    const DogPos GetPosition() const noexcept;
    const DogVelocity GetVelocity() const noexcept;

   private:
    std::string name_;
    DogDirection direction_ = DogDirection::North;
    DogVelocity velocity_ = {0.0, 0.0};
    DogPos position_;
};
}  // namespace model