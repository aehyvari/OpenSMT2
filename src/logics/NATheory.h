/*
 * Copyright (c) 2021, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2021, Martin Blicha <martin.blicha@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef OPENSMT_NATHEORY_H
#define OPENSMT_NATHEORY_H

#include "LATheory.h"
#include "ArithmeticEqualityRewriter.h"
#include "DistinctRewriter.h"
#include "DivModRewriter.h"
#include "NonlinearRewriter.h"

template<typename NonlinAlgLogic, typename LinAlgTHandler>
class NATheory : public LATheory<NonlinAlgLogic, LinAlgTHandler>
{
public:
    NATheory(SMTConfig & c, NonlinAlgLogic & logic)
            : LATheory<NonlinAlgLogic,LinAlgTHandler>(c, logic)
    { }
    ~NATheory() {};
    virtual bool simplify(const vec<PFRef>&, PartitionManager& pmanager, int) override; // Theory specific simplifications
};

namespace {

template<typename TLogic>
PTRef rewriteNonlinear(TLogic &, PTRef fla) { return fla; }

template<>
PTRef rewriteNonlinear<ArithLogic>(ArithLogic & logic, PTRef fla) {
    return NonlinearRewriter(logic).rewrite(fla);
}

}

//
// Unit propagate with simplifications and split equalities into
// inequalities.  If partitions cannot mix, only do the splitting to
// inequalities.
//
template<typename L, typename H>
bool NATheory<L,H>::simplify(const vec<PFRef>& formulas, PartitionManager& pmanager, int curr)
{
    auto & currentFrame = Theory::pfstore[formulas[curr]];
    ArithmeticEqualityRewriter equalityRewriter(LATheory<L,H>::lalogic);
    if (this->keepPartitions()) {
        vec<PTRef> & flas = currentFrame.formulas;
        for (PTRef & fla : flas) {
            PTRef old = fla;
            fla = rewriteDistincts(LATheory<L,H>::lalogic, fla);
            fla = rewriteDivMod<L>(LATheory<L,H>::lalogic, fla);
            fla = rewriteNonlinear<L>(LATheory<L,H>::lalogic, fla);
            fla = equalityRewriter.rewrite(fla);
            pmanager.transferPartitionMembership(old, fla);
        }
        currentFrame.root = LATheory<L,H>::lalogic.mkAnd(flas);
    } else {
        PTRef coll_f = Theory::getCollateFunction(formulas, curr);
        auto subs_res = Theory::computeSubstitutions(coll_f);
        PTRef finalFla = Theory::flaFromSubstitutionResult(subs_res);
        finalFla = rewriteDistincts(LATheory<L,H>::lalogic, finalFla);
        finalFla = rewriteDivMod<L>(LATheory<L,H>::lalogic, finalFla);
        finalFla = rewriteNonlinear<L>(LATheory<L,H>::lalogic, finalFla);
        currentFrame.root = equalityRewriter.rewrite(finalFla);
    }
    LATheory<L,H>::notOkToPartition = equalityRewriter.getAndClearNotOkToPartition();
    return true;
}

#endif //OPENSMT_NATHEORY_H
