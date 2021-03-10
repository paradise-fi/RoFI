#include <catch2/catch.hpp>
#include <atoms/patterns.hpp>
#include <string>

using namespace std::string_literals;

class Cat; // Forward declaration
class Dog; // Forward declaration

using AnimalVisitor = atoms::Visits< Cat, Dog >;

class Animal: public atoms::VisitableBase< Animal, AnimalVisitor > {
public:
    virtual ~Animal() = default;
};

class Cat: public atoms::Visitable< Animal, Cat > {};
class Dog: public atoms::Visitable< Animal, Dog > {};

// A traditional "hand-crafted" visitor
class TypeVisitor: public AnimalVisitor {
public:
    void operator()( Cat& a ) override {
        result = "cat"s;
    }

    void operator()( Dog& b ) override {
        result = "dog"s;
    }

    using ReturnType = std::string; // Necessary for compatibility with atoms::visit
    ReturnType result;
};

TEST_CASE( "Traditional visiting" ) {
    TypeVisitor visitor;
    Cat cat;
    Dog dog;
    Animal *hiddenCat = &cat;
    Animal *hiddenDog = &dog;

    cat.accept( visitor );
    REQUIRE( visitor.result == "cat"s );

    dog.accept( visitor );
    REQUIRE( visitor.result == "dog"s );

    hiddenCat->accept( visitor );
    REQUIRE( visitor.result == "cat"s );

    hiddenDog->accept( visitor );
    REQUIRE( visitor.result == "dog"s );
}

void catFoo( Cat& ) {}
void dogFoo( Dog& ) {}

std::string catType( Cat& ) { return "cat"s; }
std::string dogType( Dog& ) { return "dog"s; }

TEST_CASE( "In-place visitors" ) {
    Cat cat;
    Dog dog;
    Animal *hiddenCat = &cat;
    Animal *hiddenDog = &dog;

    SECTION( "Plain functions, no return value" ) {
        auto visitor = AnimalVisitor::make( catFoo, dogFoo );
        cat.accept( visitor );
        dog.accept( visitor );
        hiddenCat->accept( visitor );
        hiddenDog->accept( visitor );
    }

    SECTION( "Lambdas, no return value" ) {
        auto visitor = AnimalVisitor::make(
            []( Cat& cat ) {},
            []( Dog& dog ) {} );
        cat.accept( visitor );
        dog.accept( visitor );
        hiddenCat->accept( visitor );
        hiddenDog->accept( visitor );
    }

    SECTION( "Plain functions, return value" ) {
        auto visitor = AnimalVisitor::make( catType, dogType );
        cat.accept( visitor );
        REQUIRE( visitor.result == "cat"s );

        dog.accept( visitor );
        REQUIRE( visitor.result == "dog"s );

        hiddenCat->accept( visitor );
        REQUIRE( visitor.result == "cat"s );

        hiddenDog->accept( visitor );
        REQUIRE( visitor.result == "dog"s );
    }

    SECTION( "Lambdas functions, return value" ) {
        auto visitor = AnimalVisitor::make(
            []( Cat& cat ) { return "cat"; },
            []( Dog& dog ) { return "dog"; } );
        cat.accept( visitor );
        REQUIRE( visitor.result == "cat"s );

        dog.accept( visitor );
        REQUIRE( visitor.result == "dog"s );

        hiddenCat->accept( visitor );
        REQUIRE( visitor.result == "cat"s );

        hiddenDog->accept( visitor );
        REQUIRE( visitor.result == "dog"s );
    }
}

TEST_CASE( "Mimic std::visit" ) {
    Cat cat;
    Dog dog;
    Animal *hiddenCat = &cat;
    Animal *hiddenDog = &dog;

    SECTION( "Use with hand-crafted visitor" ) {
        auto visitor = TypeVisitor();
        REQUIRE( atoms::visit( cat, visitor ) == "cat"s );
        REQUIRE( atoms::visit( dog, visitor ) == "dog"s );
        REQUIRE( atoms::visit( *hiddenCat, visitor ) == "cat"s );
        REQUIRE( atoms::visit( *hiddenDog, visitor ) == "dog"s );
    }

    SECTION( "Use with generated visitor" ) {
        auto visitor =  AnimalVisitor::make(
            []( Cat& cat ) { return "cat"; },
            []( Dog& dog ) { return "dog"; } );
        REQUIRE( atoms::visit( cat, visitor ) == "cat"s );
        REQUIRE( atoms::visit( dog, visitor ) == "dog"s );
        REQUIRE( atoms::visit( *hiddenCat, visitor ) == "cat"s );
        REQUIRE( atoms::visit( *hiddenDog, visitor ) == "dog"s );
    }

    SECTION( "Use with in-place visitor" ) {
        REQUIRE( atoms::visit( cat,
            []( Cat& cat ) { return "cat"; },
            []( Dog& dog ) { return "dog"; } ) == "cat"s );
        REQUIRE( atoms::visit( dog,
            []( Cat& cat ) { return "cat"; },
            []( Dog& dog ) { return "dog"; } ) == "dog"s );
        REQUIRE( atoms::visit( *hiddenCat,
            []( Cat& cat ) { return "cat"; },
            []( Dog& dog ) { return "dog"; } ) == "cat"s );
        REQUIRE( atoms::visit( *hiddenDog,
            []( Cat& cat ) { return "cat"; },
            []( Dog& dog ) { return "dog"; } ) == "dog"s );
    }
}