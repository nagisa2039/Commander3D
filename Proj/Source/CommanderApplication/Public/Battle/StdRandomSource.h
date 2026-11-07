#pragma once

#include <cstdint>
#include <random>

#include "Battle/RandomSource.h"

namespace commander::application
{

// Production IRandomSource backed by the standard library. Seedable so a battle can be replayed.
class StdRandomSource : public domain::IRandomSource
{
public:
	explicit StdRandomSource(std::uint32_t Seed) : Engine(Seed) {}

	std::int32_t NextPercent() override { return Distribution(Engine); }

private:
	std::mt19937 Engine;
	std::uniform_int_distribution<std::int32_t> Distribution{0, 99};
};

} // namespace commander::application
