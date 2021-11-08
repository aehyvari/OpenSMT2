/*
 * Copyright (c) 2021, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2021, Martin Blicha <martin.blicha@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */


#ifndef OPENSMT_NONLINEARREWRITER_H
#define OPENSMT_NONLINEARREWRITER_H

#include "PTRef.h"

#include "Rewriter.h"
#include "ArithLogic.h"

#include "OsmtApiException.h"

#include <unordered_map>
#include <string>

class NonlinearConfig : public DefaultRewriterConfig {
    ArithLogic & logic;

    std::unordered_map<PTRef, PTRef, PTRefHash> nonlinCache;
    vec<PTRef> definitions;

    inline static std::string const ntimesPrefix = ".ntimes";
    inline static std::string const ndivisionPrefix = ".ndivision";
    inline static std::string const nmodPrefix = ".nmod";
    inline static std::string const ndivPrefix = ".ndiv";


    PTRef freshNonlinearity(PTRef nonlinTerm) {

        std::string id;
        std::string termName;

        for (PTRef tr : logic.getPterm(nonlinTerm)) {
            id += "_" + std::to_string(tr.x);
        }
        if (logic.isNTimes(nonlinTerm)) {
            termName = ntimesPrefix + id;
        } else if (logic.isNMod(nonlinTerm)) {
            termName = nmodPrefix + id;
        } else if (logic.isIntNDiv(nonlinTerm)) {
            termName = ndivPrefix + id;
        } else if (logic.isRealNDiv(nonlinTerm)) {
            termName = ndivisionPrefix + id;
        }
        return logic.mkVar(logic.getSortRef(nonlinTerm), termName.c_str());
    }

public:
    NonlinearConfig(ArithLogic & logic) : logic(logic) {}

    PTRef rewrite(PTRef term) override {
        if (logic.isNTimes(term) or logic.isNMod(term) or logic.isIntNDiv(term) or logic.isRealNDiv(term)) {
            // check cache first
            auto it = nonlinCache.find(term);
            bool inCache = (it != nonlinCache.end());
            PTRef nlsubst = inCache ? it->second : freshNonlinearity(term);
            if (not inCache) {
                nonlinCache.insert({term, nlsubst});
            }
            if (not inCache) {
                // todo: Add constraints for example for a nonlinear modulo?
            }
            return nlsubst;
        }
        return term;
    }

    void getDefinitions(vec<PTRef> & out) {
        for (PTRef def : definitions) {
            out.push(def);
        }
    }
};

class NonlinearRewriter : Rewriter<NonlinearConfig> {
    ArithLogic & logic;
    NonlinearConfig config;
public:
    NonlinearRewriter(ArithLogic & logic) : Rewriter<NonlinearConfig>(logic, config), logic(logic), config(logic) {}

    PTRef rewrite(PTRef term) override {
        if (term == PTRef_Undef or not logic.hasSortBool(term)) {
            throw OsmtApiException("Nonlinearity rewriting should only be called on formulas, not terms!");
        }
        PTRef rewritten = Rewriter<NonlinearConfig>::rewrite(term);
        vec<PTRef> args;
        args.push(rewritten);
        config.getDefinitions(args);
        return logic.mkAnd(args);
    }
};

// Simple single-use version
inline PTRef rewriteNonlinear(ArithLogic & logic, PTRef root) {
    return NonlinearRewriter(logic).rewrite(root);
}


#endif //OPENSMT_NONLINEARREWRITER_H
