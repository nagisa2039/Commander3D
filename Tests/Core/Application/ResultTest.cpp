#include "Common/Result.h"
#include "TestSupport.h"

#include <memory>
#include <string>

using commander::application::Result;

static void OkHoldsValue()
{
	const auto R = Result<int, std::string>::Ok(42);
	CHECK(R.IsOk());
	CHECK(static_cast<bool>(R));
	CHECK(R.Value() == 42);
}

static void ErrHoldsError()
{
	const auto R = Result<int, std::string>::Err("blocked");
	CHECK(!R.IsOk());
	CHECK(!static_cast<bool>(R));
	CHECK(R.Error() == "blocked");
}

static void SameValueAndErrorTypeAreDistinguished()
{
	const auto Ok = Result<int, int>::Ok(1);
	const auto Err = Result<int, int>::Err(1);
	CHECK(Ok.IsOk());
	CHECK(!Err.IsOk());
}

static void MoveOnlyValueIsSupported()
{
	auto R = Result<std::unique_ptr<int>, int>::Ok(std::make_unique<int>(7));
	CHECK(R.IsOk());
	CHECK(*R.Value() == 7);
}

int main()
{
	OkHoldsValue();
	ErrHoldsError();
	SameValueAndErrorTypeAreDistinguished();
	MoveOnlyValueIsSupported();
	return TEST_MAIN_RESULT();
}
