/*********************************************************************
Author: Antti Hyvarinen <antti.hyvarinen@gmail.com>

OpenSMT2 -- Copyright (C) 2012 - 2014 Antti Hyvarinen

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*********************************************************************/


#include "Interpret.h"
#include "smt2tokens.h"
#include "MainSolver.h"
#include "ArithLogic.h"
#include "MainSplitter.h"
#include "LogicFactory.h"

#include <string>
#include <sstream>
#include <cstdarg>

/***********************************************************
 * Class defining interpreter
 ***********************************************************/

Interpret::~Interpret() {
    for (int i = 0; i < term_names.size(); ++i) {
        free(term_names[i]);
    }
}

PTRef
Interpret::getParsedFormula()
{
    PTRef root = main_solver->getLogic().mkAnd(assertions);
    return root;
}

void Interpret::setInfo(ASTNode& n) {
    assert(n.getType() == UATTR_T || n.getType() == PATTR_T);

    char* name = NULL;
    if (n.getValue() == NULL) {
        ASTNode& an = **(n.children->begin());
        assert(an.getType() == UATTR_T || an.getType() == PATTR_T);
        name = strdup(an.getValue());
    }
    else // Normal option
        name = strdup(n.getValue());

    Info value(n);

    config.setInfo(name, value);
    free(name);
}

void Interpret::getInfo(ASTNode& n) {
    assert(n.getType() == INFO_T);

    const char* name;

    if (n.getValue() != NULL)
        name = n.getValue();
    else {
        assert(n.children != NULL);
        name = (**(n.children->begin())).getValue();
    }

    const Info& value = config.getInfo(name);

    if (value.isEmpty())
        notify_formatted(true, "no value for info %s", name);
    else {
        char* val_str = value.toString();
        notify_formatted(false, "%s %s", name, val_str);
        free(val_str);
    }
}

void Interpret::setOption(ASTNode& n) {
    assert(n.getType() == OPTION_T);
    char* name  = NULL;

    if (n.getValue() == NULL)  {
        // Attribute
        ASTNode& an = **(n.children->begin());
        assert(an.getType() == UATTR_T || an.getType() == PATTR_T);
        name = strdup(an.getValue());
    }
    else // Normal option
        name = strdup(n.getValue());

    SMTOption value(n);
    const char* msg = "ok";
    bool rval = config.setOption(name, value, msg);
    if (rval == false)
        notify_formatted(true, "set-option failed for %s: %s", name, msg);
    free(name);
}

void Interpret::getOption(ASTNode& n) {
    assert(n.getType() == UATTR_T || n.getType() == PATTR_T );

    assert(n.getValue() != NULL);
    const char* name = n.getValue();

    const SMTOption& value = config.getOption(name);

    if (value.isEmpty())
        notify_formatted(true, "No value for attr %s", name);
    else {
        char* str_val = value.toString();
        notify_formatted(false, "%s",str_val);
        free(str_val);
    }
}

void Interpret::exit() {
    f_exit = true;
}

using opensmt::Logic_t;
using opensmt::getLogicFromString;
using namespace osmttokens;

