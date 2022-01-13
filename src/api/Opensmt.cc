#include "Opensmt.h"

#include "Global.h"
#include "LogicFactory.h"

namespace{
opensmt::Logic_t convert(opensmt_logic logic) {
    switch (logic) {
        case qf_lra:
            return opensmt::Logic_t::QF_LRA;
        case qf_lia:
            return opensmt::Logic_t::QF_LIA;
        case qf_bool:
            return opensmt::Logic_t::QF_BOOL;
        case qf_bv:
            return opensmt::Logic_t::QF_BV;
        case qf_uf:
            return opensmt::Logic_t::QF_UF;
        case qf_cuf:
            return opensmt::Logic_t::QF_CUF;
        case qf_idl:
            return opensmt::Logic_t::QF_IDL;
        case qf_rdl:
            return opensmt::Logic_t::QF_RDL;
        case qf_ufidl:
            return opensmt::Logic_t::QF_UFIDL;
        case qf_uflra:
            return opensmt::Logic_t::QF_UFLRA;
        case qf_ct:
            return opensmt::Logic_t::QF_CT;
        default:
            return opensmt::Logic_t::UNDEF;
    }
}
}

Opensmt::Opensmt(opensmt_logic _logic, const char* name)
{
    config = std::unique_ptr<SMTConfig>(new SMTConfig());
    logic.reset(opensmt::LogicFactory::getInstance(convert(_logic)));
    mainSolver = std::unique_ptr<MainSolver>(new MainSolver(*logic, *config, name));
    mainSolver->initialize();
}

Opensmt::Opensmt(opensmt_logic logic_, const char* name, std::unique_ptr<SMTConfig> config_)
{
    this->config = std::move(config_);
    logic.reset(opensmt::LogicFactory::getInstance(convert(logic_)));
    mainSolver = std::unique_ptr<MainSolver>(new MainSolver(*logic, *this->config, name));
    mainSolver->initialize();
}
