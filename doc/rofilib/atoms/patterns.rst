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


Cloneable Object Hiearchy & ValuePtr
------------------------------------

Atoms provide means to simply implement cloneable virtual hiearchies. Consider
the following code:

.. code-block:: cpp

    class Animal {
    public:
        virtual ~Animal() = default;
    };

    class Dog: public Animal {};
    class Cat: public Animal {};

    // Our code:
    std::unique_ptr< Animal > ptr( new Dog() );
    // Let's make a copy - how to do so? We don't know what is behind Animal.
    // The following attempt won't work:
    std::unique_ptr< Animal > ptrCopy( new Animal( *ptr.get() ) ); // Does not compile

To make the it work we can add a virtual ``clone`` method. This process can be
tedious, so Atoms provide several macros to easy the task. By the way, we use
macro-based approach over CRTP-based approach as we consider it more readable
and easier to maintain.

.. code-block:: cpp

    class Animal {
    public:
        virtual ~Animal() = default;
        ATOMS_CLONEABLE_BASE( Animal );
    };

    class Dog: public Animal {
    public:
        ATOMS_CLONEABLE( Dog );
    };

    class Cat: public Animal {
    public:
        ATOMS_CLONEABLE( Cat );
    };

    // Our code:
    std::unique_ptr< Animal > ptr( new Dog() );
    // Let's make a copy:
    std::unique_ptr< Animal > ptrCopy( ptr->clone() );

Sometimes, you need to store a such hierarchy in class which you want to make
copyable. With ``unique_ptr`` you have to manually define all the copy and move
constructors. This can be tedious, therefore, atoms provide
:cpp:class:`atoms::ValuePtr` which stores objects on heap, but provides copy
semantics for them. It is somewhat similar to proposed ``std::polymorphic_value``,
however, it does not feature small-buffer optimization to provide stable
references.

.. doxygenclass:: atoms::ValuePtr
    :project: lib
