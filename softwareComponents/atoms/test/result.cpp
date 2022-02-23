#include <stdexcept>

#include <atoms/result.hpp>
#include <atoms/util.hpp>
#include <catch2/catch.hpp>


using atoms::Result;

struct Copyable {
    Copyable() = default;
    Copyable( const Copyable & ) = default;
    Copyable & operator=( const Copyable & ) = default;
};
struct Moveable {
    Moveable() = default;
    Moveable( Moveable && ) = default;
    Moveable & operator=( Moveable && ) = default;
};

enum class RefType
{
    LValue,
    CLValue,
    RValue,
    CRValue,
};
bool operator==( RefType lhs, RefType rhs )
{
    return static_cast< int >( lhs ) == static_cast< int >( rhs );
}

struct MyStruct {
    bool success()
    {
        return true;
    }
    bool isConst()
    {
        return false;
    }
    bool isConst() const
    {
        return true;
    }

    RefType refType() &
    {
        return RefType::LValue;
    }
    RefType refType() const &
    {
        return RefType::CLValue;
    }
    RefType refType() &&
    {
        return RefType::RValue;
    }
    RefType refType() const &&
    {
        return RefType::CRValue;
    }
};


static_assert( atoms::detail::is_result_v< Result< int > > );
static_assert( atoms::detail::is_result_v< Result< std::string, int > > );
static_assert( atoms::detail::is_result_v< Result< std::string, std::string > > );

static_assert( !atoms::detail::is_result_v< int > );
static_assert( !atoms::detail::is_result_v< std::variant< int, double > > );

static_assert( !atoms::detail::is_result_v< Result< int > & > );
static_assert( !atoms::detail::is_result_v< const Result< int > & > );
static_assert( !atoms::detail::is_result_v< Result< int > && > );
static_assert( !atoms::detail::is_result_v< const Result< int > && > );


TEST_CASE( "Result constructors" )
{
    auto okInt = Result< int, std::string >::value( 10 );
    auto errStr = Result< int, std::string >::error( "Error" );

    auto okIntIntDef = Result< int, int >::emplace_value();
    auto okIntIntDef2 = Result< int, int >::value( {} );

    auto okStrStr = Result< std::string >::value( {} );
    auto okStrStr2 = Result< std::string >::emplace_value();
    static_assert( std::is_same_v< typename decltype( okStrStr )::error_type, std::string > );
    static_assert( std::is_same_v< typename decltype( okStrStr2 )::error_type, std::string > );

    auto okIntInt = Result< int, int >::value( 10 );
    auto errIntInt = Result< int, int >::error( 10 );

    // Result< int, std::string >::error( "Error" ); // Causes warning

    SECTION( "default" )
    {
        auto value = Result< MyStruct >::emplace_value();
        auto value2 = Result< MyStruct >::value( {} );
        auto value3 = Result< MyStruct >::value( MyStruct{} );
    }
    SECTION( "copy" )
    {
        static_assert( std::is_copy_constructible_v< Result< Copyable > > );
        static_assert( std::is_copy_assignable_v< Result< Copyable > > );

        static_assert( std::is_copy_constructible_v< Result< int, Copyable > > );
        static_assert( std::is_copy_assignable_v< Result< int, Copyable > > );

        SECTION( "ok int" )
        {
            auto okInt = Result< int >::value( 10 );
            REQUIRE( okInt );

            auto okIntCopy = okInt;
            static_assert( std::is_same_v< decltype( okIntCopy ), Result< int > > );
            CHECK( okIntCopy );

            CHECK( okInt == okIntCopy );
            CHECK( okInt.assume_value() == okIntCopy.assume_value() );
        }
        SECTION( "err str" )
        {
            auto errStr = Result< std::string, std::string >::error( "Error" );
            REQUIRE( !errStr );

            auto errStrCopy = errStr;
            CHECK( !errStrCopy );

            CHECK( errStr == errStrCopy );
            CHECK( errStr.assume_error() == errStrCopy.assume_error() );
        }
    }
    SECTION( "move" )
    {
        static_assert( !std::is_copy_constructible_v< Result< Moveable > > );
        static_assert( !std::is_copy_assignable_v< Result< Moveable > > );
        static_assert( std::is_move_constructible_v< Result< Moveable > > );
        static_assert( std::is_move_assignable_v< Result< Moveable > > );

        static_assert( !std::is_copy_constructible_v< Result< int, Moveable > > );
        static_assert( !std::is_copy_assignable_v< Result< int, Moveable > > );
        static_assert( std::is_move_constructible_v< Result< int, Moveable > > );
        static_assert( std::is_move_assignable_v< Result< int, Moveable > > );

        Moveable moveableOrig;
        auto moveable = std::move( moveableOrig );

        auto createMoveableVal = [] { return Result< Moveable >::value( {} ); };
        auto createMoveableErr = [] { return Result< int, Moveable >::error( {} ); };

        auto value = Result< Moveable >::value( std::move( moveable ) );
        auto error = Result< int, Moveable >::error( std::move( value.assume_value() ) );

        auto error2 = Result< int, Moveable >::error( createMoveableVal().assume_value() );

        auto combined = Result< Copyable, Moveable >::error( createMoveableErr().assume_error() );

        auto combined2 = std::move( combined );
    }
}

