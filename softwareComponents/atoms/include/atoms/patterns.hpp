#pragma once

#include <utility>
#include <memory>
#include <type_traits>
#include <atoms/traits.hpp>

/**
 * \brief Declare abstract base class as cloneable.
 *
 * Typical usage: `class Animal { public: ATOMS_CLONEABLE_BASE( Animal ); }
 *
 * The method is abstract so that zou don't forget to use `ATOMS_CLONEABLE`
 * in the derived classes.
 */
#define ATOMS_CLONEABLE_BASE( Type ) \
    virtual Type *clone() const = 0

/**
 * \brief Declare derived class as cloneable.
 *
 * Typical usage: `class Dog: public Animal { public: ATOMS_CLONEABLE( Dog ); }
 *
 * The return type has to be a pointer type to allow implicit conversions to
 * the base class.
 * Method requires that the type has virtual destructor, because the usecase is
 * with classes with class hierarchy.
 */
#define ATOMS_CLONEABLE( Type ) \
    virtual Type *clone() const override { \
        static_assert( std::has_virtual_destructor_v< Type > ); \
        return new Type( *this ); \
    } \
    static_assert( true, "requires semicolon" )

namespace atoms::detail {

/**
 * CallWrapper takes a callable (function, functor - that's why there are
 * multiple overrides) and wraps it into a virtual operator() returning void and
 * possibly storing result of the call inside the Next::result attribute if the
 * return type is other than void.
 *
 * It forms a layer in a "template onion" - the next layer is specified by the
 * Next template parameter. It expects that Next provides the result.
 *
 * This is specialization for a functor
 */
template < typename Functor, typename Next >
struct VisitorCallWrapper: public Functor, public Next {
    template < typename... Extra >
    explicit VisitorCallWrapper( Functor f, Extra&&...extra ):
        Functor( std::move( f ) ), Next( std::forward< Extra >( extra )... ) {}

    using FTrait = FunctionTraits< Functor >;
    static_assert( FTrait::arity == 1 );
    using Arg = typename FTrait:: template argument< 0 >::type;

    using StrippedArg = typename std::remove_reference< Arg >::type;

    using Next::operator();
    void operator()( StrippedArg& arg ) override {
        if constexpr( std::is_same_v< void, typename FTrait::returnType > )
            Functor::operator()( arg );
        else
            Next::result = Functor::operator()( arg );
    }
};

/**
 * Specialization for function type
 */
template < typename R, typename Arg, typename Next >
struct VisitorCallWrapper< R( Arg ), Next >: public Next {
    template < typename... Extra >
    explicit VisitorCallWrapper( R ( *fun )( Arg ), Extra&&...extra ):
        Next( std::forward< Extra >( extra )... ), _fun( fun ) {}

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

/**
 * Specialization for function pointer
 */
template < typename R, typename Arg, typename Next >
struct VisitorCallWrapper< R(*)( Arg ), Next >: public Next {
    template < typename... Extra >
    explicit VisitorCallWrapper( R ( *fun )( Arg ), Extra&&...extra ):
        Next( std::forward< Extra >( extra )... ), _fun( fun ) {}

    using StrippedArg = typename std::remove_reference< Arg >::type;

    using Next::operator();
    void operator()( StrippedArg& arg ) override {
        if constexpr( std::is_same_v< void, R > )
            ( *_fun )( arg );
        else
            Next::result = ( *_fun )( arg );
    }
private:
    R ( *_fun )( Arg );
};

/**
 * Visits is a class which creates a virtual operator() for each Hosts argument.
 * Serves as a base for Visitor type, however, is not used directly so we can
 * implement top-level function make. See atoms::Visits.
 */
template <typename... Hosts>
struct Visits;

template < typename Host >
struct Visits< Host > {
    virtual ~Visits() = default;
    virtual void operator()( Host& ) = 0;
};

template < typename Host, typename...OtherHosts >
struct Visits< Host, OtherHosts... >: public Visits< OtherHosts...> {
    using Visits< OtherHosts... >::operator();
    virtual void operator()( Host& ) = 0;
};

/**
 * Visitor takes a Base class, expected type of its Results and a list of
 * callable types. It builds a single object having a "result" attribute in the
 * bottom and inheriting from base. It overrides the virtual operator().
 */
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

    explicit Visitor( F f ): Successor( std::move( f ) ) {}
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

    Visitor( F1 f1, F2 f2, Fs... fs ): Successor( std::move( f1 ), std::move( f2 ), std::move( fs )... ) {}
};

} // namespace atoms::detail

