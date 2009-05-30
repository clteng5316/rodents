// preprocessor.hpp
#pragma once

//
#define PP_NULL
#define PP_COMMA		,
#define PP_NOOP(v)		v

// 文字列化.
#define PP_QUOTE(v)		PP_QUOTE_1((v))
#define PP_QUOTE_1(p)	PP_QUOTE_0 ## p
#define PP_QUOTE_0(v)	#v

// 連結.
#define PP_CAT(a, b)	PP_CAT_1((a, b))
#define PP_CAT_1(p)		PP_CAT_0 ## p
#define PP_CAT_0(a, b)	PP_NOOP(a ## b)

// ユニークID.
#define PP_ID(base)		PP_CAT(base, __COUNTER__)

// REPEAT
#define PP_REPEAT_0(fn, sep)	
#define PP_REPEAT_1(fn, sep)	fn(1)
#define PP_REPEAT_2(fn, sep)	PP_REPEAT_1(fn, sep) sep fn(2)
#define PP_REPEAT_3(fn, sep)	PP_REPEAT_2(fn, sep) sep fn(3)
#define PP_REPEAT_4(fn, sep)	PP_REPEAT_3(fn, sep) sep fn(4)
#define PP_REPEAT_5(fn, sep)	PP_REPEAT_4(fn, sep) sep fn(5)
#define PP_REPEAT_6(fn, sep)	PP_REPEAT_5(fn, sep) sep fn(6)
#define PP_REPEAT_7(fn, sep)	PP_REPEAT_6(fn, sep) sep fn(7)
#define PP_REPEAT_8(fn, sep)	PP_REPEAT_7(fn, sep) sep fn(8)
#define PP_REPEAT_9(fn, sep)	PP_REPEAT_8(fn, sep) sep fn(9)
#define PP_REPEAT(n, fn, sep)	PP_CAT(PP_REPEAT_, n)(fn, sep)

// CSV
#define PP_CSV_0(fn)
#define PP_CSV_1(fn)		fn(1)
#define PP_CSV_2(fn)		PP_CSV_1(fn), fn(2)
#define PP_CSV_3(fn)		PP_CSV_2(fn), fn(3)
#define PP_CSV_4(fn)		PP_CSV_3(fn), fn(4)
#define PP_CSV_5(fn)		PP_CSV_4(fn), fn(5)
#define PP_CSV_6(fn)		PP_CSV_5(fn), fn(6)
#define PP_CSV_7(fn)		PP_CSV_6(fn), fn(7)
#define PP_CSV_8(fn)		PP_CSV_7(fn), fn(8)
#define PP_CSV_9(fn)		PP_CSV_8(fn), fn(9)
#define PP_CSV_10(fn)		PP_CSV_9(fn), fn(10)
#define PP_CSV(n, fn)		PP_CAT(PP_CSV_, n)(fn)

// APPEND
#define PP_APPEND_0(fn)
#define PP_APPEND_1(fn)		PP_APPEND_0(fn), fn(1)
#define PP_APPEND_2(fn)		PP_APPEND_1(fn), fn(2)
#define PP_APPEND_3(fn)		PP_APPEND_2(fn), fn(3)
#define PP_APPEND_4(fn)		PP_APPEND_3(fn), fn(4)
#define PP_APPEND_5(fn)		PP_APPEND_4(fn), fn(5)
#define PP_APPEND_6(fn)		PP_APPEND_5(fn), fn(6)
#define PP_APPEND_7(fn)		PP_APPEND_6(fn), fn(7)
#define PP_APPEND_8(fn)		PP_APPEND_7(fn), fn(8)
#define PP_APPEND_9(fn)		PP_APPEND_8(fn), fn(9)
#define PP_APPEND_10(fn)	PP_APPEND_9(fn), fn(10)
#define PP_APPEND(n, fn)	PP_CAT(PP_APPEND_, n)(fn)

// STATEMENT
#define PP_STATEMENT_0(fn)	fn(0)
#define PP_STATEMENT_1(fn)	PP_STATEMENT_0(fn) fn(1)
#define PP_STATEMENT_2(fn)	PP_STATEMENT_1(fn) fn(2)
#define PP_STATEMENT_3(fn)	PP_STATEMENT_2(fn) fn(3)
#define PP_STATEMENT_4(fn)	PP_STATEMENT_3(fn) fn(4)
#define PP_STATEMENT_5(fn)	PP_STATEMENT_4(fn) fn(5)
#define PP_STATEMENT_6(fn)	PP_STATEMENT_5(fn) fn(6)
#define PP_STATEMENT_7(fn)	PP_STATEMENT_6(fn) fn(7)
#define PP_STATEMENT_8(fn)	PP_STATEMENT_7(fn) fn(8)
#define PP_STATEMENT_9(fn)	PP_STATEMENT_8(fn) fn(9)
#define PP_STATEMENT(n, fn)	PP_CAT(PP_STATEMENT_, n)(fn)

// typename An
#define PP_TYPENAME_(n)			typename A##n
#define PP_TYPENAME(n)			PP_CSV(n, PP_TYPENAME_)
#define PP_TYPENAME_APPEND(n)	PP_APPEND(n, PP_TYPENAME_)

// An
#define PP_ARGT_(n)				A##n
#define PP_ARGT(n)				PP_CSV(n, PP_ARGT_)
#define PP_ARGT_APPEND(n)		PP_APPEND(n, PP_ARGT_)

// An An
#define PP_ARGS_(n)				A##n a##n
#define PP_ARGS(n)				PP_CSV(n, PP_ARGS_)
#define PP_ARGS_APPEND(n)		PP_APPEND(n, PP_ARGS_)

// an
#define PP_ARGV_(n)				a##n
#define PP_ARGV(n)				PP_CSV(n, PP_ARGV_)
#define PP_ARGV_APPEND(n)		PP_APPEND(n, PP_ARGV_)