TEST_CASE( "Result holds alternative" )
{
    auto okIntInt = Result< int, int >::value( 10 );
    auto errIntInt = Result< int, int >::error( 10 );

    CHECK( okIntInt.has_value() );
    CHECK( okIntInt );

    CHECK( !errIntInt.has_value() );
    CHECK( !errIntInt );
}

TEST_CASE( "Reference types on methods" )
{
    auto createResult = [] { return Result< int >::value( 100 ); };
    auto createConstResult = []() -> const auto
    {
        return Result< int >::value( 100 );
    };

    static_assert( std::is_same_v< decltype( createResult() ), Result< int > > );
    static_assert( std::is_same_v< decltype( *createResult() ), int && > );
    auto && x = createResult();
    static_assert( std::is_same_v< decltype( x ), Result< int > && > );
    auto && val = createResult().assume_value();
    static_assert( std::is_same_v< decltype( val ), int && > );
    const auto & lval = createResult().assume_value();
    static_assert( std::is_same_v< decltype( lval ), const int & > );

    auto && val2 = *createResult();
    static_assert( std::is_same_v< decltype( val2 ), int && > );
    const auto & lval2 = *createResult();
    static_assert( std::is_same_v< decltype( lval2 ), const int & > );

    static_assert( std::is_same_v< decltype( createConstResult() ), const Result< int > > );
    static_assert( std::is_same_v< decltype( *createConstResult() ), const int & > );
    auto && cx = createConstResult();
    static_assert( std::is_same_v< decltype( cx ), const Result< int > && > );
    auto && cval = createConstResult().assume_value();
    static_assert( std::is_same_v< decltype( cval ), const int & > );
    auto & clval = createConstResult().assume_value();
    static_assert( std::is_same_v< decltype( clval ), const int & > );

    auto && cval2 = *createConstResult();
    static_assert( std::is_same_v< decltype( cval2 ), const int & > );
    auto & clval2 = *createConstResult();
    static_assert( std::is_same_v< decltype( clval2 ), const int & > );
}

TEST_CASE( "Method access" )
{
    SECTION( "value" )
    {
        auto value = Result< MyStruct >::value( {} );
        CHECK( value->success() );
        CHECK( !value->isConst() );
        const auto cvalue = value;
        CHECK( cvalue->isConst() );

        CHECK( value->refType() == RefType::LValue );
        CHECK( cvalue->refType() == RefType::CLValue );

        // Arrow doesn't work with rvalues
        CHECK( std::move( value ).assume_value().refType() == RefType::RValue );
        CHECK( ( *std::move( value ) ).refType() == RefType::RValue );
        CHECK( ( *Result< MyStruct >::value( {} ) ).refType() == RefType::RValue );
    }
    SECTION( "error" )
    {
        auto error = Result< int, MyStruct >::error( {} );
        const auto cerror = error;

        CHECK( error.assume_error().refType() == RefType::LValue );
        CHECK( cerror.assume_error().refType() == RefType::CLValue );
        CHECK( std::move( error ).assume_error().refType() == RefType::RValue );
    }
}

