// Copyright (c) 2008, Jacob Burnim (jburnim@cs.berkeley.edu)
//
// This file is part of CREST, which is distributed under the revised
// BSD license.  A copy of this license can be found in the file LICENSE.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See LICENSE
// for details.

#include <algorithm>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <utility>
#include <vector>
#include <cstring>

#include "base/symbolic_interpreter.h"
#include "base/yices_solver.h"

using std::make_pair;
using std::swap;
using std::vector;

#ifdef DEBUG
#define IFDEBUG(x) x
#else
#define IFDEBUG(x)
#endif


namespace crest {

    typedef map<addr_t,SymbolicExpr*>::const_iterator ConstMemIt;


    SymbolicInterpreter::SymbolicInterpreter()
        : pred_(NULL), ex_(true), num_inputs_(0) {
            stack_.reserve(16);
            state_id = 0;
        }

    SymbolicInterpreter::SymbolicInterpreter(const vector<value_t>& input)
        : pred_(NULL), ex_(true) {
            stack_.reserve(16);
            ex_.mutable_inputs()->assign(input.begin(), input.end());
            state_id = 0;
        }

    void SymbolicInterpreter::DumpMemory() {
        FILE *tr;
        tr = fopen("trace.txt","a");
        for (ConstMemIt i = mem_.begin(); i != mem_.end(); ++i) {
            string s;
            i->second->AppendToString(&s);
            fprintf(tr, "%lu: %s [%d]\n", i->first, s.c_str(), *(int*)(i->first));
        }
        for (size_t i = 0; i < stack_.size(); i++) {
            string s;
            if (stack_[i].expr) {
                stack_[i].expr->AppendToString(&s);
            } else if ((i == stack_.size() - 1) && pred_) {
                pred_->AppendToString(&s);
            }
            fprintf(tr, "s%d: %lld [ %s ]\n", i, stack_[i].concrete, s.c_str());
        }

        fclose(tr);
    }


    void SymbolicInterpreter::ClearStack(id_t id) {
        FILE *tr;
        tr = fopen("trace.txt","a");
        IFDEBUG(fprintf(tr, "clear\n"));
        for (vector<StackElem>::const_iterator it = stack_.begin(); it != stack_.end(); ++it) {
            delete it->expr;
        }
        stack_.clear();
        ClearPredicateRegister();
        return_value_ = false;
        IFDEBUG(DumpMemory());
        fclose(tr);
    }


    void SymbolicInterpreter::Load(id_t id, addr_t addr, value_t value) {
        FILE *tr;
        tr = fopen("trace.txt","a");
        IFDEBUG(fprintf(tr, "load %lu %lld\n", addr, value));
        ConstMemIt it = mem_.find(addr);
        if (it == mem_.end()) {
            PushConcrete(value);
        } else {
            PushSymbolic(new SymbolicExpr(*it->second), value);
        }
        ClearPredicateRegister();
        IFDEBUG(DumpMemory());
        fclose(tr);
    }


    void SymbolicInterpreter::Store(id_t id, addr_t addr) {
        FILE *tr;
        tr = fopen("trace.txt","a");
        IFDEBUG(fprintf(tr, "store %lu\n", addr));
        assert(stack_.size() > 0);

        const StackElem& se = stack_.back();
        if (se.expr) {
            if (!se.expr->IsConcrete()) {
                mem_[addr] = se.expr;
            } else {
                mem_.erase(addr);
                delete se.expr;
            }
        } else {
            mem_.erase(addr);
        }

        stack_.pop_back();
        ClearPredicateRegister();
        IFDEBUG(DumpMemory());
        fclose(tr);
    }


