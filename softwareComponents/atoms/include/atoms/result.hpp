#pragma once

#include <cassert>
#include <concepts>
#include <functional>
#include <string>
#include <variant>


namespace atoms
{
template < typename T, typename E >
class [[nodiscard]] Result;

namespace detail
{
    template < typename T >
    struct is_result : std::false_type {};
    template < typename T, typename E >
    struct is_result< Result< T, E > > : std::true_type {};
    template < typename T >
    constexpr bool is_result_v = is_result< T >::value;

    template < typename F, typename ValueT, typename ErrorT >
    constexpr bool is_invocable_by_common_ref()
    {
        if constexpr ( std::common_reference_with< ValueT, ErrorT > ) {
            return std::is_invocable_v< F, std::common_reference_t< ValueT, ErrorT > >;
        } else {
            return false;
        }
    }

    template < typename F, typename ValueT, typename ErrorT >
    concept hint_match_visitor = ( std::is_invocable< F, std::true_type, ValueT >::value
                                   && std::is_invocable< F, std::false_type, ErrorT >::value
                                   && !std::is_invocable< F, ValueT >::value
                                   && !std::is_invocable< F, ErrorT >::value );

    template < typename F, typename ValueT, typename ErrorT >
    concept no_hint_match_visitor = ( std::is_invocable< F, ValueT >::value
                                      && std::is_invocable< F, ErrorT >::value
                                      && !is_invocable_by_common_ref< F, ValueT, ErrorT >()
                                      && !std::is_invocable< F, std::true_type, ValueT >::value
                                      && !std::is_invocable< F, std::false_type, ErrorT >::value );

    /**
     * \brief Result value/error type placeholder
     *
     * Used as a type placeholder so the other type of result doesn't have to
     * be specified by the user.
     *
     * The functionality of `Result` with `TypePlaceholder`
     * is similar to `std::nullopt_t`.
     */
    struct TypePlaceholder {
        TypePlaceholder() = delete;
    };
} // namespace detail

/**
 * \brief A type that holds either a value or an error.
 *
 * The usecase is similar to that of `std::optional` except
 * it can hold additional info about the failure.
 *
 * The `Result` class template is designed to be an almost drop-in replacement
 * for `std::optional` except the construction.
 */
template < typename T, typename E = std::string >
class [[nodiscard]] Result {
public:
    /**
     * \name Member types
     * \{
     */

    using value_type = T;
    using error_type = E;

    /**
     * \}
     */

    /**
     * \name Constructing static member functions
     * \{
     */

    /**
     * \brief Constructs the contained value by the copy constructor.
     */
    static constexpr Result value( const value_type & val )
    {
        return emplace_value( val );
    }
    /**
     * \brief Constructs the contained value by the move constructor.
     */
    static constexpr Result value( value_type && val )
    {
        return emplace_value( std::move( val ) );
    }
    /**
     * \brief Constructs the contained value in-place.
     */
    static constexpr Result emplace_value( auto &&... args )
    {
        return Result( std::in_place_index_t< 0 >{}, std::forward< decltype( args ) >( args )... );
    }

    /**
     * \brief Constructs the contained error by the copy constructor.
     */
    static constexpr Result error( const error_type & err )
    {
        return emplace_error( err );
    }
    /**
     * \brief Constructs the contained error by the move constructor.
     */
    static constexpr Result error( error_type && err )
    {
        return emplace_error( std::move( err ) );
    }
    /**
     * \brief Constructs the contained error in-place.
     */
    static constexpr Result emplace_error( auto &&... args )
    {
        return Result( std::in_place_index_t< 1 >{}, std::forward< decltype( args ) >( args )... );
    }

    /**
     * \}
     */

    /**
     * \name Implicit conversions
     *
     * Implicit conversions when one of types is `detail::TypePlaceholder`
     * \{
     */

    /**
     * \brief Changes the `value_type` from `detail::TypePlaceholder` by copying.
     */
    template < typename ValueT >
        requires( std::is_same_v< value_type, detail::TypePlaceholder > )
    constexpr operator Result< ValueT, error_type >() const &
    {
        return Result< ValueT, error_type >::error( assume_error() );
    }
    /**
     * \brief Changes the `value_type` from `detail::TypePlaceholder` by moving.
     */
    template < typename ValueT >
        requires( std::is_same_v< value_type, detail::TypePlaceholder > )
    constexpr operator Result< ValueT, error_type >() &&
    {
        return Result< ValueT, error_type >::error( std::move( assume_error() ) );
    }

