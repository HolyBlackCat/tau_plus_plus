#ifndef PREPROCESSOR_H_INCLUDED
#define PREPROCESSOR_H_INCLUDED

// Version 0.0.8 by HolyBlackCat

#include <cstddef>
#include <type_traits>
#include <utility>


#define PP0_F_NULL(...)
#define PP0_F_COMMA(...) ,

#define PP0_E(...) __VA_ARGS__

#define PP0_VA_FIRST(...) PP0_VA_FIRST_IMPL_(__VA_ARGS__,)
#define PP0_VA_FIRST_IMPL_(x, ...) x

#define PP0_PARENS(...) (__VA_ARGS__)
#define PP0_DEL_PARENS(...) PP0_E __VA_ARGS__
#define PP0_2_PARENS(...) ((__VA_ARGS__))
#define PP0_PARENS_COMMA(...) (__VA_ARGS__),

#define PP0_CC(a, b) PP0_CC_IMPL_(a,b)
#define PP0_CC_IMPL_(a, b) a##b

#define PP0_CALL(macro, ...) macro(__VA_ARGS__)
#define PP0_CALL_A(macro, ...) macro(__VA_ARGS__)
#define PP0_CALL_B(macro, ...) macro(__VA_ARGS__)

#define PP0_VA_SIZE(...) PP0_VA_SIZE_IMPL_(__VA_ARGS__,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0)
#define PP0_VA_SIZE_NOT_1(...) PP0_VA_SIZE_IMPL_(__VA_ARGS__,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0)
#define PP0_VA_SIZE_IMPL_(i1,i2,i3,i4,i5,i6,i7,i8,i9,i10,i11,i12,i13,i14,i15,i16,i17,i18,i19,i20,i21,i22,i23,i24,i25,i26,i27,i28,i29,i30,i31,i32,size,...) size

#define PP0_STR(...) PP0_STR_IMPL_(__VA_ARGS__)
#define PP0_STR_IMPL_(...) #__VA_ARGS__

#define PP0_VA_CALL(name, ...) PP0_CC(name, PP0_VA_SIZE(__VA_ARGS__))

#define PP0_SEQ_CALL(name, seq) PP0_CC(name, PP0_SEQ_SIZE(seq))

#define PP0_SEQ_FIRST(seq) PP0_DEL_PARENS(PP0_VA_FIRST(PP0_PARENS_COMMA seq,))

#define PP0_SEQ_EMPTY(seq) PP0_VA_SIZE_NOT_1(PP0_CC(PP0_SEQ_EMPTY_IMPL_, PP0_SEQ_SIZE(seq)))
#define PP0_SEQ_EMPTY_IMPL_0 ,

