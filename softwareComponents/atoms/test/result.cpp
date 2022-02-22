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