    /**
     * \brief Changes the `error_type` from `detail::TypePlaceholder` by copying.
     */
    template < typename ErrorT >
        requires( std::is_same_v< error_type, detail::TypePlaceholder > )
    constexpr operator Result< value_type, ErrorT >() const &
    {
        return Result< value_type, ErrorT >::value( assume_value() );
    }
    /**
     * \brief Changes the `error_type` from `detail::TypePlaceholder` by moving.
     */
    template < typename ErrorT >
        requires( std::is_same_v< error_type, detail::TypePlaceholder > )
    constexpr operator Result< value_type, ErrorT >() &&
    {
        return Result< value_type, ErrorT >::value( std::move( assume_value() ) );
    }

    /**
     * \}
     */

    /**
     * \name Observers
     * \{
     */

    /**
     * \brief Checks whether `*this` contains a value.
     *
     * \retval true if `*this` contains a value.
     * \retval false if `*this` contains an error.
     */
    constexpr bool has_value() const noexcept
    {
        return _data.index() == 0;
    }
    /**
     * \brief Checks whether `*this` contains a value.
     *
     * \retval true if `*this` contains a value.
     * \retval false if `*this` contains an error.
     */
    constexpr operator bool() const noexcept
    {
        return has_value();
    }

    /**
     * \}
     */

    /**
     * \name Accessors
     * \{
     */

    /**
     * \brief Accesses the contained value.
     *
     * \returns Pointer to the contained value.
     */
    constexpr value_type * operator->() noexcept
    {
        assert( has_value() );
        return &assume_value();
    }
    /**
     * \brief Accesses the contained value.
     *
     * \returns Pointer to the contained value.
     */
    constexpr const value_type * operator->() const noexcept
    {
        assert( has_value() );
        return &assume_value();
    }
    /**
     * \brief Accesses the contained value.
     *
     * \returns Reference to the contained value.
     */
    constexpr value_type & operator*() & noexcept
    {
        assert( has_value() );
        return assume_value();
    }
    /**
     * \brief Accesses the contained value.
     *
     * \returns Reference to the contained value.
     */
    constexpr const value_type & operator*() const & noexcept
    {
        assert( has_value() );
        return assume_value();
    }
    /**
     * \brief Accesses the contained value.
     *
     * \returns Reference to the contained value.
     */
    constexpr value_type && operator*() && noexcept
    {
        assert( has_value() );
        return std::move( assume_value() );
    }

    /**
     * \brief Assume that the result contains a value.
     *
     * \returns Reference to the contained value.
     */
    constexpr value_type & assume_value() & noexcept
    {
        assert( has_value() );
        static_assert( !std::is_same_v< value_type, detail::TypePlaceholder > );
        return *std::get_if< 0 >( &_data );
    }
    /**
     * \brief Assume that the result contains a value.
     *
     * \returns Reference to the contained value.
     */
    constexpr const value_type & assume_value() const & noexcept
    {
        assert( has_value() );
        static_assert( !std::is_same_v< value_type, detail::TypePlaceholder > );
        return *std::get_if< 0 >( &_data );
    }
    /**
     * \brief Assume that the result contains a value.
     *
     * \returns Reference to the contained value.
     */
    constexpr value_type && assume_value() && noexcept
    {
        static_assert( !std::is_same_v< value_type, detail::TypePlaceholder > );
        return std::move( assume_value() );
    }

    /**
     * \brief Assume that the result contains an error.
     *
     * \returns Reference to the contained error.
     */
    constexpr error_type & assume_error() & noexcept
    {
        assert( !has_value() );
        static_assert( !std::is_same_v< error_type, detail::TypePlaceholder > );
        return *std::get_if< 1 >( &_data );
    }
    /**
     * \brief Assume that the result contains an error.
     *
     * \returns Reference to the contained error.
     */
    constexpr const error_type & assume_error() const & noexcept
    {
        assert( !has_value() );
        static_assert( !std::is_same_v< error_type, detail::TypePlaceholder > );
        return *std::get_if< 1 >( &_data );
    }
    /**
     * \brief Assume that the result contains an error.
     *
     * \returns Reference to the contained error.
     */
    constexpr error_type && assume_error() && noexcept
    {
        static_assert( !std::is_same_v< error_type, detail::TypePlaceholder > );
        return std::move( assume_error() );
    }

