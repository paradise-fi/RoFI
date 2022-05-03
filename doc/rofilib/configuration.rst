Configuration
=============

Configuration is a way of representing any bot in an universal manner. It
contains all building-blocks of the RoFI platform such as various modules
and joints. Using these blocks, you can specify every rofibot and work
with it. Configuration computes positions of every component within the bot
and provides useful functions for manipulation with such rofibot, including
some validity checks (e.g., collision checks).

Using the configuration is pretty straightforward. The main class is
:cpp:class:`Rofibot <rofi::configuration::Rofibot>` for which you create an
instance and add all the modules you have within one bot. You have to connect
them appropriately and if you want to use absolute positions, you have to
fix one of the bot's components in space (otherwise, the configuration cannot
figure out its coordinates because everything is kept relative).

For example, let us create a small bot with two universal modules.

.. code-block:: cpp

    #include <configuration/rofibot.hpp>

    Rofibot bot;
    // add universal module with id 42 in the default state
    auto& m1 = bot.insert( UniversalModule( 42, 0_deg, 0_deg, 0_deg ) );
    // add universal module with id 42 with beta set to 45 degrees and gamma to 90 degrees
    auto& m2 = bot.insert( UniversalModule( 66, 0_deg, 45_deg, 90_deg ) );

    // connect A+X of the universal module with id = 42 to A-X of UM with id = 66
    connect( m1.connectors()[ 2 ], m2.connectors()[ 0 ], Orientation::North );
    // fix the position of the `shoe A` in { 0, 0, 0 }
    connect< RigidJoint >( m1.bodies()[ 0 ], { 0, 0, 0 }, identity );

With the `bot` in hand, you can then prepare it (i.e., compute its positions)
and check for validity.

.. code-block:: cpp

    bot.prepare();
    auto [ ok, err ] = bot.isValid( SimpleCollision() );
    if ( !ok )
        std::cerr << "invalid configuration: " << err << "\n";

    // or you can shorten the above to
    auto [ ok, err ] = bot.validate( SimpleCollision() );

    // also, the SimpleCollision model is the default one, so you can ommit it too and get
    auto [ ok, err ] = bot.validate();


Types and Constants
-------------------

.. doxygentypedef:: rofi::configuration::ModuleId
    :project: configuration

.. doxygenenum:: rofi::configuration::ComponentType
    :project: configuration

.. doxygenenum:: rofi::configuration::ModuleType
    :project: configuration

.. doxygenenum:: rofi::configuration::roficom::Orientation
    :project: configuration

Classes
-------

.. doxygenclass:: rofi::configuration::Rofibot
    :project: configuration

.. doxygenclass:: rofi::configuration::Module
    :project: configuration

.. doxygenstruct:: rofi::configuration::Joint
    :project: configuration

.. doxygenstruct:: rofi::configuration::RigidJoint
    :project: configuration

.. doxygenstruct:: rofi::configuration::RotationJoint
    :project: configuration

.. doxygenstruct:: rofi::configuration::RoficomJoint
    :project: configuration

.. doxygenstruct:: rofi::configuration::ComponentJoint
    :project: configuration

.. doxygenstruct:: rofi::configuration::SpaceJoint
    :project: configuration


.. doxygenclass:: rofi::configuration::NoCollision
    :project: configuration

.. doxygenclass:: rofi::configuration::SimpleCollision
    :project: configuration

Modules
-------

.. doxygenclass:: rofi::configuration::Pad
    :project: configuration

.. doxygenclass:: rofi::configuration::UniversalModule
    :project: configuration

.. doxygenclass:: rofi::configuration::UnknownModule
    :project: configuration

Functions
---------

.. doxygenfunction:: rofi::configuration::connect(const Component &c1, const Component &c2, roficom::Orientation o)
    :project: configuration

.. doxygenfunction:: rofi::configuration::connect(const Component &c, Vector refpoint, Args&&... args)
    :project: configuration

.. doxygenfunction:: rofi::configuration::makeComponentJoint
    :project: configuration

Serialization
-------------

Configuration also supports serialization to and from `json <https://www.json.org/json-en.html>`_
format via functions `toJSON` and `fromJSON` respectively, so that you can save your
configuration into a file and load it as needed. For the json itself we use
`nlohman::json <https://github.com/nlohmann/json>`_ library.

The configuration description consists of three main parts: `modules`,
`moduleJoints`, and `spaceJoints`.

The minimal configuration looks like this.

.. code-block:: cpp

    #include <configuration/serialization.hpp>

    // the json library supports string literals
    auto js = "{ \"modules\" : [], \"spaceJoints\" : [], \"moduleJoints\" : [] }"_json;
    Rofibot bot = fromJSON( js );
    // and we can continue as before

If we were to represent the configuration with two universal modules shown
above, we could do it with this json

