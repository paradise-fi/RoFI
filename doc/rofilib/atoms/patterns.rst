Patterns
========

Patterns aim for removing unnecessary boilerplate code from your project for
commonly used programming patterns.

Visitor Pattern
---------------

Visitors allow you to separate algorithms from your data structures in object
hierarchy. It allows you to implement new functionality for existing classes
without modifying them (i.e., adding new virtual methods). See `Wikipedia
<https://en.wikipedia.org/wiki/Visitor_pattern>`_ for more details.

To remove boilerplate, you can use Atoms.

Consider a simple class hiearchy of animals:

.. code-block:: cpp

    class Animal {
    public:
        virtual ~Animal() = default;
    };

    class Cat: public Animal {};
    class Dog: public Animal {};

To make them visitable via atoms, you need to modify your definition a little
bit:

.. code-block:: cpp

    class Cat; // Forward declaration, so we can define Visitor
    class Dog; // Forward declaration, so we can define Visitor

    // Our visitor base class has to be defined before the base class, so we can
    // specify the visitor type in the definition
    using AnimalVisitor = atoms::Visits< Cat, Dog >;

    class Animal: public atoms::VisitableBase< Animal, AnimalVisitor > {
    public:
        virtual ~Animal() = default;
    };

    class Cat: public atoms::Visitable< Animal, Cat > {}; // Make cat visitable
    class Dog: public atoms::Visitable< Animal, Dog > {}; // Make dog visitable

Now you can hand-craft a traditional visitor and use it:

.. code-block:: cpp

    class TypeVisitor: public AnimalVisitor {
    public:
        void operator()( Cat& a ) override {
            result = "cat";
        }

        void operator()( Dog& b ) override {
            result = "dog";
        }

        std::string result;
    };

    // ...and when you want to use it:
    Cat cat;
    Dog dog;
    Animal *hiddenCat = &cat;
    Animal *hiddenDog = &dog;

    TypeVisitor visitor;
    cat.accept( visitor );
    std::cout << visitor.result << "\n";

    dog.accept( visitor );
    std::cout << visitor.result << "\n";

    hiddenCat->accept( visitor );
    std::cout << visitor.result << "\n";

    hiddenDog->accept( visitor );
    std::cout << visitor.result << "\n";

With atoms, you can even construct a visitor in place from an ordinary function
or/and lambdas:

.. code-block:: cpp

    std::string catType( Cat& ) { return "cat"; }
    auto visitor = AnimalVisitor::make(
        catType,  // Cat is visited by an ordinary function
        []( Dog& ) -> std::string { return "dog" }); // Dog is visited by a lambda
    // Note that you have to cover all cases, otherwise your program won't compile

    // Then we can use it like before
    hiddenCat->accept( visitor );
    std::cout << visitor.result << "\n";

This is not all, with Atoms, you can avoid the annoying "accept then result"
construction and visit object like function call:

.. code-block:: cpp

    std::cout << atoms::visit( hiddenCat, visitor) << "\n";
    // You can even define the visitor in place:
    std::cout << atoms::visit( hiddenCat,
        []( Cat& cat ) { return "cat"; },
        []( Dog& dog ) { return "dog"; } ) << "\n";

Bellow, you can find the references of the provided classes and functions:

.. doxygenstruct:: atoms::Visits
    :project: lib

.. doxygenstruct:: atoms::VisitableBase
    :project: lib

.. doxygenstruct:: atoms::Visitable
    :project: lib

.. doxygenstruct:: atoms::Visitor
    :project: lib

.. doxygenfunction:: atoms::visit(T &object, Fs... fs)
    :project: lib

.. doxygenfunction:: atoms::visit(T &object, Visitor visitor) -> std::enable_if_t<std::is_base_of_v<typename T::VisitorType, Visitor>, typename Visitor::ReturnType>
    :project: lib