    /**
     * \}
     */

    /**
     * \name Monadic operations
     *
     * Inspired by and designed to be compatible with
     * monadic operations of `std::optional` from C++23.
     * \{
     */

    /**
     * \brief Returns the result of invocation of \p f on the contained value if it exists.
     * Otherwise, returns the contained error in the return type.
     *
     * The return type must be a specialization of `Result` with the same error type.
     *
     * \param f a suitable function or Callable object that returns a `Result`
     *
     * \returns The result of \p f or the contained error.
     */
    template < std::invocable< value_type & > F >
        requires( std::is_copy_constructible_v< error_type > )
    constexpr auto and_then( F && f ) &
    {
        using ResultT = std::remove_cvref_t< std::invoke_result_t< F, value_type & > >;
        static_assert( detail::is_result_v< ResultT >, "F must return a result" );
        static_assert( std::is_same_v< error_type, typename ResultT::error_type >,
                       "error types must be the same" );

        return has_value() ? std::invoke( std::forward< F >( f ), assume_value() )
                           : ResultT::error( assume_error() );
    }
    /**
     * \brief Returns the result of invocation of \p f on the contained value if it exists.
     * Otherwise, returns the contained error in the return type.
     *
     * The return type must be a specialization of `Result` with the same error type.
     *
     * \param f a suitable function or Callable object that returns a `Result`
     *
     * \returns The result of \p f or the contained error.
     */
    template < std::invocable< const value_type & > F >
        requires( std::is_copy_constructible_v< error_type > )
    constexpr auto and_then( F && f ) const &
    {
        using ResultT = std::remove_cvref_t< std::invoke_result_t< F, const value_type & > >;
        static_assert( detail::is_result_v< ResultT >, "F must return a result" );
        static_assert( std::is_same_v< error_type, typename ResultT::error_type >,
                       "error types must be the same" );

        return has_value() ? std::invoke( std::forward< F >( f ), assume_value() )
                           : ResultT::error( assume_error() );
    }
    /**
     * \brief Returns the result of invocation of \p f on the contained value if it exists.
     * Otherwise, returns the contained error in the return type.
     *
     * The return type must be a specialization of `Result` with the same error type.
     *
     * \param f a suitable function or Callable object that returns a `Result`
     *
     * \returns The result of \p f or the contained error.
     */
    template < std::invocable< value_type && > F >
        requires( std::is_move_constructible_v< error_type > )
    constexpr auto and_then( F && f ) &&
    {
        using ResultT = std::remove_cvref_t< std::invoke_result_t< F, value_type && > >;
        static_assert( detail::is_result_v< ResultT >, "F must return a result" );
        static_assert( std::is_same_v< error_type, typename ResultT::error_type >,
                       "error types must be the same" );

        return has_value() ? std::invoke( std::forward< F >( f ), std::move( assume_value() ) )
                           : ResultT::error( std::move( assume_error() ) );
    }

