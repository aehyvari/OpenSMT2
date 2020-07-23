#include <opensmt/opensmt2.h>
#include <opensmt/BitBlaster.h>
#include <stdio.h>

int
main(int argc, char** argv)
{

    if (argc != 4)
    {
        printf("Computes the model for a <op> x on bit width <bw>\n");
        printf("Usage: %s <bw> <op> <a>\n", argv[0]);
        return 1;
    }
    char* op = argv[2];

    const char* c1_str = argv[3];
    const char* x_str = strdup("x");
    int bw = atoi(argv[1]);

    SMTConfig c;
    CUFTheory* cuftheory = new CUFTheory(c, bw);
    THandler* thandler = new THandler(*cuftheory);
    SimpSMTSolver* solver = new SimpSMTSolver(c, *thandler);
    MainSolver* mainSolver_ = new MainSolver(*thandler, c, solver, "test solver");
    MainSolver& mainSolver = *mainSolver_;
    BVLogic& logic = cuftheory->getLogic();

    PTRef x = logic.mkBVNumVar(x_str);
    PTRef unit = logic.mkBVConst("1");

    PTRef c1 = logic.mkBVConst(c1_str);

    printf("Computing %s (%s) %s %s (%s)\n", c1_str, logic.printTerm(c1), op, x_str, logic.printTerm(x));


    PTRef op_tr;
    if (strcmp(op, "/") == 0)
        op_tr = logic.mkBVDiv(c1, x);
    else if (strcmp(op, "*") == 0)
        op_tr = logic.mkBVTimes(c1, x);
    else if (strcmp(op, "+") == 0)
        op_tr = logic.mkBVPlus(c1, x);
    else if (strcmp(op, "-") == 0)
        op_tr = logic.mkBVMinus(c1, x);
    else if (strcmp(op, "s<") == 0)
        op_tr = logic.mkBVSlt(c1, x);
    else if (strcmp(op, "s<=") == 0)
        op_tr = logic.mkBVSleq(c1, x);
    else if (strcmp(op, "u<=") == 0)
        op_tr = logic.mkBVUleq(c1, x);
    else if (strcmp(op, "u>=") == 0)
        op_tr = logic.mkBVUleq(x, c1);
    else if (strcmp(op, "s>=") == 0)
        op_tr = logic.mkBVSgeq(c1, x);
    else if (strcmp(op, "s>") == 0)
        op_tr = logic.mkBVSgt(c1, x);
    else if (strcmp(op, "<<") == 0)
        op_tr = logic.mkBVLshift(c1, x);
    else if (strcmp(op, "a>>") == 0)
        op_tr = logic.mkBVARshift(c1, x);
    else if (strcmp(op, "l>>") == 0)
        op_tr = logic.mkBVLRshift(c1, x);
    else if (strcmp(op, "%") == 0)
        op_tr = logic.mkBVMod(c1, x);
    else if (strcmp(op, "&") == 0)
        op_tr = logic.mkBVBwAnd(c1, x);
    else if (strcmp(op, "|") == 0)
        op_tr = logic.mkBVBwOr(c1, x);
    else if (strcmp(op, "=") == 0)
        op_tr = logic.mkBVEq(c1, x);
    else if (strcmp(op, "&&") == 0)
        op_tr = logic.mkBVLand(c1, x);
    else if (strcmp(op, "^") == 0)
        op_tr = logic.mkBVBwXor(c1, x);
    else if (strcmp(op, "==") == 0)
        op_tr = logic.mkBVEq(c1, x);
    else {
        printf("Unknown operator: %s\n", op);
        return 1;
    }

    vec<PtAsgn> asgns;
    vec<PTRef> foo;

    SolverId id = {42};
    BitBlaster bbb(id, c, mainSolver, logic, asgns, foo);
    BVRef output;

    lbool stat = bbb.insertEq(logic.mkBVEq(op_tr, unit), output);
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
        ValPair v = bbb.getValue(x);
        char* bin;
        opensmt::wordToBinary(atoi(v.val), bin, bw);
        printf("%s (%s)\n", v.val, bin);

    }
    else if (r == s_False)
        printf("unsat\n");
    else if (r == s_Undef)
        printf("unknown\n");
    else
        printf("error\n");

    return 0;
}