#define PP0_VA_TO_SEQ(...) PP0_VA_CALL(PP0_VA_TO_SEQ_, __VA_ARGS__,)(__VA_ARGS__,)
#define PP0_VA_TO_SEQ_DISCARD_LAST(...) PP0_VA_CALL(PP0_VA_TO_SEQ_, __VA_ARGS__)(__VA_ARGS__)
#define PP0_VA_TO_SEQ_NULL
#define PP0_VA_TO_SEQ_1(x) PP0_VA_TO_SEQ_##x##NULL // If a error message points to this line, you probably forgot a trailing comma somewhere.
#define PP0_VA_TO_SEQ_2(x, ...) (x)PP0_VA_TO_SEQ_1(__VA_ARGS__)
#define PP0_VA_TO_SEQ_3(x, ...) (x)PP0_VA_TO_SEQ_2(__VA_ARGS__)
#define PP0_VA_TO_SEQ_4(x, ...) (x)PP0_VA_TO_SEQ_3(__VA_ARGS__)
#define PP0_VA_TO_SEQ_5(x, ...) (x)PP0_VA_TO_SEQ_4(__VA_ARGS__)
#define PP0_VA_TO_SEQ_6(x, ...) (x)PP0_VA_TO_SEQ_5(__VA_ARGS__)
#define PP0_VA_TO_SEQ_7(x, ...) (x)PP0_VA_TO_SEQ_6(__VA_ARGS__)
#define PP0_VA_TO_SEQ_8(x, ...) (x)PP0_VA_TO_SEQ_7(__VA_ARGS__)
#define PP0_VA_TO_SEQ_9(x, ...) (x)PP0_VA_TO_SEQ_8(__VA_ARGS__)
#define PP0_VA_TO_SEQ_10(x, ...) (x)PP0_VA_TO_SEQ_9(__VA_ARGS__)
#define PP0_VA_TO_SEQ_11(x, ...) (x)PP0_VA_TO_SEQ_10(__VA_ARGS__)
#define PP0_VA_TO_SEQ_12(x, ...) (x)PP0_VA_TO_SEQ_11(__VA_ARGS__)
#define PP0_VA_TO_SEQ_13(x, ...) (x)PP0_VA_TO_SEQ_12(__VA_ARGS__)
#define PP0_VA_TO_SEQ_14(x, ...) (x)PP0_VA_TO_SEQ_13(__VA_ARGS__)
#define PP0_VA_TO_SEQ_15(x, ...) (x)PP0_VA_TO_SEQ_14(__VA_ARGS__)
#define PP0_VA_TO_SEQ_16(x, ...) (x)PP0_VA_TO_SEQ_15(__VA_ARGS__)
#define PP0_VA_TO_SEQ_17(x, ...) (x)PP0_VA_TO_SEQ_16(__VA_ARGS__)
#define PP0_VA_TO_SEQ_18(x, ...) (x)PP0_VA_TO_SEQ_17(__VA_ARGS__)
#define PP0_VA_TO_SEQ_19(x, ...) (x)PP0_VA_TO_SEQ_18(__VA_ARGS__)
#define PP0_VA_TO_SEQ_20(x, ...) (x)PP0_VA_TO_SEQ_19(__VA_ARGS__)
#define PP0_VA_TO_SEQ_21(x, ...) (x)PP0_VA_TO_SEQ_20(__VA_ARGS__)
#define PP0_VA_TO_SEQ_22(x, ...) (x)PP0_VA_TO_SEQ_21(__VA_ARGS__)
#define PP0_VA_TO_SEQ_23(x, ...) (x)PP0_VA_TO_SEQ_22(__VA_ARGS__)
#define PP0_VA_TO_SEQ_24(x, ...) (x)PP0_VA_TO_SEQ_23(__VA_ARGS__)
#define PP0_VA_TO_SEQ_25(x, ...) (x)PP0_VA_TO_SEQ_24(__VA_ARGS__)
#define PP0_VA_TO_SEQ_26(x, ...) (x)PP0_VA_TO_SEQ_25(__VA_ARGS__)
#define PP0_VA_TO_SEQ_27(x, ...) (x)PP0_VA_TO_SEQ_26(__VA_ARGS__)
#define PP0_VA_TO_SEQ_28(x, ...) (x)PP0_VA_TO_SEQ_27(__VA_ARGS__)
#define PP0_VA_TO_SEQ_29(x, ...) (x)PP0_VA_TO_SEQ_28(__VA_ARGS__)
#define PP0_VA_TO_SEQ_30(x, ...) (x)PP0_VA_TO_SEQ_29(__VA_ARGS__)
#define PP0_VA_TO_SEQ_31(x, ...) (x)PP0_VA_TO_SEQ_30(__VA_ARGS__)
#define PP0_VA_TO_SEQ_32(x, ...) (x)PP0_VA_TO_SEQ_31(__VA_ARGS__)
#define PP0_VA_TO_SEQ_33(x, ...) (x)PP0_VA_TO_SEQ_32(__VA_ARGS__)

