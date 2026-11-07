#include <vector>

#include "Battle/StdRandomSource.h"
#include "TestSupport.h"

using commander::application::StdRandomSource;

static void RollsStayInZeroToNinetyNine()
{
	StdRandomSource Rng(12345);
	bool SawLow = false;
	bool SawHigh = false;
	for (int I = 0; I < 10000; ++I)
	{
		const std::int32_t V = Rng.NextPercent();
		CHECK(V >= 0 && V <= 99);
		SawLow = SawLow || V < 10;
		SawHigh = SawHigh || V >= 90;
	}
	CHECK(SawLow && SawHigh); // not stuck in a narrow band
}

static void SameSeedGivesSameSequence()
{
	StdRandomSource A(7);
	StdRandomSource B(7);
	std::vector<std::int32_t> SeqA;
	std::vector<std::int32_t> SeqB;
	for (int I = 0; I < 50; ++I)
	{
		SeqA.push_back(A.NextPercent());
		SeqB.push_back(B.NextPercent());
	}
	CHECK(SeqA == SeqB);
}

static void DifferentSeedsDiffer()
{
	StdRandomSource A(1);
	StdRandomSource B(2);
	bool Differ = false;
	for (int I = 0; I < 50 && !Differ; ++I)
	{
		Differ = A.NextPercent() != B.NextPercent();
	}
	CHECK(Differ);
}

int main()
{
	RollsStayInZeroToNinetyNine();
	SameSeedGivesSameSequence();
	DifferentSeedsDiffer();
	return TEST_MAIN_RESULT();
}
