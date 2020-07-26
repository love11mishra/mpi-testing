// Copyright (c) 2008, Jacob Burnim (jburnim@cs.berkeley.edu)
//
// This file is part of CREST, which is distributed under the revised
// BSD license.  A copy of this license can be found in the file LICENSE.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See LICENSE
// for details.

#include <assert.h>
#include <unistd.h>
#include <fstream>
#include <string>
#include <vector>

#include "base/symbolic_interpreter.h"
#include "libcrest/crest.h"

using std::vector;
using namespace crest;

// The symbolic interpreter. */
static SymbolicInterpreter* SI;

// Have we read an input yet?  Until we have, generate only the
// minimal instrumentation necessary to track which branches were
// reached by the execution path.
static int pre_symbolic;

// Tables for converting from operators defined in libcrest/crest.h to
// those defined in base/basic_types.h.
static const int kOpTable[] = {  // binary arithmetic
    ops::ADD, ops::SUBTRACT, ops::MULTIPLY, ops::CONCRETE, ops::CONCRETE,
    // binary bitwise operators
    ops::CONCRETE, ops::CONCRETE, ops::CONCRETE, ops::CONCRETE, ops::CONCRETE,
    // binary logical operators
    ops::CONCRETE, ops::CONCRETE,
    // binary comparison
    ops::EQ, ops::NEQ, ops::GT, ops::LE, ops::LT, ops::GE,
    // unhandled binary operators
    ops::CONCRETE,
    // unary operators
    ops::NEGATE, ops::BITWISE_NOT, ops::LOGICAL_NOT};

static void __CrestAtExit();

void __CrestInit() {
  /* read the input */
  vector<value_t> input;
  std::ifstream in("input");
  value_t val;
  while (in >> val) {
    input.push_back(val);
  }
  in.close();

  SI = new SymbolicInterpreter(input);

  pre_symbolic = 1;

  assert(!atexit(__CrestAtExit));
}

void __CrestAtExit() {
  const SymbolicExecution& ex = SI->execution();

  /* Write the execution out to file 'szd_execution'. */
  string buff;
  buff.reserve(1 << 26);
  ex.Serialize(&buff);
  std::ofstream out("szd_execution", std::ios::out | std::ios::binary);
  out.write(buff.data(), buff.size());
  assert(!out.fail());
  out.close();
}

//
// Instrumentation functions.
//

void __CrestLoad(__CREST_ID id, __CREST_ADDR addr, __CREST_VALUE val) {
  if (!pre_symbolic) SI->Load(id, addr, val);
}

void __CrestStore(__CREST_ID id, __CREST_ADDR addr) {
  if (!pre_symbolic) SI->Store(id, addr);
}

void __CrestClearStack(__CREST_ID id) {
  if (!pre_symbolic) SI->ClearStack(id);
}

void __CrestApply1(__CREST_ID id, __CREST_OP op, __CREST_VALUE val) {
  assert((op >= __CREST_NEGATE) && (op <= __CREST_L_NOT));

  if (!pre_symbolic) SI->ApplyUnaryOp(id, static_cast<unary_op_t>(kOpTable[op]), val);
}

void __CrestApply2(__CREST_ID id, __CREST_OP op, __CREST_VALUE val) {
  assert((op >= __CREST_ADD) && (op <= __CREST_CONCRETE));

  if (pre_symbolic) return;

  if ((op >= __CREST_ADD) && (op <= __CREST_L_OR)) {
    SI->ApplyBinaryOp(id, static_cast<binary_op_t>(kOpTable[op]), val);
  } else {
    SI->ApplyCompareOp(id, static_cast<compare_op_t>(kOpTable[op]), val);
  }
}

void __CrestBranch(__CREST_ID id, __CREST_BRANCH_ID bid, __CREST_BOOL b) {
  if (pre_symbolic) {
    // Precede the branch with a fake (concrete) load.
    SI->Load(id, 0, b);
  }

  SI->Branch(id, bid, static_cast<bool>(b));
}

void __CrestCall(__CREST_ID id, __CREST_FUNCTION_ID fid) { SI->Call(id, fid); }

void __CrestReturn(__CREST_ID id) { SI->Return(id); }

void __CrestHandleReturn(__CREST_ID id, __CREST_VALUE val) {
  if (!pre_symbolic) SI->HandleReturn(id, val);
}

//
// Symbolic input functions.
//

void __CrestUChar(unsigned char* x) {
  pre_symbolic = 0;
  *x = (unsigned char)SI->NewInput(types::U_CHAR, (addr_t)x);
}

void __CrestUShort(unsigned short* x) {
  pre_symbolic = 0;
  *x = (unsigned short)SI->NewInput(types::U_SHORT, (addr_t)x);
}

void __CrestUInt(unsigned int* x) {
  pre_symbolic = 0;
  *x = (unsigned int)SI->NewInput(types::U_INT, (addr_t)x);
}

void __CrestChar(char* x) {
  pre_symbolic = 0;
  *x = (char)SI->NewInput(types::CHAR, (addr_t)x);
}

void __CrestShort(short* x) {
  pre_symbolic = 0;
  *x = (short)SI->NewInput(types::SHORT, (addr_t)x);
}

void __CrestInt(int* x) {
  pre_symbolic = 0;
  *x = (int)SI->NewInput(types::INT, (addr_t)x);
}

void __CrestUCharTrace(unsigned char* x, unsigned char c, char* iprange) {
#ifdef PRINT_FOR_TOOL
  fprintf(stderr, "INIT_SYM_VAR_UChar: %d,%s\n\n", c, iprange);
#endif
  pre_symbolic = 0;
  *x = (unsigned char)SI->NewInputValue(types::U_CHAR, (addr_t)x, c);
}

