#pragma once

// Minimal test helper (no external dependency). A failed CHECK is recorded and
// the process exits non-zero from TEST_MAIN_RESULT().

#include <cstdio>

namespace test_support
{
inline int FailureCount = 0;
}

#define CHECK(...)                                                                  \
	do                                                                             \
	{                                                                              \
		if (!(__VA_ARGS__))                                                            \
		{                                                                          \
			std::printf("%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #__VA_ARGS__);   \
			++test_support::FailureCount;                                          \
		}                                                                          \
	} while (false)

#define TEST_MAIN_RESULT() (test_support::FailureCount == 0 ? 0 : 1)