#define PP0_SEQ_SIZE(seq) PP0_CC(PP0_SEQ_SIZE_0 seq, _VAL)
#define PP0_SEQ_SIZE_0(...) PP0_SEQ_SIZE_1
#define PP0_SEQ_SIZE_1(...) PP0_SEQ_SIZE_2
#define PP0_SEQ_SIZE_2(...) PP0_SEQ_SIZE_3
#define PP0_SEQ_SIZE_3(...) PP0_SEQ_SIZE_4
#define PP0_SEQ_SIZE_4(...) PP0_SEQ_SIZE_5
#define PP0_SEQ_SIZE_5(...) PP0_SEQ_SIZE_6
#define PP0_SEQ_SIZE_6(...) PP0_SEQ_SIZE_7
#define PP0_SEQ_SIZE_7(...) PP0_SEQ_SIZE_8
#define PP0_SEQ_SIZE_8(...) PP0_SEQ_SIZE_9
#define PP0_SEQ_SIZE_9(...) PP0_SEQ_SIZE_10
#define PP0_SEQ_SIZE_10(...) PP0_SEQ_SIZE_11
#define PP0_SEQ_SIZE_11(...) PP0_SEQ_SIZE_12
#define PP0_SEQ_SIZE_12(...) PP0_SEQ_SIZE_13
#define PP0_SEQ_SIZE_13(...) PP0_SEQ_SIZE_14
#define PP0_SEQ_SIZE_14(...) PP0_SEQ_SIZE_15
#define PP0_SEQ_SIZE_15(...) PP0_SEQ_SIZE_16
#define PP0_SEQ_SIZE_16(...) PP0_SEQ_SIZE_17
#define PP0_SEQ_SIZE_17(...) PP0_SEQ_SIZE_18
#define PP0_SEQ_SIZE_18(...) PP0_SEQ_SIZE_19
#define PP0_SEQ_SIZE_19(...) PP0_SEQ_SIZE_20
#define PP0_SEQ_SIZE_20(...) PP0_SEQ_SIZE_21
#define PP0_SEQ_SIZE_21(...) PP0_SEQ_SIZE_22
#define PP0_SEQ_SIZE_22(...) PP0_SEQ_SIZE_23
#define PP0_SEQ_SIZE_23(...) PP0_SEQ_SIZE_24
#define PP0_SEQ_SIZE_24(...) PP0_SEQ_SIZE_25
#define PP0_SEQ_SIZE_25(...) PP0_SEQ_SIZE_26
#define PP0_SEQ_SIZE_26(...) PP0_SEQ_SIZE_27
#define PP0_SEQ_SIZE_27(...) PP0_SEQ_SIZE_28
#define PP0_SEQ_SIZE_28(...) PP0_SEQ_SIZE_29
#define PP0_SEQ_SIZE_29(...) PP0_SEQ_SIZE_30
#define PP0_SEQ_SIZE_30(...) PP0_SEQ_SIZE_31
#define PP0_SEQ_SIZE_31(...) PP0_SEQ_SIZE_32
#define PP0_SEQ_SIZE_32(...) PP0_SEQ_SIZE_33
#define PP0_SEQ_SIZE_0_VAL 0
#define PP0_SEQ_SIZE_1_VAL 1
#define PP0_SEQ_SIZE_2_VAL 2
#define PP0_SEQ_SIZE_3_VAL 3
#define PP0_SEQ_SIZE_4_VAL 4
#define PP0_SEQ_SIZE_5_VAL 5
#define PP0_SEQ_SIZE_6_VAL 6
#define PP0_SEQ_SIZE_7_VAL 7
#define PP0_SEQ_SIZE_8_VAL 8
#define PP0_SEQ_SIZE_9_VAL 9
#define PP0_SEQ_SIZE_10_VAL 10
#define PP0_SEQ_SIZE_11_VAL 11
#define PP0_SEQ_SIZE_12_VAL 12
#define PP0_SEQ_SIZE_13_VAL 13
#define PP0_SEQ_SIZE_14_VAL 14
#define PP0_SEQ_SIZE_15_VAL 15
#define PP0_SEQ_SIZE_16_VAL 16
#define PP0_SEQ_SIZE_17_VAL 17
#define PP0_SEQ_SIZE_18_VAL 18
#define PP0_SEQ_SIZE_19_VAL 19
#define PP0_SEQ_SIZE_20_VAL 20
#define PP0_SEQ_SIZE_21_VAL 21
#define PP0_SEQ_SIZE_22_VAL 22
#define PP0_SEQ_SIZE_23_VAL 23
#define PP0_SEQ_SIZE_24_VAL 24
#define PP0_SEQ_SIZE_25_VAL 25
#define PP0_SEQ_SIZE_26_VAL 26
#define PP0_SEQ_SIZE_27_VAL 27
#define PP0_SEQ_SIZE_28_VAL 28
#define PP0_SEQ_SIZE_29_VAL 29
#define PP0_SEQ_SIZE_30_VAL 30
#define PP0_SEQ_SIZE_31_VAL 31
#define PP0_SEQ_SIZE_32_VAL 32