TEST_CASE( "Monadic operations" )
{
    SECTION( "and then" )
    {
        auto checkPositive = []( int idx ) -> Result< size_t > {
            return idx >= 0 ? Result< size_t >::value( to_unsigned( idx ) )
                            : Result< size_t >::error( "negative index" );
        };

        SECTION( "compute success-success" )
        {
            auto compute10 = [] { return Result< int >::value( 10 ); };

            auto result = compute10().and_then( checkPositive );
            CHECK( result == Result< size_t >::value( 10 ) );
        }

        SECTION( "compute success-failure" )
        {
            auto computeNeg10 = [] { return Result< int >::value( -10 ); };

            auto result = computeNeg10().and_then( checkPositive );
            CHECK( result == Result< size_t >::error( "negative index" ) );
        }

        SECTION( "compute failure" )
        {
            auto computeErr = [] { return Result< Moveable >::error( "10" ); };
            auto dontRun = []( auto ) -> Result< int > {
                throw std::runtime_error( "unexpected run" );
            };

            auto result = computeErr().and_then( dontRun );
            CHECK( result == Result< int >::error( "10" ) );
        }
    }
    SECTION( "or else" )
    {
        SECTION( "compute success" )
        {
            auto compute10 = [] { return Result< int >::value( 10 ); };
            auto dontRun = []( auto ) -> Result< int > {
                throw std::runtime_error( "unexpected run" );
            };

            auto result = compute10().or_else( dontRun );
            CHECK( result == Result< int >::value( 10 ) );
        }

        SECTION( "compute failure-success" )
        {
            auto computeErr = [] { return Result< int >::error( "10" ); };
            auto transformTo10 = []( auto ) { return Result< int >::value( 10 ); };

            auto result = computeErr().or_else( transformTo10 );
            CHECK( result == Result< int >::value( 10 ) );
        }

        SECTION( "compute failure-failure" )
        {
            auto computeErr = [] { return Result< Moveable >::error( "hello" ); };
            auto transformErr = []( auto prev ) {
                return Result< Moveable >::error( prev + " world" );
            };

            auto result = computeErr().or_else( transformErr );
            REQUIRE_FALSE( result );
            CHECK( result.assume_error() == "hello world" );
        }
    }
    SECTION( "or else (no argument)" )
    {
        SECTION( "compute success" )
        {
            auto compute10 = [] { return Result< int >::value( 10 ); };
            auto dontRun = []() -> Result< int > { throw std::runtime_error( "unexpected run" ); };

            auto result = compute10().or_else( dontRun );
            CHECK( result == Result< int >::value( 10 ) );
        }

        SECTION( "compute failure-success" )
        {
            auto computeErr = [] { return Result< int >::error( "10" ); };
            auto transformTo10 = []() { return Result< int >::value( 10 ); };

            auto result = computeErr().or_else( transformTo10 );
            CHECK( result == Result< int >::value( 10 ) );
        }

        SECTION( "compute failure-failure" )
        {
            auto computeErr = [] { return Result< Moveable >::error( "hello" ); };
            auto transformErr = []() { return Result< Moveable >::error( " world" ); };

            auto result = computeErr().or_else( transformErr );
            REQUIRE_FALSE( result );
            CHECK( result.assume_error() == " world" );
        }
    }
    SECTION( "transform" )
    {
        auto dontRun = []( auto ) -> int { throw std::runtime_error( "unexpected run" ); };

        auto result1 = Result< int >::value( 10 ).transform( []( int v ) { return v + 5; } );
        CHECK( result1 == Result< size_t >::value( 15 ) );

        auto result2 = Result< int >::value( 10 ).transform( []( auto ) { return Moveable{}; } );
        CHECK( result2.has_value() );
        static_assert( std::is_same_v< decltype( result2 ), Result< Moveable > > );

        auto result3 = Result< int, Moveable >::value( 10 ).transform(
                []( auto ) { return Copyable{}; } );
        CHECK( result3.has_value() );
        static_assert( std::is_same_v< decltype( result3 ), Result< Copyable, Moveable > > );

        auto result4 = Result< Moveable, Copyable >::error( Copyable{} ).transform( dontRun );
        CHECK_FALSE( result4 );
        static_assert( std::is_same_v< decltype( result4 ), Result< int, Copyable > > );
    }
    SECTION( "transform error" )
    {
        auto dontRun = []( auto ) -> int { throw std::runtime_error( "unexpected run" ); };

        auto result1 = Result< Moveable >::error( "hello" ).transform_error(
                []( auto v ) { return v + " world"; } );
        CHECK( !result1 );
        CHECK( result1.assume_error() == "hello world" );

        auto result2 = Result< Moveable, Copyable >::value( Moveable{} ).transform_error( dontRun );
        CHECK( result2 );
        static_assert( std::is_same_v< decltype( result2 ), Result< Moveable, int > > );
    }
}

