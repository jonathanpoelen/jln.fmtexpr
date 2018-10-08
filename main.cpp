#include <kvasir/mpl/algorithm/count_if.hpp>
#include <kvasir/mpl/algorithm/filter.hpp>
#include <kvasir/mpl/algorithm/flatten.hpp>
#include <kvasir/mpl/algorithm/fold_left.hpp>
#include <kvasir/mpl/algorithm/remove_adjacent.hpp>
#include <kvasir/mpl/functional/bind.hpp>
#include <kvasir/mpl/functional/call.hpp>
#include <kvasir/mpl/functional/compose.hpp>
#include <kvasir/mpl/functional/fork.hpp>
#include <kvasir/mpl/sequence/front.hpp>
#include <kvasir/mpl/sequence/join.hpp>
#include <kvasir/mpl/sequence/pop_front.hpp>
#include <kvasir/mpl/sequence/push_back.hpp>
#include <kvasir/mpl/sequence/size.hpp>
#include <kvasir/mpl/sequence/drop.hpp>
#include <kvasir/mpl/types/list.hpp>
#include <kvasir/mpl/types/bool.hpp>
#include <kvasir/mpl/utility/conditional.hpp>
#include <kvasir/mpl/utility/same_as.hpp>
#include <kvasir/mpl/utility/always.hpp>

// // extra kvasir.mpl
// namespace kvasir::mpl
// {
//   // TODO remove_adjacent is probably bogus
//
//   template<class P, class C = identity>
//   struct disjunction
//   {
//     template<class... Ts>
//     using f = typename C::template f<
//       bool_<typename P::template f<Ts>::value || ...>>;
//   };
//
//   template<class P, class C = identity>
//   struct conjunction
//   {
//     template<class... Ts>
//     using f = typename C::template f<
//       bool_<typename P::template f<Ts>::value && ...>>;
//   };
// }

#include <utility>
#include <tuple>
#include <type_traits>
#include <string_view>

#define JLN_FWD(t, x) static_cast<t&&>(x)

namespace jln::fmtexpr
{
  namespace kmp = kvasir::mpl;
  namespace kmpe = kvasir::mpl::eager;

  namespace mp
  {
    using kmp::list;

    template<class F, class... Ts>
    using eager = typename F::template f<Ts...>;

    template<class... Ls>
    using join = typename kmp::join<kmp::listify>::template f<Ls...>;

    template<class, class>
    struct push_back_if_not_found;

    template<class... Ts, class T>
    struct push_back_if_not_found<list<Ts...>, T>
    {
      using type = typename kmp::conditional<(std::is_same<T, Ts>::value || ...)>
        ::template f<kmp::pop_front<>, kmp::rotate<kmp::int_<1>>>
        ::template f<T, Ts...>;
    };

    template<class C = kmp::listify>
    struct unique_append
    {
      template<class L, class... Ts>
      using f = typename kmp::fold_left<
        kmp::cfl<push_back_if_not_found>,
        kmp::unpack<C>
      >::template f<L, Ts...>;
    };

    template<class C = kmp::listify>
    struct unique
    {
      template<class... Ts>
      using f = typename unique_append<C>::template f<list<>, Ts...>;
    };
  }

  namespace detail
  {
    template<template<class> class Tpl, class T, class Default, class = void>
    struct extract_type
    {
      using type = Default;
    };

    template<template<class> class Tpl, class T, class Default>
    struct extract_type<Tpl, T, Default, std::void_t<Tpl<T>>>
    {
      using type = Tpl<T>;
    };

    template<template<class> class Tpl, class T, class Default>
    using extract_type_t = typename extract_type<Tpl, T, Default>::type;
  }

  template<class T> using binded_variable_t = typename T::binded_variable;
  template<class T> using desc_type_t = typename T::desc_type;
  template<class T> using dep_list_t = typename T::dep_list;
  template<class T> using fmtexpr_type_t = typename T::jln_fmtexpr_type;
  template<class T> using fmtexpr_expr_t = typename T::jln_fmtexpr_expr;