#define PP0_VA_AT(index, ...) PP0_CC(PP0_VA_AT_, index)(__VA_ARGS__,)
#define PP0_VA_AT_0(ret, ...) ret
#define PP0_VA_AT_1(_, ...) PP0_VA_AT_0(__VA_ARGS__)
#define PP0_VA_AT_2(_, ...) PP0_VA_AT_1(__VA_ARGS__)
#define PP0_VA_AT_3(_, ...) PP0_VA_AT_2(__VA_ARGS__)
#define PP0_VA_AT_4(_, ...) PP0_VA_AT_3(__VA_ARGS__)
#define PP0_VA_AT_5(_, ...) PP0_VA_AT_4(__VA_ARGS__)
#define PP0_VA_AT_6(_, ...) PP0_VA_AT_5(__VA_ARGS__)
#define PP0_VA_AT_7(_, ...) PP0_VA_AT_6(__VA_ARGS__)
#define PP0_VA_AT_8(_, ...) PP0_VA_AT_7(__VA_ARGS__)
#define PP0_VA_AT_9(_, ...) PP0_VA_AT_8(__VA_ARGS__)
#define PP0_VA_AT_10(_, ...) PP0_VA_AT_9(__VA_ARGS__)
#define PP0_VA_AT_11(_, ...) PP0_VA_AT_10(__VA_ARGS__)
#define PP0_VA_AT_12(_, ...) PP0_VA_AT_11(__VA_ARGS__)
#define PP0_VA_AT_13(_, ...) PP0_VA_AT_12(__VA_ARGS__)
#define PP0_VA_AT_14(_, ...) PP0_VA_AT_13(__VA_ARGS__)
#define PP0_VA_AT_15(_, ...) PP0_VA_AT_14(__VA_ARGS__)
#define PP0_VA_AT_16(_, ...) PP0_VA_AT_15(__VA_ARGS__)
#define PP0_VA_AT_17(_, ...) PP0_VA_AT_16(__VA_ARGS__)
#define PP0_VA_AT_18(_, ...) PP0_VA_AT_17(__VA_ARGS__)
#define PP0_VA_AT_19(_, ...) PP0_VA_AT_18(__VA_ARGS__)
#define PP0_VA_AT_20(_, ...) PP0_VA_AT_19(__VA_ARGS__)
#define PP0_VA_AT_21(_, ...) PP0_VA_AT_20(__VA_ARGS__)
#define PP0_VA_AT_22(_, ...) PP0_VA_AT_21(__VA_ARGS__)
#define PP0_VA_AT_23(_, ...) PP0_VA_AT_22(__VA_ARGS__)
#define PP0_VA_AT_24(_, ...) PP0_VA_AT_23(__VA_ARGS__)
#define PP0_VA_AT_25(_, ...) PP0_VA_AT_24(__VA_ARGS__)
#define PP0_VA_AT_26(_, ...) PP0_VA_AT_25(__VA_ARGS__)
#define PP0_VA_AT_27(_, ...) PP0_VA_AT_26(__VA_ARGS__)
#define PP0_VA_AT_28(_, ...) PP0_VA_AT_27(__VA_ARGS__)
#define PP0_VA_AT_29(_, ...) PP0_VA_AT_28(__VA_ARGS__)
#define PP0_VA_AT_30(_, ...) PP0_VA_AT_29(__VA_ARGS__)
#define PP0_VA_AT_31(_, ...) PP0_VA_AT_30(__VA_ARGS__)
#define PP0_VA_AT_32(_, ...) PP0_VA_AT_31(__VA_ARGS__)

