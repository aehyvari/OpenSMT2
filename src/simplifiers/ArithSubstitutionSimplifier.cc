//
// Created by prova on 20.04.22.
//

#include "ArithSubstitutionSimplifier.h"
#include "LA.h"

opensmt::pair<lbool,Logic::SubstMap> ArithSubstitutionSimplifier::retrieveSubstitutions(const vec<PtAsgn>& facts) {
    auto resAndSubsts = SubstitutionSimplifier::retrieveSubstitutions(facts);
    if (resAndSubsts.first != l_Undef) return resAndSubsts;
    vec<PTRef> top_level_arith;
    for (PtAsgn fact : facts) {
        PTRef tr = fact.tr;
        lbool sgn = fact.sgn;
        if (logic.isNumEq(tr) && sgn == l_True)
            top_level_arith.push(tr);
    }
    lbool res = arithmeticElimination(top_level_arith, resAndSubsts.second);
    return {res, std::move(resAndSubsts.second)};
}

lbool ArithSubstitutionSimplifier::arithmeticElimination(const vec<PTRef> & top_level_arith, Logic::SubstMap & substitutions)
{
    std::vector<std::unique_ptr<LAExpression>> equalities;
    // I don't know if reversing the order makes any sense but osmt1
    // does that.
    for (int i = top_level_arith.size() - 1; i >= 0; i--) {
        equalities.emplace_back(new LAExpression(logic, top_level_arith[i]));
    }

    for (auto const& equality : equalities) {
        PTRef res = equality->solve();
        if (res == PTRef_Undef) { // MB: special case where the equality simplifies to "c = 0" with c constant
            // This is a conflicting equality unless the constant "c" is also 0.
            // We do nothing here and let the main code deal with this
            continue;
        }
        auto sub = equality->getSubst();
        PTRef var = sub.first;
        assert(var != PTRef_Undef and logic.isNumVarLike(var) and sub.second != PTRef_Undef);
        if (substitutions.has(var)) {
            if (logic.isConstant(sub.second)) {
                substitutions[var] = sub.second;
            }
            // Already has substitution for this var, let the main substitution code deal with this situation
            continue;
        } else {
            substitutions.insert(var, sub.second);
        }
    }
    // To simplify this method, we do not try to detect a conflict here, so result is always l_Undef
    return l_Undef;
}