void Interpret::interp(ASTNode& n) {
    assert(n.getType() == CMD_T);
    const smt2token cmd = n.getToken();
    try {
        switch (cmd.x) {
            case t_setlogic: {
                ASTNode &logic_n = **(n.children->begin());
                const char *logic_name = logic_n.getValue();
                if (isInitialized()) {
                    notify_formatted(true, "logic has already been set to %s", main_solver->getLogic().getName().data());
                } else {
                    auto logic_type = getLogicFromString(logic_name);
                    if (logic_type == Logic_t::UNDEF) {
                        notify_formatted(true, "unknown logic %s", logic_name);
                        break;
                    }
                    initializeLogic(logic_type);
                    main_solver.reset(&createMainSolver(config, logic_name));
                    main_solver->initialize();
                    notify_success();
                }
                break;
            }
            case t_setinfo: {
                setInfo(**(n.children->begin()));
                notify_success();
                break;
            }
            case t_getinfo: {
                getInfo(**(n.children->begin()));
                break;
            }
            case t_setoption: {
                setOption(**(n.children->begin()));
                notify_success();
                break;
            }
            case t_getoption: {
                getOption(**(n.children->begin()));
                break;
            }
            case t_declaresort: {
                if (isInitialized()) {
                    assert(n.children and n.children->size() == 2);
                    auto it = n.children->begin();
                    ASTNode & symbolNode = **it;
                    assert(symbolNode.getType() == SYM_T);
                    ++it;
                    ASTNode & numNode = **it;
                    assert(numNode.getType() == NUM_T);
                    int arity = atoi(numNode.getValue()); // MB: TODO: take care of out-of-range input
                    SortSymbol symbol(symbolNode.getValue(), arity);
                    SSymRef ssref;
                    if (logic->peekSortSymbol(symbol, ssref)) {
                        notify_formatted(true, "sort %s already declared", symbolNode.getValue());
                    } else {
                        logic->declareSortSymbol(std::move(symbol));
                        notify_success();
                    }
                } else
                    notify_formatted(true, "illegal command before set-logic: declare-sort");
                break;
            }
            case t_declarefun: {
                if (isInitialized()) {
                    if (declareFun(n))
                        notify_success();
                } else
                    notify_formatted(true, "Illegal command before set-logic: declare-fun");
                break;
            }
            case t_declareconst: {
                if (isInitialized()) {
                    declareConst(n);
                } else
                    notify_formatted(true, "Illegal command before set-logic: declare-const");
                break;
            }
            case t_assert: {
                if (isInitialized()) {
                    sstat status;
                    ASTNode &asrt = **(n.children->begin());
                    LetRecords letRecords;
                    PTRef tr = parseTerm(asrt, letRecords);
                    if (tr == PTRef_Undef)
                        notify_formatted(true, "assertion returns an unknown sort");
                    else {
                        assertions.push(tr);
                        char *err_msg = NULL;
                        status = main_solver->insertFormula(tr, &err_msg);

                        if (status == s_Error)
                            notify_formatted(true, "Error");
                        else if (status == s_Undef)
                            notify_success();
                        else if (status == s_False)
                            notify_success();

                        if (err_msg != NULL && status == s_Error)
                            notify_formatted(true, err_msg);
                        if (err_msg != NULL && status != s_Error)
                            comment_formatted(err_msg);
                        free(err_msg);
                    }
                } else {
                    notify_formatted(true, "Illegal command before set-logic: assert");
                }
                break;
            }
            case t_definefun: {
                if (isInitialized()) {
                    defineFun(n);
                } else {
                    notify_formatted(true, "Illegal command before set-logic: define-fun");
                }
                break;
            }
            case t_simplify: {
                sstat status = main_solver->simplifyFormulas();
                if (status == s_Error)
                    notify_formatted(true, "Simplify");
                break;
            }
            case t_checksat: {
                checkSat();
                break;
            }
            case t_getinterpolants: {
                if (config.produce_inter()) {
                    getInterpolants(n);
                } else {
                    notify_formatted(true,
                                     "Option to produce interpolants has not been set, skipping this command ...");
                }
                break;
            }
            case t_getassignment: {
                getAssignment();
                break;
            }
            case t_getvalue: {
                getValue(n.children);
                break;
            }
            case t_getmodel: {
                if (not isInitialized()) {
                    notify_formatted(true, "Illegal command before set-logic: get-model");
                } else if (main_solver->getStatus() != s_True) {
                    notify_formatted(true, "Command get-model called, but solver is not in SAT state");
                } else {
                    getModel();
                }
                break;
            }

            case t_writestate: {
                if (main_solver->solverEmpty()) {
                    sstat status = main_solver->simplifyFormulas();
                    if (status == s_Error)
                        notify_formatted(true, "write-state");
                }
                writeState((**(n.children->begin())).getValue());
                break;
            }
            case t_writefuns: {
                const char *filename = (**(n.children->begin())).getValue();
                main_solver->writeFuns_smtlib2(filename);
                break;
            }
            case t_echo: {
                const char *str = (**(n.children->begin())).getValue();
                notify_formatted(false, "%s", str);
                break;
            }
            case t_push: {
                std::string str((**(n.children->begin())).getValue());
                try {
                    int num = std::stoi(str);
                    if (num < 0) {
                        notify_formatted(true, "Incorrect push command, value is negative.");
                        break;
                    }
                    bool success = true;
                    while (num-- and success) { success = push(); }
                    if (success) { notify_success(); }
                }
                catch (std::out_of_range const &ex) {
                    notify_formatted(true, "Incorrect push command, the value is out of range.");
                }
                break;
            }
            case t_pop: {
                std::string str((**(n.children->begin())).getValue());
                try {
                    int num = std::stoi(str);
                    if (num < 0) {
                        notify_formatted(true, "Incorrect pop command, value is negative.");
                        break;
                    }
                    bool success = true;
                    while (num-- and success) { success = pop(); }
                    if (success) { notify_success(); }
                }
                catch (std::out_of_range const &ex) {
                    notify_formatted(true, "Incorrect pop command, the value is out of range.");
                }
                break;
            }
            case t_exit: {
                exit();
                notify_success();
                break;
            }
            default: {
                notify_formatted(true, "Unknown command encountered: %s", tokenToName.at(cmd.x).c_str());
            }
        }
    } catch (OsmtApiException const &e) {
        notify_formatted(true, e.what());
    }
}


