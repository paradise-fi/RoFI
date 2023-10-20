#include <configuration/rofiworld.hpp>
#include <configuration/joints.hpp>
#include <configuration/universalModule.hpp> 
#include <configuration/serialization.hpp>

#include <isoreconfig/isomorphic.hpp>
#include <isoreconfig/equality.hpp>

using namespace rofi::configuration;
using namespace rofi::isoreconfig;

RofiWorld parseRofiWorld( const std::string& path )
{
    std::ifstream inputTarget;
    inputTarget.open( path );
    if (inputTarget.fail()) 
    {
        std::cerr << "Invalid path to rofiworld: '" << path << "'\n";
        exit(1);
    }
    RofiWorld result = readOldConfigurationFormat( inputTarget );
    
    const auto identity = arma::mat(4, 4, arma::fill::eye);
    assert( result.modules().size() > 0 );
    connect< RigidJoint >( (*result.modules().begin()).bodies().front(), { 0, 0, 0 }, identity );
    result.prepare().get_or_throw_as< std::runtime_error >();

    return result;
}

std::string shapesPath = "./data/configurations/old/shapes/";

auto A1 = parseRofiWorld( shapesPath + "A1.in" );
auto A2 = parseRofiWorld( shapesPath + "A2.in" );
auto A3 = parseRofiWorld( shapesPath + "A3.in" );
auto A4 = parseRofiWorld( shapesPath + "A4.in" );
auto B1 = parseRofiWorld( shapesPath + "B1.in" );
auto B2 = parseRofiWorld( shapesPath + "B2.in" );
auto B3 = parseRofiWorld( shapesPath + "B3.in" );
auto B4 = parseRofiWorld( shapesPath + "B4.in" );
auto C1 = parseRofiWorld( shapesPath + "C1.in" );
auto C2 = parseRofiWorld( shapesPath + "C2.in" );
auto C3 = parseRofiWorld( shapesPath + "C3.in" );
auto C4 = parseRofiWorld( shapesPath + "C4.in" );
auto D1 = parseRofiWorld( shapesPath + "D1.in" );
auto D2 = parseRofiWorld( shapesPath + "D2.in" );
auto D3 = parseRofiWorld( shapesPath + "D3.in" );
auto D4 = parseRofiWorld( shapesPath + "D4.in" );
auto E1 = parseRofiWorld( shapesPath + "E1.in" );
auto E2 = parseRofiWorld( shapesPath + "E2.in" );
auto E3 = parseRofiWorld( shapesPath + "E3.in" );
auto E4 = parseRofiWorld( shapesPath + "E4.in" );
auto E5 = parseRofiWorld( shapesPath + "E5.in" );
auto E6 = parseRofiWorld( shapesPath + "E6.in" );
auto E7 = parseRofiWorld( shapesPath + "E7.in" );
auto E8 = parseRofiWorld( shapesPath + "E8.in" );
auto F1 = parseRofiWorld( shapesPath + "F1.in" );
auto F2 = parseRofiWorld( shapesPath + "F2.in" );
auto F3 = parseRofiWorld( shapesPath + "F3.in" );
auto F4 = parseRofiWorld( shapesPath + "F4.in" );
auto F5 = parseRofiWorld( shapesPath + "F5.in" );
auto F6 = parseRofiWorld( shapesPath + "F6.in" );
auto F7 = parseRofiWorld( shapesPath + "F7.in" );
auto F8 = parseRofiWorld( shapesPath + "F8.in" );
auto G1 = parseRofiWorld( shapesPath + "G1.in" );
auto G2 = parseRofiWorld( shapesPath + "G2.in" );
auto H1 = parseRofiWorld( shapesPath + "H1.in" );
auto H2 = parseRofiWorld( shapesPath + "H2.in" );

void testOne90APos()
{
    assert( equalShape( A1, A1 ) );
    assert( equalShape( A1, A2 ) );
    assert( equalShape( A1, A3 ) );
    assert( equalShape( A1, A4 ) );

    assert( equalShape( A2, A1 ) );
    assert( equalShape( A2, A2 ) );
    assert( equalShape( A2, A3 ) );
    assert( equalShape( A2, A4 ) );

    assert( equalShape( A3, A1 ) );
    assert( equalShape( A3, A2 ) );
    assert( equalShape( A3, A3 ) );
    assert( equalShape( A3, A4 ) );

    assert( equalShape( A4, A1 ) );
    assert( equalShape( A4, A2 ) );
    assert( equalShape( A4, A3 ) );
    assert( equalShape( A4, A4 ) );
}

