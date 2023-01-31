#pragma once

#include <atoms/result.hpp>
#include <atoms/unreachable.hpp>
#include <configuration/rofiworld.hpp>
#include <configuration/universalModule.hpp>
#include <configuration/pad.hpp>
#include <configuration/cube.hpp>
#include <configuration/unknownModule.hpp>

#include <nlohmann/json.hpp>

namespace rofi::configuration::serialization {

    inline std::string componentTypeToString( ComponentType c ) {
        switch ( c ) {
            case ComponentType::Roficom:
                return "roficom";
            case ComponentType::UmBody:
                return "UM body";
            case ComponentType::UmShoe:
                return "UM shoe";
            case ComponentType::CubeBody:
                return "Cube body";
        }
        ROFI_UNREACHABLE( "Unknown component type" );
    }

    inline atoms::Result< ComponentType > stringToComponentType( const std::string& str ) {
        if ( str == "roficom" )
            return atoms::result_value( ComponentType::Roficom );
        if ( str == "UM body" )
            return atoms::result_value( ComponentType::UmBody );
        if ( str == "UM shoe" )
            return atoms::result_value( ComponentType::UmShoe );
        if ( str == "Cube body" )
            return atoms::result_value( ComponentType::CubeBody );

        return atoms::result_error< std::string >( "String does not represent a component type" );
    }

    namespace details {

    /* partial specialization on functions is a problem, so this hack uses partial specialization
     * on a struct with a static member function which we want to partialy specialize – the good
     * thing is it is hidden within the next moduleFromJSON function (the one which does not use
     * the ModuleId as an argument)
     */
    template< std::derived_from< Module > M, typename Callback >
    struct partial_spec {
        static M moduleFromJSON( const nlohmann::json& j, Callback& cb );
    };

    template< std::derived_from< Module > M, typename Callback >
    M moduleFromJSON( const nlohmann::json& j, Callback& cbAttr ) {
        return partial_spec< M, Callback >::moduleFromJSON( j, cbAttr );
    }

    inline nlohmann::json matrixToJSON( const Matrix& m ) {
        nlohmann::json j = nlohmann::json::array();
        for ( int i = 0; i < 4; i++ )
            j[ i ] = { m( i, 0 ), m( i, 1 ), m( i, 2 ), m( i, 3 ) };

        return j;
    }

    inline Matrix matrixFromJSON( const nlohmann::json& js ) {
        if ( js.is_string() ) {
            if ( js == "identity" )
                return rofi::configuration::matrices::identity;

            throw std::runtime_error( "matrix is a string, but the string does not correspond to a supported matrix" );
        }

        Matrix m;
        for ( int i = 0; i < 4; i++ ) {
            for ( int j = 0; j < 4; j++ ) {
                m( i, j ) = js[ i ][ j ];
            }
        }
        return m;
    }

    /** \brief Adds attributes obtained from callback and returns true if some were added, false otherwise.
     *
     * see `toJSON` for details about the callback
     */
    template< typename Callback, typename...Args >
    inline bool addAttributes( nlohmann::json& js, Callback& attrCb, Args&& ... args ) {
        static_assert( std::is_invocable_r_v< nlohmann::json, Callback, Args ... > );
        nlohmann::json j = attrCb( std::forward< Args >( args )... );
        if ( !j.is_null() )
            js[ "attributes" ] = j;

        return js.contains( "attributes" );
    }

    /** \brief Processes attributes if present with the callback
     *
     * see `fromJSON` for details about the callback
     */
    template< typename Callback, typename ...Args >
    inline void processAttributes( const nlohmann::json& js, Callback& cb, Args&& ... args ) {
        static_assert( std::is_invocable_r_v< void, Callback, nlohmann::json, Args ... > );
        if ( js.contains( "attributes" ) )
            cb( js[ "attributes" ], std::forward< Args >( args )... );
    }

