#include "Theory.h"
#include "Substitutor.h"
//#include "MainSolver.h"
//#include "logics/Logic.h"


// The Collate function is constructed from all frames up to the current
// one and will be used to simplify the formulas in the current frame
// formulas[curr].
// (1) Construct the collate function as a conjunction of formulas up to curr.
// (2) From the coll function compute units_{curr}
// (3) Use units_1 /\ ... /\ units_{curr} to simplify formulas[curr]
//
// In addition we add the transitivie equalities as a further
// simplification (critical for the eq_diamond instances in QF_UF of
// smtlib).
//
PTRef Theory::getCollateFunction(const vec<PFRef> & formulas, int curr)
{
    assert(curr < formulas.size());
    // XXX
//    getLogic().dumpHeaderToFile(std::cout);
//    getLogic().dumpFormulaToFile(std::cout, pfstore[formulas[1]].formulas[0]);
    vec<PTRef> coll_f_args;
    // compute coll_f as (a_1^0 /\ ... /\ a_{n_1}^0) /\ ... /\ (a_1^{curr} /\ ... /\ a_{n_k}^{curr})
    for (int i = 0; i <= curr; i++)
    {
        for (int j = 0; j < pfstore[formulas[i]].size(); j++)
            coll_f_args.push(pfstore[formulas[i]][j]);
    }
    return getLogic().mkAnd(std::move(coll_f_args));
}


void
Theory::printFramesAsQuery(const vec<PFRef> & frames, std::ostream & s) const
{
    getLogic().dumpHeaderToFile(s);
    for (int i = 0; i < frames.size(); i++) {
        if (i > 0)
            s << "(push 1)\n";
        getLogic().dumpFormulaToFile(s, pfstore[frames[i]].root);
    }
    getLogic().dumpChecksatToFile(s);
}


PTRef Theory::applySubstitutionBasedSimplificationIfEnabled(PTRef root) {
    if (config.do_substitutions()) {
        auto result = computeSubstitutions(root);
        return getLogic().mkAnd({result.result, getLogic().mkAnd(result.usedSubstitution)});
    }
    return root;
}
