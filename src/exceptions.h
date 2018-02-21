#ifndef EXCEPTIONS_H_INCLUDED
#define EXCEPTIONS_H_INCLUDED

#include <algorithm>
#include <exception>
#include <string>
#include <type_traits>

#include "preprocessor.h"
#include "reflection.h"
#include "strings.h"

/* Syntax:
 *
 *   DefineExceptionBase(my_base [, : upper_base])
 */
#define DefineExceptionBase(...) PP0_VA_CALL(DefineExceptionBase_A_, __VA_ARGS__)(__VA_ARGS__)
#define DefineExceptionBase_A_1(name_       ) DefineExceptionBase_A_2(name_, : ::std::exception)
#define DefineExceptionBase_A_2(name_, base_) struct name_ base_ {const char *what() const noexcept override = 0; virtual ~name_() {}};

/* Syntax:
 *
 *   DefineException[Inline|Static](my_exception, [: my_base,] "Stuff happened.",
 *     (int,foo,"Foo")
 *     (float,bar,"Bar")
 *   ) // |    |    |
 *     // |    |    `- display name
 *     // |    `- name
 *     // `- type
 *
 * Names starting with `_` are reserved for the class implementation. `what` is also reserved.
 */
#define DefineException(name_, ...)       [[nodiscard]] PP0_VA_CALL(DefineException_A_, __VA_ARGS__)(name_, __VA_ARGS__)
#define DefineExceptionStatic(name_, ...) [[nodiscard]] static PP0_VA_CALL(DefineException_A_, __VA_ARGS__)(name_, __VA_ARGS__)
#define DefineExceptionInline(name_, ...) [[nodiscard]] inline PP0_VA_CALL(DefineException_A_, __VA_ARGS__)(name_, __VA_ARGS__)
#define DefineException_A_2(name_,        text_, seq_) DefineException_A_3(name_, : ::std::exception, text_, seq_)
#define DefineException_A_3(name_, base_, text_, seq_) \
    auto name_( PP0_SEQ_APPLY(seq_, DefineException_B_Param, PP0_F_COMMA,) ) \
    { \
        struct name_##_t base_ \
        { \
            /* Description. */\
            mutable ::std::string _description; \
            /* Fields. */\
            PP0_SEQ_APPLY(seq_, DefineException_B_Field, PP0_F_NULL,) \
            /* Generate description. */\
            void _generate_description() const \
            { \
                _description = text_; \
                PP0_SEQ_APPLY(seq_, DefineException_B_String, PP0_F_NULL,) \
            } \
            /* What? */\
            const char *what() const noexcept override \
            { \
                try \
                { \
                    _generate_description(); \
                } \
                catch (...) \
                { \
                    return text_; \
                } \
                return _description.c_str(); \
            } \
            /* Constructors. */\
            PP0_CC(DefineException_B_Ctor_, PP0_SEQ_EMPTY(seq_))(name_) \
            name_##_t( PP0_SEQ_APPLY(seq_, DefineException_B_Param, PP0_F_COMMA,) ) \
            { \
                PP0_SEQ_APPLY(seq_, DefineException_B_ParamAssign, PP0_F_NULL,) \
            } \
            /* Destructor. */\
            virtual ~name_##_t() {} \
        } \
        ret { PP0_SEQ_APPLY(seq_, DefineException_B_ParamNames, PP0_F_COMMA,) }; \
        return ret; \
    }
#define DefineException_B_Ctor_1(name_)
#define DefineException_B_Ctor_0(name_) name_##_t() {}
#define DefineException_B_Param(i_, data_, type_, name_, pretty_name_) \
    ::std::enable_if_t<1, type_> _param_##name_
#define DefineException_B_ParamNames(i_, data_, type_, name_, pretty_name_) \
    _param_##name_
#define DefineException_B_ParamAssign(i_, data_, type_, name_, pretty_name_) \
    name_ = _param_##name_;
#define DefineException_B_Field(i_, data_, type_, name_, pretty_name_) \
    ::std::enable_if_t<1, type_> name_;
#define DefineException_B_String(i_, data_, type_, name_, pretty_name_) \
    _description += "\n" pretty_name_; \
    _description += ": "; \
    if constexpr (std::is_same_v<::std::remove_cv_t<type_>, ::std::string>) \
    { \
        ::std::string _tmp = ::Strings::Escape(Strings::Trim((const ::std::string &)name_)); \
        if (::std::find(_tmp.begin(), _tmp.end(), '\n') != _tmp.end()) \
            _description += "\n"; \
        _description += _tmp; \
    } \
    else \
        _description += ::Reflection::to_string(name_);

#endif
