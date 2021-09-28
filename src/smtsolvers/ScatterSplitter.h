//
// Created by prova on 31.03.21.
//

#ifndef OPENSMT_SCATTERSPLITTER_H
#define OPENSMT_SCATTERSPLITTER_H

#include "SimpSMTSolver.h"
#include "SplitData.h"
#include "SplitConfig.h"
#include "unordered_set"
#include "../../partition-channel-src/src/Channel.h"

class ScatterSplitter : public SimpSMTSolver {
public:
    std::vector<SplitData> splits;
    ScatterSplitter(SMTConfig & c, THandler &, Channel& ch);
    void setSplitConfig_split_on() { splitConfig.split_on = true; };
private:
    std::vector<vec<Lit>> split_assumptions;
    SplitConfig splitConfig;
    std::unordered_set<Var> assumptionVars;
    Channel& channel;
    int trail_sent;
    bool firstTime;
    std::atomic_bool clauseLearnTimeOut;

    void     updateSplitState();                                                       // Update the state of the splitting machine.
    bool     scatterLevel();                                                           // Are we currently on a scatter level.
    bool     createSplit_scatter(bool last);                                           // Create a split formula and place it to the splits vector.
    bool     excludeAssumptions(vec<Lit>& neg_constrs);                                // Add a clause to the database and propagate

protected:

    virtual lbool solve_() override;

    bool learnSomeClauses(std::vector<std::pair<string ,int>> & _toPublishTerms);

    inline bool findInAssumptionVar(Var  v)   const   { return assumptionVars.find(v) != assumptionVars.end(); }

    inline int findInAssumption(Var v) const
    {
        for (int i = 0; i < this->assumptions.size(); i++)
            if (var(this->assumptions[i]) == v)
                return i;
        return -1;
    }

    bool branchLitRandom() override;
    Var doActivityDecision() override;
    bool  okContinue() const;                                                   // Check search termination conditions
    void  runPeriodics();                                                        // Run periodically to learn some clauses
    lbool search(int nof_conflicts, int nof_learnts) override;                  // Search for a given number of conflicts.
    lbool zeroLevelConflictHandler() override;                                  // Common handling of zero-level conflict as it can happen at multiple places
};


#endif //OPENSMT_SCATTERSPLITTER_H
