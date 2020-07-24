#include <opensmt/opensmt2.h>
#include <opensmt/BitBlaster.h>
#include <stdio.h>

int
main(int argc, char** argv)
{

    const char* customer_id_1_str = "customer_id1";
    const char* customer_id_2_str = "customer_id2";
    const char* warehouse_id_str = "warehouse_id";
    const char* item_id_str = "item_id";
    int bw = atoi(argv[1]);

    SMTConfig c;
    CUFTheory* cuftheory = new CUFTheory(c, bw);
    THandler* thandler = new THandler(*cuftheory);
    SimpSMTSolver* solver = new SimpSMTSolver(c, *thandler);
    MainSolver* mainSolver_ = new MainSolver(*thandler, c, solver, "test solver");
    MainSolver& mainSolver = *mainSolver_;
    BVLogic& logic = cuftheory->getLogic();

    PTRef unit = logic.mkBVConst("1");
    PTRef zero = logic.mkBVConst("0");
    PTRef c2 = logic.mkBVConst("2");
    PTRef c10 = logic.mkBVConst("10");
    PTRef c100 = logic.mkBVConst("100");

    PTRef cid1 = logic.mkBVNumVar(customer_id_1_str);
    PTRef cid2 = logic.mkBVNumVar(customer_id_2_str);
    PTRef wid = logic.mkBVNumVar(warehouse_id_str);
    PTRef iid = logic.mkBVNumVar(item_id_str);

    // Customer 1 range
    PTRef cid1_lb = logic.mkBVEq(logic.mkBVUleq(zero, cid1), unit);
    PTRef cid1_ub = logic.mkBVEq(logic.mkBVUlt(cid1, c10), unit);

    // Customer 2 range
    PTRef cid2_lb = logic.mkBVEq(logic.mkBVUleq(zero, cid2), unit);
    PTRef cid2_ub = logic.mkBVEq(logic.mkBVUlt(cid2, c10), unit);

    // Warehouse range
    PTRef wid_lb = logic.mkBVEq(logic.mkBVUleq(zero, wid), unit);
    PTRef wid_ub = logic.mkBVEq(logic.mkBVUlt(wid, c10), unit);

    // Item range
    PTRef iid_lb = logic.mkBVEq(logic.mkBVUleq(zero, iid), unit);
    PTRef iid_ub = logic.mkBVEq(logic.mkBVUlt(iid, c10), unit);

    // Customer modularity constraint
    PTRef cid1_mod2 = logic.mkBVMod(cid1, c2);
    PTRef cid2_mod2 = logic.mkBVMod(cid2, c2);
    PTRef cid_mod_equality = logic.mkBVEq(cid1_mod2, cid2_mod2);

    vec<PtAsgn> asgns;
    vec<PTRef> foo;

    SolverId id = {42};
    BitBlaster bbb(id, c, mainSolver, logic, asgns, foo);
    BVRef output;
    cout << logic.pp(cid1_lb) << "\n" << logic.pp(cid1_ub) << endl;

    lbool stat;
    stat = bbb.insertEq(cid1_lb, output);
    stat = bbb.insertEq(cid1_ub, output);
    stat = bbb.insertEq(cid2_lb, output);
    stat = bbb.insertEq(cid2_ub, output);
    stat = bbb.insertEq(wid_lb, output);
    stat = bbb.insertEq(wid_ub, output);
    stat = bbb.insertEq(iid_lb, output);
    stat = bbb.insertEq(iid_ub, output);
    stat = bbb.insertEq(cid_mod_equality, output);

    if (stat == l_False)  {
        cout << "Trivially unsat" << endl;
    }

    char* msg;
    mainSolver.insertFormula(logic.getTerm_true(), &msg);

    sstat r = mainSolver.check();

    mainSolver.dumpInitialCnfToFile("foo.cnf");

    if (r == s_True) {
        printf("sat\n");
        bbb.computeModel();
        ValPair c1_v = bbb.getValue(cid1);
        ValPair c2_v = bbb.getValue(cid2);
        ValPair w_v = bbb.getValue(wid);
        ValPair i_v = bbb.getValue(iid);
        char* c1_bin;
        char* c2_bin;
        char* w_bin;
        char* i_bin;
        opensmt::wordToBinary(atoi(c1_v.val), c1_bin, bw);
        opensmt::wordToBinary(atoi(c2_v.val), c2_bin, bw);
        opensmt::wordToBinary(atoi(w_v.val), w_bin, bw);
        opensmt::wordToBinary(atoi(i_v.val), i_bin, bw);
        printf("customer 1 %s (%s)\n", c1_v.val, c1_bin);
        printf("customer 2 %s (%s)\n", c2_v.val, c2_bin);
        printf("warehouse %s (%s)\n", w_v.val, w_bin);
        printf("item %s (%s)\n", i_v.val, i_bin);
    }
    else if (r == s_False)
        printf("unsat\n");
    else if (r == s_Undef)
        printf("unknown\n");
    else
        printf("error\n");

    return 0;
}
