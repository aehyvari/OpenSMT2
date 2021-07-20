#include "LIATHandler.h"
#include "TreeOps.h"
#include <liasolver/LIASolver.h>
#include "OsmtInternalException.h"

LIATHandler::LIATHandler(SMTConfig & c, LIALogic & l)
        : TSolverHandler(c)
        , logic(l)
{
    liasolver = new LIASolver(config, logic);
    SolverId my_id = liasolver->getId();
    tsolvers[my_id.id] = liasolver;
    solverSchedule.push(my_id.id);
}

LIATHandler::~LIATHandler() { }

Logic &LIATHandler::getLogic()
{
    return logic;
}

const Logic &LIATHandler::getLogic() const
{
    return logic;
}

PTRef LIATHandler::getInterpolant(ipartitions_t const &, std::unordered_map<PTRef, icolor_t, PTRefHash> * labels, PartitionManager &)
{
    if (labels == nullptr) {
        throw OsmtInternalException("LIA interpolation requires partitioning map, but no map was provided");
    }
    return liasolver->getInterpolant(*labels);
}
