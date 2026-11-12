# Contributing

## Branch and pull-request workflow

1. Start from an up-to-date `main`: `git switch main` then `git pull --ff-only`.
2. Create a focused branch: `git switch -c feat/short-description` (or `fix/` / `docs/`).
3. Make a small change, run the checks below and inspect `git diff --check` and `git diff`.
4. Commit with a descriptive message, push the branch and open a pull request against `main`.
5. Explain the reason, validation results and hardware limitations. Resolve failing checks before merging.

Do not commit credentials, personal recordings, local environments or generated builds. Preserve third-party notices and research provenance. Report a reproducible issue with environment/tool versions before proposing a fix.

## Local checks

Run from the repository root with Python 3.12. Install the listed tools in a virtual environment; system tools must be on PATH.

```sh
python tools/check_repository.py
python -m pip install clang-format==18.1.8
python tools/format_sources.py --check
cppcheck --enable=warning,performance,portability --error-exitcode=1 --inline-suppr --suppress=missingIncludeSystem --suppress=missingInclude --suppress=unmatchedSuppression --language=c++ --std=c++11 -DARDUINO Audio_Test.ino Speaker_Test.ino
python -m pip install platformio==6.1.18
pio run
```

## What CI establishes

CI checks local Markdown links, source syntax and unresolved merge markers. External links are not polled on every push. clang-format 18 covers application files; vendored STM32 drivers are excluded. cppcheck analyzes application sources without claiming to replace the target compiler; SDK include-resolution messages are suppressed. PlatformIO builds each Arduino sketch separately for ESP32-S3 with pinned platform and library versions. This is a compile check; audio, PSRAM and network behavior still require the physical board.

These workflows run on pushes and pull requests with read-only repository permissions and bounded execution time. A green check applies to the checks listed here; it does not establish physical hardware validation.
