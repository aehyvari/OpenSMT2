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
        // Get U_i
        auto new_units = getNewFacts(simp_formula);

        auto [res, newsubsts] = retrieveSubstitutions(new_units);

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
opensmt::pair<lbool,Logic::SubstMap> SubstitutionSimplifier::retrieveSubstitutions(Facts & facts)
{
    MapWithKeys<PTRef,PtAsgn,PTRefHash> substs;
    for (PTRef tr : facts) {
        // Join equalities
        if (logic.isEquality(tr)) {
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
        } else {
            auto [pureTr, sign] = purify(tr);
            if (logic.isBoolAtom(pureTr)) {
                PTRef term = sign == l_True ? logic.getTerm_true() : logic.getTerm_false();
                substs.insert(tr, PtAsgn(term, l_True));
            }
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

namespace {
    struct PtaIndex {
        uint32_t idx;
        PtaIndex(PTId idx, lbool sign) : idx((Idx(idx) << 1) | (sign == l_False)) {}
    };
    inline uint32_t Idx(PtaIndex ptai) { return ptai.idx; }
}

SubstitutionSimplifier::Facts SubstitutionSimplifier::getNewFacts(PTRef root) {

    struct DFSEntry {
        explicit DFSEntry(PtAsgn termAsgn) : termAsgn(termAsgn) {}
        PtAsgn termAsgn;
        int nextChild = 0;
    };

    auto termMarks = logic.getTermMarks(PtaIndex(logic.getPterm(root).getId(), l_False));
    std::vector<DFSEntry> toProcess;
    auto [p, sign] = purify(root);
    toProcess.emplace_back(PtAsgn{p, sign});

    Map<PtAsgn,bool,PtAsgnHash> isdup;
    vec<PtAsgn> queue;
    Facts facts;

    while (not toProcess.empty()) {
        auto & currentEntry = toProcess.back();

        PtAsgn currentTermAsgn = currentEntry.termAsgn;
        PTRef currentRef = currentTermAsgn.tr;
        lbool currentSign = currentTermAsgn.sgn;
        auto currentId = PtaIndex(logic.getPterm(currentTermAsgn.tr).getId(), currentSign);

        assert(not termMarks.isMarked(currentId));

        Pterm const & term = logic.getPterm(currentRef);
        int childrenCount = term.size();
        if (currentEntry.nextChild < childrenCount) {
            ++currentEntry.nextChild;
            if (logic.isAnd(currentRef) and currentSign == l_True) {
                auto [childTr, childSign] = purify(term[currentEntry.nextChild-1]);
                auto childId = PtaIndex(logic.getPterm(childTr).getId(), childSign);
                if (not termMarks.isMarked(childId)) {
                    toProcess.emplace_back(PtAsgn(childTr, childSign));
                }
                continue;
            } else if (logic.isOr(currentRef) and currentSign == l_False) {
                auto [childTr, childSign] = purify(term[currentEntry.nextChild-1]);
                auto childId = PtaIndex(logic.getPterm(childTr).getId(), childSign ^ true);
                if (not termMarks.isMarked(childId)) {
                    toProcess.emplace_back(PtAsgn(childTr, childSign ^ true));
                }
                continue;
            }
        }

        // If we are here, we have already processed all children
        // Visit
        assert(not termMarks.isMarked(currentId));

        if (logic.isEquality(currentRef) and currentSign == l_True) {
            facts.push(currentRef);
        } else if (logic.isUP(currentRef)) {
            facts.push(currentSign == l_True ? currentRef : logic.mkNot(currentRef));
        } else if (logic.isXor(currentRef) and currentSign == l_True) {
            Pterm const & xorTerm = logic.getPterm(currentRef);
            facts.push(logic.mkEq(xorTerm[0], logic.mkNot(xorTerm[1])));
        } else {
            if (logic.isBoolAtom(currentRef)) {
                facts.push(currentSign == l_True ? currentRef : logic.mkNot(currentRef));
            }
        }
        termMarks.mark(currentId);
        toProcess.pop_back();
    }
    return facts;
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
