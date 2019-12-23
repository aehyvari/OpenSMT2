#include <gtest/gtest.h>
#include <Real.h>
#include <stdlib.h>
#include <Vec.h>
#include <Sort.h>
#include <SMTConfig.h>
#include <lasolver/Simplex.h>
#include <lasolver/LASolver.h>


TEST(LIACutSolver_test, test_computeEqualityBasis)
{

    SMTConfig c;
    LAVarStore vs;


    //our system
    //2x-y-3<=0;
    //x+2y-4<=0;
    // x>=2

    LVRef x = vs.getNewVar();
    LVRef y = vs.getNewVar();

    //2x-y
    LVRef two_x_minus_y = vs.getNewVar();
    auto p_two_x_minus_y = std::unique_ptr<Polynomial>(new Polynomial());

    p_two_x_minus_y->addTerm(x, 2);
    p_two_x_minus_y->addTerm(y, -1);

    //cout << p_y_minus_x << endl;
    ASSERT_TRUE(p_two_x_minus_y->contains(x));
    ASSERT_TRUE(p_two_x_minus_y->contains(y));
    EXPECT_EQ(p_two_x_minus_y->getCoeff(x), 2);
    EXPECT_EQ(p_two_x_minus_y->getCoeff(y), -1);

    //x+2y
    LVRef x_plus_two_y = vs.getNewVar();
    auto p_x_plus_two_y = std::unique_ptr<Polynomial>(new Polynomial());

    p_x_plus_two_y->addTerm(x, 1);
    p_x_plus_two_y->addTerm(y, 2);

    ASSERT_TRUE(p_x_plus_two_y->contains(x));
    ASSERT_TRUE(p_x_plus_two_y->contains(y));
    EXPECT_EQ(p_x_plus_two_y->getCoeff(x), 1);
    EXPECT_EQ(p_x_plus_two_y->getCoeff(y), 2);

    //-x
    LVRef minus_x = vs.getNewVar();
    auto p_minus_x = std::unique_ptr<Polynomial>(new Polynomial());

    p_minus_x->addTerm(x, -1);
    //p_minus_x->addTerm(y, 0);

    ASSERT_TRUE(p_minus_x->contains(x));
    //ASSERT_TRUE(p_minus_x->contains(y));
    //EXPECT_EQ(p_minus_x->getCoeff(y), 0);
    EXPECT_EQ(p_minus_x->getCoeff(x), -1);

    LABoundStore bs(vs);

    LABoundStore::BoundInfo two_x_minus_y_nostrict_3 = bs.allocBoundPair(two_x_minus_y, 3, false);  // 2x - y - 3 <= 0
    LABoundStore::BoundInfo x_plus_two_y_nostrict_4 = bs.allocBoundPair(x_plus_two_y, 4, false);    // x + 2y -4 <= 0
    LABoundStore::BoundInfo minus_x_nostrict_m2 = bs.allocBoundPair(minus_x, -2, false); // -x <= -2

    bs.buildBounds();
    int t = bs.getBoundListSize(x_plus_two_y);
    cout << t << endl; //why 4? what this gives? minus plus infinit, and upper lower bounds
    int t1 = bs.nVars();
    cout << t1 << endl; //why 5? all vs.getnewVar
    char* t2 = bs.printBounds(x_plus_two_y);
    cout << t2 << endl;
    free(t2); //deallocate space taken by char*

    //EXPECT_EQ(bs.size(), 0);

    ///TODO get a constraint system that reflects our planes graph and work out examples of first case and second case

    //Simplex s(c, m, bs);
    auto s = std::unique_ptr<Simplex>(new Simplex(bs));

    s->newNonbasicVar(x);
    s->newNonbasicVar(y);
    s->newRow(two_x_minus_y, std::move(p_two_x_minus_y));
    s->newRow(x_plus_two_y, std::move(p_x_plus_two_y));
    s->newRow(minus_x, std::move(p_minus_x));

    s->initModel();

    //1 case: if all have non-strict bounds, and upper bound in this case, then it is SAT with single solution

    s->assertBoundOnVar(two_x_minus_y, two_x_minus_y_nostrict_3.lb); //PS. enabling bounds, makes bounds active
    s->assertBoundOnVar(x_plus_two_y, x_plus_two_y_nostrict_4.lb);
    s->assertBoundOnVar(minus_x, minus_x_nostrict_m2.lb);

    //2 case:  if at least one of them has strict bound, then UNSAT, where expalantion says that it implied from others

    Simplex::Explanation explanation = s->checkSimplex();
    ASSERT_EQ(explanation.size(), 0); //this property has to be failed as the system is UNSAT then explanation size has to be >0

}


/*
 *
vec<PtAsgn> ex;
ex.clear();
bool complete;
ex = s->check_simplex(complete);
ASSERT_EQ(ex.size(), 0);

Real d = s->computeDelta();
Delta x_val = s.getValuation(x);
cout << x_val.R() + x_val.D() * d << endl;

Delta y_val = s.getValuation(y);
cout << y_val.R() + y_val.D() * d << endl;

Delta sum = s.getValuation(y_minus_x);
cout << sum.R() + sum.D() * d << endl;

//call initialize function and check delta part to see if initilize did its job

///up to now we created vars, polynomial, checked simplex on them and if simplex not UNSAT then compute equal basis

std::unique_ptr<LRAModel> eqBasisModel = model.copyFlat(); // copy the model as a snapshot of the most recent values
auto eqBasisSimplex = std::unique_ptr<Simplex>(new Simplex(config, *eqBasisModel, boundStore));
eqBasisSimplex->initFromSimplex(s);  // Initialize the new simplex (mostly tableau) from the old simplex

//removes all bounds that cannot produce equalities and turns Ax<=b to Ax<b
initialize(*eqBasisSimplex, *eqBasisModel);

//if delta part is zero it means <=, otherwise <. Hence call simplex to solve it

std::vector<LABoundRef> explanationBounds;
eqBasisSimplex->checkSimplex(explanationBounds, explanationCoefficients);

//size of explanation now has to increase, should not be zero

d = eqBasisSimplex->computeDelta();
Delta x_val = eqBasisSimplex.getValuation(x);
cout << x_val.R() + x_val.D() * d << endl;
cout << x_val.D() << endl;

Delta y_val = eqBasisSimplex.getValuation(y);
cout << y_val.R() + y_val.D() * d << endl;
cout << y_val.D() << endl;

Delta sum = eqBasisSimplex.getValuation(y_minus_x);
cout << sum.R() + sum.D() * d << endl;
cout << sum.D() << endl;

//1. give an example of inequallity system that implies equality
//2. give it bounds
//3. initialize removes all bounds that cannot produce equalities and turns Ax<=b to Ax<b
//4. ckeck simplex on Ax<b if Ax<b is UNSAT then AX<=b implies equality
*/

