#ifndef LIATHandler_H
#define LIATHandler_H

#include "TSolverHandler.h"

#include "LIALogic.h"

class LIASolver;
class LIALogic;

class LIATHandler : public TSolverHandler
{
  private:
    LIALogic& logic;
    LIASolver *liasolver;
  public:
    LIATHandler(SMTConfig & c, LIALogic & l);
    virtual ~LIATHandler();
    virtual Logic& getLogic() override;
    virtual const Logic& getLogic() const override;

    virtual PTRef getInterpolant(const ipartitions_t& mask, std::unordered_map<PTRef, icolor_t, PTRefHash> *labels, PartitionManager &pmanager) override;
};

#endif
