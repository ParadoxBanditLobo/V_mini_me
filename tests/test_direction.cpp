#include "Direction.hpp"

#include <cassert>
#include <iostream>

int main() {
    constexpr int w = 1000;
    constexpr int h = 1000;

    assert(determineDirection(500, 500, w, h, 0.1, DirectionMode::EightWay) == Direction::Center);
    assert(determineDirection(900, 500, w, h, 0.1, DirectionMode::EightWay) == Direction::Right);
    assert(determineDirection(100, 500, w, h, 0.1, DirectionMode::EightWay) == Direction::Left);
    assert(determineDirection(500, 100, w, h, 0.1, DirectionMode::EightWay) == Direction::Up);
    assert(determineDirection(500, 900, w, h, 0.1, DirectionMode::EightWay) == Direction::Down);
    assert(determineDirection(900, 100, w, h, 0.1, DirectionMode::Quadrants) == Direction::UpRight);
    assert(determineDirection(100, 900, w, h, 0.1, DirectionMode::Quadrants) == Direction::DownLeft);
    assert(determineDirection(900, 650, w, h, 0.1, DirectionMode::FourWay) == Direction::Right);
    assert(determineDirection(600, 100, w, h, 0.1, DirectionMode::FourWay) == Direction::Up);

    std::cout << "direction tests passed\n";
    return 0;
}
