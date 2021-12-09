//
// Created by prova on 08.12.21.
//

#ifndef OPENSMT_FSBVBITBLASTER_H
#define OPENSMT_FSBVBITBLASTER_H

#include "FSBVLogic.h"
#include "bvsolver/Bvector.h"
#include "bvsolver/BVStore.h"
#include "OsmtInternalException.h"

class FSBVBitBlaster {
    FSBVLogic & logic;
    BVStore bs;

    static constexpr const char * s_add = "add";
    static constexpr const char * s_mul = "mul";
    static constexpr const char * s_ule = "ule";

    // Predicates
    PTRef bbUlt(PTRef tr);

    void notImplemented(PTRef tr) { throw OsmtInternalException(std::string("Not implemented: ") + logic.getSymName(tr)); }
    BVRef bbConcat(PTRef tr) { notImplemented(tr); return BVRef_Undef; }
    BVRef bbNot(PTRef tr) { notImplemented(tr); return BVRef_Undef; }
    BVRef bbNeg(PTRef tr) { notImplemented(tr); return BVRef_Undef; }
    BVRef bbAnd(PTRef tr) { notImplemented(tr); return BVRef_Undef; }
    BVRef bbOr(PTRef tr) { notImplemented(tr); return BVRef_Undef; }
    BVRef bbAdd(PTRef tr);
    BVRef bbMul(PTRef tr);
    BVRef bbUdiv(PTRef tr) { notImplemented(tr); return BVRef_Undef; }
    BVRef bbUrem(PTRef tr) { notImplemented(tr); return BVRef_Undef; }
    BVRef bbShl(PTRef tr) { notImplemented(tr); return BVRef_Undef; }
    BVRef bbLshr(PTRef tr) { notImplemented(tr); return BVRef_Undef; }
    BVRef bbConstant(PTRef tr);
    BVRef bbVar(PTRef);

    vec<PTRef> getBVVars(std::string const & base, BitWidth_t width);
public:
    FSBVBitBlaster(FSBVLogic & fsbvLogic) : logic(fsbvLogic) { }

    PTRef bbPredicate(PTRef);

    BVRef bbTerm(PTRef);
};
#endif //OPENSMT_FSBVBITBLASTER_H