  template<class T>
  using get_deps = detail::extract_type_t<dep_list_t, T, kmp::list<>>;

  template<class... Ts>
  using get_deps_xs = mp::join<get_deps<Ts>...>;

  template<class T>
  using get_desc = detail::extract_type_t<desc_type_t, T, void>;

  template<class T>
  using get_binded_variable
    = detail::extract_type_t<binded_variable_t, T, void>;

  template<class T>
  using get_fmtexpr_or_type = detail::extract_type_t<fmtexpr_type_t, T, T>;

  template<class T>
  using get_fmtexpr_or_expr = detail::extract_type_t<fmtexpr_expr_t, T, T>;

  template<class Var, class Desc, class T>
  struct value
  {
    using jln_fmtexpr_type = value;
    using binded_variable = Var;
    using desc_type = Desc;
    using dep_list = get_deps_xs<Desc, T>;

    Desc desc;
    T x;

    template<class X>
    constexpr auto& operator()(X&& x) const
    {
      return *this;
    }
  };

  template<class Var, class Desc>
  struct statement
  {
    using jln_fmtexpr_type = statement;
    using desc_type = Desc;
    using dep_list = mp::join<
      kmpe::conditional<std::is_same_v<Var, void>, kmp::list<>, kmp::list<Var>>,
      get_deps<Desc>>;

    Desc desc;

    template<class X>
    constexpr value<Var, Desc, X> operator()(X&& x) const
    {
      return {desc, JLN_FWD(X, x)};
    }
  };

  class unspecified {};

  template<class Var>
  using variable = statement<Var, unspecified>;

  template<class Desc>
  using primitive = statement<void, Desc>;

  template<class Var, class Desc>
  using typed_variable = statement<Var, Desc>;

  template<class Op, class Lhs, class Rhs>
  struct expression
  {
    using dep_list = kmpe::join<get_deps<Lhs>, get_deps<Rhs>>;

    Op op;
    Lhs lhs;
    Rhs rhs;

    template<class X>
    constexpr auto operator()(X&& x) const
    {
      return op(lhs(JLN_FWD(X, x)), rhs(JLN_FWD(X, x)));
    }
  };

  template<class Op, class Lhs, class Rhs>
  expression(Op, Lhs, Rhs) -> expression<Op, Lhs, Rhs>;



#define JLN_FMT_MAKE_IS(type)                    \
  template<class T>                              \
  struct is_##type : std::false_type             \
  {};                                            \
                                                 \
  template<class... Ts>                          \
  struct is_##type<type<Ts...>> : std::true_type \
  {};                                            \
                                                 \
  template<class T>                              \
  constexpr inline bool is_##type##_v = is_##type<T>::value

  JLN_FMT_MAKE_IS(value);
  JLN_FMT_MAKE_IS(statement);
  JLN_FMT_MAKE_IS(expression);

#undef JLN_FMT_MAKE_IS

  namespace detail
  {
    template<class S1>
    struct statement_minimal
    {
      using type = S1;
    };

    template<class Var, class Desc>
    struct statement_minimal<statement<Var, statement<void, Desc>>>
    {
      using type = statement<Var, Desc>;
    };
  }

#define JLN_FMTEXPR_VAR(name)      \
  constexpr struct name :          \
    ::jln::fmtexpr::variable<name> \
  {} name {}

#define JLN_FMTEXPR_TYPED_VAR(name, desc)         \
  constexpr struct name :                         \
    ::jln::fmtexpr::detail::statement_minimal<    \
      ::jln::fmtexpr::typed_variable<             \
        name, desc_type_t<decltype(desc)>>>::type \
  {} name {}

  template<class Name>
  variable<Name> named(Name /*deps*/)
  {
    return {};
  }

