"""Tests for check_domain_purity.py. Run: python -m unittest discover -s Tools/CheckDomainPurity"""

import tempfile
import unittest
from pathlib import Path

import check_domain_purity as cdp


class PurityTestCase(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self._tmp.cleanup)
        self.source = Path(self._tmp.name)

    def write(self, relative, text):
        path = self.source / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text, encoding="utf-8")
        return path

    def check(self):
        return cdp.check_source_root(self.source)

    def messages(self):
        return [v.message for v in self.check()]


class CleanCodeTest(PurityTestCase):
    def test_standard_only_code_passes(self):
        self.write(
            "CommanderDomain/Public/Battle/GridCoord.h",
            "#pragma once\n#include <cstdint>\n#include <vector>\n"
            "namespace commander::domain { struct GridCoord { std::int32_t X = 0; }; }\n",
        )
        self.assertEqual(self.check(), [])

    def test_missing_modules_are_ignored(self):
        self.assertEqual(self.check(), [])

    def test_module_registration_file_is_exempt(self):
        self.write(
            "CommanderDomain/Private/CommanderDomainModule.cpp",
            '#include "Modules/ModuleManager.h"\nIMPLEMENT_MODULE(FDefaultModuleImpl, CommanderDomain)\n',
        )
        self.assertEqual(self.check(), [])

    def test_own_headers_may_be_included(self):
        self.write("CommanderDomain/Public/Battle/GridCoord.h", "#pragma once\n")
        self.write(
            "CommanderDomain/Private/Battle/Move.cpp",
            '#include "Battle/GridCoord.h"\n#include "Helper.h"\n',
        )
        self.write("CommanderDomain/Private/Battle/Helper.h", "#pragma once\n")
        self.assertEqual(self.check(), [])

    def test_application_may_include_domain_public(self):
        self.write("CommanderDomain/Public/Battle/GridCoord.h", "#pragma once\n")
        self.write("CommanderApplication/Private/Session.cpp", '#include "Battle/GridCoord.h"\n')
        self.assertEqual(self.check(), [])


class IncludeTest(PurityTestCase):
    def test_core_minimal_is_rejected(self):
        self.write("CommanderDomain/Public/A.h", '#include "CoreMinimal.h"\n')
        violations = self.check()
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0].line, 1)

    def test_generated_header_is_rejected(self):
        self.write("CommanderDomain/Public/A.h", '#include "A.generated.h"\n')
        self.assertEqual(len(self.check()), 1)

    def test_unknown_angle_include_is_rejected(self):
        self.write("CommanderDomain/Public/A.h", "#include <windows.h>\n")
        self.assertEqual(len(self.check()), 1)

    def test_c_compat_headers_are_allowed(self):
        self.write("CommanderDomain/Public/A.h", "#include <stdint.h>\n#include <cstddef>\n")
        self.assertEqual(self.check(), [])

    def test_commented_include_is_ignored(self):
        self.write("CommanderDomain/Public/A.h", '// #include "CoreMinimal.h"\n/* #include "Foo.h" */\n')
        self.assertEqual(self.check(), [])


class TokenTest(PurityTestCase):
    def test_forbidden_tokens_are_rejected(self):
        for token in ("TArray<int>", "FString", "FName", "UObject*", "AActor*", "TMap<int,int>", "int32", "UE_LOG(x)", "TEXT(\"a\")"):
            with self.subTest(token=token):
                self.write("CommanderDomain/Public/A.h", f"{token} X;\n")
                self.assertEqual(len(self.check()), 1, token)

    def test_ue_prefixed_type_pattern_is_rejected(self):
        self.write("CommanderDomain/Public/A.h", "struct FBattleThing {};\n")
        self.assertEqual(len(self.check()), 1)

    def test_normal_names_are_not_flagged(self):
        self.write(
            "CommanderDomain/Public/A.h",
            "struct Fighter {}; struct UnitId {}; struct AIPlan {}; struct IRandomSource {};\n"
            "enum class Faction { Player };\nstd::int32_t Attack = 0; std::uint8_t Tile = 0;\n",
        )
        self.assertEqual(self.check(), [])

    def test_comments_and_strings_are_ignored(self):
        self.write(
            "CommanderDomain/Public/A.h",
            '// FString in a comment\n/* TArray\n over lines */\nconst char* S = "FString TArray";\n',
        )
        self.assertEqual(self.check(), [])

    def test_line_numbers_survive_block_comments(self):
        self.write("CommanderDomain/Public/A.h", "/* a\nb\nc */\nFString X;\n")
        self.assertEqual(self.check()[0].line, 4)

    def test_cpp_files_are_scanned_too(self):
        self.write("CommanderApplication/Private/S.cpp", "TArray<int> X;\n")
        self.assertEqual(len(self.check()), 1)

    def test_other_modules_are_not_scanned(self):
        self.write("Commander3D/Public/A.h", '#include "CoreMinimal.h"\nFString X;\n')
        self.assertEqual(self.check(), [])


class CliTest(PurityTestCase):
    def test_exit_codes(self):
        self.assertEqual(cdp.main(["--source", str(self.source)]), 0)
        self.write("CommanderDomain/Public/A.h", "FString X;\n")
        self.assertEqual(cdp.main(["--source", str(self.source)]), 1)


if __name__ == "__main__":
    unittest.main()
