# crashfinder

## 📄 Description
A custom-built fuzzer written in C++ for discovering memory-safety bugs
(buffer overflows, etc.) in C/C++ programs. Built from scratch as an
independent project — no fuzzing frameworks used — to understand fuzzing
internals: process execution and monitoring, crash detection and input
handling.

It runs a target program thousands of times with random inputs and saves
every input that makes it crash, so the crash can be replayed later.

This is a personal project, not affiliated with any university course.

## 🎯 Motivation
Fuzzers like AFL are among the most widely used tools in real-world
vulnerability research (Google, Mozilla, etc. run them continuously).
This project builds a simplified version from the ground up to
understand *why* they work: how process isolation, signal handling and
crash detection combine to find bugs no human review would catch.

## ✅ Features
- Random input generation (random length, random bytes)
- Target execution with `fork()` + `execve()`, feeding the input through a pipe to the target's stdin
- Crash detection, both by signal (`SIGSEGV`, ...) and by AddressSanitizer exit code
- Crash storage in `crashes/`, named by a FNV-1a hash of the input, so identical inputs are saved only once
- `--replay` mode to re-run a saved crash against the target
- Run statistics: executions, crashes, unique crashes, time and executions per second

## 🧠 How It Works
1. **Input generation**: `main.cpp` generates a random input.
2. **Target execution**: `run_target()` creates a pipe and forks. The child
   redirects its stdin to the pipe, silences stdout/stderr and replaces itself
   with the target through `execve()`. The parent writes the input to the pipe
   and waits with `waitpid()`.
3. **Sanitized targets**: test programs are compiled with `-fsanitize=address`
   so memory bugs fail loudly instead of corrupting memory silently.
4. **Crash detection**: an input counts as a crash if the child died from a
   signal, or exited with code 86, the code AddressSanitizer is configured to
   use when it detects an error.
5. **Storage**: crashing inputs are saved to `crashes/`, named by their hash.

## 🚀 How to Run
Requires Linux (it uses POSIX calls) and `g++` with C++17.

```bash
# Build the fuzzer and the vulnerable test target
g++ -std=c++17 -g -o crashfinder src/main.cpp src/target_runner.cpp
g++ -fsanitize=address -g -o targets/bug1 targets/bug1.cpp

# Fuzz a target N times
./crashfinder <target> <executions>

# Re-run a saved crash
./crashfinder --replay <target> <crash_file>
```

Example:

```
$ ./crashfinder targets/bug1 1000
Executions:     1000
Crashes:        823
Unique crashes: 823
Normal exits:   177
Time (s):       9.77558
Exec/sec:       102.296

$ ./crashfinder --replay targets/bug1 crashes/crash_000c85525078703e.bin
Replay de crashes/crash_000c85525078703e.bin (162 bytes)
Resultado: CRASH reproducido
```

## ⚡ Performance Note
The first version ran at about 3.6 executions/second, far too slow for
fuzzing. Measuring each part separately showed that the time was not spent
in the fuzzer itself but waiting for Ubuntu's crash handler (`apport`),
triggered every time ASan aborted the process with `SIGABRT`. Configuring
ASan to exit with a fixed code (`abort_on_error=0:exitcode=86`) instead of
aborting avoids the handler, and the fuzzer went from about 3.6 to about
100 executions/second. Disabling symbolization and leak detection also helps.

## 📁 Structure
```
src/       fuzzer source (main.cpp, target_runner.cpp/.h)
targets/   intentionally vulnerable test programs
crashes/   inputs that triggered a crash (auto-generated)
corpus/    reserved for seed inputs (future work)
```

## ⚠️ Limitations
- Random-only generation: no mutation of valid inputs and no coverage feedback, so it only finds shallow bugs
- No timeout detection: a target that hangs will block the fuzzer
- Input is delivered through stdin only
- Only identical inputs are deduplicated; different inputs that trigger the same bug are saved separately
- A target that exits with code 86 by itself would be reported as a crash

## 🗺️ Roadmap
- [x] Random fuzzing loop with statistics
- [x] Crash storage with hash-based deduplication
- [x] Crash replay
- [ ] Timeout detection
- [ ] Mutation-based fuzzing from a seed corpus
- [ ] Coverage-guided fuzzing
- [ ] Crash metadata (signal, size, timestamp)
- [ ] More vulnerable test targets (use-after-free, double free)

## 💻 Technologies Used
- Language: C++17
- Environment: Ubuntu (Linux), relies on POSIX syscalls (`fork`, `execve`, `pipe`, `dup2`, `waitpid`)
- Editor: VS Code
- Memory-bug detection: AddressSanitizer (ASan)

## 📚 Background
Builds on systems-programming concepts from coursework: process creation,
inter-process communication, and signal handling.