    inline nlohmann::json jointToJSON( Joint& j ) {
        nlohmann::json res;
        res[ "positions" ] = nlohmann::json::array();
        for ( auto& p : j.positions() )
            res[ "positions" ].push_back( p );

        atoms::visit( j,
            [ &res ]( RigidJoint& rj ) {
                res[ "type" ] = "rigid";
                res[ "sourceToDestination" ] = matrixToJSON( rj.sourceToDest() );
            },
            [ &res ]( RotationJoint& rj ) {
                res[ "type" ] = "rotational";
                res[ "limits" ][ "min" ] = Angle::rad( rj.jointLimits()[ 0 ].first  ).deg();
                res[ "limits" ][ "max" ] = Angle::rad( rj.jointLimits()[ 0 ].second ).deg();;
                res[ "preMatrix" ]  = matrixToJSON( rj.pre() );
                res[ "postMatrix" ] = matrixToJSON( rj.post() );
                res[ "axis" ] = rj.axis();
            },
            [ &res ]( ModularRotationJoint& mrj ) {
                res[ "type" ] = "modRotational";
                res[ "modulo" ] = Angle::rad( mrj.jointLimits()[ 0 ].second ).deg();;
                res[ "preMatrix" ]  = matrixToJSON( mrj.pre() );
                res[ "postMatrix" ] = matrixToJSON( mrj.post() );
                res[ "axis" ] = mrj.axis();
            }
        );
        return res;
    }

    inline void onUMTranslateDocs( nlohmann::json& j, ModuleType t, int component ) {
        if ( t == ModuleType::Universal && component < 6 )
            j = std::string( UniversalModule::translateComponent( component ) );
        else
            j = component;
    }

    template< typename F >
    inline std::pair< ModuleId, int > connectionComponentFromJSON( const nlohmann::json& j
                                                                 , F&& f = []( ModuleId ) { return ModuleType::Unknown; } ) {
        static_assert( std::is_invocable_r_v< ModuleType, F, ModuleId > );

        ModuleId id = j[ "id" ];
        if ( j.contains( "component" ) && j.contains( "connector" ) ) {
            throw std::runtime_error( "You can specify only one of 'component', 'connector'" );
        }
        const nlohmann::json & connJson = j.contains( "connector" ) ? j[ "connector" ] : j[ "component" ];
        int connId = f( id ) == ModuleType::Universal && connJson.is_string()
                        ? UniversalModule::translateComponent( connJson.get< std::string >() )
                        : connJson.get< int >();

        return std::pair( id, connId );
    }

    inline void connectionToJSON( nlohmann::json& j, const std::string& where
                                , const std::string& c, ModuleId id, ModuleType mType, int cId ) {
        assert( where == "to" || where == "from" );
        assert( c == "component" || c == "connector" );

        j[ where ][ "id" ] = id;
        onUMTranslateDocs( j[ where ][ c ], mType, cId );
    }

    template< typename Callback >
    inline nlohmann::json moduleToJSON( const UniversalModule& m, Callback&& attrCb ) {
        using namespace nlohmann;
        json j;
        j[ "id" ]    = m.getId();
        j[ "type"  ] = "universal";
        j[ "alpha" ] = m.getAlpha().deg();
        j[ "beta"  ] = m.getBeta().deg();
        j[ "gamma" ] = m.getGamma().deg();

        addAttributes( j, attrCb, m );
        return j;
    }

    template< typename Callback >
    struct partial_spec< UniversalModule, Callback > {
        static inline UniversalModule moduleFromJSON( const nlohmann::json& j, Callback& cb ) {
            assert( j[ "type" ] == "universal" );

            ModuleId id = j[ "id" ];
            Angle alpha = Angle::deg( j[ "alpha" ] );
            Angle beta  = Angle::deg( j[ "beta"  ] );
            Angle gamma = Angle::deg( j[ "gamma" ] );

            UniversalModule um( id, alpha, beta, gamma );
            processAttributes( j, cb, um );
            return um;
        }
    };

    template< typename Callback >
    inline nlohmann::json moduleToJSON( const Pad& m, Callback& attrCb ) {
        using namespace nlohmann;
        json j;
        j[ "id" ]     = m.getId();
        j[ "type"   ] = "pad";
        j[ "width"  ] = m.width;
        j[ "height" ] = m.height;

        addAttributes( j, attrCb, m );
        return j;
    }

    template< typename Callback >
    struct partial_spec< Pad, Callback > {
        static inline Pad moduleFromJSON( const nlohmann::json& j, Callback& cb ) {
            assert( j[ "type" ] == "pad" );

            ModuleId id = j[ "id" ];
            int width  = j[ "width" ];
            int height = j[ "height" ];
            Pad m( id, width, height );
            processAttributes( j, cb, m );
            return m;
        }
    };