    /**
     * \brief Returns the contained value in the return type if it contains a value.
     * Otherwise, returns the result of invocation of \p f on the contained error.
     *
     * The return type must be a specialization of `Result` with the same value type.
     *
     * \param f a suitable function or Callable object that returns a `Result`
     *
     * \returns The contained value or the result of \p f .
     */
    template < std::invocable< error_type & > F >
        requires( std::is_copy_constructible_v< value_type > )
    constexpr auto or_else( F && f ) &
    {
        using ResultT = std::remove_cvref_t< std::invoke_result_t< F, error_type & > >;
        static_assert( detail::is_result_v< ResultT >, "F must return a result" );
        static_assert( std::is_same_v< value_type, typename ResultT::value_type >,
                       "value types must be the same" );

        return has_value() ? ResultT::value( assume_value() )
                           : std::invoke( std::forward< F >( f ), assume_error() );
    }
    /**
     * \brief Returns the contained value in the return type if it contains a value.
     * Otherwise, returns the result of invocation of \p f on the contained error.
     *
     * The return type must be a specialization of `Result` with the same value type.
     *
     * \param f a suitable function or Callable object that returns a `Result`
     *
     * \returns The contained value or the result of \p f .
     */
    template < std::invocable< const error_type & > F >
        requires( std::is_copy_constructible_v< value_type > )
    constexpr auto or_else( F && f ) const &
    {
        using ResultT = std::remove_cvref_t< std::invoke_result_t< F, const error_type & > >;
        static_assert( detail::is_result_v< ResultT >, "F must return a result" );
        static_assert( std::is_same_v< value_type, typename ResultT::value_type >,
                       "value types must be the same" );

        return has_value() ? ResultT::value( assume_value() )
                           : std::invoke( std::forward< F >( f ), assume_error() );
    }
    /**
     * \brief Returns the contained value in the return type if it contains a value.
     * Otherwise, returns the result of invocation of \p f on the contained error.
     *
     * The return type must be a specialization of `Result` with the same value type.
     *
     * \param f a suitable function or Callable object that returns a `Result`
     *
     * \returns The contained value or the result of \p f .
     */
    template < std::invocable< error_type && > F >
        requires( std::is_move_constructible_v< value_type > )
    constexpr auto or_else( F && f ) &&
    {
        using ResultT = std::remove_cvref_t< std::invoke_result_t< F, error_type && > >;
        static_assert( detail::is_result_v< ResultT >, "F must return a result" );
        static_assert( std::is_same_v< value_type, typename ResultT::value_type >,
                       "value types must be the same" );

        return has_value() ? ResultT::value( std::move( assume_value() ) )
                           : std::invoke( std::forward< F >( f ), std::move( assume_error() ) );
    }

    /**
     * \brief Returns the contained value in the return type if it contains a value.
     * Otherwise, returns the result of \p f .
     *
     * The return type must be a specialization of `Result` with the same value type.
     *
     * \param f a suitable function or Callable object that returns a `Result`
     *
     * \returns The contained value or the result of \p f .
     */
    template < std::invocable<> F >
        requires( std::is_copy_constructible< value_type >::value
                  && !std::is_invocable< F, error_type & >::value
                  && !std::is_invocable< F, const error_type & >::value
                  && !std::is_invocable< F, error_type && >::value )
    constexpr auto or_else( F && f ) const
    {
        using ResultT = std::remove_cvref_t< std::invoke_result_t< F > >;
        static_assert( detail::is_result_v< ResultT >, "F must return a result" );
        static_assert( std::is_same_v< value_type, typename ResultT::value_type >,
                       "value types must be the same" );

        return has_value() ? ResultT::value( assume_value() )
                           : std::invoke( std::forward< F >( f ) );
    }
    /**
     * \brief Returns the contained value in the return type if it contains a value.
     * Otherwise, returns the result of \p f .
     *
     * The return type must be a specialization of `Result` with the same value type.
     *
     * \param f a suitable function or Callable object that returns a `Result`
     *
     * \returns The contained value or the result of \p f .
     */
    template < std::invocable<> F >
        requires( std::is_move_constructible< value_type >::value
                  && !std::is_invocable< F, error_type & >::value
                  && !std::is_invocable< F, const error_type & >::value
                  && !std::is_invocable< F, error_type && >::value )
    constexpr auto or_else( F && f ) &&
    {
        using ResultT = std::remove_cvref_t< std::invoke_result_t< F > >;
        static_assert( detail::is_result_v< ResultT >, "F must return a result" );
        static_assert( std::is_same_v< value_type, typename ResultT::value_type >,
                       "value types must be the same" );

        return has_value() ? ResultT::value( std::move( assume_value() ) )
                           : std::invoke( std::forward< F >( f ) );
    }

    /**
     * \}
     */

    /**
     * \name Transform operations
     * \{
     */

