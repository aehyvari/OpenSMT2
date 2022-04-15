//
// Created by prova on 20.04.22.
//

#include "SubstitutionSimplifier.h"
#include "Rewriter.h"
#include "Substitutor.h"
#include "SubstLoopBreaker.h"

SubstitutionSimplifier::SubstitutionResult SubstitutionSimplifier::computeSubstitutions(const PTRef fla) {
    class ForestSubstitutionConfig : public DefaultRewriterConfig {
        UnionForest & termToNode;
    public:
        ForestSubstitutionConfig(UnionForest & termToNode) : termToNode(termToNode) {}
        PTRef rewrite(PTRef tr) override {
            auto n = termToNode.find(tr);
            if (n != termToNode.end()) {
                return n->second->tr;
            }
            return tr;
        }
    };
    SubstitutionResult result;

    PTRef root = fla;
    // l_True : exists and is valid
    // l_False : exists but has been disabled to break symmetries
    Logic::SubstMap allsubsts;

    UnionForest termToNode;

    // This computes the new unit clauses to curr_frame.units until closure
    for (int i = 0; i < 3; i++) {
        // update the current simplification formula
        PTRef simp_formula = root;
        vec<PtAsgn> current_units_vec;
        // Get U_i
        auto [rval, new_units] = getNewFacts(simp_formula);
        if (not rval) {
            return SubstitutionResult{{}, logic.getTerm_false()};
        }
        // Add the newly obtained units to the list of all substitutions
        // Clear the previous units
        const auto & new_units_vec = new_units.getKeys();
        for (PTRef key : new_units_vec) {
            current_units_vec.push(PtAsgn{key, new_units[key]});
        }

        auto [res, newsubsts] = retrieveSubstitutions(current_units_vec);

        // Insert here the closure algorithm.
        std::unordered_set<PTRef,PTRefHash> toBeSubstituted;
        updateSubstitutionClosure(termToNode, toBeSubstituted, newsubsts);
        ForestSubstitutionConfig substitutionConfig(termToNode);
        Rewriter<ForestSubstitutionConfig> rewriter(logic, substitutionConfig);
        PTRef new_root = rewriter.rewrite(root);

        if (res != l_Undef) {
            root = (res == l_True ? logic.getTerm_true() : logic.getTerm_false());
        }

        bool cont = new_root != root;
        root = new_root;
        if (!cont) break;
        break;
    }
    vec<PTRef> allSubsts;
    for (auto & pair : termToNode) {
        PTRef tr = pair.first;
        PTRef target = pair.second->tr;
        if (tr != target) {
            allSubsts.push(logic.mkEq({tr, target}));
        }
    }
#ifdef SIMPLIFY_DEBUG
    cerr << "Number of substitutions: " << allsubsts.elems() << endl;
    vec<Map<PTRef,PtAsgn,PTRefHash>::Pair> subst_vec;
    allsubsts.getKeysAndVals(subst_vec);
    printf("Substitutions:\n");
    for (int i = 0; i < subst_vec.size(); i++) {
        PTRef source = subst_vec[i].key;
        PTRef target = subst_vec[i].data.tr;
        lbool sgn = subst_vec[i].data.sgn;
        printf("  %s -> %s (%s)\n", getLogic().printTerm(source), getLogic().printTerm(target), sgn == l_True ? "enabled" : "disabled");
    }
#endif
    result.result = root;
    result.usedSubstitution = std::move(allSubsts);
    return result;
}