    void SymbolicInterpreter::ApplyUnaryOp(id_t id, unary_op_t op, value_t value) {
        FILE *tr;
        tr = fopen("trace.txt","a");
        IFDEBUG(fprintf(tr, "apply1 %d %lld\n", op, value));
        assert(stack_.size() >= 1);
        StackElem& se = stack_.back();

        if (se.expr) {
            switch (op) {
                case ops::NEGATE:
                    se.expr->Negate();
                    ClearPredicateRegister();
                    break;
                case ops::LOGICAL_NOT:
                    if (pred_) {
                        pred_->Negate();
                        break;
                    }
                    // Otherwise, fall through to the concrete case.
                default:
                    // Concrete operator.
                    delete se.expr;
                    se.expr = NULL;
                    ClearPredicateRegister();
            }
        }

        se.concrete = value;
        IFDEBUG(DumpMemory());
        fclose(tr);
    }


    void SymbolicInterpreter::ApplyBinaryOp(id_t id, binary_op_t op, value_t value) {
        FILE *tr;
        tr = fopen("trace.txt","a");
        IFDEBUG(fprintf(tr, "apply2 %d %lld\n", op, value));
        assert(stack_.size() >= 2);
        StackElem& a = *(stack_.rbegin()+1);
        StackElem& b = stack_.back();

        if (a.expr || b.expr) {
            switch (op) {
                case ops::ADD:
                    if (a.expr == NULL) {
                        swap(a, b);
                        *a.expr += b.concrete;
                    } else if (b.expr == NULL) {
                        *a.expr += b.concrete;
                    } else {
                        *a.expr += *b.expr;
                        delete b.expr;
                    }
                    break;

                case ops::SUBTRACT:
                    if (a.expr == NULL) {
                        b.expr->Negate();
                        swap(a, b);
                        *a.expr += b.concrete;
                    } else if (b.expr == NULL) {
                        *a.expr -= b.concrete;
                    } else {
                        *a.expr -= *b.expr;
                        delete b.expr;
                    }
                    break;

                case ops::MULTIPLY:
                    if (a.expr == NULL) {
                        swap(a, b);
                        *a.expr *= b.concrete;
                    } else if (b.expr == NULL) {
                        *a.expr *= b.concrete;
                    } else {
                        swap(a, b);
                        *a.expr *= b.concrete;
                        delete b.expr;
                    }
                    break;

                default:
                    // Concrete operator.
                    delete a.expr;
                    delete b.expr;
                    a.expr = NULL;
            }
        }

        a.concrete = value;
        stack_.pop_back();
        ClearPredicateRegister();
        IFDEBUG(DumpMemory());
        fclose(tr);
    }

    void SymbolicInterpreter::FreeMap(map<addr_t,string*> z)
    {
        for (map<addr_t,string*>::const_iterator it=z.begin(); it!=z.end(); it++)
        {
            if (it->second != NULL)
                delete it->second;
        }
    }

    void SymbolicInterpreter::ApplyCompareOp(id_t id, compare_op_t op, value_t value) {
        FILE *tr;
        tr = fopen("trace.txt","a");
        IFDEBUG(fprintf(tr, "compare2 %d %lld\n", op, value));
        assert(stack_.size() >= 2);
        StackElem& a = *(stack_.rbegin()+1);
        StackElem& b = stack_.back();

        if (a.expr || b.expr) {
            // Symbolically compute "a -= b".
            if (a.expr == NULL) {
                b.expr->Negate();
                swap(a, b);
                *a.expr += b.concrete;
            } else if (b.expr == NULL) {
                *a.expr -= b.concrete;
            } else {
                *a.expr -= *b.expr;
                delete b.expr;
            }
            // Construct a symbolic predicate (if "a - b" is symbolic), and
            // store it in the predicate register.
            if (!a.expr->IsConcrete()) {
                pred_ = new SymbolicPred(op, a.expr);
            } else {
                ClearPredicateRegister();
                delete a.expr;
            }
            // We leave a concrete value on the stack.
            a.expr = NULL;
        }

        a.concrete = value;
        stack_.pop_back();
        IFDEBUG(DumpMemory());
        fclose(tr);
    }