    /**
     * \brief Returns `Result` with the result of invocation of \p f
     * on the contained value as the new value if it exists.
     * Otherwise, returns the contained error in the return type.
     *
     * The return type of \p f will be the value type of the returned `Result`.
     *
     * \param f a suitable function or Callable object that returns the new value type
     *
     * \returns The result of \p f or the contained error.
     */
    template < std::invocable< value_type & > F >
    constexpr auto transform( F && f ) &
    {
        using ValueT = std::remove_cvref_t< std::invoke_result_t< F, value_type & > >;
        using ResultT = Result< ValueT, error_type >;
        return has_value() ? ResultT::value( std::invoke( std::forward< F >( f ), assume_value() ) )
                           : ResultT::error( assume_error() );
    }
    /**
     * \brief Returns `Result` with the result of invocation of \p f
     * on the contained value as the new value if it exists.
     * Otherwise, returns the contained error in the return type.
     *
     * The return type of \p f will be the value type of the returned `Result`.
     *
     * \param f a suitable function or Callable object that returns the new value type
     *
     * \returns The result of \p f or the contained error.
     */
    template < std::invocable< const value_type & > F >
    constexpr auto transform( F && f ) const &
    {
        using ValueT = std::remove_cvref_t< std::invoke_result_t< F, const value_type & > >;
        using ResultT = Result< ValueT, error_type >;
        return has_value() ? ResultT::value( std::invoke( std::forward< F >( f ), assume_value() ) )
                           : ResultT::error( assume_error() );
    }
    /**
     * \brief Returns `Result` with the result of invocation of \p f
     * on the contained value as the new value if it exists.
     * Otherwise, returns the contained error in the return type.
     *
     * The return type of \p f will be the value type of the returned `Result`.
     *
     * \param f a suitable function or Callable object that returns the new value type
     *
     * \returns The result of \p f or the contained error.
     */
    template < std::invocable< value_type && > F >
    constexpr auto transform( F && f ) &&
    {
        using ValueT = std::remove_cvref_t< std::invoke_result_t< F, value_type && > >;
        using ResultT = Result< ValueT, error_type >;
        return has_value() ? ResultT::value(
                       std::invoke( std::forward< F >( f ), std::move( assume_value() ) ) )
                           : ResultT::error( std::move( assume_error() ) );
    }

    /**
     * \brief Returns the contained value in the return type if it contains a value.
     * Otherwise, returns `Result` with the result of invocation of \p f
     * on the contained error as the new error.
     *
     * The return type of \p f will be the error type of the returned `Result`.
     *
     * \param f a suitable function or Callable object that returns a `Result`
     *
     * \returns The contained value or the result of \p f .
     */
    template < std::invocable< error_type & > F >
    constexpr auto transform_error( F && f ) &
    {
        using ErrorT = std::remove_cvref_t< std::invoke_result_t< F, error_type & > >;
        using ResultT = Result< value_type, ErrorT >;
        return has_value()
                     ? ResultT::value( assume_value() )
                     : ResultT::error( std::invoke( std::forward< F >( f ), assume_error() ) );
    }
    /**
     * \brief Returns the contained value in the return type if it contains a value.
     * Otherwise, returns `Result` with the result of invocation of \p f
     * on the contained error as the new error.
     *
     * The return type of \p f will be the error type of the returned `Result`.
     *
     * \param f a suitable function or Callable object that returns a `Result`
     *
     * \returns The contained value or the result of \p f .
     */
    template < std::invocable< const error_type & > F >
    constexpr auto transform_error( F && f ) const &
    {
        using ErrorT = std::remove_cvref_t< std::invoke_result_t< F, const error_type & > >;
        using ResultT = Result< value_type, ErrorT >;
        return has_value()
                     ? ResultT::value( assume_value() )
                     : ResultT::error( std::invoke( std::forward< F >( f ), assume_error() ) );
    }
    /**
     * \brief Returns the contained value in the return type if it contains a value.
     * Otherwise, returns `Result` with the result of invocation of \p f
     * on the contained error as the new error.
     *
     * The return type of \p f will be the error type of the returned `Result`.
     *
     * \param f a suitable function or Callable object that returns a `Result`
     *
     * \returns The contained value or the result of \p f .
     */
    template < std::invocable< error_type && > F >
    constexpr auto transform_error( F && f ) &&
    {
        using ErrorT = std::remove_cvref_t< std::invoke_result_t< F, error_type && > >;
        using ResultT = Result< value_type, ErrorT >;
        return has_value() ? ResultT::value( std::move( assume_value() ) )
                           : ResultT::error( std::invoke( std::forward< F >( f ),
                                                          std::move( assume_error() ) ) );
    }

