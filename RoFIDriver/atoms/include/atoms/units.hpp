#pragma once

constexpr double pi = 3.14159265358979323846;

class Angle {
public:
    static Angle rad( float r ) { return Angle( 180 * r / pi ); }
    static Angle deg( float d ) { return Angle( d ); }

    Angle& operator+=( Angle a ) { _degs += a._degs; return *this; }
    Angle& operator-=( Angle a ) { _degs -= a._degs; return *this; }
    Angle& operator*=( double c ) { _degs *= c; return *this; }
    Angle& operator/=( double c ) { _degs /= c; return *this; }

    float deg() const { return _degs; }
    float rad() const { return _degs / 180.0 * pi; }
private:
    float _degs;
    Angle( float d ): _degs( d ) {}
};

inline Angle operator+( Angle a, Angle b ) {
    a += b;
    return a;
}

inline Angle operator-( Angle a, Angle b ) {
    a -= b;
    return a;
}

inline Angle operator*( Angle a, float c ) {
    a *= c;
    return a;
}

inline Angle operator/( Angle a, float c ) {
    a /= c;
    return a;
}

inline Angle operator"" _deg ( long double d ) {
    return Angle::deg( d );
}

inline Angle operator"" _rad ( long double r ) {
    return Angle::rad( r );
}

inline Angle operator"" _deg ( unsigned long long int d ) {
    return Angle::deg( d );
}

inline Angle operator"" _rad ( unsigned long long int r ) {
    return Angle::rad( r );
}
