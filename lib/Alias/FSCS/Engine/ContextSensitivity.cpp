// Initialization only
#include "Alias/FSCS/Engine/ContextSensitivity.h"

namespace tpa
{
    // Initialize the default policy to use UniformKLimit
    ContextSensitivityPolicy::Policy ContextSensitivityPolicy::activePolicy = ContextSensitivityPolicy::Policy::UniformKLimit;
} 