    void SymbolicInterpreter::ClearAllMaps()
    {
        FreeMap(names_);
        FreeMap(names_trigger_);
        names_.clear();
        names_typs_.clear();
        names_trigger_.clear();
    }

    void SymbolicInterpreter::Call(id_t id, function_id_t fid) {
        ex_.mutable_path()->Push(kCallId);
        ClearAllMaps();
        //names_.clear(); // so that the local variable names in the caller don't persist
        // names_typs_.clear(); // so that the local variable names in the caller don't persist
        // names_trigger_.clear(); // so that the local variable names in the caller don't persist
    }


    void SymbolicInterpreter::Return(id_t id) {
        ex_.mutable_path()->Push(kReturnId);
        ClearAllMaps();

        // There is either exactly one value on the stack -- the current function's
        // return value -- or the stack is empty.
        assert(stack_.size() <= 1);

        return_value_ = (stack_.size() == 1);
    }


    void SymbolicInterpreter::HandleReturn(id_t id, value_t value) {
        if (return_value_) {
            // We just returned from an instrumented function, so the stack
            // contains a single element -- the (possibly symbolic) return value.
            assert(stack_.size() == 1);
            return_value_ = false;
        } else {
            // We just returned from an uninstrumented function, so the stack
            // still contains the arguments to that function.  Thus, we clear
            // the stack and push the concrete value that was returned.
            ClearStack(-1);
            PushConcrete(value);
        }
    }


    void SymbolicInterpreter::Branch(id_t id, branch_id_t bid, bool pred_value) {
        FILE *tr;
        tr = fopen("trace.txt","a");
        IFDEBUG(fprintf(tr, "branch %d %d\n", bid, pred_value));
        assert(stack_.size() == 1);
        stack_.pop_back();

        if (pred_ && !pred_value) {
            pred_->Negate();
        }

        ex_.mutable_path()->Push(bid, pred_);
        pred_ = NULL;
        IFDEBUG(DumpMemory());
    }


    value_t SymbolicInterpreter::NewInputValue(type_t type, addr_t addr, value_t v) {
        //ex_.mutable_inputs()->push_back(v);
        this->NewInputTemp(type, addr,v);
    }

    value_t SymbolicInterpreter::NewInputTemp(type_t type, addr_t addr, value_t val) {
        static unsigned int ghatiya = 0;
        num_inputs_ = ghatiya;
        mem_[addr] = new SymbolicExpr(1, num_inputs_);
        ex_.mutable_vars()->insert(make_pair(num_inputs_ ,type));

        value_t ret = val;
        if (num_inputs_ < ex_.inputs().size()) {
            ret = ex_.inputs()[num_inputs_];
        } else {
            // New inputs are initially zero.  (Could randomize instead.)
            ex_.mutable_inputs()->push_back(val);
        }

        num_inputs_ ++;
        ghatiya++;
        return ret;
    }

    value_t SymbolicInterpreter::NewInput(type_t type, addr_t addr) {
        static unsigned int ghatiya = 0;
        num_inputs_ = ghatiya;
        mem_[addr] = new SymbolicExpr(1, num_inputs_);
        ex_.mutable_vars()->insert(make_pair(num_inputs_ ,type));

        value_t ret = 0;
        if (num_inputs_ < ex_.inputs().size()) {
            ret = ex_.inputs()[num_inputs_];
        } else {
            // New inputs are initially zero.  (Could randomize instead.)
            ex_.mutable_inputs()->push_back(0);
        }

        num_inputs_ ++;
        ghatiya++;
        //fprintf(stderr,"New Input = %u\n",num_inputs_);
        return ret;
    }


    void SymbolicInterpreter::PushConcrete(value_t value) {
        PushSymbolic(NULL, value);
    }