TEST_CASE( "Match" )
{
    SECTION( "basic" )
    {
        auto value = Result< int >::value( 11 );

        CHECK( value.match( overload{
                []( int v ) { return v == 11; },
                []( const std::string & ) { return false; },
        } ) );

        auto value2 = Result< int >::value( 10 );
        CHECK( !value2.match( overload{
                []( std::true_type, auto && v ) { return v == 11; },
                []( std::false_type, auto && ) { return true; },
        } ) );

        auto moveableResult = Result< Moveable >::value( {} );
        auto moveable = moveableResult.match( overload{
                []( Moveable & m ) { return std::move( m ); },
                []( const std::string & str ) -> Moveable { throw std::runtime_error( str ); },
        } );

        auto moveableResult2 = Result< Moveable >::value( {} );
        auto moveable2 = std::move( moveableResult2 )
                                 .match( overload{
                                         []( Moveable && m ) { return std::move( m ); },
                                         []( const std::string & str ) -> Moveable {
                                             throw std::runtime_error( str );
                                         },
                                 } );

        auto moveable3 = Result< Moveable >::value( {} ).match( overload{
                []( Moveable && m ) { return std::move( m ); },
                []( const std::string & str ) -> Moveable { throw std::runtime_error( str ); },
        } );
    }
    SECTION( "same" )
    {
        auto checkValue11CLRef = overload{
                []( std::true_type, const int & v ) { return v == 11; },
                []( std::false_type, const int & ) { return false; },
        };
        auto checkValue11LRef = overload{
                []( std::true_type, int & v ) { return v == 11; },
                []( std::false_type, int & ) { return false; },
        };
        auto checkValue11RRef = overload{
                []( std::true_type, int && v ) { return v == 11; },
                []( std::false_type, int && ) { return false; },
        };

        // Have to match exactly the arguments because of the reference
        auto value11 = Result< int, int >::value( 11 );
        auto value14 = Result< int, int >::value( 14 );
        auto error11 = Result< int, int >::error( 11 );

        value11.match( overload{ []( std::true_type, int ) {}, []( std::false_type, int ) {} } );
        CHECK( value11.match( checkValue11LRef ) );
        CHECK( !value14.match( checkValue11LRef ) );
        CHECK( !error11.match( checkValue11LRef ) );

        CHECK( value11.match( checkValue11CLRef ) );
        CHECK( !value14.match( checkValue11CLRef ) );
        CHECK( !error11.match( checkValue11CLRef ) );

        const auto cvalue11 = Result< int, int >::value( 11 );
        const auto cvalue14 = Result< unsigned, long >::value( 14 );
        const auto cerror11 = Result< long, int >::error( 11 );

        CHECK( cvalue11.match( checkValue11CLRef ) );
        CHECK( !cvalue14.match( checkValue11CLRef ) );
        CHECK( !cerror11.match( checkValue11CLRef ) );

        CHECK( Result< int, int >::value( 11 ).match( checkValue11RRef ) );
        CHECK( !Result< int, long >::value( 14 ).match( checkValue11RRef ) );
        CHECK( !Result< int, long >::error( 11 ).match( checkValue11RRef ) );

        CHECK( Result< int, int >::value( 11 ).match( checkValue11CLRef ) );
        CHECK( !Result< int, long >::value( 14 ).match( checkValue11CLRef ) );
        CHECK( !Result< int, long >::error( 11 ).match( checkValue11CLRef ) );
    }
}

TEST_CASE( "Operator piping" )
{
    SECTION( "and then (>>)" )
    {
        SECTION( "compute success-success" )
        {
            auto checkPositive = []( int idx ) -> Result< size_t > {
                return idx >= 0 ? Result< size_t >::value( to_unsigned( idx ) )
                                : Result< size_t >::error( "negative index" );
            };

            auto result = Result< int >::value( 10 ) >> checkPositive;
            CHECK( result == Result< size_t >::value( 10 ) );
        }

        SECTION( "compute success-failure" )
        {
            auto result = Result< int >::value( -10 ) >> []( int idx ) {
                return idx >= 0 ? Result< size_t >::value( to_unsigned( idx ) )
                                : Result< size_t >::error( "negative index" );
            };
            CHECK( result == Result< size_t >::error( "negative index" ) );
        }

        SECTION( "compute failure" )
        {
            auto result = Result< Moveable >::error( "10" ) >> []( auto ) -> Result< int > {
                throw std::runtime_error( "unexpected run" );
            };
            CHECK( result == Result< int >::error( "10" ) );
        }
    }
}