void testOne90BPos()
{
    assert( equalShape( B1, B1 ) );
    assert( equalShape( B1, B2 ) ); // Reflection
    assert( equalShape( B1, B3 ) );
    assert( equalShape( B1, B4 ) );

    assert( equalShape( B2, B1 ) );
    assert( equalShape( B2, B2 ) );
    assert( equalShape( B2, B3 ) );
    assert( equalShape( B2, B4 ) );

    assert( equalShape( B3, B1 ) );
    assert( equalShape( B3, B2 ) );
    assert( equalShape( B3, B3 ) );
    assert( equalShape( B3, B4 ) );

    assert( equalShape( B4, B1 ) );
    assert( equalShape( B4, B2 ) );
    assert( equalShape( B4, B3 ) );
    assert( equalShape( B4, B4 ) );
}

void testOne90CPos()
{
    assert( equalShape( C1, C1 ) );
    assert( equalShape( C1, C2 ) );
    assert( equalShape( C1, C3 ) );
    assert( equalShape( C1, C4 ) );

    assert( equalShape( C2, C1 ) );
    assert( equalShape( C2, C2 ) );
    assert( equalShape( C2, C3 ) );
    assert( equalShape( C2, C4 ) );

    assert( equalShape( C3, C1 ) );
    assert( equalShape( C3, C2 ) );
    assert( equalShape( C3, C3 ) );
    assert( equalShape( C3, C4 ) );

    assert( equalShape( C4, C1 ) );
    assert( equalShape( C4, C2 ) );
    assert( equalShape( C4, C3 ) );
    assert( equalShape( C4, C4 ) );
}

void testOne90DPos()
{
    assert( equalShape( D1, D1 ) );
    assert( equalShape( D1, D2 ) );
    assert( equalShape( D1, D3 ) );
    assert( equalShape( D1, D4 ) );

    assert( equalShape( D2, D1 ) );
    assert( equalShape( D2, D2 ) );
    assert( equalShape( D2, D3 ) );
    assert( equalShape( D2, D4 ) );

    assert( equalShape( D3, D1 ) );
    assert( equalShape( D3, D2 ) );
    assert( equalShape( D3, D3 ) );
    assert( equalShape( D3, D4 ) );

    assert( equalShape( D4, D1 ) );
    assert( equalShape( D4, D2 ) );
    assert( equalShape( D4, D3 ) );
    assert( equalShape( D4, D4 ) );
}

void testOne90EPos()
{
    assert( equalShape( E1, E1 ) );
    assert( equalShape( E1, E2 ) );
    assert( equalShape( E1, E3 ) );
    assert( equalShape( E1, E4 ) );
    assert( equalShape( E1, E5 ) );
    assert( equalShape( E1, E6 ) );
    assert( equalShape( E1, E7 ) );
    assert( equalShape( E1, E8 ) );

    assert( equalShape( E2, E1 ) );
    assert( equalShape( E2, E2 ) );
    assert( equalShape( E2, E3 ) );
    assert( equalShape( E2, E4 ) );
    assert( equalShape( E2, E5 ) );
    assert( equalShape( E2, E6 ) );
    assert( equalShape( E2, E7 ) );
    assert( equalShape( E2, E8 ) );

    assert( equalShape( E3, E1 ) );
    assert( equalShape( E3, E2 ) );
    assert( equalShape( E3, E3 ) );
    assert( equalShape( E3, E4 ) );
    assert( equalShape( E3, E5 ) );
    assert( equalShape( E3, E6 ) );
    assert( equalShape( E3, E7 ) );
    assert( equalShape( E3, E8 ) );

    assert( equalShape( E4, E1 ) );
    assert( equalShape( E4, E2 ) );
    assert( equalShape( E4, E3 ) );
    assert( equalShape( E4, E4 ) );
    assert( equalShape( E4, E5 ) );
    assert( equalShape( E4, E6 ) );
    assert( equalShape( E4, E7 ) );
    assert( equalShape( E4, E8 ) );
    
    assert( equalShape( E5, E1 ) );
    assert( equalShape( E5, E2 ) );
    assert( equalShape( E5, E3 ) );
    assert( equalShape( E5, E4 ) );
    assert( equalShape( E5, E5 ) );
    assert( equalShape( E5, E6 ) );
    assert( equalShape( E5, E7 ) );
    assert( equalShape( E5, E8 ) );
    
    assert( equalShape( E6, E1 ) );
    assert( equalShape( E6, E2 ) );
    assert( equalShape( E6, E3 ) );
    assert( equalShape( E6, E4 ) );
    assert( equalShape( E6, E5 ) );
    assert( equalShape( E6, E6 ) );
    assert( equalShape( E6, E7 ) );
    assert( equalShape( E6, E8 ) );
    
    assert( equalShape( E7, E1 ) );
    assert( equalShape( E7, E2 ) );
    assert( equalShape( E7, E3 ) );
    assert( equalShape( E7, E4 ) );
    assert( equalShape( E7, E5 ) );
    assert( equalShape( E7, E6 ) );
    assert( equalShape( E7, E7 ) );
    assert( equalShape( E7, E8 ) );
    
    assert( equalShape( E8, E1 ) );
    assert( equalShape( E8, E2 ) );
    assert( equalShape( E8, E3 ) );
    assert( equalShape( E8, E4 ) );
    assert( equalShape( E8, E5 ) );
    assert( equalShape( E8, E6 ) );
    assert( equalShape( E8, E7 ) );
    assert( equalShape( E8, E8 ) );
}