bool Interpret::addLetFrame(const vec<char *> & names, vec<PTRef> const& args, LetRecords& letRecords) {
    assert(names.size() == args.size());
    if (names.size() > 1) {
        // check that they are pairwise distinct;
        std::unordered_set<const char*, StringHash, Equal<const char*>> namesAsSet(names.begin(), names.end());
        if (namesAsSet.size() != names.size_()) {
            comment_formatted("Overloading let variables makes no sense");
            return false;
        }
    }
    for (int i = 0; i < names.size(); ++i) {
        const char* name = names[i];
        if (logic->hasSym(name) && logic->getSym(logic->symNameToRef(name)[0]).noScoping()) {
            comment_formatted("Names marked as no scoping cannot be overloaded with let variables: %s", name);
            return false;
        }
        letRecords.addBinding(name, args[i]);
    }
    return true;
}

//
// Determine whether the term refers to some let definition
//
PTRef Interpret::letNameResolve(const char* s, const LetRecords& letRecords) const {
    return letRecords.getOrUndef(s);
}


PTRef Interpret::parseTerm(const ASTNode& term, LetRecords& letRecords) {
    ASTType t = term.getType();
    if (t == TERM_T) {
        const char* name = (**(term.children->begin())).getValue();
//        comment_formatted("Processing term %s", name);
        vec<SymRef> params;
        //PTRef tr = logic->resolveTerm(name, vec_ptr_empty, &msg);
        PTRef tr = PTRef_Undef;
        try {
            tr = logic->mkConst(name);
        } catch (OsmtApiException const & e) {
            comment_formatted("While processing %s: %s", name, e.what());
        }
        return tr;
    }

    else if (t == QID_T) {
        const char* name = (**(term.children->begin())).getValue();
//        comment_formatted("Processing term with symbol %s", name);
        PTRef tr = letNameResolve(name, letRecords);
        char* msg = NULL;
        if (tr != PTRef_Undef) {
//            comment_formatted("Found a let reference to term %d", tr);
            return tr;
        }
        tr = logic->resolveTerm(name, {}, &msg);
        if (tr == PTRef_Undef)
            comment_formatted("unknown qid term %s: %s", name, msg);
        free(msg);
        return tr;
    }

    else if ( t == LQID_T ) {
        // Multi-argument term
        auto node_iter = term.children->begin();
        vec<PTRef> args;
        const char* name = (**node_iter).getValue(); node_iter++;
        // Parse the arguments
        for (; node_iter != term.children->end(); node_iter++) {
            PTRef arg_term = parseTerm(**node_iter, letRecords);
            if (arg_term == PTRef_Undef)
                return PTRef_Undef;
            else
                args.push(arg_term);
        }
        assert(args.size() > 0);
        char* msg = NULL;
        PTRef tr = PTRef_Undef;
        try {
            tr = logic->resolveTerm(name, std::move(args), &msg);
        }
        catch (ArithDivisionByZeroException & ex) {
            notify_formatted(true, ex.what());
            return PTRef_Undef;
        }
        if (tr == PTRef_Undef) {
            notify_formatted(true, "No such symbol %s: %s", name, msg);
            comment_formatted("The symbol %s is not defined for the given sorts", name);
            if (logic->hasSym(name)) {
                comment_formatted("candidates are:");
                const vec<SymRef>& trefs = logic->symNameToRef(name);
                for (int j = 0; j < trefs.size(); j++) {
                    SymRef ctr = trefs[j];
                    const Symbol& t = logic->getSym(ctr);
                    comment_formatted(" candidate %d", j);
                    for (uint32_t k = 0; k < t.nargs(); k++) {
                        comment_formatted("  arg %d: %s", k, logic->printSort(t[k]).c_str());
                    }
                }
            }
            else
                comment_formatted("There are no candidates.");
            free(msg);
            return PTRef_Undef;
        }


        return tr;
    }

    else if (t == LET_T) {
        auto ch = term.children->begin();
        auto vbl = (**ch).children->begin();
        vec<PTRef> tmp_args;
        vec<char*> names;
        // use RAII idiom to guard the scope of new LetFrame (and ensure the cleaup of names)
        class Guard {
            LetRecords& rec;
            vec<char*>& names;
        public:
            Guard(LetRecords& rec, vec<char*>& names): rec(rec), names(names) { rec.pushFrame(); }
            ~Guard() { rec.popFrame(); for (int i = 0; i < names.size(); i++) { free(names[i]); }}
        } scopeGuard(letRecords, names);
        // First read the term declarations in the let statement
        while (vbl != (**ch).children->end()) {
            PTRef let_tr = parseTerm(**((**vbl).children->begin()), letRecords);
            if (let_tr == PTRef_Undef) return PTRef_Undef;
            tmp_args.push(let_tr);
            char* name = strdup((**vbl).getValue());
            names.push(name);
            vbl++;
        }
        // Only then insert them to the table
        bool success = addLetFrame(names, tmp_args, letRecords);
        if (not success) {
            comment_formatted("Let name addition failed");
            return PTRef_Undef;
        }
        ch++;
        // This is now constructed with the let declarations context in let_branch
        PTRef tr = parseTerm(**(ch), letRecords);
        if (tr == PTRef_Undef) {
            comment_formatted("Failed in parsing the let scoped term");
            return PTRef_Undef;
        }
        return tr;
    }

    else if (t == BANG_T) {
        assert(term.children->size() == 2);
        auto ch = term.children->begin();
        ASTNode& named_term = **ch;
        ASTNode& attr_l = **(++ ch);
        assert(attr_l.getType() == GATTRL_T);
        assert(attr_l.children->size() == 1);
        ASTNode& name_attr = **(attr_l.children->begin());

        PTRef tr = parseTerm(named_term, letRecords);
        if (tr == PTRef_Undef) return tr;

        if (strcmp(name_attr.getValue(), ":named") == 0) {
            ASTNode& sym = **(name_attr.children->begin());
            assert(sym.getType() == SYM_T);
            if (nameToTerm.has(sym.getValue())) {
                notify_formatted(true, "name %s already exists", sym.getValue());
                return PTRef_Undef;
            }
            char* name = strdup(sym.getValue());
            // MB: term_names becomes the owner of the string and is responsible for deleting
            term_names.push(name);
            nameToTerm.insert(name, tr);
            if (!termToNames.has(tr)) {
                vec<const char*> v;
                termToNames.insert(tr, v);
            }
            termToNames[tr].push(name);
        }
        return tr;
    }
    else
        comment_formatted("Unknown term type");
    return PTRef_Undef;
}

