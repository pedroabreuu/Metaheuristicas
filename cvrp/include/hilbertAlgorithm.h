#pragma once

#include "VRPInstance.h"
#include "Solution.h"
#include "hilbert.h"

Solution constroiSolucaoHilbert(const VRPInstance& instance, int p);