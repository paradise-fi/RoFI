#pragma once

#include <atoms/unreachable.hpp>
#include <configuration/rofibot.hpp>
#include <configuration/universalModule.hpp>
#include <configuration/pad.hpp>
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
        }
        ROFI_UNREACHABLE( "Unknown component type" );
    }

    inline ComponentType stringToComponentType( const std::string& str ) {
        if ( str == "roficom" )
            return ComponentType::Roficom;
        else if ( str == "UM body" )
            return ComponentType::UmBody;
        else if ( str == "UM shoe" )
            return ComponentType::UmShoe;
        else
            ROFI_UNREACHABLE( "String does not represent a component type" );
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
            }
        );
        return res;
    }

    inline void onUMTranslateDocs( nlohmann::json& j, ModuleType t, int component ) {
        if ( t == ModuleType::Universal && component < 6 )
            j = std::move( UniversalModule::translateComponent( component ) );
        else
            j = component;
    }

    template< bool isConnector = false, typename F >
    inline std::pair< ModuleId, int > connectionFromJSON( const nlohmann::json& j, const std::string& where
                                                        , F&& f = []( ModuleId ) { return ModuleType::Unknown; } ) {
        static_assert( std::is_invocable_r_v< ModuleType, F, ModuleId > );
        assert( where == "to" || where == "from" );

        ModuleId id = j[ where ][ "id" ];
        std::string conn( isConnector ? "connector" : "component" );
        int connId = f( id ) == ModuleType::Universal && isConnector && j[ where ][ conn ].is_string()
                        ? UniversalModule::translateComponent( j[ where ][ conn ].get< std::string >() )
                        : j[ where ][ conn ].get< int >();

        return std::pair( id, connId );
    }

    template< typename F >
    inline std::pair< ModuleId, int > connectorFromJSON( const nlohmann::json& j, const std::string& where, F&& f ) {
        return connectionFromJSON< true >( j, where, std::forward< F >( f ) );
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
                ComponentType t = stringToComponentType( c[ "type" ] );
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
                } else {
                    ROFI_UNREACHABLE( "Unknown module was given an unknown ComponentJoint" );
                }
            }

            auto m = UnknownModule( components, 0, joints, id, std::nullopt );
            processAttributes( j, cb, m );
            return m;
        }
    };

    } // namespace details


    /** \brief Serialize given Rofibot to json.
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
    inline nlohmann::json toJSON( const Rofibot& bot, Callback attrCb ) {
        using namespace nlohmann;
        json res;
        res[ "modules" ] = json::array();
        res[ "moduleJoints" ] = json::array();
        res[ "spaceJoints"  ] = json::array();

        for ( const auto& m : bot.modules() ) {
            json j;
            switch ( m.module->type ) {
                case ModuleType::Universal:
                    j = details::moduleToJSON( dynamic_cast< const UniversalModule& >( *m.module ), attrCb );
                    break;
                case ModuleType::Pad:
                    j = details::moduleToJSON( dynamic_cast< const Pad& >( *m.module ), attrCb );
                    break;
                case ModuleType::Unknown:
                    j = details::moduleToJSON( dynamic_cast< const UnknownModule& >( *m.module ), attrCb );
                    break;
                default:
                    ROFI_UNREACHABLE( "Unknown type of a module" );
            }

            res[ "modules" ].push_back( j );
        }

        for ( const RoficomJoint& rj : bot.roficomConnections() ) {
            json j;
            details::connectionToJSON( j, "from", "connector", bot.getModule( rj.sourceModule )->getId()
                                     , bot.getModule( rj.sourceModule )->type, rj.sourceConnector );

            details::connectionToJSON( j, "to", "connector", bot.getModule( rj.destModule )->getId()
                                     , bot.getModule( rj.destModule )->type, rj.destConnector );

            j[ "orientation" ] = orientationToString( rj.orientation );
            details::addAttributes( j, attrCb, rj );
            res[ "moduleJoints" ].push_back( j );
        }

        for ( const SpaceJoint& sj : bot.referencePoints() ) {
            json j;
            details::connectionToJSON( j, "to", "component", bot.getModule( sj.destModule )->getId()
                                     , bot.getModule( sj.destModule )->type, sj.destComponent );

            j[ "point" ] = { sj.refPoint[ 0 ], sj.refPoint[ 1 ], sj.refPoint[ 2 ] };
            assert( sj.joint.get() && "joint is nullptr" );
            j[ "joint" ] = details::jointToJSON( *sj.joint );
            details::addAttributes( j, attrCb, sj );
            res[ "spaceJoints" ].push_back( j );
        }

        return res;
    }

    inline nlohmann::json toJSON( const Rofibot& bot ) {
        return toJSON( bot, []( auto&& ... ){ return nlohmann::json{}; } );
    }

    /** \brief Load a Rofibot from given json.
     *
     * \param j json with a Rofibot
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
    inline Rofibot fromJSON( const nlohmann::json& j, Callback attrCb ) {
        Rofibot bot;

        for ( size_t i = 0; i < j[ "modules" ].size(); i++ ) {
            auto& jm = j[ "modules" ][ i ];

            if ( jm[ "type" ] == "universal" )
                bot.insert( details::moduleFromJSON< UniversalModule >( jm, attrCb ) );
            else if ( jm[ "type" ] == "pad" )
                bot.insert( details::moduleFromJSON< Pad >( jm, attrCb ) );
            else if ( jm[ "type" ] == "unknown" )
                bot.insert( details::moduleFromJSON< UnknownModule >( jm, attrCb ) );
            else
                ROFI_UNREACHABLE( "Unknown type of a module" );
        }

        // function for translation of components to docs if necessary
        auto f = [ &bot ]( ModuleId id ) -> ModuleType { return bot.getModule( id )->type; };

        for ( const auto& jj : j[ "moduleJoints" ] ) {
            roficom::Orientation o = roficom::stringToOrientation( jj[ "orientation" ] );

            auto [ sourceModule, sourceConnector ] = details::connectorFromJSON( jj, "from", f );
            auto [ destinationModule, destinationConnector ] = details::connectorFromJSON( jj, "to", f );

            auto conn = connect( bot.getModule( sourceModule )->connectors()[ sourceConnector ]
                               , bot.getModule( destinationModule )->connectors()[ destinationConnector ]
                               , o );

            details::processAttributes( jj, attrCb, conn );
        }

        for ( const auto& sj : j[ "spaceJoints" ] ) {
            auto [ destinationModule, destinationComponent ] = details::connectionFromJSON( sj, "to", f );

            Vector fixedPoint = { sj[ "point" ][ 0 ], sj[ "point" ][ 1 ], sj[ "point" ][ 2 ] };
            if ( sj[ "joint" ][ "type" ] == "rigid" ) {
                auto conn = connect< RigidJoint >( bot.getModule( destinationModule )->components()[ destinationComponent ]
                                                 , fixedPoint
                                                 , details::matrixFromJSON( sj[ "joint" ][ "sourceToDestination" ] ) );
                details::processAttributes( sj, attrCb, conn );
            } else if ( sj[ "joint" ][ "type" ] == "rotational" ) {
                auto& jj = sj[ "joint" ];
                std::vector< float > positions = jj[ "positions" ];

                auto conn = connect< RotationJoint >( bot.getModule( destinationModule )->components()[ destinationComponent ]
                                                    , fixedPoint
                                                    , details::matrixFromJSON( jj[ "preMatrix" ] )
                                                    , Vector{ jj[ "axis" ][ 0 ]
                                                    , jj[ "axis" ][ 1 ]
                                                    , jj[ "axis" ][ 2 ] }
                                                    , details::matrixFromJSON( jj[ "postMatrix" ] )
                                                    , Angle::deg( jj[ "limits" ][ "min" ] )
                                                    , Angle::deg( jj[ "limits" ][ "max" ] ) );
                bot.setSpaceJointPositions( conn, positions );
                details::processAttributes( sj, attrCb, conn );
            } else {
                ROFI_UNREACHABLE( "Unknown joint type" );
            }
        }

        return bot;
    }

    inline Rofibot fromJSON( const nlohmann::json& j ) {
        return fromJSON( j, []( auto&& ... ) { return; } );
    }

} // namespace rofi::configuration
