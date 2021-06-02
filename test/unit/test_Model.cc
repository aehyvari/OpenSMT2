//
// Created by Martin Blicha on 14.06.20.
//

#include <gtest/gtest.h>
#include <LRALogic.h>
#include <SMTConfig.h>
#include <Model.h>
#include <ModelBuilder.h>
#include <Opensmt.h>

#include <memory>

class UFModelTest : public ::testing::Test {
protected:
    UFModelTest(): logic{} {}
    virtual void SetUp() {
        char* err;
        S = logic.declareSort("U", &err);
        x = logic.mkVar(S, "x");
        y = logic.mkVar(S, "y");
        z = logic.mkVar(S, "z");
        f_sym = logic.declareFun("f", S, {S, S}, &err);
        f = logic.mkUninterpFun(f_sym, {x, y});

        v0 = logic.mkConst(S, "@0");
        v1 = logic.mkConst(S, "@1");

        a = logic.mkBoolVar("a");
        b = logic.mkBoolVar("b");
    }
    SMTConfig config;
    Logic logic;

    SRef S;

    PTRef x;
    PTRef y;
    PTRef z;
    PTRef f;
    PTRef a;
    PTRef b;

    PTRef v0;
    PTRef v1;

    SymRef f_sym;

    std::unique_ptr<Model> getModel() {
        Model::Evaluation eval {
                std::make_pair(x, v0),
                std::make_pair(y, v0),
                std::make_pair(z, v1),
                std::make_pair(a, logic.getTerm_true()),
                std::make_pair(b, logic.getTerm_false()),
        };
        Model::SymbolDefinition symDef {
            std::make_pair(f_sym, TemplateFunction("f", {x, y}, S, logic.mkIte(logic.mkEq(x, v1), v0, v1)))
        };
        return std::unique_ptr<Model>(new Model(logic, eval, symDef));
    }
};

class UFModelBuilderTest : public ::testing::Test {
protected:
    UFModelBuilderTest(): logic{} {}
    virtual void SetUp() {
        char* err;
        S = logic.declareSort("U", &err);
        x = logic.mkVar(S, "x");
        y = logic.mkVar(S, "y");
        z = logic.mkVar(S, "z");
        f_sym = logic.declareFun("f", S, {S, S}, &err);
        f = logic.mkUninterpFun(f_sym, {x, y});

        v0 = logic.mkConst(S, "@0");
        v1 = logic.mkConst(S, "@1");

        a = logic.mkBoolVar("a");
        b = logic.mkBoolVar("b");
    }
    SMTConfig config;
    Logic logic;

    SRef S;

    PTRef x;
    PTRef y;
    PTRef z;
    PTRef f;
    PTRef a;
    PTRef b;

    PTRef v0;
    PTRef v1;

    SymRef f_sym;

    std::unique_ptr<Model> getModel() {
        ModelBuilder mb(logic);
        Model::Evaluation eval {
                std::make_pair(x, v0),
                std::make_pair(y, v0),
                std::make_pair(z, v1),
                std::make_pair(a, logic.getTerm_true()),
                std::make_pair(b, logic.getTerm_false()),
        };
        Model::SymbolDefinition symDef {
                std::make_pair(f_sym, TemplateFunction("f", {x, y}, S, logic.mkIte(logic.mkEq(x, v1), v0, v1)))
        };
        mb.addVarValues(eval.begin(), eval.end());
        mb.addFunctionDefinitions(symDef.begin(), symDef.end());
        return mb.build();
    }
};

TEST_F(UFModelTest, test_varAndFunctionEvaluation) {
    auto model = getModel();
    EXPECT_EQ(model->evaluate(x), v0);
    EXPECT_EQ(model->evaluate(y), v0);
    EXPECT_EQ(model->evaluate(z), v1);
    EXPECT_EQ(model->evaluate(f), v1);
    EXPECT_EQ(model->evaluate(logic.mkUninterpFun(f_sym, {logic.mkUninterpFun(f_sym, {x, y}), x})), v0);
    EXPECT_EQ(model->evaluate(a), logic.getTerm_true());
    EXPECT_EQ(model->evaluate(b), logic.getTerm_false());
}

TEST_F(UFModelBuilderTest, test_modelBuilderVarAndFunction) {
    auto model = getModel();
    EXPECT_EQ(model->evaluate(x), v0);
    EXPECT_EQ(model->evaluate(y), v0);
    EXPECT_EQ(model->evaluate(z), v1);
    EXPECT_EQ(model->evaluate(f), v1);
    EXPECT_EQ(model->evaluate(logic.mkUninterpFun(f_sym, {logic.mkUninterpFun(f_sym, {x, y}), x})), v0);
    EXPECT_EQ(model->evaluate(a), logic.getTerm_true());
    EXPECT_EQ(model->evaluate(b), logic.getTerm_false());
}

