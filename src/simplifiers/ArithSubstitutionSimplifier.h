//
// Created by prova on 20.04.22.
//

#ifndef OPENSMT_ARITHSUBSTITUTIONSIMPLIFIER_H
#define OPENSMT_ARITHSUBSTITUTIONSIMPLIFIER_H

#include "SubstitutionSimplifier.h"
#include "ArithLogic.h"

class ArithSubstitutionSimplifier : public SubstitutionSimplifier {
    ArithLogic & logic;
    lbool arithmeticElimination(vec<PTRef> const & top_level_arith, Logic::SubstMap & substitutions);
protected:
    opensmt::pair<lbool,Logic::SubstMap> retrieveSubstitutions(vec<PtAsgn> const & facts) override;
public:
    ArithSubstitutionSimplifier(ArithLogic & logic) : SubstitutionSimplifier(logic), logic(logic) {}
};

#endif //OPENSMT_ARITHSUBSTITUTIONSIMPLIFIER_H
