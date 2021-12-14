/*********************************************************************
Author: Antti Hyvarinen <antti.hyvarinen@gmail.com>

OpenSMT2 -- Copyright (C) 2012 - 2014 Antti Hyvarinen
                         2008 - 2012 Roberto Bruttomesso

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

#ifndef SMTSOLVER_H
#define SMTSOLVER_H

#include "SolverTypes.h"
#include "TypeUtils.h"
#include "PtStructs.h"
#include "THandler.h"
#include "Proof.h"

class THandler; // Forward declaration
struct SMTConfig; // Forward declaration
// 
// Interface that a SATSolver should implement 
//
class SMTSolver {

protected:
    SMTConfig & config;         // Stores Config
    THandler  & theory_handler; // Handles theory
    bool stop;
    vec<lbool>  model;          // Stores model

public:

  SMTSolver (SMTConfig& c, THandler& t) : config(c), theory_handler(t), stop(false) { }

  virtual ~SMTSolver ( ) { }

    virtual lbool  solve         (const vec<Lit>& assumps) = 0;
    virtual void   setFrozen     ( Var, bool )                = 0;
    virtual bool   isOK          () const                     = 0;
    virtual void   addVar        (Var v)                      = 0;
    virtual bool   addOriginalSMTClause(const vec<Lit> & smt_clause, opensmt::pair<CRef, CRef> & inOutCRefs) = 0;
    virtual bool   addOriginalClause(const vec<Lit> & ps) = 0;
    virtual lbool  modelValue (Lit p) const = 0;
    virtual void   fillBooleanVars(ModelBuilder & modelBuilder) = 0;
    virtual void   initialize() = 0;
    virtual void   pushBacktrackPoint() = 0;
    virtual void   popBacktrackPoint() = 0;
    virtual void   restoreOK() = 0;
    virtual Proof const & getProof() const = 0;
    virtual int    nVars() const = 0;

    virtual int getConflictFrame() const = 0;

    virtual void clearSearch() = 0;  // Backtrack SAT solver and theories to decision level 0

    void setStop() { stop = true; }
protected:
    virtual Var    newVar        ( bool = true, bool = true ) = 0;
};

#endif
