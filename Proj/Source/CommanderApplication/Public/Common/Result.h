#pragma once

#include <cassert>
#include <utility>
#include <variant>

namespace commander::application
{

// Success value or error. Failures are returned, never thrown (project rule 6).
// Value() / Error() require the matching state; check IsOk() first.
template <typename T, typename E>
class Result
{
public:
	static Result Ok(T Value) { return Result(std::in_place_index<0>, std::move(Value)); }
	static Result Err(E Error) { return Result(std::in_place_index<1>, std::move(Error)); }

	bool IsOk() const { return Storage.index() == 0; }
	explicit operator bool() const { return IsOk(); }

	const T& Value() const
	{
		assert(IsOk());
		return *std::get_if<0>(&Storage);
	}

	const E& Error() const
	{
		assert(!IsOk());
		return *std::get_if<1>(&Storage);
	}

private:
	template <typename... Args>
	explicit Result(Args&&... InitArgs) : Storage(std::forward<Args>(InitArgs)...)
	{
	}

	std::variant<T, E> Storage;
};

} // namespace commander::application
