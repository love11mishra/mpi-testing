(* This module was generated automatically by code in Makefile and machdep-ml.c *)
type mach = {
  version_major: int;     (* Major version number *)
  version_minor: int;     (* Minor version number *)
  version: string;        (* gcc version string *)
  underscore_name: bool;  (* If assembly names have leading underscore *)
  sizeof_short: int;      (* Size of "short" *)
  sizeof_int: int;        (* Size of "int" *)
  sizeof_bool: int;       (* Size of "_Bool" *)
  sizeof_long: int ;      (* Size of "long" *)
  sizeof_longlong: int;   (* Size of "long long" *)
  sizeof_ptr: int;        (* Size of pointers *)
  sizeof_float: int;      (* Size of "float" *)
  sizeof_double: int;     (* Size of "double" *)
  sizeof_longdouble: int; (* Size of "long double" *)
  sizeof_void: int;       (* Size of "void" *)
  sizeof_fun: int;        (* Size of function *)
  size_t: string;         (* Type of "sizeof(T)" *)
  wchar_t: string;        (* Type of "wchar_t" *)
  alignof_short: int;     (* Alignment of "short" *)
  alignof_int: int;       (* Alignment of "int" *)
  alignof_bool: int;      (* Alignment of "_Bool" *)
  alignof_long: int;      (* Alignment of "long" *)
  alignof_longlong: int;  (* Alignment of "long long" *)
  alignof_ptr: int;       (* Alignment of pointers *)
  alignof_enum: int;      (* Alignment of enum types *)
  alignof_float: int;     (* Alignment of "float" *)
  alignof_double: int;    (* Alignment of "double" *)
  alignof_longdouble: int;  (* Alignment of "long double" *)
  alignof_str: int;       (* Alignment of strings *)
  alignof_fun: int;       (* Alignment of function *)
  alignof_aligned: int;   (* Alignment of anything with the "aligned" attribute *)
  char_is_unsigned: bool; (* Whether "char" is unsigned *)
  const_string_literals: bool; (* Whether string literals have const chars *)
  little_endian: bool; (* whether the machine is little endian *)
  __thread_is_keyword: bool; (* whether __thread is a keyword *)
  __builtin_va_list: bool; (* whether __builtin_va_list is builtin (gccism) *)
}
let gcc = {
(* Generated by code in src/machdep-ml.c *)
	 version_major    = 9;
	 version_minor    = 3;
	 version          = "9.3.0";
	 sizeof_short          = 2;
	 sizeof_int            = 4;
	 sizeof_bool           = 1;
	 sizeof_long           = 8;
	 sizeof_longlong       = 8;
	 sizeof_ptr            = 8;
	 sizeof_float          = 4;
	 sizeof_double         = 8;
	 sizeof_longdouble     = 16;
	 sizeof_void           = 1;
	 sizeof_fun            = 1;
	 size_t                = "unsigned long";
	 wchar_t               = "int";
	 alignof_short         = 2;
	 alignof_int           = 4;
	 alignof_bool          = 1;
	 alignof_long          = 8;
	 alignof_longlong      = 8;
	 alignof_ptr           = 8;
	 alignof_enum          = 4;
	 alignof_float         = 4;
	 alignof_double        = 8;
	 alignof_longdouble    = 16;
	 alignof_str           = 1;
	 alignof_fun           = 1;
	 alignof_aligned       = 16;
	 char_is_unsigned      = false;
	 const_string_literals = true;
	 underscore_name       = false;
	 __builtin_va_list     = true;
	 __thread_is_keyword   = true;
	 little_endian         = true;
}
let hasMSVC = false
let msvc = gcc
let theMachine : mach ref = ref gcc
