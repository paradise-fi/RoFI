#pragma once

#include <atoms/util.hpp>

class Angle {
public:
    using underlying_type = float;

    static constexpr underlying_type pi = 3.141592653589793238462643383279502884f;

    static constexpr Angle rad( underlying_type rads ) noexcept { return Angle( rads ); }
    static constexpr Angle deg( underlying_type degs ) noexcept { return Angle( pi * degs / 180 ); }

    constexpr Angle& operator+=( Angle other ) noexcept { _rads += other._rads; return *this; }
    constexpr Angle& operator-=( Angle other ) noexcept { _rads -= other._rads; return *this; }
    constexpr Angle& operator*=( underlying_type value ) noexcept { _rads *= value; return *this; }
    constexpr Angle& operator/=( underlying_type value ) noexcept { _rads /= value; return *this; }

    constexpr bool operator==( Angle other ) const noexcept { return _rads == other._rads; }
    constexpr bool operator!=( Angle other ) const noexcept { return !( *this == other ); }

    constexpr underlying_type deg() const noexcept { return 180 * _rads / pi; }
    constexpr underlying_type rad() const noexcept { return _rads; }

private:
    constexpr explicit Angle( underlying_type rads ): _rads( rads ) {}

    underlying_type _rads;
};

inline constexpr Angle operator+( Angle lhs, Angle rhs ) noexcept {
    lhs += rhs;
    return lhs;
}

inline constexpr Angle operator-( Angle lhs, Angle rhs ) noexcept {
    lhs -= rhs;
    return lhs;
}

inline constexpr Angle operator*( Angle angle, Angle::underlying_type value ) noexcept {
    angle *= value;
    return angle;
}

inline constexpr Angle operator*( Angle::underlying_type value, Angle angle ) noexcept {
    return angle * value;
}

inline constexpr Angle operator/( Angle angle, Angle::underlying_type value ) noexcept {
    angle /= value;
    return angle;
}

inline constexpr Angle operator"" _deg ( long double deg ) noexcept {
    return Angle::deg( static_cast< Angle::underlying_type >( deg ) );
}

inline constexpr Angle operator"" _rad ( long double rad ) noexcept {
    return Angle::rad( static_cast< Angle::underlying_type >( rad ) );
}

inline constexpr Angle operator"" _deg ( unsigned long long deg ) noexcept {
    return Angle::deg( static_cast< Angle::underlying_type >( deg ) );
}

inline constexpr Angle operator"" _rad ( unsigned long long rad ) noexcept {
    return Angle::rad( static_cast< Angle::underlying_type >( rad ) );
}

inline Angle clamp( Angle value, Angle min, Angle max ) {
    return Angle::rad( clamp( value.rad(), min.rad(), max.rad() ) );
}