bool Interpret::checkSat() {
    sstat res;
    if (isInitialized()) {
        res = main_solver->check();

        if (res == s_True) {
            notify_formatted(false, "sat");
        }
        else if (res == s_False)
            notify_formatted(false, "unsat");
        else
            notify_formatted(false, "unknown");

        const Info& status = config.getInfo(":status");
        if (!status.isEmpty()) {
            std::string statusString(std::move(status.toString()));
            if ((statusString.compare("sat") == 0) && (res == s_False)) {
                notify_formatted(false, "(error \"check status which says sat\")");

            }
            else if ((statusString.compare("unsat") == 0) && (res == s_True)) {
                notify_formatted(false, "(error \"check status which says unsat\")");
            }
        }
    }
    else {
        notify_formatted(true, "Illegal command before set-logic: check-sat");
        return false;
    }
    if (res == s_Undef) {
        const SMTOption& o_dump_state = config.getOption(":dump-state");
        const SpType o_split = config.sat_split_type();
        char* name = config.dump_state();
        if (!o_dump_state.isEmpty() && o_split == spt_none)
            writeState(name);
        else if (o_split != spt_none and strcmp(config.output_dir(),"") != 0) {
            writeSplits_smtlib2(name);
        }
        free(name);
    }

    return true;
}

bool Interpret::push()
{
    if (config.isIncremental()) {
        main_solver->push();
        return true;
    }
    else {
        notify_formatted(true, "push encountered but solver not in incremental mode");
        return false;
    }
}

bool Interpret::pop()
{
    if (config.isIncremental()) {
        if (main_solver->pop())
            return true;
        else {
            notify_formatted(true, "Attempt to pop beyond the top of the stack");
            return false;
        }
    }
    else {
        notify_formatted(true, "pop encountered but solver not in incremental mode");
        return false;
    }
}

bool Interpret::getAssignment() {
    if (not isInitialized()) {
       notify_formatted(true, "Illegal command before set-logic");
       return false;
    }
    if (main_solver->getStatus() != s_True) {
       notify_formatted(true, "Last solver call not satisfiable");
       return false;
    }
    std::stringstream ss;
    ss << '(';
    for (int i = 0; i < term_names.size(); i++) {
        const char* name = term_names[i];
        PTRef tr = nameToTerm[name];
        lbool val = main_solver->getTermValue(tr);
        ss << '(' << name << ' ' << (val == l_True ? "true" : (val == l_False ? "false" : "unknown"))
            << ')' << (i < term_names.size() - 1 ? " " : "");
    }
    ss << ')';
    const std::string& out = ss.str();
    notify_formatted(false, out.c_str());
    return true;
}

void Interpret::getValue(const std::vector<ASTNode*>* terms)
{
    auto model = main_solver->getModel();
    Logic& logic = main_solver->getLogic();
    std::vector<opensmt::pair<PTRef,PTRef>> values;
    for (auto term_it = terms->begin(); term_it != terms->end(); ++term_it) {
        const ASTNode& term = **term_it;
        LetRecords tmp;
        PTRef tr = parseTerm(term, tmp);
        if (tr != PTRef_Undef) {
            values.emplace_back(opensmt::pair<PTRef,PTRef>{tr, model->evaluate(tr)});
            char* pt_str = logic.printTerm(tr);
            comment_formatted("Found the term %s", pt_str);
            free(pt_str);
        } else
            comment_formatted("Error parsing the term %s", (**(term.children->begin())).getValue());
    }
    printf("(");
    for (auto const & valPair : values) {
        char* name = logic.printTerm(valPair.first);
        char* value = logic.printTerm(valPair.second);
        printf("(%s %s)", name, value);
        free(name);
        free(value);
    }
    printf(")\n");
}

