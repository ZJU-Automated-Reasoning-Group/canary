/**
 * @file SMTConfigure.cpp
 * @brief Global configuration settings for the SMT solving subsystem
 *
 * This file implements the SMTConfigure class that manages global settings 
 * for the SMT solver components. Currently it handles:
 * - Global timeout settings for solver operations
 * 
 * The configuration module allows for consistent solver behavior across
 * different components of the system.
 */

#include "Solvers/SMT/SMTConfigure.h"
//#include "z3++.h"

int SMTConfigure::Timeout;

static int SolverTimetout = 0;

void SMTConfigure::init(int T) {
  if (T != -1 && SolverTimetout == 0) {
    Timeout = T;
  }
} 