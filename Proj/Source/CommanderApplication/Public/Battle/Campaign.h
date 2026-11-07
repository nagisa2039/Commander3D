#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Battle/BattleSetup.h"
#include "Battle/Preparation.h"
#include "Roster/Roster.h"

namespace commander::application
{

// One chapter: its name, the battle (map, enemies, conditions, shops; no player units) and the cells the sortie
// members start on (their number is the deploy limit).
struct Chapter
{
	std::string Name;
	BattleSetup Setup;
	std::vector<domain::GridCoord> StartCells;
};

enum class CampaignStep
{
	BattleOngoing,   // the battle is not finished; nothing changed
	Retry,           // lost: same chapter again, roster as it was before the battle
	NextChapter,     // won: results carried over, the next chapter is current
	CampaignCleared, // won the last chapter
};

// The run through the chapters. Owns the roster and the current chapter's preparation; the Presentation plays the
// battle in between and reports it with Complete.
class Campaign
{
public:
	// StartIndex resumes at a later chapter (a loaded save); past the last chapter it is clamped to "finished".
	Campaign(domain::Roster InRoster, domain::Difficulty InDifficulty, std::vector<Chapter> InChapters, std::size_t StartIndex = 0)
		: RosterState(std::move(InRoster)), Mode(InDifficulty), Chapters(std::move(InChapters)), Index(std::min(StartIndex, Chapters.size()))
	{
	}

	bool IsFinished() const { return Index >= Chapters.size(); }
	std::size_t ChapterIndex() const { return Index; }
	domain::Difficulty Difficulty() const { return Mode; }

	// Requires !IsFinished().
	const Chapter& CurrentChapter() const
	{
		assert(!IsFinished());
		return Chapters[Index];
	}

	// The roster including item changes already made in the current preparation.
	const domain::Roster& Roster() const { return Prep ? Prep->Roster() : RosterState; }

	// The current chapter's preparation (created on first use). Requires !IsFinished().
	Preparation& Preparation()
	{
		assert(!IsFinished());
		if (!Prep)
		{
			Prep.emplace(RosterState, Chapters[Index].Setup, Chapters[Index].StartCells);
		}
		return *Prep;
	}

	// Reports the battle started from this chapter's preparation. Won: results go into the roster and the next
	// chapter becomes current. Lost: the same chapter is retried (item changes made in the preparation stay,
	// battle results are discarded). Requires !IsFinished().
	CampaignStep Complete(const domain::Battle& Finished, const std::vector<domain::UnitId>& Sortied)
	{
		assert(!IsFinished());
		switch (Finished.Outcome())
		{
		case domain::BattleOutcome::Ongoing:
			return CampaignStep::BattleOngoing;
		case domain::BattleOutcome::PlayerLost:
			return CampaignStep::Retry;
		case domain::BattleOutcome::PlayerWon:
			break;
		}
		Preparation().Finish(Finished, Sortied, Mode);
		RosterState = Prep->Roster();
		Prep.reset();
		++Index;
		return IsFinished() ? CampaignStep::CampaignCleared : CampaignStep::NextChapter;
	}

private:
	domain::Roster RosterState;
	domain::Difficulty Mode;
	std::vector<Chapter> Chapters;
	std::size_t Index;
	std::optional<application::Preparation> Prep;
};

} // namespace commander::application
