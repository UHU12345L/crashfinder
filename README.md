# crashfinder

## 📄 Description
A custom-built fuzzer written in C++ for discovering memory-safety bugs
(buffer overflows, use-after-free, etc.) in C/C++ programs. Built from
scratch as an independent project — no fuzzing frameworks used — to
understand fuzzing internals: process execution and monitoring,
mutation strategies, crash detection, and coverage-guided input generation.

This is an ongoing, long-term personal project (not affiliated with any
university course), developed incrementally over several months.

## 🎯 Motivation
Fuzzers like AFL are among the most widely used tools in real-world
vulnerability research (Google, Mozilla, etc. run them continuously).
This project builds a simplified version from the ground up to
understand *why* they work: how process isolation, signal handling,
and mutation strategies combine to find bugs no human review would catch.

## 🧠 How It Works (current stage)
1. **Target execution**: `fork()` + `execve()` to run the target program
   as a child process, with input redirected via stdin.
2. **Crash detection**: `waitpid()` checks whether the child terminated
   normally or was killed by a signal (`SIGSEGV`, `SIGABRT`).
3. **Sanitized targets**: test programs are compiled with
   `-fsanitize=address` so memory bugs fail loudly and deterministically
   instead of corrupting memory silently.
4. **Input generation**: currently random byte generation (Phase 1);
   mutation-based generation from a seed corpus is next.

## 🗺️ Roadmap
- [x] Phase 1 — Random fuzzing: fork/exec target runner, crash detection, first vulnerable test targets
- [ ] Phase 2 — Mutation-based fuzzing (seed corpus, bit-flips, byte mutations)
- [ ] Phase 3 — Coverage-guided fuzzing (basic block tracking, prioritizing inputs that discover new code paths)
- [ ] Phase 4 — Crash triage (deduplication, input minimization)
- [ ] Phase 5 — Testing against real-world small utilities with known historical CVEs

## 📁 Structure
src/ → fuzzer source code
targets/ → intentionally vulnerable test programs
crashes/ → inputs that triggered a crash (auto-generated)
corpus/ → seed inputs used for mutation

## 💻 Technologies Used
- Language: C++
- Environment: Ubuntu (Linux), relies on POSIX syscalls (`fork`, `execve`, `waitpid`)
- Editor: VS Code
- Memory-bug detection: AddressSanitizer (ASan)

## 🚀 How to Run
*(will be filled in as the fuzzer takes shape — currently in early development)*

## 📚 Background
Builds on systems-programming concepts from coursework: process creation,
inter-process communication, and signal handling.