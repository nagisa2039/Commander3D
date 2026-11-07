#include "Battle/Grid.h"
#include "TestSupport.h"

using commander::domain::Grid;
using commander::domain::GridCoord;

static void CreateRejectsNonPositiveSize()
{
	CHECK(!Grid::Create(0, 5).has_value());
	CHECK(!Grid::Create(5, 0).has_value());
	CHECK(!Grid::Create(-1, 5).has_value());
	CHECK(!Grid::Create(5, -1).has_value());
}

static void CreateKeepsSize()
{
	const auto G = Grid::Create(4, 3);
	CHECK(G.has_value());
	CHECK(G->Width() == 4);
	CHECK(G->Height() == 3);
	CHECK(G->CellCount() == 12);
}

static void ContainsIsInclusiveOfOriginAndExclusiveOfSize()
{
	const auto G = Grid::Create(4, 3);
	CHECK(G->Contains({0, 0}));
	CHECK(G->Contains({3, 2}));
	CHECK(!G->Contains({4, 2}));
	CHECK(!G->Contains({3, 3}));
	CHECK(!G->Contains({-1, 0}));
	CHECK(!G->Contains({0, -1}));
}

static void IndexOfIsRowMajor()
{
	const auto G = Grid::Create(4, 3);
	CHECK(G->IndexOf({0, 0}) == 0);
	CHECK(G->IndexOf({3, 0}) == 3);
	CHECK(G->IndexOf({0, 1}) == 4);
	CHECK(G->IndexOf({3, 2}) == 11);
}

int main()
{
	CreateRejectsNonPositiveSize();
	CreateKeepsSize();
	ContainsIsInclusiveOfOriginAndExclusiveOfSize();
	IndexOfIsRowMajor();
	return TEST_MAIN_RESULT();
}