    template< typename Callback >
    inline nlohmann::json moduleToJSON( const Cube& m, Callback& attrCb ) {
        using namespace nlohmann;
        json j;
        j[ "id" ]     = m.getId();
        j[ "type"   ] = "cube";

        addAttributes( j, attrCb, m );
        return j;
    }

    template< typename Callback >
    struct partial_spec< Cube, Callback > {
        static inline Cube moduleFromJSON( const nlohmann::json& j, Callback& cb ) {
            assert( j[ "type" ] == "cube" );

            ModuleId id = j[ "id" ];
            Cube m( id );
            processAttributes( j, cb, m );
            return m;
        }
    };

    template< typename Callback >
    inline nlohmann::json moduleToJSON( const UnknownModule& m, Callback& attrCb ) {
        using namespace nlohmann;
        json j;

        j[ "id" ]   = m.getId();
        j[ "type" ] = nullptr;
        int i = 0;

        for ( const auto& c : m.components() ) {
            json js;
            if ( c.parent )
                js[ "parent" ] = c.parent->getId();
            else
                js[ "parent" ] = nullptr;
            js[ "type" ] = componentTypeToString( c.type );

            addAttributes( js, attrCb, c, i );
            j[ "components" ].push_back( js );
            i++;
        }

        i = 0;
        for ( auto& jt : m.joints() ) {
            json js;
            js[ "from" ] = jt.sourceComponent;
            js[ "to"   ] = jt.destinationComponent;
            js[ "joint" ] = jointToJSON( *jt.joint );

            addAttributes( js, attrCb, jt, i );
            j[ "joints" ].push_back( js );
            i++;
        }

        addAttributes( j, attrCb, m );
        return j;
    }

    template< typename Callback >
    struct partial_spec< UnknownModule, Callback > {
        static inline UnknownModule moduleFromJSON( const nlohmann::json& j, Callback& cb ) {
            assert( j[ "type" ] && "type is not null" );

            std::vector< Component > components;
            std::vector< ComponentJoint > joints;

            ModuleId id = j[ "id" ];

            int i = 0;
            for ( const auto& c : j[ "components" ] ) {
                ComponentType t = stringToComponentType( c[ "type" ] )
                                          .get_or_throw_as< std::logic_error >();
                // Parent can be nullptr as it will be set in Module's constructor.
                components.push_back( Component( t, {}, {}, nullptr ) );
                processAttributes( j[ "components" ][ i ], cb, components.back(), i );
                i++;
            }

            i = 0;
            for ( const auto& js : j[ "joints" ] ) {
                int source = js[ "from" ];
                int destination = js[ "destination" ];
                Matrix m;
                for ( int ix = 0; ix < 4; ix++ ) {
                    m[ ix ] = js[ "sourceToDestination" ][ ix ];
                }
                if ( js[ "joint"][ "type" ] == "rigid" ) {
                    joints.push_back( makeComponentJoint< RigidJoint >( source, destination, m ) );
                } else if ( js[ "joint" ][ "type" ] == "rotational" ) {
                    Vector axis;
                    for ( int ix = 0; ix < 4; ix++ )
                        axis[ ix ] = js[ "joint" ][ "axis" ][ ix ];

                    joints.push_back( makeComponentJoint< RotationJoint >( source, destination
                                                                        , matrixFromJSON( js[ "joint" ][ "preMatrix" ] )
                                                                        , axis
                                                                        , matrixFromJSON( js[ "joint" ][ "postMatrix" ] )
                                                                        , Angle::deg( js[ "joint" ][ "min" ] )
                                                                        , Angle::deg( js[ "joint" ][ "max" ] ) ) );
                    details::processAttributes( j[ "joints" ][ i ], cb, joints.back(), i );
                    i++;
                } else if ( js[ "joint" ][ "type" ] == "modRotational" ) {
                    Vector axis;
                    for ( int ix = 0; ix < 4; ix++ )
                        axis[ ix ] = js[ "joint" ][ "axis" ][ ix ];

                    joints.push_back( makeComponentJoint< ModularRotationJoint >( source, destination
                                                                        , matrixFromJSON( js[ "joint" ][ "preMatrix" ] )
                                                                        , axis
                                                                        , matrixFromJSON( js[ "joint" ][ "postMatrix" ] )
                                                                        , Angle::deg( js[ "joint" ][ "modulo" ] ) ) );
                    details::processAttributes( j[ "joints" ][ i ], cb, joints.back(), i );
                    i++;
                } else {
                    throw std::logic_error( "Unknown module was given an unknown ComponentJoint" );
                }
            }

            auto m = UnknownModule( components, 0, joints, id, std::nullopt );
            processAttributes( j, cb, m );
            return m;
        }
    };