#define PP0_SEQ_AT(index, seq) PP0_SEQ_FIRST( PP0_CC(PP0_SEQ_AT_EAT_, index)(seq) )
#define PP0_SEQ_AT_EAT_0(seq) seq
#define PP0_SEQ_AT_EAT_1(seq) PP0_SEQ_AT_EAT_0(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_2(seq) PP0_SEQ_AT_EAT_1(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_3(seq) PP0_SEQ_AT_EAT_2(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_4(seq) PP0_SEQ_AT_EAT_3(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_5(seq) PP0_SEQ_AT_EAT_4(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_6(seq) PP0_SEQ_AT_EAT_5(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_7(seq) PP0_SEQ_AT_EAT_6(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_8(seq) PP0_SEQ_AT_EAT_7(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_9(seq) PP0_SEQ_AT_EAT_8(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_10(seq) PP0_SEQ_AT_EAT_9(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_11(seq) PP0_SEQ_AT_EAT_10(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_12(seq) PP0_SEQ_AT_EAT_11(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_13(seq) PP0_SEQ_AT_EAT_12(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_14(seq) PP0_SEQ_AT_EAT_13(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_15(seq) PP0_SEQ_AT_EAT_14(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_16(seq) PP0_SEQ_AT_EAT_15(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_17(seq) PP0_SEQ_AT_EAT_16(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_18(seq) PP0_SEQ_AT_EAT_17(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_19(seq) PP0_SEQ_AT_EAT_18(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_20(seq) PP0_SEQ_AT_EAT_19(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_21(seq) PP0_SEQ_AT_EAT_20(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_22(seq) PP0_SEQ_AT_EAT_21(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_23(seq) PP0_SEQ_AT_EAT_22(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_24(seq) PP0_SEQ_AT_EAT_23(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_25(seq) PP0_SEQ_AT_EAT_24(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_26(seq) PP0_SEQ_AT_EAT_25(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_27(seq) PP0_SEQ_AT_EAT_26(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_28(seq) PP0_SEQ_AT_EAT_27(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_29(seq) PP0_SEQ_AT_EAT_28(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_30(seq) PP0_SEQ_AT_EAT_29(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_31(seq) PP0_SEQ_AT_EAT_30(PP0_F_NULL seq)
#define PP0_SEQ_AT_EAT_32(seq) PP0_SEQ_AT_EAT_31(PP0_F_NULL seq)
// macro(i, data, element...)
#define PP0_SEQ_APPLY(seq, macro, macro_sep_f, data) PP0_SEQ_CALL(PP0_SEQ_APPLY_, seq)(macro, macro_sep_f, data, seq, )
#define PP0_SEQ_APPLY_0(macro, macro_sep_f, data, seq, seq_)
#define PP0_SEQ_APPLY_1(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq))
#define PP0_SEQ_APPLY_2(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_3(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_4(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_5(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_6(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_7(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_8(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_9(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_10(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_11(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_12(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_13(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_14(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_15(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_16(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_17(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_18(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_19(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_20(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_21(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_22(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_23(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_24(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_25(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_26(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_27(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_28(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_29(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_30(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_31(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_32(macro, macro_sep_f, data, seq, seq_) PP0_CALL(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A(seq, macro, macro_sep_f, data) PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, seq)(macro, macro_sep_f, data, seq, )
#define PP0_SEQ_APPLY_A_0(macro, macro_sep_f, data, seq, seq_)
#define PP0_SEQ_APPLY_A_1(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq))
#define PP0_SEQ_APPLY_A_2(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_3(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_4(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_5(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_6(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_7(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_8(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_9(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_10(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_11(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_12(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_13(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_14(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_15(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_16(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_17(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_18(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_19(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_20(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_21(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_22(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_23(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_24(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_25(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_26(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_27(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_28(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_29(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_30(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_31(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)
#define PP0_SEQ_APPLY_A_32(macro, macro_sep_f, data, seq, seq_) PP0_CALL_A(macro, PP0_SEQ_SIZE(seq_), data, PP0_SEQ_FIRST(seq)) macro_sep_f() PP0_SEQ_CALL(PP0_SEQ_APPLY_A_, PP0_F_NULL seq)(macro, macro_sep_f, data, PP0_F_NULL seq, (PP0_SEQ_FIRST(seq)) seq_)


#endif