void Interpret::getModel() {

    auto model = main_solver->getModel();
    std::stringstream ss;
    ss << "(\n";
    for (int i = 0; i < user_declarations.size(); ++i) {
        SymRef symref = user_declarations[i];
        const Symbol & sym = logic->getSym(symref);
        if (sym.size() == 1) {
            // variable, just get its value
            const char* s = logic->getSymName(symref);
            SRef symSort = sym.rsort();
            PTRef term = logic->mkVar(symSort, s);
            PTRef val = model->evaluate(term);
            ss << printDefinitionSmtlib(term, val);
        }
        else {
            // function
            const TemplateFunction & templ = model->getDefinition(symref);
            ss << printDefinitionSmtlib(templ);
        };
    }
    ss << ')';
    std::cout << ss.str() << std::endl;
}

/**
 *
 * @param tr the term to print
 * @param val its value
 * @return the term value in an smtlib2 compliant format
 * Example:
 * (; U is sort of cardinality 2
 *   (define-fun a () U
 *     (as @0 U))
 *   (define-fun b () U
 *     (as @1 U))
 *   (define-fun f ((x U)) U
 *     (ite (= x (as @1 U)) (as @0 U)
 *       (as @1 U))
 *   )
 * )
 */
std::string Interpret::printDefinitionSmtlib(PTRef tr, PTRef val) {
    std::stringstream ss;
    const char *s = logic->protectName(logic->getSymName(tr));
    SRef sortRef = logic->getSym(tr).rsort();
    ss << "  (define-fun " << s << " () " << logic->printSort(sortRef) << '\n';
    char* val_string = logic->printTerm(val);
    ss << "    " << val_string << ")\n";
    free(val_string);
    return ss.str();
}

std::string Interpret::printDefinitionSmtlib(const TemplateFunction & templateFun) const {
    std::stringstream ss;
    ss << "  (define-fun " << templateFun.getName() << " (";
    const vec<PTRef>& args = templateFun.getArgs();
    for (int i = 0; i < args.size(); i++) {
        char* tmp = logic->pp(args[i]);
        auto sortString = logic->printSort(logic->getSortRef(args[i]));
        ss << "(" << tmp << " " << sortString << ")" << (i == args.size()-1 ? "" : " ");
        free(tmp);
    }
    ss << ")" << " " << logic->printSort(templateFun.getRetSort()) << "\n";
    char* tmp = logic->pp(templateFun.getBody());
    ss << "    " << tmp << ")\n";
    free(tmp);
    return ss.str();
}

void Interpret::writeState(const char* filename)
{
    char* msg;
    bool rval;

    rval = main_solver->writeSolverState_smtlib2(filename, &msg);

    if (!rval) {
        notify_formatted("%s", msg);
    }
}

void Interpret::writeSplits_smtlib2(const char* filename)
{
    char* msg;
    ((MainSplitter&) getMainSolver()).writeSolverSplits_smtlib2(filename, &msg);
}

bool Interpret::declareFun(ASTNode const & n) // (const char* fname, const vec<SRef>& args)
{
    auto it = n.children->begin();
    ASTNode const & name_node = **(it++);
    ASTNode const & args_node = **(it++);
    ASTNode const & ret_node  = **(it++);
    assert(it == n.children->end());

    const char* fname = name_node.getValue();

    vec<SRef> args;

    SRef retSort = sortFromASTNode(ret_node);
    if (retSort != SRef_Undef) {
        args.push(retSort);
    } else {
        notify_formatted(true, "Unknown return sort %s of %s", sortSymbolFromASTNode(ret_node).name.c_str(), fname);
        return false;
    }

    for (auto childNode : *(args_node.children)) {
        SRef argSort = sortFromASTNode(*childNode);
        if (argSort != SRef_Undef) {
            args.push(argSort);
        } else {
            notify_formatted(true, "Undefined sort %s in function %s", sortSymbolFromASTNode(*childNode).name.c_str(), fname);
            return false;
        }
    }

    SRef rsort = args[0];
    vec<SRef> args2;

    for (int i = 1; i < args.size(); i++)
        args2.push(args[i]);

    SymRef rval = logic->declareFun(fname, rsort, args2);

    if (rval == SymRef_Undef) {
        comment_formatted("Error while declare-fun %s", fname);
        return false;
    }
    user_declarations.push(rval);
    return true;
}

