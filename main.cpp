#include <kvasir/mpl/types/list.hpp>
#include <kvasir/mpl/sequence/size.hpp>

#include <utility>
#include <type_traits>
#include <string_view>

#define JLN_FWD(t, x) static_cast<t&&>(x)

namespace jln::fmtexpr
{
  namespace kmp = kvasir::mpl;
  namespace kmpe = kvasir::mpl::eager;

  namespace detail
  {
    template<class T, class = void>
    struct decl_or_t_impl
    {
      using type = T;
    };

    template<class T>
    struct decl_or_t_impl<T, std::void_t<typename T::decl_type>>
    {
      using type = typename T::decl_type;
    };

    template<class T, class = void>
    struct get_deps_impl
    {
      using type = kmp::list<>;
    };

    template<class T>
    struct get_deps_impl<T, std::void_t<typename T::dep_list>>
    {
      using type = typename T::dep_list;
    };

    template<class T, class = void>
    struct get_name_impl
    {
      using type = void;
    };

    template<class T>
    struct get_name_impl<T, std::void_t<typename T::name_type>>
    {
      using type = typename T::name_type;
    };
  }

  template<class T>
  using get_deps = typename detail::get_deps_impl<typename T::desc_type>::type;
  template<class T>
  using get_decl = typename detail::decl_or_t_impl<typename T::desc_type>::type;
  template<class T>
  using get_name = typename detail::get_name_impl<typename T::desc_type>::type;

  template<class T>
  using decl_t = typename T::decl_type;


  template<class Desc, class T>
  struct value
  {
    using desc_type = Desc;

    T x;
  };

  template<class Desc>
  struct declaration
  {
    using desc_type = Desc;

    template<class X>
    value<Desc, X> operator()(X&& x) const
    {
      return {JLN_FWD(X, x)};
    }
  };

  template<class Decl>
  struct primitive
  {
    using desc_type = primitive;
    using decl_type = Decl;

    template<class X>
    value<primitive, X> operator()(X&& x) const
    {
      return {JLN_FWD(X, x)};
    }
  };

#define JLN_FMTEXPR_VAR(name, decl)   \
  constexpr struct name :             \
    ::jln::fmtexpr::declaration<name> \
  {                                   \
    using name_type = name;           \
    using decl_type = decltype(decl); \
  } name {}

  template<class Name, class Decl>
  struct named_decl
  {
    using name_type = Name;
    using decl_type = get_decl<Decl>;
    using dep_list = get_deps<Decl>;
  };

  template<class Name, class Decl>
  declaration<named_decl<Name, Decl>> named(Name /*deps*/, Decl /*decl*/)
  {
    return {};
  }

  template<class Name, class Decl>
  declaration<named_decl<Name, Decl>> named(Decl /*decl*/)
  {
    return {};
  }

  namespace mp
  {
    template<class T>
    struct is_value : std::false_type
    {};

    template<class Desc, class T>
    struct is_value<value<Desc, T>> : std::true_type
    {};
  }

  namespace mf
  {
    template<class T>
    constexpr bool check_value(T const&)
    {
      static_assert(mp::is_value<T>::value, "T isn't jln::fmtexpr::value<...> type");
      return true;
    }
  }

  template<class... Xs>
  struct pack_t : Xs...
  {
    template<class F>
    auto apply(F f) &&
    {
      return f(static_cast<Xs&&>(*this)...);
    }

    template<class F>
    auto apply(F f) &
    {
      return f(static_cast<Xs&>(*this)...);
    }

    template<class F>
    auto apply(F f) const&
    {
      return f(static_cast<Xs const&>(*this)...);
    }
  };

  template<class... Xs>
  pack_t<std::remove_reference_t<Xs>...> pack(Xs&&... xs)
  {
    static_assert((mf::check_value(xs) && ...));
    return {xs...};
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
    template<class T>
    std::string_view print_name()
    {
      const std::string_view s = __PRETTY_FUNCTION__;
      const std::size_t start = 74;
      const std::size_t end = 50;
      return {s.begin()+start, s.size() - start - end};
    }

    template<class... Ts>
    void print_deps(kmp::list<Ts...>)
    {
      ((std::cout << detail::print_name<Ts>() << " "), ...);
    }

    template<class Value>
    void print_element(Value const& value)
    {
      std::cout << print_name<Value>() << "\n";
      std::cout << "  name: " << print_name<get_name<Value>>() << "\n";
      std::cout << "  dependencies (" << kmpe::size<get_deps<Value>>::value << "): "; print_deps(get_deps<Value>{}); std::cout << "\n";
      std::cout << "  type: " << print_name<get_decl<Value>>() << "\n";
      std::cout << "  value_type: " << print_name<decltype(value.x)>() << "\n";
      std::cout << "  value: " << value.x << "\n";
    }
  }

  template<class Pack>
  void introspection(Pack const& pack)
  {
    pack.apply([](auto&... xs){
      (detail::print_element(xs), ...);
    });
  }
}


using namespace jln::fmtexpr;
// using namespace jln::fmtexpr::literals;
using namespace jln::fmtexpr::ext::print;
using namespace jln::fmtexpr::ext::print::types;

int main(/*int argc, char **argv*/)
{
  JLN_FMTEXPR_VAR(xy, int_);
  auto zx = named<class zx>(int_);
  int i = 3;

  auto packet = pack(xy(2), zx(i), int_(4));
  introspection(packet);
}
