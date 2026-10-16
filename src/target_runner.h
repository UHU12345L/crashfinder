// Include guard: evita que el compilador lea este archivo dos veces
#ifndef TARGET_RUNNER_H
#define TARGET_RUNNER_H

#include <string>

bool run_target(const std::string &target_path, const std::string &input);

#endif