void __CrestUShortTrace(unsigned short* x, unsigned short c, char* iprange) {
#ifdef PRINT_FOR_TOOL
  fprintf(stderr, "INIT_SYM_VAR_UShort: %d,%s\n\n", c, iprange);
#endif
  pre_symbolic = 0;
  *x = (unsigned short)SI->NewInputValue(types::U_SHORT, (addr_t)x, c);
}

void __CrestUIntTrace(unsigned int* x, unsigned int c, char* iprange) {
#ifdef PRINT_FOR_TOOL
  fprintf(stderr, "INIT_SYM_VAR_UInt: %d,%s\n\n", c, iprange);
#endif
  pre_symbolic = 0;
  *x = (unsigned int)SI->NewInputValue(types::U_INT, (addr_t)x, c);
}

void __CrestCharTrace(char* x, char c, char* iprange) {
#ifdef PRINT_FOR_TOOL
  fprintf(stderr, "INIT_SYM_VAR_Char: %d,%s\n\n", c, iprange);
#endif
  pre_symbolic = 0;
  *x = (char)SI->NewInputValue(types::CHAR, (addr_t)x, c);
}

void __CrestShortTrace(short* x, short c, char* iprange) {
#ifdef PRINT_FOR_TOOL
  fprintf(stderr, "INIT_SYM_VAR_Short: %d,%s\n\n", c, iprange);
#endif
  pre_symbolic = 0;
  *x = (short)SI->NewInputValue(types::SHORT, (addr_t)x, c);
}

void __CrestIntTrace(int* x, int c, char* iprange) {
#ifdef PRINT_FOR_TOOL
  // fprintf(stderr, "INIT_SYM_VAR_Int: %d,%s \n\n", c, iprange);
#endif
  pre_symbolic = 0;
  *x = (int)SI->NewInputValue(types::INT, (addr_t)x, c);
}

void __CrestIntTrace_1(int* x, int c, char* iprange) {
#ifdef PRINT_FOR_TOOL
  FILE* tr;
  tr = fopen("trace.txt", "a");
  static int i = 0;
  fprintf(tr, "INPUT:x%d=%d \n\n", i, c);
  i++;
  fclose(tr);
  FILE* local_ptr;
  local_ptr = fopen("local_var.txt", "a");
  fprintf(local_ptr, "INIT_SYM_VAR_Int: %d,%s \n\n", c, iprange);
  fclose(local_ptr);
#endif
  pre_symbolic = 0;
  *x = (int)SI->NewInputValue(types::INT, (addr_t)x, c);
}

void __CrestVarMap(void* addr, char* name, int tp, char* trigger = "true") {
  string* n = new string(name);
  string* tg = new string(trigger);
  SI->CreateVarMap((long unsigned int)addr, n, tp, tg);
}
void __CrestVarMap_1(void* addr, char* name, int tp, char* trigger) {
  string* n = new string(name);
  string* tg = new string(trigger);

  SI->CreateVarMap((long unsigned int)addr, n, tp, tg);
}

void __CrestVarMap_gdb(long unsigned int addr, char* name, int tp, char* trigger) {
  string* n = new string(name);
  string* tg = new string(trigger);

  SI->CreateVarMap(addr, n, tp, tg);
}

void __CrestLogState(unsigned int x, int r_w, int line, char* varname, int val, int* addr) {
  SI->ApplyLogState(x, r_w, line, varname, val, addr);
}

void __CrestPrint(unsigned int x, int r_w, int line, char* varname, int val, int* addr) {
  SI->print(x, r_w, line, varname, val, addr);
}

int __CrestGetTimeStamp() { return (SI->GetTimeStamp()); }

void __CrestLogState_1(unsigned int x) { SI->ApplyLogState_1(x); }

void __CrestLogState_gdb(unsigned int x) { SI->ApplyLogState_gdb(x); }

void __CrestPrintInput(char* name, int val) { SI->PrintInput(name, val); }
/*void __CrestLogState_2(unsigned int x) {
  SI->ApplyLogState_2(x);
}*/

void __CrestLogPC(unsigned int x) { SI->ApplyLogPC(x); }
void __CrestLogPCOnGdbQuery(unsigned int x) { SI->ApplyLogPC_gdb(x); }

void __CrestLogSpec(char* op, int* op1, int* op2) { SI->ApplyLogSpec(op, op1, op2); }

// MPI wrapper function definitions

int __CR_MPI_Init(int* argc, char*** argv) { MPI_Init(argc, argv); }
int __CR_MPI_Finalize(void) { MPI_Finalize(); }
int __CR_MPI_Comm_size(MPI_Comm comm, int* size) { MPI_Comm_size(comm, size); }

int __CR_MPI_Comm_rank(MPI_Comm comm, int* rank) { MPI_Comm_rank(comm, rank); }

int __CR_MPI_Send(const void* buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
  MPI_Send(buf, count, datatype, dest, tag, comm);
}

int __CR_MPI_Recv(void* buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status* status) {
  MPI_Recv(buf, count, datatype, source, tag, comm, status);
}

int __CR_MPI_Bcast(void* buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
  MPI_Bcast(buffer, count, datatype, root, comm);
}

int __CR_MPI_Reduce(const void* sendbuf, void* recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root,
                    MPI_Comm comm) {
  MPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm);
}

int __CR_MPI_Barrier(MPI_Comm comm) { MPI_Barrier(comm); }

int __CR_MPI_Scatter(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                     MPI_Datatype recvtype, int root, MPI_Comm comm) {

  MPI_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
}

int __CR_MPI_Gather(const void* sendbuf, int sendcount, MPI_Datatype sendtype, void* recvbuf, int recvcount,
                    MPI_Datatype recvtype, int root, MPI_Comm comm) {

  MPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm);
}