    void SymbolicInterpreter::PushSymbolic(SymbolicExpr* expr, value_t value) {
        stack_.push_back(StackElem());
        StackElem& se = stack_.back();
        se.expr = expr;
        se.concrete = value;
    }


    void SymbolicInterpreter::ClearPredicateRegister() {
        delete pred_;
        pred_ = NULL;
    }

    void SymbolicInterpreter::CreateVarMap(addr_t addr, string* name, int tp, string* trigger) {
        //map<addr_t,string*>::const_iterator it = names_.find(addr);
        //if (it != names_.end())
        //    names_.erase(addr);
        //printf("name = %s, addr = %lu\n",name,addr);
        names_[addr] = name;
        names_typs_[addr] = tp;
        names_trigger_[addr] = trigger;
    }

    int foo(int state_id){
        FILE *f = fopen("state_id","w");
        fprintf(f,"%d\n",state_id-1);
        fclose(f);
    }

    int  SymbolicInterpreter::GetTimeStamp() {//aakanksha

        state_id++;
        foo(state_id);
        return state_id;

    }


    void  SymbolicInterpreter::PrintInput(char *name, int val) {//aakanksha

        FILE *tr;
        tr = fopen("trace.txt","a");
        fprintf(tr,"INPUT-%s,%d,%d\n",name,state_id,val);
        fclose(tr);

    }