bool Interpret::declareConst(ASTNode& n) //(const char* fname, const SRef ret_sort)
{
    assert(n.children and n.children->size() == 3);
    auto it = n.children->begin();
    ASTNode const & name_node = **(it++);
    it++; // args_node
    ASTNode const & ret_node = **(it++);
    const char* fname = name_node.getValue();
    SRef ret_sort = sortFromASTNode(ret_node);
    if (ret_sort == SRef_Undef) {
        notify_formatted(true, "Failed to declare constant %s", fname);
        notify_formatted(true, "Unknown return sort %s of %s", sortSymbolFromASTNode(ret_node).name.c_str(), fname);
        return false;
    }

    SymRef rval = logic->declareFun(fname, ret_sort, {});
    if (rval == SymRef_Undef) {
        comment_formatted("Error while declare-const %s", fname);
        return false;
    }
    user_declarations.push(rval);
    notify_success();
    return true;
}

bool Interpret::defineFun(const ASTNode& n)
{
    auto it = n.children->begin();
    ASTNode const & name_node = **(it++);
    ASTNode const & args_node = **(it++);
    ASTNode const & ret_node  = **(it++);
    ASTNode const & term_node = **(it++);
    assert(it == n.children->end());

    const char* fname = name_node.getValue();

    // Get the argument sorts
    vec<SRef> arg_sorts;
    vec<PTRef> arg_trs;
    for (auto childNodePtr : *args_node.children) {
        ASTNode const & childNode = *childNodePtr;
        assert(childNode.children->size() == 1);
        std::string varName = childNode.getValue();
        ASTNode const & sortNode = **(childNode.children->begin());
        SRef sortRef = sortFromASTNode(sortNode);
        if (sortRef == SRef_Undef) {
            notify_formatted(true, "Undefined sort %s in function %s", sortSymbolFromASTNode(sortNode).name.c_str(), fname);
            return false;
        }
        arg_sorts.push(sortRef);
        PTRef pvar = logic->mkVar(arg_sorts.last(), varName.c_str());
        arg_trs.push(pvar);
    }

    // The return sort
    SRef ret_sort = sortFromASTNode(ret_node);
    if (ret_sort == SRef_Undef) {
        notify_formatted(true, "Unknown return sort %s of %s", sortSymbolFromASTNode(ret_node).name.c_str(), fname);
        return false;
    }
    sstat status;
    LetRecords letRecords;
    PTRef tr = parseTerm(term_node, letRecords);
    if (tr == PTRef_Undef) {
        notify_formatted(true, "define-fun returns an unknown sort");
        return false;
    }
    else if (logic->getSortRef(tr) != ret_sort) {
        notify_formatted(true, "define-fun term and return sort do not match: %s and %s\n", logic->printSort(logic->getSortRef(tr)).c_str(), logic->printSort(ret_sort).c_str());
        return false;
    }
    bool rval = logic->defineFun(fname, arg_trs, ret_sort, tr);
    if (rval) notify_success();
    else {
        notify_formatted(true, "define-fun failed");
        return false;
    }

    return rval;
}


void Interpret::comment_formatted(const char* fmt_str, ...) const {
    va_list ap;
    int d;
    char c1, *t;
    if (config.verbosity() < 2) return;
    cout << "; ";

    va_start(ap, fmt_str);
    while (true) {
        c1 = *fmt_str++;
        if (c1 == '%') {
            switch (*fmt_str++) {
            case 's':
                t = va_arg(ap, char *);
                cout << t;
                break;
            case 'd':
                d = va_arg(ap, int);
                cout << d;
                break;
            case '%':
                cout << '%';
                break;
            }
        }
        else if (c1 != '\0')
            cout << c1;
        else break;
    }
    va_end(ap);
    cout << endl;
}


void Interpret::notify_formatted(bool error, const char* fmt_str, ...) {
    va_list ap;
    int d;
    char c1, *t;
    if (error)
        cout << "(error \"";

    va_start(ap, fmt_str);
    while (true) {
        c1 = *fmt_str++;
        if (c1 == '%') {
            switch (*fmt_str++) {
            case 's':
                t = va_arg(ap, char *);
                cout << t;
                break;
            case 'd':
                d = va_arg(ap, int);
                cout << d;
                break;
            case '%':
                cout << '%';
                break;
            }
        }
        else if (c1 != '\0')
            cout << c1;
        else break;
    }
    va_end(ap);
    if (error)
        cout << "\")" << endl;
//    else
//        cout << ")" << endl;
    cout << endl;
}

void Interpret::notify_success() {
    if (config.printSuccess()) {
        std::cout << "success" << std::endl;
    }
}

void Interpret::execute(const ASTNode* r) {
    auto i = r->children->begin();
    for (; i != r->children->end() && !f_exit; i++) {
        interp(**i);
        delete *i;
        *i = nullptr;
    }
}