//
// The substitutions for the term riddance from osmt1
//
opensmt::pair<lbool,Logic::SubstMap> SubstitutionSimplifier::retrieveSubstitutions(const vec<PtAsgn>& facts)
{
    MapWithKeys<PTRef,PtAsgn,PTRefHash> substs;
    for (auto [tr, sgn] : facts) {
        // Join equalities
        if (logic.isEquality(tr) and sgn == l_True) {
#ifdef SIMPLIFY_DEBUG
            cerr << "Identified an equality: " << printTerm(tr) << endl;
#endif
            const Pterm& t = logic.getPterm(tr);
            // n will be the reference
            if (logic.isUFEquality(tr) || logic.isIff(tr)) {
                // This is the simple replacement to elimiate enode terms where possible
                assert(t.size() == 2);
                // One of them should be a var
                const Pterm& a1 = logic.getPterm(t[0]);
                const Pterm& a2 = logic.getPterm(t[1]);
                if (a1.size() == 0 || a2.size() == 0) {
                    PTRef var = a1.size() == 0 ? t[0] : t[1];
                    PTRef trm = a1.size() == 0 ? t[1] : t[0];
                    if (logic.contains(trm, var)) continue;
                    if (logic.isConstant(var)) {
                        assert(!logic.isConstant(trm));
                        PTRef tmp = var;
                        var = trm;
                        trm = tmp;
                    }
#ifdef SIMPLIFY_DEBUG
                    if (substs.has(var)) {
                        cerr << "Double substitution:" << endl;
                        cerr << " " << printTerm(var) << "/" << printTerm(trm) << endl;
                        cerr << " " << printTerm(var) << "/" << printTerm(substs[var].tr) << endl;
                        if (substs[var].sgn == l_False)
                            cerr << "  disabled" << endl;
                    } else {
                        char* tmp1 = printTerm(var);
                        char* tmp2 = printTerm(trm);
                        cerr << "Substituting " << tmp1 << " with " << tmp2 << endl;
                        ::free(tmp1); ::free(tmp2);
                    }
#endif
                    if (!substs.has(var)) {
                        substs.insert(var, PtAsgn(trm, l_True));
                    }
                }
            }
        } else if (logic.isBoolAtom(tr)) {
            PTRef term = sgn == l_True ? logic.getTerm_true() : logic.getTerm_false();
            substs.insert(tr, PtAsgn(term, l_True));
        }
    }
    SubstLoopBreaker slb(logic);
    return {l_Undef, slb(std::move(substs))};
}

void SubstitutionSimplifier::substitutionsTransitiveClosure(Logic::SubstMap & substs) {
    bool changed = true;
    const auto & keys = substs.getKeys(); // We can use direct pointers, since no elements are inserted or deleted in the loop
    std::vector<char> notChangedElems(substs.getSize(), 0); // True if not changed in last iteration, initially False
    while (changed) {
        changed = false;
        for (int i = 0; i < keys.size(); ++i) {
            if (notChangedElems[i]) { continue; }
            PTRef & val = substs[keys[i]];
            PTRef oldVal = val;
            PTRef newVal = Substitutor(logic, substs).rewrite(oldVal);
            if (oldVal != newVal) {
                changed = true;
                val = newVal;
            }
            else {
                notChangedElems[i] = 1;
            }
        }
    }
}