  template<class Desc, class Name>
  typed_variable<Name, Desc> typed(variable<Name> /*var*/, Desc desc)
  {
    return {desc};
  }

  template<class Desc, class Name>
  typed_variable<Name, Desc> typed(variable<Name> /*var*/)
  {
    return {};
  }

  template<class Name, class Desc>
  typed_variable<Name, Desc> typed_name(Name /*deps*/, Desc desc)
  {
    return {desc};
  }

  template<class Name, class Desc>
  typed_variable<Name, Desc> typed_name(Desc desc)
  {
    return {desc};
  }


  namespace mf
  {
    template<class T>
    constexpr bool check_value(T const&)
    {
      static_assert(is_value_v<T>, "T isn't jln::fmtexpr::value<...> type");
      return true;
    }
  }

  template<class Pack, class F>
  auto my_apply(Pack&& pack, F&& f)
  {
    return std::apply(JLN_FWD(F, f), JLN_FWD(Pack, pack));
  }

  using std::apply;
  template<class... Xs>
  using pack_t = std::tuple<Xs...>;

  template<class... Xs>
  pack_t<std::remove_reference_t<Xs>...> pack(Xs&&... xs)
  {
    // static_assert((mf::check_value(xs) && ...));
    return {JLN_FWD(Xs, xs)...};
  }


  namespace ext
  {
    // Deps for more readable errors
    template<class Deps, class Decl, class... Xs>
    constexpr auto make_value(Decl const& decl, Xs&&... xs)
    -> decltype(decl(JLN_FWD(Xs, xs)...))
    {
      return decl(JLN_FWD(Xs, xs)...);
    }

    template<class Deps, class Decl, class... Xs>
    constexpr auto make_value(Decl const& /*decl*/, Xs&&... xs)
    {
      return Decl{JLN_FWD(Xs, xs)...};
    }
  }

  // template<class Named>
  // maker<Tag, named_param<Named, T>> operator[](Named) const
  // {
  //   static_assert(!has_named_type<T>::value);
  //   return {};
  // }
  template<std::size_t n> struct static_size;
  template<std::size_t n> struct limited_size;
  struct dyn_size;

  template<class T>
  using safe_int = T;

  namespace mt
  {
    template<std::size_t i>
    using size_t = std::integral_constant<std::size_t, i>;

    template<char... c>
    struct string_c
    {
      static constexpr char value[sizeof...(c) + 1] = {c..., 0};
    };
  }

  namespace mf
  {
    template<std::size_t... i, class F>
    void for_n(std::integer_sequence<std::size_t, i...>, F f)
    {
      f(mt::size_t<i>{}...);
    }

    template<std::size_t n, class F>
    void for_n(F f)
    {
      for_n(std::make_index_sequence<n>{}, f);
    }
  }

  namespace literals
  {
    template<class Ch, Ch...c>
    constexpr mt::string_c<c...> operator""_c()
    {
      return {};
    }
  }
}

#include <iostream>

namespace jln::fmtexpr::ext::print
{
  class tag;

  namespace types
  {
    /**
     * fixed width integer types
     * @{
     */
    template<class T>
    struct integer
    {
      static_assert(std::is_integral<T>::value);

      using sizeof_ = static_size<sizeof(T)>;

      safe_int<T> x;

      constexpr sizeof_ serialize(uint8_t* p) const
      {
        mf::for_n<sizeof(T)>([&](auto...i){
          ((p[i] = x >> ((sizeof(T)-1-i) * 8)), ...);
        });
        return sizeof_{};
      }
    };

    inline auto int_ = primitive<types::integer<int>>{};
  }