    /**
     * \}
     */

    /**
     * \name Match operations
     * \{
     */

    /**
     * \brief Returns the result of invocation of \p f on `std::true_type`
     * and the contained value if it exists.
     * Otherwise, returns the result of invocation of \p f on `std::false_type`
     * and the contained error.
     *
     * \p f has to be invocable with `std::true_type, value_type &` and with
     * `std::false_type, error_type &` and has to have the same return type.
     *
     * \p f cannot be invocable just with `value_type &` or just with `error_type &`.
     *
     * \param f a suitable function or Callable object
     *
     * \returns The result of \p f .
     */
    template < detail::hint_match_visitor< value_type &, error_type & > Vis >
    constexpr auto match( Vis && vis ) &
    {
        return has_value() ? std::forward< Vis >( vis )( std::true_type{}, assume_value() )
                           : std::forward< Vis >( vis )( std::false_type{}, assume_error() );
    }
    /**
     * \brief Returns the result of invocation of \p f on `std::true_type`
     * and the contained value if it exists.
     * Otherwise, returns the result of invocation of \p f on `std::false_type`
     * and the contained error.
     *
     * \p f has to be invocable with `std::true_type, const value_type &` and with
     * `std::false_type, const error_type &` and has to have the same return type.
     *
     * \p f cannot be invocable just with `const value_type &` or just with `const error_type &`.
     *
     * \param f a suitable function or Callable object
     *
     * \returns The result of \p f .
     */
    template < detail::hint_match_visitor< const value_type &, const error_type & > Vis >
    constexpr auto match( Vis && vis ) const &
    {
        return has_value() ? std::forward< Vis >( vis )( std::true_type{}, assume_value() )
                           : std::forward< Vis >( vis )( std::false_type{}, assume_error() );
    }
    /**
     * \brief Returns the result of invocation of \p f on `std::true_type`
     * and the contained value if it exists.
     * Otherwise, returns the result of invocation of \p f on `std::false_type`
     * and the contained error.
     *
     * \p f has to be invocable with `std::true_type, value_type &&` and with
     * `std::false_type, error_type &&` and has to have the same return type.
     *
     * \p f cannot be invocable just with `value_type &&` or just with `error_type &&`.
     *
     * \param f a suitable function or Callable object
     *
     * \returns The result of \p f .
     */
    template < detail::hint_match_visitor< value_type &&, error_type && > Vis >
    constexpr auto match( Vis && vis ) &&
    {
        return has_value()
                     ? std::forward< Vis >( vis )( std::true_type{}, std::move( assume_value() ) )
                     : std::forward< Vis >( vis )( std::false_type{}, std::move( assume_error() ) );
    }
    /**
     * \brief Returns the result of invocation of \p f on the contained value if it exists.
     * Otherwise, returns the result of invocation of \p f on the contained error.
     *
     * \p f has to be invocable with `value_type &` and with
     * `error_type &` and has to have the same return type.
     *
     * \p f cannot be invocable with `std::true_type, value_type &` or with
     * `std::false_type, error_type &`.
     *
     * \param f a suitable function or Callable object
     *
     * \returns The result of \p f .
     */
    template < detail::no_hint_match_visitor< value_type &, error_type & > Vis >
    constexpr auto match( Vis && vis ) &
    {
        return has_value() ? std::forward< Vis >( vis )( assume_value() )
                           : std::forward< Vis >( vis )( assume_error() );
    }
    /**
     * \brief Returns the result of invocation of \p f on the contained value if it exists.
     * Otherwise, returns the result of invocation of \p f on the contained error.
     *
     * \p f has to be invocable with `const value_type &` and with
     * `const error_type &` and has to have the same return type.
     *
     * \p f cannot be invocable with `std::true_type, const value_type &` or with
     * `std::false_type, const error_type &`.
     *
     * \param f a suitable function or Callable object
     *
     * \returns The result of \p f .
     */
    template < detail::no_hint_match_visitor< const value_type &, const error_type & > Vis >
    constexpr auto match( Vis && vis ) const &
    {
        return has_value() ? std::forward< Vis >( vis )( assume_value() )
                           : std::forward< Vis >( vis )( assume_error() );
    }
    /**
     * \brief Returns the result of invocation of \p f on the contained value if it exists.
     * Otherwise, returns the result of invocation of \p f on the contained error.
     *
     * \p f has to be invocable with `value_type &&` and with
     * `error_type &&` and has to have the same return type.
     *
     * \p f cannot be invocable with `std::true_type, value_type &&` or with
     * `std::false_type, error_type &&`.
     *
     * \param f a suitable function or Callable object
     *
     * \returns The result of \p f .
     */
    template < detail::no_hint_match_visitor< value_type &&, error_type && > Vis >
    constexpr auto match( Vis && vis ) &&
    {
        return has_value() ? std::forward< Vis >( vis )( std::move( assume_value() ) )
                           : std::forward< Vis >( vis )( std::move( assume_error() ) );
    }

