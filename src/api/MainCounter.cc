/*
 * Copyright (c) 2021, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2021, Martin Blicha <martin.blicha@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include "MainCounter.h"

void MainCounter::countModels(vec<PTRef> const & terms) {
    initialize();
    sstat rval = MainSolver::simplifyFormulas();
    auto & modelCounter = dynamic_cast<ModelCounter&>(*smt_solver);
    modelCounter.count(terms);
}
