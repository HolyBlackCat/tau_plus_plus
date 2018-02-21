#ifndef TEMPLATE_H_INCLUDED
#define TEMPLATE_H_INCLUDED

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

namespace TemplateUtils
{
    namespace impl
    {
        template <typename T> struct func_types {};
        template <typename R, typename ...P> struct func_types<R(P...)>
        {
            using return_type = R;
            using param_types = std::tuple<P...>;
        };
        template <typename R, typename ...P> struct func_types<R(P...) noexcept>
        {
            using return_type = R;
            using param_types = std::tuple<P...>;
        };
    }
    template <typename T> using return_type = typename impl::func_types<T>::return_type;
    template <typename T> using param_types = typename impl::func_types<T>::param_types;


    // It's a CRTP base. It expects T to have `static void OnMove(T &&from, T &to)` and will call it on move ctor/assignment.
    template <typename T> struct MoveFunc
    {
        MoveFunc() noexcept {}

        MoveFunc(const MoveFunc &) noexcept {}

        MoveFunc(MoveFunc &&o) noexcept(noexcept(T::OnMove(std::declval<T &&>(), std::declval<T &>())))
        {
            static_assert(std::is_base_of_v<MoveFunc<T>, T>, "This is a CRTP base.");
            T::OnMove(static_cast<T &&>(o), static_cast<T &>(*this));
        }

        MoveFunc &operator=(const MoveFunc &) noexcept {}

        MoveFunc &operator=(MoveFunc &&o) noexcept(noexcept(T::OnMove(std::declval<T &&>(), std::declval<T &>())))
        {
            static_assert(std::is_base_of_v<MoveFunc<T>, T>, "This is a CRTP base.");
            if (&o == this)
                return *this;
            T::OnMove(static_cast<T &&>(o), static_cast<T &>(*this));
            return *this;
        }
    };


    template <typename T> struct ResetOnMove
    {
        T obj;
      public:
        ResetOnMove() = default;
        ~ResetOnMove() = default;

        ResetOnMove(const ResetOnMove &) = delete;
        ResetOnMove &operator=(const ResetOnMove &) = delete;

        ResetOnMove(ResetOnMove &&other) : obj(std::move(other.obj))
        {
            other.obj = {};
        }
        ResetOnMove &operator=(ResetOnMove &&other)
        {
            if (&other == this)
                return *this;
            obj = std::move(other.obj);
            other.obj = {};
            return *this;
        }

        template <typename P, typename = std::enable_if_t<!std::is_same_v<ResetOnMove, std::remove_cv_t<std::remove_reference_t<P>>>>> ResetOnMove(P &&param) : obj(std::forward<P>(param)) {}
        template <typename ...P, typename = std::enable_if_t<(sizeof...(P) > 1)>> ResetOnMove(P &&... params) : obj(std::forward<P>(params)...) {}

        template <typename P, typename = std::enable_if_t<!std::is_same_v<ResetOnMove, std::remove_cv_t<std::remove_reference_t<P>>>>> T &operator=(P &&param)
        {
            obj = std::forward<P>(param);
            return obj;
        }

        [[nodiscard]] operator       T &()       {return obj;}
        [[nodiscard]] operator const T &() const {return obj;}

        [[nodiscard]]       T &value()       {return obj;}
        [[nodiscard]] const T &value() const {return obj;}

        T operator->() {return obj;}
    };


    template <typename F, std::size_t ...Seq> static void for_each(std::index_sequence<Seq...>, F &&f)
    {
        (f(std::integral_constant<std::size_t, Seq>{}) , ...);
    }

    inline namespace CexprStr
    {
        template <char ...C> struct str_lit
        {
            static constexpr char value[] {C..., '\0'};
        };

        namespace impl
        {
            template <typename ...P> struct str_lit_cat {using type = str_lit<>;};
            template <typename A, typename B, typename ...P> struct str_lit_cat<A, B, P...> {using type = typename str_lit_cat<typename str_lit_cat<A, B>::type, P...>::type;};
            template <char ...A, char ...B> struct str_lit_cat<str_lit<A...>, str_lit<B...>> {using type = str_lit<A..., B...>;};
            template <typename T> struct str_lit_cat<T> {using type = T;};
        }
        template <typename ...P> using str_lit_cat = typename impl::str_lit_cat<P...>::type;
    }
}

#endif