TEST_F(UFModelBuilderTest, test_NameCollision) {
    ModelBuilder mb(logic);
    char* err;
    std::string symName("x0");
    SymRef x1_sym = logic.declareFun(symName.c_str(), S, {S}, &err);
    mb.addToTheoryFunction(x1_sym, {v0}, v0);
    auto m = mb.build();
    auto templateFun = m->getDefinition(x1_sym);
    std::cout << "testing name collision: the following should be different from `x0`" << std::endl;
    for (PTRef tr : templateFun.getArgs()) {
        std::string argName(logic.getSymName(tr));
        std::cout << argName << std::endl;
        ASSERT_NE(argName, symName);
    }
}

class LAModelTest : public ::testing::Test {
protected:
    LAModelTest(): logic{} {}
    virtual void SetUp() {
        x = logic.mkNumVar("x");
        y = logic.mkNumVar("y");
        z = logic.mkNumVar("z");
        a = logic.mkBoolVar("a");
        b = logic.mkBoolVar("b");
    }
    SMTConfig config;
    LRALogic logic;
    PTRef x;
    PTRef y;
    PTRef z;
    PTRef a;
    PTRef b;

    std::unique_ptr<Model> getModel() {
        Model::Evaluation eval {
                std::make_pair(x, logic.getTerm_NumOne()),
                std::make_pair(y, logic.getTerm_NumZero()),
                std::make_pair(z, logic.getTerm_NumOne()),
                std::make_pair(a, logic.getTerm_true()),
                std::make_pair(b, logic.getTerm_false())
        };
        return std::unique_ptr<Model>(new Model(logic, eval));
    }

};



TEST_F(LAModelTest, test_inputVarValues) {
    auto model = getModel();
    EXPECT_EQ(model->evaluate(x), logic.getTerm_NumOne());
    EXPECT_EQ(model->evaluate(y), logic.getTerm_NumZero());
    EXPECT_EQ(model->evaluate(z), logic.getTerm_NumOne());
    EXPECT_EQ(model->evaluate(a), logic.getTerm_true());
    EXPECT_EQ(model->evaluate(b), logic.getTerm_false());
}

TEST_F(LAModelTest, test_constants) {
    auto model = getModel();
    PTRef tval = logic.getTerm_true();
    PTRef fval = logic.getTerm_false();
    EXPECT_EQ(model->evaluate(fval), fval);
    EXPECT_EQ(model->evaluate(tval), tval);

    PTRef fortytwo = logic.mkConst(42);
    PTRef one = logic.mkConst(1);
    PTRef zero = logic.mkConst(opensmt::Real(0));
    EXPECT_EQ(model->evaluate(fortytwo), fortytwo);
    EXPECT_EQ(model->evaluate(one), logic.getTerm_NumOne());
    EXPECT_EQ(model->evaluate(zero), logic.getTerm_NumZero());
}

TEST_F(LAModelTest, test_derivedBooleanTerms) {
    auto model = getModel();
    PTRef tval = logic.getTerm_true();
    PTRef fval = logic.getTerm_false();

    PTRef and_false = logic.mkAnd(a,b);
    PTRef or_true = logic.mkOr(a,b);
    PTRef not_true = logic.mkNot(b);
    PTRef not_false = logic.mkNot(a);
    EXPECT_EQ(model->evaluate(and_false), fval);
    EXPECT_EQ(model->evaluate(or_true), tval);
    EXPECT_EQ(model->evaluate(not_true), tval);
    EXPECT_EQ(model->evaluate(not_false), fval);
}

TEST_F(LAModelTest, test_derivedArithmeticTerms) {
    auto model = getModel();
    PTRef two = logic.mkConst(2);
    PTRef five = logic.mkConst(5);
    vec<PTRef> args {x,z};
    EXPECT_EQ(model->evaluate(logic.mkNumPlus(args)), two);
    EXPECT_EQ(model->evaluate(logic.mkNumTimes(five, x)), five);
    EXPECT_EQ(model->evaluate(logic.mkNumTimes(five, y)), logic.getTerm_NumZero());
}

TEST_F(LAModelTest, test_derivedArithmeticAtoms) {
    auto model = getModel();
    PTRef tval = logic.getTerm_true();
    PTRef fval = logic.getTerm_false();

    EXPECT_EQ(model->evaluate(logic.mkNumLeq(x, y)), fval);
    EXPECT_EQ(model->evaluate(logic.mkNumLt(x, y)), fval);
    EXPECT_EQ(model->evaluate(logic.mkNumGeq(x, y)), tval);
    EXPECT_EQ(model->evaluate(logic.mkNumGt(x, y)), tval);

    EXPECT_EQ(model->evaluate(logic.mkNumLeq(y, z)), tval);
    EXPECT_EQ(model->evaluate(logic.mkNumLt(y, z)), tval);
    EXPECT_EQ(model->evaluate(logic.mkNumGeq(y, z)), fval);
    EXPECT_EQ(model->evaluate(logic.mkNumGt(y, z)), fval);

    EXPECT_EQ(model->evaluate(logic.mkNumLeq(x, z)), tval);
    EXPECT_EQ(model->evaluate(logic.mkNumLt(x, z)), fval);
    EXPECT_EQ(model->evaluate(logic.mkNumGeq(x, z)), tval);
    EXPECT_EQ(model->evaluate(logic.mkNumGt(x, z)), fval);

}