int Interpret::interpFile(FILE* in) {
    Smt2newContext context(in);
    int rval = smt2newparse(&context);

    if (rval != 0) return rval;

    const ASTNode* r = context.getRoot();
    execute(r);
    return rval;
}

int Interpret::interpFile(char *content){
    Smt2newContext context(content);
    int rval = smt2newparse(&context);

    if (rval != 0) return rval;
    const ASTNode* r = context.getRoot();
    execute(r);
    return rval;
}


// For reading from pipe
int Interpret::interpPipe() {

    int buf_sz  = 16;
    char* buf   = (char*) malloc(sizeof(char)*buf_sz);
    int rd_head = 0;
    int par     = 0;
    int i       = 0;

    bool inComment = false;
    bool inString = false;
    bool inQuotedSymbol = false;

    bool done  = false;
    buf[0] = '\0';
    while (!done) {
        assert(i >= 0 and i <= rd_head);
        assert(buf[rd_head] == '\0');
        assert(rd_head < buf_sz);
        if (rd_head == buf_sz - 1) {
            buf_sz *= 2;
            buf = (char*) realloc(buf, sizeof(char)*buf_sz);
        }
        int rd_chunk = buf_sz - rd_head - 1;
        assert(rd_chunk > 0);
        int bts_rd = read(STDIN_FILENO, &buf[rd_head], rd_chunk);
        if (bts_rd == 0) {
            // Read EOF
            break;
        }
        if (bts_rd < 0) {
            // obtain the error string
            char const * err_str = strerror(errno);
            // format the error
            notify_formatted(true, err_str);
            break;
        }

        rd_head += bts_rd;
        buf[rd_head] = '\0';

        for (; i < rd_head; i++) {

            char c = buf[i];

            if (inComment || (not inQuotedSymbol and not inString and c == ';')) {
                inComment = (c != '\n');
            }
            if (inComment) {
                continue;
            }
            assert(not inComment);
            if (inQuotedSymbol) {
                inQuotedSymbol = (c != '|');
            } else if (not inString and c == '|') {
                inQuotedSymbol = true;
            }
            if (inQuotedSymbol) {
                continue;
            }
            assert (not inComment and not inQuotedSymbol);
            if (inString) {
                inString = (c != '\"');
            } else if (c == '\"') {
                inString = true;
            }
            if (inString) {
                continue;
            }

            if (c == '(') {
                par ++;
            }
            else if (c == ')') {
                par --;
                if (par == 0) {
                    // prepare parse buffer
                    char* buf_out = (char*) malloc(sizeof(char)*i+2);
                    // copy contents to the parse buffer
                    for (int j = 0; j <= i; j++)
                        buf_out[j] = buf[j];
                    buf_out[i+1] = '\0';

                    // copy the part after a top-level balanced parenthesis to the start of buf
                    for (int j = i+1; j < rd_head; j++)
                        buf[j-i-1] = buf[j];
                    buf[rd_head-i-1] = '\0';

                    // update the end position of buf to reflect the removal of the string to be parsed
                    rd_head = rd_head-i-1;

                    i = -1; // will be incremented to 0 by the loop condition.
                    Smt2newContext context(buf_out);
                    int rval = smt2newparse(&context);
                    if (rval != 0)
                        notify_formatted(true, "scanner");
                    else {
                        const ASTNode* r = context.getRoot();
                        execute(r);
                        done = f_exit;
                    }
                    free(buf_out);
                }
                if (par < 0) {
                    notify_formatted(true, "pipe reader: unbalanced parentheses");
                    done = true;
                }
            }
        }
    }
    free(buf);
    return 0;
}

SortSymbol Interpret::sortSymbolFromASTNode(ASTNode const & node) {
    if (node.getType() == SYM_T) {
        return {node.getValue(), 0};
    } else {
        assert(node.getType() == LID_T and node.children and not node.children->empty());
        ASTNode const & name = **(node.children->begin());
        return {name.getValue(), static_cast<unsigned int>(node.children->size() - 1)};
    }
}

SRef Interpret::sortFromASTNode(ASTNode const & node) const {
    if (node.getType() == SYM_T) {
        SortSymbol symbol(node.getValue(), 0);
        SSymRef symRef;
        bool known = logic->peekSortSymbol(symbol, symRef);
        if (not known) { return SRef_Undef; }
        return logic->getSort(symRef, {});
    } else if (node.getType() == LID_T) {
        assert(node.children and not node.children->empty());
        ASTNode const & name = **(node.children->begin());
        SortSymbol symbol(name.getValue(), node.children->size() - 1);
        SSymRef symRef;
        bool known = logic->peekSortSymbol(symbol, symRef);
        if (not known) { return SRef_Undef; }
        vec<SRef> args;
        for (auto it = node.children->begin() + 1; it != node.children->end(); ++it) {
            SRef argSortRef = sortFromASTNode(**it);
            if (argSortRef == SRef_Undef) { return SRef_Undef; }
            args.push(argSortRef);
        }
        return logic->getSort(symRef, std::move(args));
    } else if (node.getType() == IDX_T) {
        assert(node.children and not node.children->empty());
        assert(node.children->size() == 2);
        ASTNode const * symNode = (*node.children)[0];
        ASTNode const * idxNode = (*node.children)[1];
        assert(symNode->getType() == SYM_T);
        assert(idxNode->getType() == NUM_T);
        SRef typeSort = sortFromASTNode(*symNode);
        return logic->getIndexedSort(typeSort, idxNode->getValue());
    }
    assert(false);
    return SRef_Undef;
}

