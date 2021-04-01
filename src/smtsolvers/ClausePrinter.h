//
// Created by prova on 10.12.21.
//

#ifndef OPENSMT_CLAUSEPRINTER_H
#define OPENSMT_CLAUSEPRINTER_H

#include "SMTSolver.h"

class ClausePrinter : public SMTSolver {
    ClauseAllocator ca;
    Proof proof;
    Map<Var, bool, VarHash> vars;
    int numberOfVars;
    std::vector<vec<Lit>> clauses;
public:
    ClausePrinter(SMTConfig & c, THandler & t) : SMTSolver(c, t), ca(0), proof(ca), numberOfVars(0) { }
    lbool solve (vec<Lit> const & assumps) override;
    void setFrozen(Var, bool) override { }
    bool isOK() const override { return true; }
    void addVar(Var v) override;
    bool addOriginalSMTClause(vec<Lit> const & smt_clause, opensmt::pair<CRef, CRef> & inOutCRefs) override;
    bool addOriginalClause(vec<Lit> const &) override;
    lbool modelValue(Lit p) const override { return l_Undef; }
    void fillBooleanVars(ModelBuilder &) override { }
    void initialize() override { };
    void pushBacktrackPoint() override { }
    void popBacktrackPoint() override { }
    void restoreOK() override { }
    Proof const & getProof() const override;
    int nVars() const override;

    int getConflictFrame() const override { return -1; }
    void clearSearch() override {}

protected:
    Var newVar (bool = true, bool = true) override;
private:
    int nClauses() const { return clauses.size(); }
};


#endif //OPENSMT_CLAUSEPRINTER_H
