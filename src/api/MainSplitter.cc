//
// Created by prova on 01.04.21.
//

#include <LookaheadSplitter.h>
#include "MainSplitter.h"
#include "SplitData.h"
#include "ScatterSplitter.h"
#include <filesystem>

bool MainSplitter::writeSolverSplits_smtlib2(const char* file, char** msg) const
{
    std::vector<SplitData>& splits =(config.sat_split_type()==spt_scatter) ? static_cast<ScatterSplitter&>(ts.solver).splits
            : static_cast<LookaheadSplitter&>(ts.solver).splits;
    std::cout <<";Number of partition splits requested : "<<config.sat_split_num() << endl;
    std::cout<<";Number of partition splits created : " << splits.size() << endl;
    std::cout<<";Partition splits are located at \""<<config.output_dir()<<"\""<<endl;
    std::filesystem::create_directory(config.output_dir());
    int i = 0;
    for (const auto & split : splits) {
        vec<PTRef> conj_vec;
        std::vector<vec<PtAsgn> > constraints;
        split.constraintsToPTRefs(constraints, *thandler);
        addToConj(constraints, conj_vec);

        std::vector<vec<PtAsgn> > learnts;
        split.learntsToPTRefs(learnts, *thandler);
        addToConj(learnts, conj_vec);

        if (config.smt_split_format_length() == spformat_full)
            conj_vec.push(root_instance.getRoot());

        PTRef problem = logic.mkAnd(conj_vec);
        char* name;
        int written = asprintf(&name, "%s-%02d.smt2", file, i ++);
        assert(written >= 0);
        (void)written;
        std::ofstream file;
        file.open(name);
        if (file.is_open()) {
            logic.dumpHeaderToFile(file);
            logic.dumpFormulaToFile(file, problem);

            if (config.smt_split_format_length() == spformat_full)
                logic.dumpChecksatToFile(file);

            file.close();
        }
        else {
            written = asprintf(msg, "Failed to open file %s\n", name);
            assert(written >= 0);
            free(name);
            return false;
        }
        free(name);
    }
    return true;
}

std::vector<std::string> MainSplitter::getPartitionSplits()
{
    std::vector<std::string> partitions;
    std::vector<SplitData>& splits = static_cast<ScatterSplitter&>(this->getSMTSolver()).splits;
    for (size_t i = 0; i < splits.size(); i++) {
        std::vector<vec<PtAsgn> > constraints;
        splits[i].constraintsToPTRefs(constraints, getTHandler());
        vec<PTRef> clauses;
        for (size_t j = 0; j < constraints.size(); j++) {
            vec<PTRef> clause;
            for (int k = 0; k < constraints[j].size(); k++) {
                PTRef pt =
                        constraints[j][k].sgn == l_True ?
                        constraints[j][k].tr :
                        this->getLogic().mkNot(constraints[j][k].tr);
                clause.push(pt);
            }
            clauses.push(getLogic().mkOr(clause));
        }
        string str = getTHandler().getLogic().
                printString(getLogic().mkAnd(clauses));
        partitions.push_back(str);

    }
    return partitions;
}
std::unique_ptr<SimpSMTSolver> MainSplitter::createInnerSolver(SMTConfig & config, THandler & thandler, Channel& ch) {
    SimpSMTSolver* solver = nullptr;
    if (config.sat_split_type() == spt_scatter) {
        solver = new ScatterSplitter(config, thandler, ch);
    }
        // to do
    else if (config.sat_split_type() == spt_lookahead)  {
        solver = new LookaheadSplitter(config, thandler);
    }
    return std::unique_ptr<SimpSMTSolver>(solver);
}