.. code-block:: json

    {
        "modules" : [
            {
                "id" : 42,
                "type" : "universal",
                "alpha" : 0,
                "beta"  : 0,
                "gamma" : 0
            },
            {
                "id" : 66,
                "type" : "universal",
                "alpha" : 0,
                "beta"  : 45,
                "gamma" : 90
            }
        ],

        "moduleJoints" : [
            {
                "orientation" : "East",
                "from" : 66,
                "fromCon" : "A+X",
                "to" : 42,
                "toCon" : "A-X"
            }
        ],

        "spaceJoints" : [
            {
                "point" : [ 0, 0, 0 ],
                "to" : {
                         "id" : 42,
                         "component" : 6
                },
                "joint" : {
                            "type" : "rigid",
                            "sourceToDestination" : [ [1, 0, 0, 0]
                                                    , [0, 1, 0, 0]
                                                    , [0, 0, 1, 0]
                                                    , [0, 0, 0, 1] ]
                        }
            }
        ]
    }

You are not limited to universal modules only, currently we support a module
`Pad` representing a 5x4 pad of RoFICoMs which can be represented as

.. code-block:: json

    {
        "id" : 66,
        "type"   : "pad",
        "width"  : 5,
        "height" : 4
    }

and there is also a representation of an arbitrary module corresponding to
the `UnknownModule`. Its attributes mirror the class

.. code-block:: json

    {
        "id" : 66,
        "components" : [ < array of components > ],
        "joints"     : [ < array of joints >     ]
    }

where the `component` has three possible values

.. code-block:: json

    [
        {
            "type" : "roficom"
        },
        {
            "type" : "UM shoe"
        },
        {
            "type" : "UM body"
        }
    ]

and `joint` is represented as

.. code-block:: json

    {
        "from" : < component >,
        "destination" : < component >,
        "sourceToDestination" : < matrix >,
        "joint" : < joint >
    }

where possible values of the `joint` are either `RigidJoint` represented as

.. code-block:: json

    {
        "type" : "rigid"
    }

or the `RotationJoint` which requires appropriate matrices

.. code-block:: json

    {
        "type" : "rotational",
        "axis" : < 4-dimensional array >,
        "preMatrix"  : < matrix >,
        "postMatrix" : < matrix >,
        "min" : < lower-limit – number >,
        "max" : < upper-limit – number >
    }

Matrices are, as shown above, represented by 4x4 dimensional array. Or, for
the identity matrix, you can use a string representation, just write `"identity"`
instead of `[[1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]]`.


.. doxygenfunction:: rofi::configuration::serialization::toJSON( const Rofibot& bot, Callback attrCb )
    :project: configuration

The callback is optional. It provides you with the ability to extend the json representation with
an `"attributes"` property, which can be added to any object within the `json`. It can contain some
metadata you might use when working with and sharing the configuration description. For details, see
the section below.

.. doxygenfunction:: rofi::configuration::serialization::toJSON( const Rofibot& bot )
    :project: configuration

.. doxygenfunction:: rofi::configuration::serialization::fromJSON( const nlohmann::json& j, Callback attrCb )
    :project: configuration

Here you can provide a callback function, that is used for parsing the optional `"attributes"` field. If no
callback is provided, the field, if present, is ignored. The callback is written in the same way as for `toJSON`.

.. doxygenfunction:: rofi::configuration::serialization::fromJSON( const nlohmann::json& j )
    :project: configuration

Attributes callback
-------------------

You can extend the `json` description of a configuration with `"attributes"` field. This field can be present
in any object within the configuration, so the callback function has to be able to accept every corresponding
type. The possible callback for `toJSON` that stores a `ModuleId` to `"attributes"` looks like

.. code-block:: cpp

    overload{ []( const Module& m ) { return nlohmann::json( m.getId() ); },
              []( const ComponentJoint&, int jointIndex ) { return nlohmann::json{}; },
              []( const Component&, int componentIndex  ) { return nlohmann::json{}; },
              []( const RoficomJoint& ) { return nlohmann::json{}; },
              []( const SpaceJoint&   ) { return nlohmann::json{}; }
    };

You can see that every function returns a `nlohman::json <https://json.nlohmann.me/>`_ which is then
stored to appropriate `"attributes"` field.

To collect these attributes you can then use this callback

.. code-block:: cpp

    std::vector< ModuleId > ids;
    
    overload{ [ &ids ]( const nlohmann::json& j, const Module& m ) {
                        ids.push_back( j );
              },
              []( const nlohmann::json&, const ComponentJoint&, int jointIndex )  { return; },
              []( const nlohmann::json&, const Component&, int componentIndex  )  { return; },
              []( const nlohmann::json&, Rofibot::RoficomJointHandle ) { return; },
              []( const nlohmann::json&, Rofibot::SpaceJointHandle )   { return; },
    };

See, that the main difference is in the arguments – callback given to `fromJSON` takes a `json` that
is the content of the respective `"attributes"` field.