void testOne90FPos()
{
    assert( equalShape( F1, F1 ) );
    assert( equalShape( F1, F2 ) );
    assert( equalShape( F1, F3 ) );
    assert( equalShape( F1, F4 ) );
    assert( equalShape( F1, F5 ) );
    assert( equalShape( F1, F6 ) );
    assert( equalShape( F1, F7 ) );
    assert( equalShape( F1, F8 ) );

    assert( equalShape( F2, F1 ) );
    assert( equalShape( F2, F2 ) );
    assert( equalShape( F2, F3 ) );
    assert( equalShape( F2, F4 ) );
    assert( equalShape( F2, F5 ) );
    assert( equalShape( F2, F6 ) );
    assert( equalShape( F2, F7 ) );
    assert( equalShape( F2, F8 ) );

    assert( equalShape( F3, F1 ) );
    assert( equalShape( F3, F2 ) );
    assert( equalShape( F3, F3 ) );
    assert( equalShape( F3, F4 ) );
    assert( equalShape( F3, F5 ) );
    assert( equalShape( F3, F6 ) );
    assert( equalShape( F3, F7 ) );
    assert( equalShape( F3, F8 ) );

    assert( equalShape( F4, F1 ) );
    assert( equalShape( F4, F2 ) );
    assert( equalShape( F4, F3 ) );
    assert( equalShape( F4, F4 ) );
    assert( equalShape( F4, F5 ) );
    assert( equalShape( F4, F6 ) );
    assert( equalShape( F4, F7 ) );
    assert( equalShape( F4, F8 ) );
    
    assert( equalShape( F5, F1 ) );
    assert( equalShape( F5, F2 ) );
    assert( equalShape( F5, F3 ) );
    assert( equalShape( F5, F4 ) );
    assert( equalShape( F5, F5 ) );
    assert( equalShape( F5, F6 ) );
    assert( equalShape( F5, F7 ) );
    assert( equalShape( F5, F8 ) );
    
    assert( equalShape( F6, F1 ) );
    assert( equalShape( F6, F2 ) );
    assert( equalShape( F6, F3 ) );
    assert( equalShape( F6, F4 ) );
    assert( equalShape( F6, F5 ) );
    assert( equalShape( F6, F6 ) );
    assert( equalShape( F6, F7 ) );
    assert( equalShape( F6, F8 ) );
    
    assert( equalShape( F7, F1 ) );
    assert( equalShape( F7, F2 ) );
    assert( equalShape( F7, F3 ) );
    assert( equalShape( F7, F4 ) );
    assert( equalShape( F7, F5 ) );
    assert( equalShape( F7, F6 ) );
    assert( equalShape( F7, F7 ) );
    assert( equalShape( F7, F8 ) );
    
    assert( equalShape( F8, F1 ) );
    assert( equalShape( F8, F2 ) );
    assert( equalShape( F8, F3 ) );
    assert( equalShape( F8, F4 ) );
    assert( equalShape( F8, F5 ) );
    assert( equalShape( F8, F6 ) );
    assert( equalShape( F8, F7 ) );
    assert( equalShape( F8, F8 ) );
}

void testOne90GPos()
{
    assert( equalShape( G1, G1 ) );
    assert( equalShape( G1, G2 ) );

    assert( equalShape( G2, G1 ) );
    assert( equalShape( G2, G2 ) );
}

