/**
 * @file SMTObject.cpp
 * @brief Implementation of the base SMTObject class
 *
 * This file implements the SMTObject class, which serves as the base class for
 * all SMT-related objects in the system. It provides:
 * - Common functionality for all SMT objects
 * - Factory tracking to ensure proper object lifetime management
 * - Virtual destructor for proper cleanup of derived classes
 *
 * All SMT-related classes (SMTExpr, SMTSolver, SMTModel, etc.) inherit from
 * this base class to ensure consistent behavior and proper resource management.
 */

#include "Solvers/SMT/SMTObject.h"

SMTObject::~SMTObject() {}