    template< typename Callback >
    inline nlohmann::json moduleByTypeToJSON( const Module& mod, Callback& attrCb ) {
        switch ( mod.type ) {
            case ModuleType::Unknown:
                return details::moduleToJSON( dynamic_cast< const UnknownModule& >( mod ), attrCb );
            case ModuleType::Universal:
                return details::moduleToJSON( dynamic_cast< const UniversalModule& >( mod ), attrCb );
            case ModuleType::Pad:
                return details::moduleToJSON( dynamic_cast< const Pad& >( mod ), attrCb );
            case ModuleType::Cube:
                return details::moduleToJSON( dynamic_cast< const Cube& >( mod ), attrCb );
        }
        ROFI_UNREACHABLE( "Unknown type of a module" );
    }

    } // namespace details


    /** \brief Serialize given RofiWorld to json.
     *
     * \param attrCb a suitable function or Callable object that returns a `nlohmann::json` which is then
     *               stored into appropriate `"attributes"` field
     *
     * Callback for attributes has to cover all of these possible arguments:
     *
     * - callback for a module gets: const reference to the Module itself
     * - callback for a joint  gets: const reference to the Joint itself and its index within the module
     * - callback for a component gets: const reference to the Component or ComponentJoint and its index
     * - callback for a SpaceJoint or a RoficomJoint gets a const reference to the appropriate joint
     */
    template< typename Callback >
    inline nlohmann::json toJSON( const RofiWorld& world, Callback attrCb ) {
        using namespace nlohmann;
        json res;
        res[ "modules" ] = json::array();
        res[ "moduleJoints" ] = json::array();
        res[ "spaceJoints"  ] = json::array();

        for ( const auto& m : world.modules() ) {
            res[ "modules" ].push_back( details::moduleByTypeToJSON( *m.module, attrCb ) );
        }

        for ( const RoficomJoint& rj : world.roficomConnections() ) {
            json j;
            details::connectionToJSON( j, "from", "connector", world.getModule( rj.sourceModule )->getId()
                                     , world.getModule( rj.sourceModule )->type, rj.sourceConnector );

            details::connectionToJSON( j, "to", "connector", world.getModule( rj.destModule )->getId()
                                     , world.getModule( rj.destModule )->type, rj.destConnector );

            j[ "orientation" ] = orientationToString( rj.orientation );
            details::addAttributes( j, attrCb, rj );
            res[ "moduleJoints" ].push_back( j );
        }

        for ( const SpaceJoint& sj : world.referencePoints() ) {
            json j;
            details::connectionToJSON( j, "to", "component", world.getModule( sj.destModule )->getId()
                                     , world.getModule( sj.destModule )->type, sj.destComponent );

            j[ "point" ] = { sj.refPoint[ 0 ], sj.refPoint[ 1 ], sj.refPoint[ 2 ] };
            assert( sj.joint.get() && "joint is nullptr" );
            j[ "joint" ] = details::jointToJSON( *sj.joint );
            details::addAttributes( j, attrCb, sj );
            res[ "spaceJoints" ].push_back( j );
        }

        return res;
    }

    inline nlohmann::json toJSON( const RofiWorld& world ) {
        return toJSON( world, []( auto&& ... ){ return nlohmann::json{}; } );
    }

