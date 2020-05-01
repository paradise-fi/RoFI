#pragma once

#include <utility>
#include <traits.hpp>

namespace atoms::detail {

template < typename Functor, typename Next >
struct VisitorCallWrapper: public Functor, public Next {
    template < typename... Extra >
    VisitorCallWrapper( Functor f, Extra...extra ): Functor( f ), Next( extra... ) {}

    using FTrait = FunctionTraits< Functor >;
    static_assert( FTrait::arity == 1 );
    using Arg = typename FTrait:: template argument< 0 >::type;

    using StrippedArg = typename std::remove_reference< Arg >::type;

    void operator()( StrippedArg& arg ) override {
        if constexpr( std::is_same_v< void, typename FTrait::returnType > )
            Functor::operator()( arg );
        else
            Next::result = Functor::operator()( arg );
    }
};

template < typename R, typename Arg, typename Next >
struct VisitorCallWrapper< R( Arg ), Next >: public Next {
    template < typename... Extra >
    VisitorCallWrapper( R ( *fun )( Arg ), Extra...extra ): _fun( fun ), Next( extra... ) {}

    using StrippedArg = typename std::remove_reference< Arg >::type;

    void operator()( StrippedArg& arg ) override {
        if constexpr( std::is_same_v< void, R > )
            ( *_fun )( arg );
        else
            Next::result = ( *_fun )( arg );
    }
private:
    R ( *_fun )( Arg );
};

template < typename R, typename Arg, typename Next >
struct VisitorCallWrapper< R(*)( Arg ), Next >: public Next {
    template < typename... Extra >
    VisitorCallWrapper( R ( *fun )( Arg ), Extra...extra ): _fun( fun ), Next( extra... ) {}

    using StrippedArg = typename std::remove_reference< Arg >::type;

    void operator()( StrippedArg& arg ) override {
        if constexpr( std::is_same_v< void, R > )
            ( *_fun )( arg );
        else
            Next::result = ( *_fun )( arg );
    }
private:
    R ( *_fun )( Arg );
};

template <typename... Hosts>
struct Visits;

template < typename Host >
struct Visits< Host > {
    virtual ~Visits() {};
    virtual void operator()( Host& ) = 0;
};

template < typename Host, typename...OtherHosts >
struct Visits< Host, OtherHosts... >: public Visits< OtherHosts...> {
    using Visits< OtherHosts... >::operator();
    virtual void operator()( Host& ) = 0;
};

template < typename Base, typename Result, typename... Overloads >
struct Visitor;

template < typename Base >
struct Visitor< Base, void >: public Base {
    using Base::operator();
};

template < typename Base, typename Result >
struct Visitor< Base, Result >: public Base {
    using Base::operator();
    typename std::remove_reference< Result >::type result;
};

template < typename Base, typename Result, typename F >
struct Visitor< Base, Result, F >:
    public detail::VisitorCallWrapper< F, Visitor< Base, Result > >
{
    using Successor = detail::VisitorCallWrapper< F, Visitor< Base, Result > >;
    using Successor::operator();

    Visitor( F f ): Successor( f ) {}
};

template < typename Base, typename Result, typename F1, typename F2, typename... Fs >
struct Visitor< Base, Result, F1, F2, Fs... >:
    public detail::VisitorCallWrapper< F1, Visitor< Base, Result, F2, Fs... > >
{
    using Successor = detail::VisitorCallWrapper< F1, Visitor< Base, Result, F2, Fs... > >;
    using Successor::operator();

    using F1Trait = FunctionTraits< F1 >;
    using F2Trait = FunctionTraits< F2 >;
    static_assert( std::is_same_v< typename F1Trait::returnType, typename F2Trait::returnType > );

    Visitor( F1 f1, F2 f2, Fs... fs ): Successor( f1, f2, fs... ) {}
};

} // namespace atoms::detail

namespace atoms {

template < typename Base, typename F, typename... Fs >
struct Visitor:
    public detail::Visitor< Base, typename FunctionTraits< F >::returnType, F, Fs... >
{
    using ReturnType = typename FunctionTraits< F >::returnType;
    using Successor = detail::Visitor< Base, ReturnType, F, Fs... >;
    using Successor::operator();
    Visitor( F f, Fs... fs ): Successor( f, fs... ) {}
};


template < typename... Hosts >
struct Visits: public detail::Visits< Hosts... > {
    using detail::Visits< Hosts... >::operator();

    template <  typename... Fs >
    static auto make( Fs... fs ) {
        return Visitor< Visits, Fs... >( fs... );
    }
};

template < typename Self, typename Visitor >
struct VisitableBase {
    using VisitorType = Visitor;

    virtual ~VisitableBase() = default;
    virtual void accept( VisitorType& visitor ) = 0;
};

template < typename Base, typename Self >
struct Visitable: public Base {
    void accept( typename Base::VisitorType& visitor ) override {
        visitor( static_cast< Self & >( *this ) );
    }
};

template< typename T, typename Visitor >
auto visit( T& object, Visitor visitor ) -> std::enable_if_t<
    std::is_base_of_v< typename T::VisitorType, Visitor >,
    typename Visitor::ReturnType >
{
    object.accept( visitor );
    if constexpr ( !std::is_same_v< void, typename Visitor::ReturnType > )
        return visitor.result;
}

template < typename T, typename... Fs >
auto visit( T& object, Fs... fs ) {
    return visit( object, T::VisitorType::make( fs... ) );
}

} // namespace atoms