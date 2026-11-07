#pragma once

#include <cstdint>

namespace commander::domain
{

// Source of randomness for combat, injected so tests can fix the outcome.
class IRandomSource
{
public:
	virtual ~IRandomSource() = default;

	// Uniform integer in 0..99.
	virtual std::int32_t NextPercent() = 0;
};

} // namespace commander::domain
