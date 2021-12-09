/*
 * Copyright (c) 2021, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "FSBVTheory.h"
#include "OsmtInternalException.h"
#include "FSBVBitBlaster.h"
#include "TreeOps.h"

static SolverDescr descr_bb_solver("BitBlaster", "BitBlaster for counting models?");

bool FSBVTheory::simplify(vec<PFRef> const & formulas, PartitionManager & pmanager, int curr) {
    if (keepPartitions()) {
        throw OsmtInternalException("Mode not supported for QF_BV yet");
    } else {

        PTRef coll_f = getCollateFunction(formulas, curr);
        PTRef trans = getLogic().learnEqTransitivity(coll_f);
        coll_f = getLogic().mkAnd(coll_f, trans);
        auto subs_res = computeSubstitutions(coll_f);
        PTRef fla = flaFromSubstitutionResult(subs_res);

        vec<PTRef> bvFormulas;
        FSBVBitBlaster bitBlaster(logic);
        topLevelConjuncts(logic, fla, bvFormulas);
        for (PTRef tr : bvFormulas) {
            if (logic.isBoolAtom(tr)) continue;
            std::cout << logic.pp(tr) << std::endl;
            PTRef out = bitBlaster.bbPredicate(tr);
            std::cout << logic.pp(out) << std::endl;
        }
        return false;
    }
}