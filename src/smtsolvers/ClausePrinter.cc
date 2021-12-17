/*
 * Copyright (c) 2021, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "ClausePrinter.h"
#include "Proof.h"
#include "FSBVTheory.h"

void ModelCounter::count(vec<PTRef> const & terms) const {
    // print all clauses
    auto & theory = dynamic_cast<FSBVTheory&>(theory_handler.getTheory());
    unsigned int totalNumOfVars = nVars();

    // Include the vars that need to be counted but were optimised away in simplification to total var count
    for (PTRef countTerm : terms) {
        BitWidth_t bitWidth = theory.getLogic().getRetSortBitWidth(countTerm);
        if (bvTermToVars.find(countTerm) != bvTermToVars.end()) {
            auto const & varSet = bvTermToVars.at(countTerm);
            assert(varSet.size() <= bitWidth);
            totalNumOfVars += (bitWidth - varSet.size());
        } else {
            totalNumOfVars += bitWidth;
        }
    }

    std::cout << "p cnf " + std::to_string(totalNumOfVars) + " " + std::to_string(nClauses()) << std::endl;
    for (vec<Lit> const & smtClause : clauses) {
        for (Lit l: smtClause) {
            Var v = var(l);
            std::cout << (sign(l) ? -(v + 1) : (v + 1)) << " ";
        }
        std::cout << "0" << std::endl;
    }
}

void ModelCounter::addVar(Var v) {
    if (not vars.has(v)) {
        vars.insert(v, true);
        ++ numberOfVarsSeen;
    }
}

bool ModelCounter::addOriginalSMTClause(vec<Lit> const & smtClause, opensmt::pair<CRef, CRef> &inOutCRfs) {
    auto & theory = dynamic_cast<FSBVTheory&>(theory_handler.getTheory());
    auto & bbTermToBVTerm = theory.getBBTermToBVTerm();
    for (Lit l : smtClause) {
        Var v = var(l);
        if (not vars.has(v)) {
            // A new variable
            vars.insert(v, true);
            numberOfVarsSeen++;

            PTRef bbTerm = theory_handler.varToTerm(v);
            if (bbTermToBVTerm.find(bbTerm) != bbTermToBVTerm.end()) {
                // The variable originates from bit-blasted gate
                // Update the bits of the gate
                PTRef bvTerm = bbTermToBVTerm.at(bbTerm);
                bvTermToVars[bvTerm].insert(v);
            }
        }
    }
    vec<Lit> outClause;
    smtClause.copyTo(outClause);
    clauses.push_back(std::move(outClause));
    return true;
}

bool ModelCounter::addOriginalClause(vec<Lit> const & smtClause) {
    assert(false); // This code should not be called in the intended workflow
    return true;
}

Var ModelCounter::newVar(bool, bool) {
    assert(false); // This code should not be called in the intended workflow
    return Var();
}

int ModelCounter::nVars() const {
    return numberOfVarsSeen;
}

Proof const & ModelCounter::getProof() const {
    assert(false); // This code should not be called in the intended workflow
    return proof;
}
