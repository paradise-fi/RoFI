#pragma once

#include <cassert>
#include <string>
#include <variant>


namespace atoms
{
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
    using value_type = T;
    using error_type = E;

    /// Constructing static member functions

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

    /// Observers

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

    /// Accessors

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
        return *std::get_if< 0 >( &_data );
    }
    /**
     * \brief Assume that the result contains a value.
     *
     * \returns Reference to the contained value.
     */
    constexpr value_type && assume_value() && noexcept
    {
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
        return *std::get_if< 1 >( &_data );
    }
    /**
     * \brief Assume that the result contains an error.
     *
     * \returns Reference to the contained error.
     */
    constexpr error_type && assume_error() && noexcept
    {
        return std::move( assume_error() );
    }

    /// Comparison operator

    /**
     * \brief Checks equality of two results.
     *
     * \param other result to check equality against
     *
     * \returns whether both `*this` and \p other contain value and the values
     * are equal or both contain error and the errors are equal.
     */
    bool operator==( const Result & other ) const = default;

private:
    using data_type = std::variant< value_type, error_type >;

    template < size_t I, typename... Args >
    explicit constexpr Result( std::in_place_index_t< I > i, Args &&... args )
            : _data( i, std::forward< Args >( args )... )
    {}

    data_type _data;
};

} // namespace atoms