void testOne90HPos()
{
    assert( equalShape( H1, H1 ) );
    assert( equalShape( H1, H2 ) );

    assert( equalShape( H2, H1 ) );
    assert( equalShape( H2, H2 ) );
}

void testOne90Neg()
{
    assert( !equalShape( A1, B1 ) );
    assert( !equalShape( A1, C1 ) );
    assert( !equalShape( A1, D1 ) );
    assert( !equalShape( A1, E1 ) );
    assert( !equalShape( A1, F1 ) );
    assert( !equalShape( A1, G1 ) );
    assert( !equalShape( A1, H1 ) );

    assert( !equalShape( B1, A1 ) );
    assert( !equalShape( B1, C1 ) );
    assert( !equalShape( B1, D1 ) );
    assert( !equalShape( B1, E1 ) );
    assert( !equalShape( B1, F1 ) );
    assert( !equalShape( B1, G1 ) );
    assert( !equalShape( B1, H1 ) );

    assert( !equalShape( C1, A1 ) );
    assert( !equalShape( C1, B1 ) );
    assert( !equalShape( C1, D1 ) ); // Reflections
    assert( !equalShape( C1, E1 ) );
    assert( !equalShape( C1, F1 ) );
    assert( !equalShape( C1, G1 ) );
    assert( !equalShape( C1, H1 ) );

    assert( !equalShape( D1, A1 ) );
    assert( !equalShape( D1, B1 ) );
    assert( !equalShape( D1, C1 ) ); // Reflections
    assert( !equalShape( D1, E1 ) );
    assert( !equalShape( D1, F1 ) );
    assert( !equalShape( D1, G1 ) );
    assert( !equalShape( D1, H1 ) );

    assert( !equalShape( E1, A1 ) );
    assert( !equalShape( E1, B1 ) );
    assert( !equalShape( E1, C1 ) );
    assert( !equalShape( E1, D1 ) );
    assert( !equalShape( E1, F1 ) );
    assert( !equalShape( E1, G1 ) );
    assert( !equalShape( E1, H1 ) );

    assert( !equalShape( F1, A1 ) );
    assert( !equalShape( F1, B1 ) );
    assert( !equalShape( F1, C1 ) );
    assert( !equalShape( F1, D1 ) );
    assert( !equalShape( F1, E1 ) );
    assert( !equalShape( F1, G1 ) );
    assert( !equalShape( F1, H1 ) );

    assert( !equalShape( G1, A1 ) );
    assert( !equalShape( G1, B1 ) );
    assert( !equalShape( G1, C1 ) );
    assert( !equalShape( G1, D1 ) );
    assert( !equalShape( G1, E1 ) );
    assert( !equalShape( G1, F1 ) );
    assert( !equalShape( G1, H1 ) );

    assert( !equalShape( H1, A1 ) );
    assert( !equalShape( H1, B1 ) );
    assert( !equalShape( H1, C1 ) );
    assert( !equalShape( H1, D1 ) );
    assert( !equalShape( H1, E1 ) );
    assert( !equalShape( H1, F1 ) );
    assert( !equalShape( H1, G1 ) );
}

void testOne90()
{
    testOne90APos();
    testOne90BPos();
    testOne90CPos();
    testOne90DPos();
    testOne90EPos();
    testOne90FPos();
    testOne90GPos();
    testOne90HPos();
    testOne90Neg();
}

auto TripleA1 = parseRofiWorld( shapesPath + "TripleA1.in" );
auto TripleA2 = parseRofiWorld( shapesPath + "TripleA2.in" );
auto TripleB1 = parseRofiWorld( shapesPath + "TripleB1.in" );

void testThree()
{
    assert( equalShape( TripleA1, TripleA1 ) );
    assert( equalShape( TripleA1, TripleA2 ) );
    assert( equalShape( TripleA2, TripleA1 ) );
    assert( equalShape( TripleA2, TripleA2 ) );
    assert( !equalShape( TripleA1, TripleB1 ) );
    assert( !equalShape( TripleB1, TripleA1 ) );
}

void testStrictEquality()
{
    assert( equalConfiguration( A1, A1 ) );
    assert( !equalConfiguration( A1, A2 ) );
    assert( !equalConfiguration( A1, A3 ) );
    assert( !equalConfiguration( A1, A4 ) );

    RofiWorld A1Copy = A1;
    assert( equalConfiguration( A1Copy, A1 ) );
}

int main(int argc, char** argv) 
{
    testOne90();
    testThree();
    testStrictEquality();
    std::cout << "All tests have passed.\n";
}
