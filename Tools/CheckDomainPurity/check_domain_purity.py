"""Detects Unreal Engine dependencies in the UE-free modules (CommanderDomain, CommanderApplication).

Usage: python Tools/CheckDomainPurity/check_domain_purity.py [--source Proj/Source]
Exit code: 0 = clean, 1 = violations found.

Heuristic text scan, not a compiler. It is a guard rail; the CMake build in Tests/Core
(which has no Unreal include paths) is the authoritative check.
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path

CHECKED_MODULES = ("CommanderDomain", "CommanderApplication")

# UE module registration needs UE headers; these are the only files allowed to use them.
EXEMPT_FILES = frozenset({"CommanderDomainModule.cpp", "CommanderApplicationModule.cpp"})

SOURCE_SUFFIXES = (".h", ".hpp", ".cpp", ".inl")

FORBIDDEN_TOKENS = (
    "TArray", "TMap", "TSet", "TSharedPtr", "TSharedRef", "TUniquePtr", "TOptional", "TFunction",
    "TVariant", "TPair", "TEXT", "UE_LOG", "UCLASS", "USTRUCT", "UENUM", "UPROPERTY", "UFUNCTION",
    "GENERATED_BODY", "GENERATED_USTRUCT_BODY", "check", "checkf", "ensure", "verify",
    "int8", "int16", "int32", "int64", "uint8", "uint16", "uint32", "uint64",
)
_TOKEN_RE = re.compile(r"\b(?:" + "|".join(FORBIDDEN_TOKENS) + r")\b")
# UE naming: U/A/F/T prefix + capital + lowercase (UObject, AActor, FString, TArray). Not E/I: those
# prefixes are common in plain C++ (IRandomSource), and enums are covered by the include check.
_UE_TYPE_RE = re.compile(r"\b[UAFT][A-Z][a-z]\w*")

_INCLUDE_RE = re.compile(r'^\s*#\s*include\s*(?:<([^>]+)>|"([^"]+)")')
# Standard headers have no extension; C compatibility headers are <name.h> from this set.
_C_COMPAT_HEADERS = frozenset({
    "assert.h", "ctype.h", "errno.h", "float.h", "inttypes.h", "limits.h", "locale.h", "math.h",
    "setjmp.h", "signal.h", "stdarg.h", "stdbool.h", "stddef.h", "stdint.h", "stdio.h", "stdlib.h",
    "string.h", "time.h", "wchar.h",
})


@dataclass(frozen=True)
class Violation:
    path: Path
    line: int
    message: str

    def __str__(self) -> str:
        return f"{self.path}:{self.line}: {self.message}"


def _strip_comments_and_strings(text: str) -> str:
    """Blanks comments and string/char literals, keeping newlines so line numbers stay valid."""
    out: list[str] = []
    i, n = 0, len(text)
    while i < n:
        two = text[i:i + 2]
        if two == "//":
            while i < n and text[i] != "\n":
                i += 1
        elif two == "/*":
            end = text.find("*/", i + 2)
            end = n if end == -1 else end + 2
            out.append("".join("\n" if c == "\n" else " " for c in text[i:end]))
            i = end
        elif text[i] in "\"'":
            quote = text[i]
            out.append(quote)
            i += 1
            while i < n and text[i] != quote and text[i] != "\n":
                i += 2 if text[i] == "\\" else 1
            out.append(quote)
            i += 1
        else:
            out.append(text[i])
            i += 1
    return "".join(out)


def _include_is_allowed(file: Path, angle: str | None, quoted: str | None, roots: list[Path]) -> bool:
    if angle is not None:
        return "." not in angle or angle in _C_COMPAT_HEADERS
    return any((root / quoted).is_file() for root in [file.parent, *roots])


def _include_roots(source: Path) -> list[Path]:
    roots: list[Path] = []
    for module in CHECKED_MODULES:
        roots += [source / module / "Public", source / module / "Private"]
    return roots


def check_file(file: Path, roots: list[Path]) -> list[Violation]:
    text = file.read_text(encoding="utf-8", errors="replace")
    # Include targets look like string literals, so they are read from the comment-blanked text only.
    include_view = _blank_comments_only(text).splitlines()
    code_view = _strip_comments_and_strings(text).splitlines()
    violations: list[Violation] = []
    for number, (raw, code) in enumerate(zip(include_view, code_view), 1):
        match = _INCLUDE_RE.match(raw)
        if match:
            angle, quoted = match.groups()
            if not _include_is_allowed(file, angle, quoted, roots):
                violations.append(Violation(file, number, f"include not allowed in UE-free module: {angle or quoted}"))
            continue
        for found in _TOKEN_RE.finditer(code):
            violations.append(Violation(file, number, f"forbidden UE token: {found.group()}"))
        for found in _UE_TYPE_RE.finditer(code):
            if not _TOKEN_RE.fullmatch(found.group()):  # already reported above
                violations.append(Violation(file, number, f"UE-style type name: {found.group()}"))
    return violations


def _blank_comments_only(text: str) -> str:
    """Blanks comments but leaves string literals alone (needed to read #include "..." targets)."""
    out: list[str] = []
    i, n = 0, len(text)
    while i < n:
        two = text[i:i + 2]
        if two == "//":
            while i < n and text[i] != "\n":
                i += 1
        elif two == "/*":
            end = text.find("*/", i + 2)
            end = n if end == -1 else end + 2
            out.append("".join("\n" if c == "\n" else " " for c in text[i:end]))
            i = end
        else:
            out.append(text[i])
            i += 1
    return "".join(out)


def check_source_root(source: Path) -> list[Violation]:
    roots = _include_roots(source)
    violations: list[Violation] = []
    for module in CHECKED_MODULES:
        module_dir = source / module
        if not module_dir.is_dir():
            continue
        for file in sorted(module_dir.rglob("*")):
            if file.suffix in SOURCE_SUFFIXES and file.name not in EXEMPT_FILES:
                violations += check_file(file, roots)
    return violations


def main(argv: list[str] | None = None) -> int:
    default_source = Path(__file__).resolve().parents[2] / "Proj" / "Source"
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--source", type=Path, default=default_source, help="UE Source directory")
    args = parser.parse_args(argv)

    violations = check_source_root(args.source)
    for violation in violations:
        print(violation)
    if violations:
        print(f"CheckDomainPurity: {len(violations)} violation(s)", file=sys.stderr)
        return 1
    print("CheckDomainPurity: OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