class ModelIntegrationTest : public ::testing::Test {
protected:
    ModelIntegrationTest() {}
    std::unique_ptr<Opensmt> getLRAOsmt() {
        return std::unique_ptr<Opensmt>(new Opensmt(opensmt_logic::qf_lra, "test"));
    }

    std::unique_ptr<Opensmt> getUFOsmt() {
        return std::unique_ptr<Opensmt>(new Opensmt(opensmt_logic::qf_uf, "test"));
    }

};

TEST_F(ModelIntegrationTest, testSingleAssert) {
    auto osmt = getLRAOsmt();
    LRALogic& logic = osmt->getLRALogic();
    PTRef x = logic.mkNumVar("x");
    PTRef y = logic.mkNumVar("y");
    PTRef fla = logic.mkNumLt(x, y);
    MainSolver& mainSolver = osmt->getMainSolver();
    mainSolver.insertFormula(fla);
    sstat res = mainSolver.check();
    ASSERT_EQ(res, s_True);
    auto model = mainSolver.getModel();
    EXPECT_EQ(model->evaluate(fla), logic.getTerm_true());
}

TEST_F(ModelIntegrationTest, testSubstitutions) {
    auto osmt = getLRAOsmt();
    Logic& logic = osmt->getLogic();
    PTRef x = logic.mkBoolVar("x");
    PTRef y = logic.mkBoolVar("y");
    PTRef fla = logic.mkEq(x, logic.mkNot(y));
    MainSolver& mainSolver = osmt->getMainSolver();
    mainSolver.insertFormula(fla);
    sstat res = mainSolver.check();
    ASSERT_EQ(res, s_True);
//    auto xval = mainSolver.getValue(x);
//    auto yval = mainSolver.getValue(y);
//    std::cout << logic.printTerm(xval.tr) << " : " << xval.val << '\n';
//    std::cout << logic.printTerm(yval.tr) << " : " << yval.val << std::endl;
    auto model = mainSolver.getModel();
    EXPECT_EQ(model->evaluate(fla), logic.getTerm_true());
}

TEST_F(ModelIntegrationTest, testTrivialBooleanSubstitutionNegation) {
    auto osmt = getLRAOsmt();
    Logic& logic = osmt->getLogic();
    PTRef x = logic.mkBoolVar("x");
    PTRef fla = logic.mkNot(x);
    MainSolver& mainSolver = osmt->getMainSolver();
    mainSolver.insertFormula(fla);
    sstat res = mainSolver.check();
    ASSERT_EQ(res, s_True);
    auto model = mainSolver.getModel();
    EXPECT_EQ(model->evaluate(x), logic.getTerm_false());
    EXPECT_EQ(model->evaluate(fla), logic.getTerm_true());
}

TEST_F(ModelIntegrationTest, testIteWithSubstitution) {
    auto osmt = getLRAOsmt();
    LRALogic& logic = osmt->getLRALogic();
    PTRef x = logic.mkNumVar("x");
    PTRef y = logic.mkNumVar("y");
    PTRef ite = logic.mkIte(logic.mkBoolVar("c"), x, logic.mkNumPlus(x, logic.getTerm_NumOne()));
    PTRef fla = logic.mkEq(y, ite);
    MainSolver & mainSolver = osmt->getMainSolver();
    mainSolver.insertFormula(fla);
    auto res = mainSolver.check();
    ASSERT_EQ(res, s_True);
    auto model = mainSolver.getModel();
    EXPECT_EQ(model->evaluate(fla), logic.getTerm_true());
}

TEST_F(ModelIntegrationTest, testIteWithSubstitution_SubtermsHaveValue) {
    auto osmt = getLRAOsmt();
    LRALogic& logic = osmt->getLRALogic();
    PTRef x = logic.mkNumVar("x");
    PTRef y = logic.mkNumVar("y");
    PTRef c = logic.mkBoolVar("c");
    PTRef one = logic.getTerm_NumOne();
    PTRef ite = logic.mkIte(c, x, logic.mkNumPlus(x, one));
    PTRef eq = logic.mkEq(y, ite);
    PTRef fla = logic.mkAnd({eq, logic.mkNumLeq(one, y), logic.mkNumLeq(x, logic.getTerm_NumZero())});
    MainSolver & mainSolver = osmt->getMainSolver();
    mainSolver.insertFormula(fla);
    auto res = mainSolver.check();
    ASSERT_EQ(res, s_True);
    auto model = mainSolver.getModel();
//    std::cout << logic.printTerm(x) << " := " << logic.printTerm(model->evaluate(x)) << '\n';
//    std::cout << logic.printTerm(y) << " := " << logic.printTerm(model->evaluate(y)) << '\n';
//    std::cout << logic.printTerm(c) << " := " << logic.printTerm(model->evaluate(c)) << '\n';
    EXPECT_EQ(model->evaluate(fla), logic.getTerm_true());
    EXPECT_EQ(model->evaluate(c), logic.getTerm_false());
}