  namespace detail
  {
#if __clang__
    template<class T>
    std::string_view get_type_name()
    {
      const std::string_view s = __PRETTY_FUNCTION__;
      const std::size_t start = 72;
      const std::size_t end = 1;
      return {s.begin()+start, s.size() - start - end};
    }
#else
    template<class T>
    std::string_view get_type_name()
    {
      const std::string_view s = __PRETTY_FUNCTION__;
      const std::size_t start = 74;
      const std::size_t end = 50;
      return {s.begin()+start, s.size() - start - end};
    }
#endif

    template<class... Ts>
    void print_deps(kmp::list<Ts...>)
    {
      ((std::cout << detail::get_type_name<Ts>() << "  "), ...);
    }

    template<class Value>
    void print_element(Value const& value)
    {
      using fmtexpr_or_type = get_fmtexpr_or_type<Value>;
      std::cout << get_type_name<Value>() << "\n";
      if (!std::is_same_v<fmtexpr_or_type, Value>)
      {
        std::cout << "  fmtexpr type: " << get_type_name<fmtexpr_or_type>() << "\n";
      }
      std::cout << "  linked to: " << get_type_name<get_binded_variable<Value>>() << "\n";
      std::cout << "  dependencies (" << kmpe::size<get_deps<Value>>::value << "): "; print_deps(get_deps<Value>{}); std::cout << "\n";
      std::cout << "  description: " << get_type_name<get_desc<Value>>() << "\n";
      if constexpr (is_value_v<fmtexpr_or_type>)
      {
        std::cout << "  value_type: " << get_type_name<decltype(value.x)>() << "\n";
        std::cout << "  value: " << value.x << "\n";
      }
      else if constexpr (is_statement_v<fmtexpr_or_type>)
      {
        std::cout << "  value: (unevaluated statement)\n";
      }
      else if constexpr (is_expression_v<fmtexpr_or_type>)
      {
        std::cout << "  value: (unevaluated expression)\n";
      }
      else
      {
        std::cout << "  value: " << value << "\n";
      }
      std::cout << "\n";
    }
  }

  template<class Pack>
  void introspection(Pack const& pack)
  {
    my_apply(pack, [](auto&... xs){
      (detail::print_element(xs), ...);

#define T(x) std::remove_reference_t<decltype(x)>
      using deps = mp::eager<
        kmp::join<mp::unique<>>,
        get_deps<T(xs)>...>;
      using satisfactory_deps = mp::eager<
        kmp::remove_if<kmp::same_as<void>, mp::unique<>>,
        get_binded_variable<T(xs)>...>;
      using unsatisfactory_deps = mp::eager<
        kmp::unpack<mp::unique_append<kmp::drop<kmpe::size<satisfactory_deps>>>>, deps, satisfactory_deps>;
#undef T

      std::cout << "dependencies (" << kmpe::size<deps>::value << "): ";
      detail::print_deps(deps{}); std::cout << "\n";

      std::cout << "satisfactory dependencies (" << kmpe::size<satisfactory_deps>::value << "): ";
      detail::print_deps(satisfactory_deps{}); std::cout << "\n";

      std::cout << "unsatisfactory dependencies (" << kmpe::size<unsatisfactory_deps>::value << "): ";
      detail::print_deps(unsatisfactory_deps{}); std::cout << "\n";
    });
  }
}


using namespace jln::fmtexpr;
// using namespace jln::fmtexpr::literals;
using namespace jln::fmtexpr::ext::print;
using namespace jln::fmtexpr::ext::print::types;

int main(/*int argc, char **argv*/)
{
  JLN_FMTEXPR_VAR(a);
  JLN_FMTEXPR_TYPED_VAR(b, int_);
  auto c = variable<class c>();
  auto d = typed_name<class d>(int_);
  auto e = expression{std::plus{}, a, c};
  int i = 3;
  // auto expr = named<class r>(a + b) = eval(var<class r>);

  auto packet = pack(
    0, int_, int_(i), int_(0),
    a, a(1),
    a, a(1),
    b, /*b(2),*/
    c, c(i),
    d, d(4),
    e
  );
  introspection(packet);
}
