#include "Battle/GridCoord.h"
#include "TestSupport.h"

using commander::domain::GridCoord;
using commander::domain::ManhattanDistance;

static void DefaultIsOrigin()
{
	constexpr GridCoord C;
	static_assert(C.X == 0 && C.Y == 0);
	CHECK(C == GridCoord{0, 0});
}

static void EqualityComparesBothAxes()
{
	CHECK(GridCoord{1, 2} == GridCoord{1, 2});
	CHECK(!(GridCoord{1, 2} == GridCoord{2, 1}));
	CHECK(!(GridCoord{1, 2} == GridCoord{1, 3}));
}

static void ManhattanDistanceIsSymmetricAndOrthogonal()
{
	CHECK(ManhattanDistance({0, 0}, {0, 0}) == 0);
	CHECK(ManhattanDistance({0, 0}, {3, 4}) == 7);
	CHECK(ManhattanDistance({3, 4}, {0, 0}) == 7);
	CHECK(ManhattanDistance({-2, 5}, {1, 1}) == 7);
	static_assert(ManhattanDistance({1, 1}, {2, 2}) == 2);
}

int main()
{
	DefaultIsOrigin();
	EqualityComparesBothAxes();
	ManhattanDistanceIsSymmetricAndOrthogonal();
	return TEST_MAIN_RESULT();
}
