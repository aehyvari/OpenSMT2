//
// Created by prova on 20.04.22.
//

#ifndef OPENSMT_SUBSTITUTIONSIMPLIFIER_H
#define OPENSMT_SUBSTITUTIONSIMPLIFIER_H

#include "MapWithKeys.h"
#include "Logic.h"

#include <unordered_map>
#include <unordered_set>


class EqClass {
public:
    class Node {
    public:
        PTRef canonicalTerm = PTRef_Undef;
        PTRef tr = PTRef_Undef;
        unsigned size = 0;
        Node * next = nullptr;
        Node(PTRef tr) : canonicalTerm(tr), tr(tr), size(1), next(this) {}
    };
    Node root;
    unsigned size;
};

class SubstitutionSimplifier {
    using UnionForest = std::unordered_map<PTRef,std::unique_ptr<EqClass::Node>,PTRefHash>;
    using Facts = MapWithKeys<PTRef,bool,PTRefHash>;
    Logic & logic;
    bool updateSubstitutionClosure(UnionForest & termToNode, std::unordered_set<PTRef, PTRefHash> & toBeSubstituted, MapWithKeys<PTRef,PTRef,PTRefHash> const & rawSubstitutions);

    PtAsgn purify(PTRef r) const { PTRef p = r; lbool sgn = l_True; while (logic.isNot(p)) { sgn = sgn^true; p = logic.getPterm(p)[0]; } return PtAsgn{p, sgn}; };

public:
    SubstitutionSimplifier(Logic & logic) : logic(logic) {}
    struct SubstitutionResult {
        vec<PTRef> usedSubstitution;
        PTRef result;
    };
    SubstitutionResult computeSubstitutions(PTRef fla);
    virtual opensmt::pair<lbool,Logic::SubstMap> retrieveSubstitutions(const vec<opensmt::pair<PTRef,bool>>& units);
    Facts getNewFacts(PTRef root);
    void substitutionsTransitiveClosure(Logic::SubstMap & substs);
};


#endif //OPENSMT_SUBSTITUTIONSIMPLIFIER_H
