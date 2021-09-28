//
// Created by prova on 01.04.21.
//

#ifndef OPENSMT_MAINSPLITTER_H
#define OPENSMT_MAINSPLITTER_H

#include "MainSolver.h"
#include "ScatterSplitter.h"

class MainSplitter : public MainSolver {

public:
    static std::unique_ptr<SimpSMTSolver> createInnerSolver(SMTConfig& config, THandler& thandler, Channel& ch);
    MainSplitter(std::unique_ptr<Theory> t,std::unique_ptr<TermMapper> tm, std::unique_ptr<THandler> th,
                 std::unique_ptr<SimpSMTSolver> ss, Logic& logic, SMTConfig& config, std::string name)
            :
            MainSolver(
                    std::move(t),
                    std::move(tm),
                    std::move(th),
                    std::move(ss),
                    logic,
                    config,
                    std::move(name)
            ){}

    bool writeSolverSplits_smtlib2(const char* file, char** msg) const;

    std::vector<std::string> getSolverPartitions();


};


#endif //OPENSMT_MAINSPLITTER_H
