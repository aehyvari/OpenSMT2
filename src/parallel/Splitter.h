/*
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PARALLEL_SPLITTER_H
#define PARALLEL_SPLITTER_H

#include "SplitData.h"
#include "SplitContext.h"

class Splitter {

protected:
    SplitContext splitContext;

public:
    Splitter(SMTConfig & c)
    : splitContext(c)
    {}

    std::vector<SplitData>      const & getSplits() { return splitContext.getSplits(); }

    bool isSplitTypeScatter()   const  { return splitContext.isSplitTypeScatter(); }

    bool isSplitTypeLookahead()  const { return splitContext.isSplitTypeLookahead(); }

    bool isSplitTypeNone()       const { return splitContext.isSplitTypeNone(); }

    void resetSplitType() { splitContext.resetSplitType(); }
};

#endif //PARALLEL_SPLITTER_H