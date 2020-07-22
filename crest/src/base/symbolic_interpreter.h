// Copyright (c) 2008, Jacob Burnim (jburnim@cs.berkeley.edu)
//
// This file is part of CREST, which is distributed under the revised
// BSD license.  A copy of this license can be found in the file LICENSE.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See LICENSE
// for details.

#ifndef BASE_SYMBOLIC_INTERPRETER_H__
#define BASE_SYMBOLIC_INTERPRETER_H__

#include <stdio.h>

#include <ext/hash_map>
#include <map>
#include <vector>

#include "base/basic_types.h"
#include "base/symbolic_execution.h"
#include "base/symbolic_expression.h"
#include "base/symbolic_path.h"
#include "base/symbolic_predicate.h"

using std::map;
using std::vector;
using __gnu_cxx::hash_map;

namespace crest {

class SymbolicInterpreter {
 public:
  SymbolicInterpreter();
  explicit SymbolicInterpreter(const vector<value_t>& input);

  void ClearStack(id_t id);
  void Load(id_t id, addr_t addr, value_t value);
  void Store(id_t id, addr_t addr);
 
  // Number of symbolic inputs so far.
  unsigned int num_inputs_;
  unsigned int total_inputs_;

  void ApplyUnaryOp(id_t id, unary_op_t op, value_t value);
  void ApplyBinaryOp(id_t id, binary_op_t op, value_t value);
  void ApplyCompareOp(id_t id, compare_op_t op, value_t value);
  void PrintInput(char *name,int val);
  int GetTimeStamp();
  void Call(id_t id, function_id_t fid);
  void Return(id_t id);
  void HandleReturn(id_t id, value_t value);

  void Branch(id_t id, branch_id_t bid, bool pred_value);

  value_t NewInputValue(type_t type, addr_t addr, value_t v);
  value_t NewInput(type_t type, addr_t addr);
  value_t NewInputTemp(type_t type, addr_t addr,value_t val);

  // Accessor for symbolic execution so far.
  const SymbolicExecution& execution() const { return ex_; }
  
  

  // Debugging.
  void DumpMemory();
  void DumpPath();
  
  void CreateVarMap(addr_t addr, string* name, int tp, string *trigger);
  void ApplyLogState(int x, int r_w, int line, char* varname, int val, int *addr);//aakanksha
  void print(int x, int r_w, int line, char* varname, int val,int *addr);//aakanksha
  void ApplyLogState_1(int x);
  void ApplyLogState_gdb(int x);

  void ApplyLogPC(int x);
  void ApplyLogPC_gdb(int x);
  void ApplyLogSpec(char *op,int *op1,int *op2);

	int state_id;
 private:
  struct StackElem {
    SymbolicExpr* expr;  // NULL to indicate concrete.
    value_t concrete;
  };

  // Stack.
  vector<StackElem> stack_;

  // Predicate register (for when top of stack is a symbolic predicate).
  SymbolicPred* pred_;

  // Is the top of the stack a function return value?
  bool return_value_;

  // Memory map.
  map<addr_t,SymbolicExpr*> mem_;

  // Variable name map.
  map<addr_t,string*> names_;
  map<addr_t,int> names_typs_;
  map<addr_t,string*> names_trigger_;
  
  // The symbolic execution (program path and inputs).
  SymbolicExecution ex_;




  // Helper functions.
  inline void PushConcrete(value_t value);
  inline void PushSymbolic(SymbolicExpr* expr, value_t value);
  inline void ClearPredicateRegister();
	inline void ClearAllMaps();
	inline void FreeMap(map<addr_t,string*> z);
};

}  // namespace crest

#endif  // BASE_SYMBOLIC_INTERPRETER_H__