    /** \brief Load a RofiWorld from given json.
     *
     * \param j json with a RofiWorld
     * \param attrCb a suitable function or Callable object that process appropriate `"attributes"` fields
     *
     * Callback for attributes has to cover all of these possible arguments:
     *
     * - callback for a Module gets: const reference for the json[ "attributes" ]
     *                               and const reference to the Module itself
     * - callback for a Joint  gets: const reference for the json[ "attributes" ]
     *                               and const reference to the Joint itself
     *                               and its index within the module
     * - callback for a Component and ComponentJoint gets: const reference for the json[ "attributes" ]
     *                                                     and a const reference to the Component itself
     *                                                     and its index within the module
     * - callback for a SpaceJoint or a RoficomJoint gets: const reference for the json[ "attributes" ]
     *                                                     and a handle for given connection
     */
    template< typename Callback >
    inline RofiWorld fromJSON( const nlohmann::json& j, Callback attrCb ) {
        RofiWorld world;

        if ( !j.contains( "modules" ) ) {
            throw std::logic_error( "Cannot find modules in rofi world json" );
        }

        for ( const auto& jm : j[ "modules" ] ) {
            if ( jm[ "type" ] == "unknown" )
                world.insert( details::moduleFromJSON< UnknownModule >( jm, attrCb ) );
            else if ( jm[ "type" ] == "universal" )
                world.insert( details::moduleFromJSON< UniversalModule >( jm, attrCb ) );
            else if ( jm[ "type" ] == "pad" )
                world.insert( details::moduleFromJSON< Pad >( jm, attrCb ) );
            else if ( jm[ "type" ] == "cube" )
                world.insert( details::moduleFromJSON< Cube >( jm, attrCb ) );
            else
                throw std::logic_error( "Unknown type of a module" );
        }

        // function for translation of components to docs if necessary
        auto f = [ &world ]( ModuleId id ) -> ModuleType { return world.getModule( id )->type; };

        if ( j.contains( "moduleJoints" ) ) {
            for ( const auto& jj : j[ "moduleJoints" ] ) {
                roficom::Orientation o = roficom::stringToOrientation( jj[ "orientation" ] )
                                                .get_or_throw_as< std::runtime_error >();

                auto [ sourceModule, sourceConnector ] = details::connectionComponentFromJSON( jj[ "from" ], f );
                auto [ destinationModule, destinationConnector ] = details::connectionComponentFromJSON( jj[ "to" ], f );

                auto conn = connect( world.getModule( sourceModule )->connectors()[ sourceConnector ]
                                , world.getModule( destinationModule )->connectors()[ destinationConnector ]
                                , o );

                details::processAttributes( jj, attrCb, conn );
            }
        }

        if ( j.contains( "spaceJoints" ) ) {
            for ( const auto& sj : j[ "spaceJoints" ] ) {
                auto [ destinationModule, destinationComponent ] = details::connectionComponentFromJSON( sj[ "to" ], f );

                Vector fixedPoint = { sj[ "point" ][ 0 ], sj[ "point" ][ 1 ], sj[ "point" ][ 2 ] };
                if ( sj[ "joint" ][ "type" ] == "rigid" ) {
                    auto conn = connect< RigidJoint >( world.getModule( destinationModule )->components()[ destinationComponent ]
                                                    , fixedPoint
                                                    , details::matrixFromJSON( sj[ "joint" ][ "sourceToDestination" ] ) );
                    details::processAttributes( sj, attrCb, conn );
                } else if ( sj[ "joint" ][ "type" ] == "rotational" ) {
                    auto& jj = sj[ "joint" ];
                    std::vector< float > positions = jj[ "positions" ];

                    auto conn = connect< RotationJoint >( world.getModule( destinationModule )->components()[ destinationComponent ]
                                                        , fixedPoint
                                                        , details::matrixFromJSON( jj[ "preMatrix" ] )
                                                        , Vector{ jj[ "axis" ][ 0 ]
                                                        , jj[ "axis" ][ 1 ]
                                                        , jj[ "axis" ][ 2 ] }
                                                        , details::matrixFromJSON( jj[ "postMatrix" ] )
                                                        , Angle::deg( jj[ "limits" ][ "min" ] )
                                                        , Angle::deg( jj[ "limits" ][ "max" ] ) );
                    world.setSpaceJointPositions( conn, positions );
                    details::processAttributes( sj, attrCb, conn );
                } else {
                    throw std::logic_error( "Unknown joint type" );
                }
            }
        }

        return world;
    }

    inline RofiWorld fromJSON( const nlohmann::json& j ) {
        return fromJSON( j, []( auto&& ... ) { return; } );
    }

} // namespace rofi::configuration
