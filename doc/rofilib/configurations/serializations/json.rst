Json Serialization
==================

Configuration also supports serialization to and from `json <https://www.json.org/json-en.html>`_
format via functions :cpp:func:`toJSON <rofi::configuration::serialization::toJSON>`
and :cpp:func:`fromJSON <rofi::configuration::serialization::fromJSON>` respectively,
so that you can save your configuration into a file and load it as needed.
For the json itself we use `nlohman::json <https://github.com/nlohmann/json>`_ library.

.. contents::

Format Description
------------------

The configuration description consists of three main parts: `modules`,
`moduleJoints`, and `spaceJoints`.

The minimal configuration looks like this.

.. code-block:: cpp

    #include <configuration/serialization.hpp>

    // the json library supports string literals
    auto js = "{ \"modules\" : [], \"spaceJoints\" : [], \"moduleJoints\" : [] }"_json;
    RofiWorld world = fromJSON( js );
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
                "from" : { "id" : 66, "connector" : "A+X" },
                "to" :   { "id" : 42, "connector" : "A-X" }
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
        "min" : < lower-limit - number >,
        "max" : < upper-limit - number >
    }

Matrices are, as shown above, represented by 4x4 dimensional array. Or, for
the identity matrix, you can use a string representation, just write `"identity"`
instead of `[[1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]]`.

Functions
---------

.. doxygenfunction:: rofi::configuration::serialization::toJSON( const RofiWorld& world )
    :project: configuration

.. doxygenfunction:: rofi::configuration::serialization::toJSON( const RofiWorld& world, Callback attrCb )
    :project: configuration

The callback is optional. It provides you with the ability to extend the json representation with
an `"attributes"` property, which can be added to any object within the `json`. It can contain some
metadata you might use when working with and sharing the configuration description. For details, see
the section below.


.. doxygenfunction:: rofi::configuration::serialization::fromJSON( const nlohmann::json& j )
    :project: configuration

.. doxygenfunction:: rofi::configuration::serialization::fromJSON( const nlohmann::json& j, Callback attrCb )
    :project: configuration

Here you can provide a callback function, that is used for parsing the optional `"attributes"` field. If no
callback is provided, the field, if present, is ignored. The callback is written in the same way as for `toJSON`.

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

You can see that every function returns a `nlohman::json <https://json.nlohmann.me/>`__ which is then
stored to appropriate `"attributes"` field.

To collect these attributes you can then use this callback

.. code-block:: cpp

    std::vector< ModuleId > ids;
    
    overload{ [ &ids ]( const nlohmann::json& j, const Module& m ) {
                        ids.push_back( j );
              },
              []( const nlohmann::json&, const ComponentJoint&, int jointIndex )  { return; },
              []( const nlohmann::json&, const Component&, int componentIndex  )  { return; },
              []( const nlohmann::json&, RofiWorld::RoficomJointHandle ) { return; },
              []( const nlohmann::json&, RofiWorld::SpaceJointHandle )   { return; },
    };

See, that the main difference is in the arguments – callback given to `fromJSON` takes a `json` that
is the content of the respective `"attributes"` field.
