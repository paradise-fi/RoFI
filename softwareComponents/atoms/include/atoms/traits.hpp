#pragma once

#include <type_traits>
#include <utility>

namespace atoms {

// inspired by https://functionalcpp.wordpress.com/2013/08/05/function-traits/
template < typename F >
struct FunctionTraits;

template < typename R, typename... Args >
struct FunctionTraits< R(*)( Args... ) > : public FunctionTraits< R( Args... ) >
{};

template < typename R, typename... Args >
struct FunctionTraits< R( Args... ) > {
    using returnType = R;
    static constexpr std::size_t arity = sizeof...( Args );

    template < std::size_t N >
    struct argument {
        static_assert(N < arity, "Index out of range" );
        using type = typename std::tuple_element< N, std::tuple< Args...> >::type;
    };
};

// member function pointer
template< typename C, typename R, typename... Args >
struct FunctionTraits< R( C::* )( Args... ) > : public FunctionTraits< R( C&, Args... ) >
{};

// const member function pointer
template< typename C, typename R, typename... Args >
struct FunctionTraits< R( C::* )( Args... ) const > : public FunctionTraits< R( C&, Args... ) >
{};

// member object pointer
template< typename C, typename R >
struct FunctionTraits< R( C::* )> : public FunctionTraits< R( C& ) >
{};

template< typename F >
struct FunctionTraits {
private:
    using call_type = FunctionTraits< decltype( &F::operator() ) >;
public:
    using returnType = typename call_type::returnType;

    static constexpr std::size_t arity = call_type::arity - 1;

    template < std::size_t N >
    struct argument {
        static_assert( N < arity, "Index out of range" );
        using type = typename call_type::template argument< N + 1 >::type;
    };
};

// For pre-concept era it might be useful to detect if method exists or not.
// Therefore, we provide an implementation inspired by N4502:
// https://en.cppreference.com/w/cpp/experimental/is_detected

template <typename...>
using void_t = void;

// Primary template handles all types not supporting the operation.
template < typename, template < typename > class, typename = void_t<> >
struct detect : std::false_type {};

// Specialization recognizes/validates only types supporting the archetype.
template < typename T, template < typename > class Op >
struct detect< T, Op, void_t< Op< T > > > : std::true_type {};

} // namespace atoms