namespace atoms {

/**
 * \brief This type takes a desired visitor type (created by templateing
 * Visits), list of callables and produces a visitor out of the callable.
 *
 * Prefer calling Visits::make() for creating instances as it automatically
 * deduces all the types.
 */
template < typename Base, typename F, typename... Fs >
struct Visitor:
    public detail::Visitor< Base, typename FunctionTraits< F >::returnType, F, Fs... >
{
    using ReturnType = typename FunctionTraits< F >::returnType;
    using Successor = detail::Visitor< Base, ReturnType, F, Fs... >;
    using Successor::operator();
    explicit Visitor( F f, Fs... fs ): Successor( std::move( f ), std::move( fs )... ) {}
};

/**
 * \brief Visitor type for given set of derived classes (Hosts).
 *
 * Typical use case is `using MyTypeVisitor = atoms::Visits< Derived1, Derived2
 * >`
 */
template < typename... Hosts >
struct Visits: public detail::Visits< Hosts... > {
    using detail::Visits< Hosts... >::operator();

    /**
     * \brief Build a visitor out of collables (functors, free functions)
     *
     * It expects that all callables have the same return value and each of the
     * accepts exactly one overload of the base class.
     */
    template <  typename... Fs >
    static auto make( Fs... fs ) {
        return Visitor< Visits, Fs... >( std::move( fs )... );
    }
};

/**
 * \brief Make a base class visitable.
 *
 * This is a CRTP class, inherit from it in your base class and pass the base
 * class type and a base visitor type as template arguments. The base visitor
 * type should be created as a using to Visits.
 *
 * Typical use case `class Base: public atoms::VisitableBase< Base, BaseVisitor > {};`
 */
template < typename Self, typename Visitor >
struct VisitableBase {
    using VisitorType = Visitor;

    virtual ~VisitableBase() = default;
    virtual void accept( VisitorType& /* visitor */ ) {
        throw std::logic_error( "accept not implemented" );
    };
};

/**
 * \brief Implement visits interface in derived class.
 *
 * This is a CRTP class, inherit from it in you class and pass base class and
 * your derived class as the template arguments.
 *
 * Typical use case `class Derived: public Visitable< Base, Derived > {};`
 */
template < typename Base, typename Self >
struct Visitable: public Base {
    template < typename ... Ts >
    explicit Visitable( Ts &&... ts ) : Base( std::forward< Ts >( ts )... ) {}

    void accept( typename Base::VisitorType& visitor ) override {
        visitor( static_cast< Self & >( *this ) );
    }
};


/**
 *
 * \brief Visit given object by a visitor.
 *
 * Gives visiting the same interface as std::visits provides and removes the
 * hassle of extracting the return value out of the visitor.
 *
 * Expects that T inherits from atoms::VisitableBase.
 *
 * Expects that Visitor::ReturnType and Visitor::result (in case of non-void
 * return type) exists. Both of these are guaranteed if atoms::Visitor is used.
 */
template< typename T, typename Visitor >
auto visit( T& object, Visitor visitor ) -> std::enable_if_t<
    std::is_base_of_v< typename T::VisitorType, Visitor >,
    typename Visitor::ReturnType >
{
    object.accept( visitor );
    if constexpr ( !std::is_same_v< void, typename Visitor::ReturnType > )
        return visitor.result;
}

/**
 * \brief Visit given object by se of callables.
 *
 * Expects that T inherits from atoms::VisitableBase.
 *
 * Expects that all callables have the same return value and each of them
 * accepts exactly one overload of T.
 */
template < typename T, typename... Fs >
auto visit( T& object, Fs... fs ) {
    return visit( object, T::VisitorType::make( std::move( fs )... ) );
}


/**
 * \brief Store object on a heap (to allow virtual function calls and
 * to provide stable address on move) and provide deep copies
 *
 * It requires that T provides `T *clone() const`.
 * Use `ATOMS_CLONEABLE_BASE` and `ATOMS_CLONEABLE` for providing this method.
 */
template < typename T >
class ValuePtr {
private:
    std::unique_ptr< T > _ptr;
public:
    ValuePtr() = default;
    explicit ValuePtr( std::unique_ptr< T > ptr ): _ptr( std::move( ptr ) ) {}
    explicit ValuePtr( const T& t ): _ptr( t.clone() ) {}

    template< std::derived_from< T > Derived >
        requires( std::has_virtual_destructor_v< T > && !std::is_same_v< Derived, T > )
    explicit ValuePtr( std::unique_ptr< T > ptr ): _ptr( std::move( ptr ) ) {}

    ValuePtr( ValuePtr&& ) noexcept = default;
    ValuePtr& operator=( ValuePtr&& ) noexcept = default;

    ValuePtr clone() const {
        return _ptr ? ValuePtr( *_ptr ) : ValuePtr();
    }

    void swap(ValuePtr& other) noexcept {
        using std::swap;
        swap( this->_ptr, other._ptr );
    }

    T& operator*() const noexcept {
        assert( _ptr );
        return *_ptr;
    }
    T* operator->() const noexcept {
        assert( _ptr );
        return _ptr.get();
    }

    T* get() const noexcept {
        return _ptr.get();
    }
    explicit operator bool() const noexcept {
        return bool( _ptr );
    }
};

} // namespace atoms