    //void SymbolicInterpreter::ApplyLogState(int x) {
    void SymbolicInterpreter::ApplyLogState(int x, int r_w, int line, char *varname=NULL, int val=0 ,int *a=NULL) {//aakanksha
        FILE *tr;
        tr = fopen("trace.txt","a");
        
        FILE *observer;
        observer = fopen("observer.txt","a");


        //if (x == 100)
        //{
            //observer = fopen("observer.txt","w");
            //fprintf(observer,"OBSERVER\n");
        //}

        map<addr_t,string*>::const_iterator it;
        
        
        if (x == 100)
        {

            int q = -1;
            fprintf(observer, "\nLocation(State):%d,%d,%d,%d\n", q, state_id++,r_w,line);//aakanksha //YES
        }
        else
        {
            fprintf(tr, "\nLocation(State):%d,%d,%d,%d\n", x, state_id++,r_w,line);//aakanksha //YES
        }


        foo(state_id);
        for (ConstMemIt i = mem_.begin(); i != mem_.end(); ++i) {
            string s;
            it = names_.find(i->first);
            if (it == names_.end())
                continue;
            // c = it->second->c_str();
            s.clear();
            int tp = names_typs_.find(i->first)->second;
            string* trg = names_trigger_.find(i->first)->second;
            i->second->AppendToString(&s, tp);
            const char *c = "??";
            c = it->second->c_str();//change char pointer into string.

            fprintf(tr,"" );
            if (tp == 'i') // Integer
            {
                if (varname == NULL)
                {
                    if (x==100)
                    {
                        fprintf(observer, "%lu<%s>: %s [%d] {%s}\n", a, c, s.c_str(), *(int*)(i->first), trg->c_str());
                    }
                    //fprintf(tr,"hi how are you .1......... ....... ........... %lu \n",i->first);
                    else
                    {

                        fprintf(tr, "%lu<%s>: %s [%d] {%s}\n", a, c, s.c_str(), *(int*)(i->first), trg->c_str());
                    }

                }
            }
            else if (tp == 'c')
                if (x == 100){observer,(tr, "%lu<%s>: %s [%d] {%s}\n", i->first, c, s.c_str(), *(char*)(i->first), trg->c_str());}
                else{fprintf(tr, "%lu<%s>: %s [%d] {%s}\n", i->first, c, s.c_str(), *(char*)(i->first), trg->c_str());}
            else
            { 
                //fprintf(tr,"hi how are you 2.......... ....... ........... %lu \n",i->first);
                if (x == 100){fprintf(observer, "%lu<%s>: %s [%d] {%s}\n", i->first, c, s.c_str(), *(int*)(i->first), trg->c_str());}
                else{fprintf(tr, "%lu<%s>: %s [%d] {%s}\n", i->first, c, s.c_str(), *(int*)(i->first), trg->c_str());}
            }
        }

        // Log the concrete values --- that is those missing in the mem_ map
        for (map<addr_t,string*>::const_iterator i = names_.begin(); i != names_.end(); ++i)
        {

            int tp = names_typs_.find(i->first)->second;
            string* trg = names_trigger_.find(i->first)->second;
            //fprintf("%s",i->second->coeff);
            if (mem_.find(i->first) == mem_.end())
#ifdef PRINT_FOR_TOOL
                if (tp == 105) // Integer
                {
                    if (varname == NULL){
                        //fprintf(tr,"hi how are you 3.......... ....... ........... %lu \n",i->first);

                        if(x==100){fprintf(observer, "%lu<%s>: {const=%d} [%d] {%s}\n",a, i->second->c_str(), *(int*)(i->first) , *(int*)(i->first), trg->c_str());}
                        else{fprintf(tr, "%lu<%s>: {const=%d} [%d] {%s}\n",a, i->second->c_str(), *(int*)(i->first) , *(int*)(i->first), trg->c_str());}
                    }

                    else{
                        //fprintf(tr,"hi how are you 5........ ....... ........... %lu \n",i->first);
                        if(x == 100){fprintf(observer, "%lu<%s>:{const=%d} {%s}\n", a, varname, val,trg->c_str()) ;}
                        else{fprintf(tr, "%lu<%s>:{const=%d} {%s}\n", a, varname, val,trg->c_str()) ;}
                    }
                }
                else if (tp == 'c') // Integer
                    if (x == 100){fprintf(observer, "%lu<%s>: {const=%d} [%d] {%s}\n", i->first, i->second->c_str(), *(char*)(i->first) , *(char*)(i->first), trg->c_str());}
                    else{fprintf(tr, "%lu<%s>: {const=%d} [%d] {%s}\n", i->first, i->second->c_str(), *(char*)(i->first) , *(char*)(i->first), trg->c_str());}
                else
                {
                    //fprintf(tr,"hi how are you 4.......... ....... ........... %lu \n",i->first);
                    if(x == 100){fprintf(observer, "%lu<%s>: {const=%d} [%d] {%s}\n", i->first, i->second->c_str(), *(int*)(i->first) , *(int*)(i->first), trg->c_str());}
                    else{fprintf(tr, "%lu<%s>: {const=%d} [%d] {%s}\n", i->first, i->second->c_str(), *(int*)(i->first) , *(int*)(i->first), trg->c_str());}
                }
#else
            //fprintf(tr,"hi how are you 6.......... ....... ........... %lu \n",i->first);
            if(x == 100){fprintf(observer, "%lu<%s>: %s [%d]\n", a, i->second->c_str(), "concrete" , *(int*)(i->first));}
            else{fprintf(tr, "%lu<%s>: %s [%d]\n", a, i->second->c_str(), "concrete" , *(int*)(i->first));}
#endif
        }


        if (x == 100){fprintf(observer, "%s\n", "END");}
        else{fprintf(tr, "%s\n", "END");}

        ClearAllMaps();
        fclose(tr);
        if (x == 100)
            fclose(observer);

        // stack is the computation stack---not needed for us
        /*
           for (size_t i = 0; i < stack_.size(); i++) {
           string s;
           if (stack_[i].expr) {
           stack_[i].expr->AppendToString(&s);
           } else if ((i == stack_.size() - 1) && pred_) {
           pred_->AppendToString(&s);
           }
           it = names_.find(i);
           const char *c = "??";
           if (it != names_.end())
           c = it->second->c_str();
           fprintf(stderr, "s%d <%s>: %lld [ %s ]\n", i, c, stack_[i].concrete, s.c_str());
           }*/
    }


