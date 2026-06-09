#pragma once

#include "core/VRPInstance.h"
#include "core/Solution.h"

Solution orOptIntra(Solution solucao, const VRPInstance& instance, int maxSegmento = 3);
Solution orOptIntra2(Solution solucao, const VRPInstance& instance);
Solution orOptIntra3(Solution solucao, const VRPInstance& instance);

Solution orOptInter(Solution solucao, const VRPInstance& instance, int maxSegmento = 3);
Solution orOptInter2(Solution solucao, const VRPInstance& instance);
Solution orOptInter3(Solution solucao, const VRPInstance& instance);

Solution randomOrOptInter(Solution solucao, const VRPInstance& instance, int maxSegmento = 3);
Solution randomOrOptInter3(Solution solucao, const VRPInstance& instance);