void Interpret::getInterpolants(const ASTNode& n)
{
    auto exps = *n.children;
    vec<PTRef> grouping; // Consists of PTRefs that we want to group
    LetRecords letRecords;
    letRecords.pushFrame();
    for (auto key : nameToTerm.getKeys()) {
        letRecords.addBinding(key, nameToTerm[key]);
    }

    for (auto e : exps) {
        ASTNode& c = *e;
        PTRef tr = parseTerm(c, letRecords);
//        printf("Itp'ing a term %s\n", logic->pp(tr));
        grouping.push(tr);
    }
    letRecords.popFrame();

    if (!(config.produce_inter() > 0))
        opensmt_error("Cannot interpolate");

    assert(grouping.size() >= 2);
    std::vector<ipartitions_t> partitionings;
    ipartitions_t p = 0;
    // We assume that together the groupings cover all query, so we ignore the last argument, since that should contain all that was missing at that point
    for (int i = 0; i < grouping.size() - 1; i++)
    {
        PTRef group = grouping[i];
        if (is_top_level_assertion(group))
        {
            int assertion_index = get_assertion_index(group);
            assert(assertion_index >= 0);
            opensmt::setbit(p, static_cast<unsigned int>(assertion_index));
        }
        else {
            bool ok = group != PTRef_Undef && logic->isAnd(group);
            if (ok) {
                Pterm const & and_t = logic->getPterm(group);
                for (int j = 0; j < and_t.size(); j++) {
                    PTRef tr = and_t[j];
                    ok = is_top_level_assertion(tr);
                    if (!ok) { break; }
                    int assertion_index = get_assertion_index(tr);
                    assert(assertion_index >= 0);
                    opensmt::setbit(p, static_cast<unsigned int>(assertion_index));
                }
            }
            if (!ok) {
                // error in interpolation command
                notify_formatted(true, "Invalid arguments of get-interpolants command");
                return;
            }
        }
        partitionings.emplace_back(p);
    }
    if (main_solver->getStatus() != s_False) {
        notify_formatted(true, "Cannot interpolate, solver is not in UNSAT state!");
        return;
    }
    auto interpolationContext = main_solver->getInterpolationContext();
//        cerr << "Computing interpolant with mask " << p << endl;
    vec<PTRef> itps;
    if (partitionings.size() > 1) {
        interpolationContext->getPathInterpolants(itps, partitionings);
    } else {
        interpolationContext->getSingleInterpolant(itps, partitionings[0]);
    }

    for (int j = 0; j < itps.size(); j++) {
        char * itp = logic->pp(itps[j]);
        notify_formatted(false, "%s%s%s",
                         (j == 0 ? "(" : " "), itp, (j == itps.size() - 1 ? ")" : ""));
        free(itp);
    }
}

bool Interpret::is_top_level_assertion(PTRef ref) {
    return get_assertion_index(ref) >= 0;
}

int Interpret::get_assertion_index(PTRef ref) {
    for (int i = 0; i < assertions.size(); ++i) {
        if (ref == assertions[i]) { return i;}
    }
    return -1;
}

void Interpret::initializeLogic(opensmt::Logic_t logicType) {
    logic.reset(opensmt::LogicFactory::getInstance(logicType));
}

MainSolver& Interpret::createMainSolver(SMTConfig & config, const char*  logic_name) {

    if (config.sat_split_type() == spt_none) {
//        std::cout << "; \033[1;32m [t solve] SimpleSMTSolver\033[0m"<< std::endl;
        return *new MainSolver(*logic, config, std::string(logic_name) + " solver");
    }
    else {
        auto th = MainSolver::createTheory(*logic, config);
        auto tm = std::unique_ptr<TermMapper>(new TermMapper(*logic));
        auto thandler = new THandler(*th,*tm);
        return *new MainSplitter(std::move(th),
                                 std::move(tm),
                                 std::unique_ptr<THandler>(thandler),
                                 MainSplitter::createInnerSolver(config, *thandler, channel),
                                 *logic,
                                 config,
                                 std::string(logic_name)
                                 + " splitter");
    }
}