    /* void SymbolicInterpreter::ApplyLogState_1(int x) {
       map<addr_t,string*>::const_iterator it;
       fprintf(stderr, "\nLocation(State): %d, %d\n", x, state_id++);
       for (ConstMemIt i = mem_.begin(); i != mem_.end(); ++i) {
       string s;
       it = names_.find(i->first);
       if (it == names_.end())
       continue;
    // c = it->second->c_str();
    s.clear();
    int tp = names_typs_.find(i->first)->second;
    string* trg = names_trigger_.find(i->first)->second;
    i->second->AppendToString(&s, tp);
    const char *c = "??";
    c = it->second->c_str();
    if (tp == 'i') // Integer
    fprintf(stderr, "%lu<%s>: %s [%d] {%s}\n", i->first, c, s.c_str(), *(int*)(i->first), trg->c_str());
    else if (tp == 'c')
    fprintf(stderr, "%lu<%s>: %s [%d] {%s}\n", i->first, c, s.c_str(), *(char*)(i->first), trg->c_str());
    else
    fprintf(stderr, "%lu<%s>: %s [%d] {%s}\n", i->first, c, s.c_str(), *(int*)(i->first), trg->c_str());
    }

    // Log the concrete values --- that is those missing in the mem_ map
    for (map<addr_t,string*>::const_iterator i = names_.begin(); i != names_.end(); ++i)
    {
    int tp = names_typs_.find(i->first)->second;
    string* trg = names_trigger_.find(i->first)->second;
    if (mem_.find(i->first) == mem_.end())
#ifdef PRINT_FOR_TOOL
if (tp == 'i') // Integer
fprintf(stderr, "%lu<%s>: {const=%d} [%d] {%s}\n", i->first, i->second->c_str(), *(int*)(i->first) , *(int*)(i->first), trg->c_str());
else if (tp == 'c') // Integer
fprintf(stderr, "%lu<%s>: {const=%d} [%d] {%s}\n", i->first, i->second->c_str(), *(char*)(i->first) , *(char*)(i->first), trg->c_str());
else
fprintf(stderr, "%lu<%s>: {const=%d} [%d] {%s}\n", i->first, i->second->c_str(), *(int*)(i->first) , *(int*)(i->first), trg->c_str());
#else
fprintf(stderr, "%lu<%s>: %s [%d]\n", i->first, i->second->c_str(), "concrete" , *(int*)(i->first));
#endif
}

fprintf(stderr, "%s\n", "END");

ClearAllMaps();

// stack is the computation stack---not needed for us
//comment start
for (size_t i = 0; i < stack_.size(); i++) {
string s;
if (stack_[i].expr) {
stack_[i].expr->AppendToString(&s);
} else if ((i == stack_.size() - 1) && pred_) {
pred_->AppendToString(&s);
}
it = names_.find(i);
const char *c = "??";
if (it != names_.end())
c = it->second->c_str();
fprintf(stderr, "s%d <%s>: %lld [ %s ]\n", i, c, stack_[i].concrete, s.c_str());
}// comment end
}*/
void SymbolicInterpreter::ApplyLogState_1(int x) {// aakanksha
    FILE *fp;
    static int id = 0;
    fp = fopen("local_var.txt","a");
    map<addr_t,string*>::const_iterator it;
    fprintf(fp, "\nLocation(State): %d, %d\n", x, id++);
    for (ConstMemIt i = mem_.begin(); i != mem_.end(); ++i) {
        string s;
        it = names_.find(i->first);
        if (it == names_.end())
            continue;
        // c = it->second->c_str();
        s.clear();
        int tp = names_typs_.find(i->first)->second;
        string* trg = names_trigger_.find(i->first)->second;
        i->second->AppendToString(&s, tp);
        const char *c = "??";
        c = it->second->c_str();
        if (tp == 'i') //Integer
            fprintf(fp, "%lu<%s>: %s [%d] {%s}\n", i->first, c, s.c_str(), *(int*)(i->first), trg->c_str());
        else if (tp == 'c')
            fprintf(fp, "%lu<%s>: %s [%d] {%s}\n", i->first, c, s.c_str(), *(char*)(i->first), trg->c_str());
        else
            fprintf(fp, "%lu<%s>: %s [%d] {%s}\n", i->first, c, s.c_str(), *(int*)(i->first), trg->c_str());
    }

    // Log the concrete values --- that is those missing in the mem_ map
    for (map<addr_t,string*>::const_iterator i = names_.begin(); i != names_.end(); ++i)
    {
        int tp = names_typs_.find(i->first)->second;
        string* trg = names_trigger_.find(i->first)->second;
        if (mem_.find(i->first) == mem_.end())
#ifdef PRINT_FOR_TOOL
            if (tp == 'i') // Integer
                fprintf(fp, "%lu<%s>: {const=%d} [%d] {%s}\n", i->first, i->second->c_str(), *(int*)(i->first) , *(int*)(i->first), trg->c_str());
            else if (tp == 'c') // Integer
                fprintf(fp, "%lu<%s>: {const=%d} [%d] {%s}\n", i->first, i->second->c_str(), *(char*)(i->first) , *(char*)(i->first), trg->c_str());
            else
                fprintf(fp, "%lu<%s>: {const=%d} [%d] {%s}\n", i->first, i->second->c_str(), *(int*)(i->first) , *(int*)(i->first), trg->c_str());
#else
        fprintf(fp, "%lu<%s>: %s [%d]\n", i->first, i->second->c_str(), "concrete" , *(int*)(i->first));
#endif
    }

    fprintf(fp, "%s\n", "END");
    fclose(fp);
    ClearAllMaps();

    // stack is the computation stack---not needed for us
}

void SymbolicInterpreter::ApplyLogState_gdb(int x) {// aakanksha
    FILE *fp;
    static int id = 0;
    fp = fopen("logged_var.txt","a");
    map<addr_t,string*>::const_iterator it;
    fprintf(fp, "\nLocation(State): %d, %d\n", x, id++);
    for (ConstMemIt i = mem_.begin(); i != mem_.end(); ++i) {
        string s;
        it = names_.find(i->first);
        if (it == names_.end())
            continue;
        // c = it->second->c_str();
        s.clear();
        int tp = names_typs_.find(i->first)->second;
        string* trg = names_trigger_.find(i->first)->second;
        i->second->AppendToString(&s, tp);
        const char *c = "??";
        c = it->second->c_str();



        if (tp == 'i') //Integer
            fprintf(fp, "%lu<%s>: %s [%d] {%s}\n", i->first, c, s.c_str(), *(int*)(i->first), trg->c_str());
        else if (tp == 'c')
            fprintf(fp, "%lu<%s>: %s [%d] {%s}\n", i->first, c, s.c_str(), *(char*)(i->first), trg->c_str());
        else
            fprintf(fp, "%lu<%s>: %s [%d] {%s}\n", i->first, c, s.c_str(), *(int*)(i->first), trg->c_str());
    }

    // Log the concrete values --- that is those missing in the mem_ map
    for (map<addr_t,string*>::const_iterator i = names_.begin(); i != names_.end(); ++i)
    {
        int tp = names_typs_.find(i->first)->second;
        string* trg = names_trigger_.find(i->first)->second;
        if (mem_.find(i->first) == mem_.end())
#ifdef PRINT_FOR_TOOL
            if (tp == 'i') // Integer
                fprintf(fp, "%lu<%s>: {const=%d} [%d] {%s}\n", i->first, i->second->c_str(), *(int*)(i->first) , *(int*)(i->first), trg->c_str());
            else if (tp == 'c') // Integer
                fprintf(fp, "%lu<%s>: {const=%d} [%d] {%s}\n", i->first, i->second->c_str(), *(char*)(i->first) , *(char*)(i->first), trg->c_str());
            else
                fprintf(fp, "%lu<%s>: {const=%d} [%d] {%s}\n", i->first, i->second->c_str(), *(int*)(i->first) , *(int*)(i->first), trg->c_str());
#else
        fprintf(fp, "%lu<%s>: %s [%d]\n", i->first, i->second->c_str(), "concrete" , *(int*)(i->first));
#endif
    }

    fprintf(fp, "%s\n", "END");
    fclose(fp);
    ClearAllMaps();

    // stack is the computation stack---not needed for us
}
void SymbolicInterpreter::ApplyLogPC(int x) {
    FILE *tr;
    tr = fopen("trace.txt","a");
    string s;
    fprintf(tr, "\nLocation(PC): %d\n", x);//NO
    const SymbolicExecution& ex = execution();
    const SymbolicPath& path = ex.path();
    const vector<SymbolicPred*> constraints = path.constraints();
    for (size_t i = 0; i < constraints.size(); i++) {
        s.clear();
        constraints[i]->AppendToString(&s);
        fprintf(tr, "%s\n", s.c_str());
    }

    fprintf(tr, "%s\n", "END");
    fclose(tr);
}

void SymbolicInterpreter::ApplyLogPC_gdb(int x) {
    FILE *dp,*dp1;
    //dp = fopen("dump.txt","a");
    dp1 = fopen("dump1.txt","w");
    string s;
    //fprintf(dp, "\nLocation(PC): %d, %d\n", x, state_id++);
    fprintf(dp1, "\nLocation(PC): %d\n", x);
    foo(state_id);
    const SymbolicExecution& ex = execution();
    const SymbolicPath& path = ex.path();
    const vector<SymbolicPred*> constraints = path.constraints();
    for (size_t i = 0; i < constraints.size(); i++) {
        s.clear();
        constraints[i]->AppendToString(&s);
        //fprintf(dp, "%s\n", s.c_str());
        fprintf(dp1, "%s\n", s.c_str());
    }

    //fprintf(dp, "%s\n", "END");
    fprintf(dp1, "%s\n", "END");
    //fclose(dp);
    fclose(dp1);
}

void SymbolicInterpreter::print(int tid, int r_w, int line, char* name, int val, int *addr){
    FILE *tr;
    tr = fopen("trace.txt","a");
    fprintf(tr, "\nLocation(State):%d,%d,%d,%d\n", tid, state_id++,r_w,line);//aakanksha
    fprintf(tr,"%lu<%s>: {const=%d} {True}\n",addr,name,val);
    fprintf(tr, "%s\n", "END");
    fclose(tr);

}
void SymbolicInterpreter::ApplyLogSpec(char *op,int *op1,int *op2) {
    //string s; 
    FILE *tr;
    tr = fopen("trace.txt","a");
    int x=1;
    fprintf(tr, "\nLocation(PC): %d, %d\n", x, state_id++);
    SymbolicExpr *val1, *val2;
    ConstMemIt it, it2; 
    string s;
    it = mem_.find((long unsigned int)op1);
    if (it == mem_.end())
    {
        SymbolicExpr s(*op1);
        val1 = &s;
    }
    else{
        val1 = it->second;
        it2 = mem_.find((long unsigned int)op2);
        if (it2 == mem_.end())
        {
            SymbolicExpr s(*op2);
            val2 = &s;
        }
        else{
            val2 = it2->second;
        }    
    }
    *val1 -= *val2 ;
    //int tp = names_typs_.find(op1)->second;
    //string* trg = names_trigger_.find(i->first)->second;
    //i->second->AppendToString(&s, tp);
    val1->AppendToString(&s, 'i');

    fprintf(tr,"(%s %s %d)\n" , op, s.c_str(), 0);
    fprintf(tr, "%s\n", "END");
    fclose(tr);
}

}  // namespace crest