//
// TODO: Also this should most likely be dependent on the theory being
// used.  Depending on the theory a fact should either be added on the
// top level or left out to reduce e.g. simplex matrix size.
//
opensmt::pair<bool,SubstitutionSimplifier::Facts> SubstitutionSimplifier::getNewFacts(PTRef root) {
    Map<PtAsgn,bool,PtAsgnHash> isdup;
    vec<PtAsgn> queue;
    Facts facts;
    PTRef p;
    lbool sign;
    logic.purify(root, p, sign);
    queue.push(PtAsgn(p, sign));

    while (queue.size() != 0) {
        PtAsgn pta = queue.last(); queue.pop();

        if (isdup.has(pta)) continue;
        isdup.insert(pta, true);

        Pterm const & t = logic.getPterm(pta.tr);

        if (logic.isAnd(pta.tr) and pta.sgn == l_True) {
            for (PTRef l : t) {
                PTRef c;
                lbool c_sign;
                logic.purify(l, c, c_sign);
                queue.push(PtAsgn(c, c_sign));
            }
        } else if (logic.isOr(pta.tr) and (pta.sgn == l_False)) {
            for (PTRef l : t) {
                PTRef c;
                lbool c_sign;
                logic.purify(l, c, c_sign);
                queue.push(PtAsgn(c, c_sign ^ true));
            }
        } else {
            lbool prev_val = facts.has(pta.tr) ? facts[pta.tr] : l_Undef;
            if (prev_val != l_Undef && prev_val != pta.sgn)
                return {false, std::move(facts)}; // conflict
            else if (prev_val == pta.sgn)
                continue; // Already seen

            assert(prev_val == l_Undef);
            if (logic.isEquality(pta.tr) and pta.sgn == l_True) {
                facts.insert(pta.tr, pta.sgn);
            } else if (logic.isUP(pta.tr) and pta.sgn == l_True) {
                facts.insert(pta.tr, pta.sgn);
            } else if (logic.isXor(pta.tr) and pta.sgn == l_True) {
                Pterm const & xorTerm = logic.getPterm(pta.tr);
                facts.insert(logic.mkEq(xorTerm[0], logic.mkNot(xorTerm[1])), l_True);
            } else {
                PTRef c;
                lbool c_sign;
                logic.purify(pta.tr, c, c_sign);
                if (logic.isBoolAtom(c)) {
                    facts.insert(c, c_sign^(pta.sgn == l_False));
                }
            }
        }
    }
    return {true, std::move(facts)};
#ifdef SIMPLIFY_DEBUG
    cerr << "True facts" << endl;
    vec<Map<PTRef,lbool,PTRefHash>::Pair> facts_dbg;
    facts.getKeysAndVals(facts_dbg);
    for (int i = 0; i < facts_dbg.size(); i++)
        cerr << (facts_dbg[i].data == l_True ? "" : "not ") << printTerm(facts_dbg[i].key) << " (" << facts_dbg[i].key.x << ")" << endl;
#endif
}

bool SubstitutionSimplifier::updateSubstitutionClosure(UnionForest & termToNode, std::unordered_set<PTRef, PTRefHash> & toBeSubstituted, MapWithKeys<PTRef,PTRef,PTRefHash> const & rawSubstitutions) {

    bool unsatisfiable = false;

    const auto & entries = rawSubstitutions.getKeys();

    for (auto lhs : entries) {
        auto rhs = rawSubstitutions[lhs];
        auto getOrCreateNodePointer = [&termToNode](PTRef tr) {
            EqClass::Node *nodePtr;
            auto nodeIter = termToNode.find(tr);
            if (nodeIter == termToNode.end()) {
                auto tmp = std::make_unique<EqClass::Node>(tr);
                nodePtr = tmp.get();
                termToNode.insert({tr, std::move(tmp)});
            } else {
                nodePtr = (nodeIter->second).get();
            }
            return nodePtr;
        };
        auto lhsNode = getOrCreateNodePointer(lhs);
        auto rhsNode = getOrCreateNodePointer(rhs);

        EqClass::Node & lhsCanon = *termToNode[lhsNode->canonicalTerm];
        EqClass::Node & rhsCanon = *termToNode[rhsNode->canonicalTerm];

        if (lhsCanon.size > rhsCanon.size and not logic.isConstant(rhsCanon.tr)) {
            std::swap(lhsCanon, rhsCanon);
        }

        unsatisfiable |= (lhsCanon.tr != rhsCanon.tr and logic.isConstant(lhsCanon.tr) and logic.isConstant(rhsCanon.tr));
        // rhsCanon will be the substitution target
        PTRef canonTerm = rhsCanon.canonicalTerm;
        toBeSubstituted.insert(lhsNode->canonicalTerm);
        EqClass::Node * next = lhsNode;
        do  {
            next->canonicalTerm = canonTerm;
            next = next->next;
        } while (next->tr != lhsNode->tr);

        std::swap(lhsNode->next, rhsNode->next);
        rhsCanon.size += lhsCanon.size;
    }
    return unsatisfiable;
}