    /**
     * \}
     */

    /**
     * \name Comparison operator
     * \{
     */

    /**
     * \brief Checks equality of two results.
     *
     * \param other result to check equality against
     *
     * \returns whether both `*this` and \p other contain value and the values
     * are equal or both contain error and the errors are equal.
     */
    bool operator==( const Result & other ) const = default;

    /**
     * \}
     */

private:
    using data_type = std::variant< value_type, error_type >;

    template < size_t I, typename... Args >
    explicit constexpr Result( std::in_place_index_t< I > i, Args &&... args )
            : _data( i, std::forward< Args >( args )... )
    {}

    data_type _data;
};


/**
 * \name Result creation helper functions
 * \{
 */

/**
 * \brief Creates `Result` from value.
 *
 * The created `Result` is implicitly convertible to `Result` with any error type.
 *
 * \param value the value to construct the `Result` object with
 *
 * \returns the contructed `Result` object.
 */
template < typename ValueT >
inline constexpr auto result_value( ValueT value ) -> Result< ValueT, detail::TypePlaceholder >
{
    return Result< ValueT, detail::TypePlaceholder >::value( std::move( value ) );
}
/**
 * \brief Creates `Result` with value constructed in-place from \p args... .
 *
 * The created `Result` is implicitly convertible to `Result` with any error type.
 *
 * \param args arguments to be passed to the constructor of value
 *
 * \returns the contructed `Result` object.
 */
template < typename ValueT, typename... Args >
inline constexpr auto make_result_value( Args &&... args )
        -> Result< ValueT, detail::TypePlaceholder >
{
    return Result< ValueT, detail::TypePlaceholder >::emplace_value(
            std::forward< Args >( args )... );
}

/**
 * \brief Creates `Result` from error.
 *
 * The created `Result` is implicitly convertible to `Result` with any value type.
 *
 * \param error the error to construct the `Result` object with
 *
 * \returns the contructed `Result` object.
 */
template < typename ErrorT >
inline constexpr auto result_error( ErrorT error ) -> Result< detail::TypePlaceholder, ErrorT >
{
    return Result< detail::TypePlaceholder, ErrorT >::error( std::move( error ) );
}
/**
 * \brief Creates `Result` with error constructed in-place from \p args... .
 *
 * The created `Result` is implicitly convertible to `Result` with any value type.
 *
 * \param args arguments to be passed to the constructor of error
 *
 * \returns the contructed `Result` object.
 */
template < typename ErrorT, typename... Args >
inline constexpr auto make_result_error( Args &&... args )
        -> Result< detail::TypePlaceholder, ErrorT >
{
    return Result< detail::TypePlaceholder, ErrorT >::emplace_error(
            std::forward< Args >( args )... );
}

/**
 * \}
 */

/**
 * \name Monadic operators
 * \{
 */

/**
 * \brief Returns the result of invocation of \p f on the contained value in \p result if it exists.
 * Otherwise, returns the contained error in \p result in the return type.
 *
 * The return type of \p f must be a specialization of `Result` with the same error type.
 *
 * \param result result on whose value to apply \p f
 * \param f a suitable function or Callable object that returns a `Result`
 *
 * \returns The result of \p f on value of \p result or the contained error in \p result .
 */
template < typename ResultT, typename F >
    requires( detail::is_result_v< std::remove_cvref_t< ResultT > > )
inline constexpr auto operator>>( ResultT && result, F && f )
{
    return std::forward< ResultT >( result ).and_then( std::forward< F >( f ) );
}

/**
 * \}
 */

} // namespace atoms
