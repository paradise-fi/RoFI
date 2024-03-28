Result
======

Result provides a way to return a value or an error.
The concept is similar to `Either
<https://hackage.haskell.org/package/base-4.7.0.1/docs/Data-Either.html>`_
in Haskell or `Result <https://doc.rust-lang.org/std/result/>`_ in Rust.

Usage
-----

Use `value`, `error`, `emplace_value` and `emplace_error` to create new `Result`.

.. code-block:: cpp

    auto value42 = atoms::Result< int >::value( 22 );
    auto value0 = atoms::Result< int, std::runtime_error >::emplace_value();
    auto errStr = atoms::Result< double >::error( "string is default error type" );
    auto errExcept = atoms::Result< int, std::runtime_error >::emplace_error( "passed to constructor" );

When returning `Result` from a function, you can use helper creation functions.

.. code-block:: cpp

    atoms::Result< int > div( int lhs, int rhs ) {
        if ( rhs == 0 ) {
            return atoms::result_error< std::string >( "division by 0" );
        }
        return atoms::result_value( lhs / rhs );
    }

Use `make_` helper creation function variants to pass arguments directly to constuctors.

.. code-block:: cpp

    atoms::Result< std::vector< double > > repeat( int count, double value ) {
        if ( count < 0 ) {
            return atoms::make_result_error< std::string >( "count is negative" );
        }
        return atoms::make_result_value( count, value );
    }


You can check if a `Result` contains a value and dereference the `Result` to get it.

.. code-block:: cpp

    auto result = atoms::Result< int >::value( 10 );

    if ( result ) { // or `result.has_value()`
        std::cout << "Have result " << *result << "\n"; // or `result.assume_value()`
    } else {
        std::cout << "Have error " << result.assume_error() << "\n";
    }


You can use monadic methods based on `std::optional`.
Note that when using monadic methods, the other type has to be copy/move constructible.

.. code-block:: cpp

    auto result = atoms::Result< int >::value( 10 );

    auto newResult = result.and_then( []( int value ){
            return atoms::result_error( std::to_string( value ) );
        } )
        .or_else( []( const std::string & str ) {
            return atoms::value( static_cast< int >( str.size() ) );
        } )
        .or_else( []() {
            return atoms::value( 12 );
        } )
        .transform( []( int value ){
            return static_cast< unsigned >( value + 2 );
        } );

    // If you `std::move` the result, the content will be moved as well
    unsigned newValue = std::move( newResult ).transform_error( []( std::string str ){
            return "Error: " + std::move( str );
        } )
        .value_or( []( std::string error ){
            throw std::runtime_error( std::move( error ) );
        } );

For matching on a result, you can use `Result::match` methods.

.. code-block:: cpp

    #include <atoms/util.hpp> // for `overloaded`

    // ...

    auto correctAnswer = atoms::Result< int >::value( 10 );

    correctAnswer.match( overloaded{
        []( int value ) { std::cout << "Got answer " << value << "\n"; }
        []( const std::string & error ) { std::cerr << "Got error: " << error << "\n"; }
    } );

    // With compatible value and error types
    auto correctStrAnswer = atoms::Result< std::string, std::string >::value( "Apple" );

    /**
     * Doesn't compile (compiler cannot deduce value/error from type)
     * Also doesn't work on compatible types (e.g. int/unsigned) for the programmer's safety.
     */
    // correctAnswer.match( overloaded{
    //     []( const std::string & value ) { std::cout << "Got answer " << value << "\n"; }
    //     []( const std::string & error ) { std::cerr << "Got error: " << error << "\n"; }
    // } );

    correctAnswer.match( overloaded{
        []( std::true_type, const std::string & value ) { std::cout << "Got answer " << value << "\n"; }
        []( std::false_type, const std::string & error ) { std::cerr << "Got error: " << error << "\n"; }
    } );


Classes reference
-----------------

.. doxygenclass:: atoms::Result
    :project: lib

.. doxygenstruct:: atoms::detail::TypePlaceholder
    :project: lib

Functions reference
-------------------

.. doxygenfunction:: atoms::result_value
    :project: lib
.. doxygenfunction:: atoms::make_result_value
    :project: lib

.. doxygenfunction:: atoms::result_error
    :project: lib
.. doxygenfunction:: atoms::make_result_error
    :project: lib
