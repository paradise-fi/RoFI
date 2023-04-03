#pragma once

#include <concepts>
#include <map>
#include <optional>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include <magic_enum.hpp>

#include "atoms/result.hpp"
#include "atoms/units.hpp"
#include "atoms/unreachable.hpp"
#include "configuration/rofiworld.hpp"
#include "configuration/universalModule.hpp"


namespace rofi::voxel
{

constexpr double precision = 1e-3;

inline auto toInteger( double value ) -> std::optional< int >
{
    auto result = int( std::round( value ) );
    if ( std::abs( value - result ) <= precision ) {
        return { result };
    }
    return std::nullopt;
}
inline auto toIntegerArray( const rofi::configuration::matrices::Vector & values )
        -> std::optional< std::array< int, 3 > >
{
    assert( values.size() >= 3 );
    auto result = std::array< int, 3 >();
    for ( size_t i = 0; i < result.size(); i++ ) {
        if ( auto value = toInteger( values[ i ] ) ) {
            result[ i ] = *value;
        } else {
            return std::nullopt;
        }
    }
    return result;
}
inline auto toMatrixVector( const std::array< int, 3 > & position, double w = 0. )
        -> rofi::configuration::matrices::Vector
{
    auto [ x, y, z ] = position;
    return { double( x ), double( y ), double( z ), w };
}

using Position = std::array< int, 3 >;

enum class Axis : int {
    X = 0,
    Y,
    Z
};
inline auto toAxis( std::underlying_type_t< Axis > i ) -> Axis
{
    auto axis = magic_enum::enum_cast< Axis >( i );
    assert( axis );
    return axis ? *axis : throw std::logic_error( "Axis index is out-of-bounds" );
}
inline auto fromAxis( Axis axis ) -> std::underlying_type_t< Axis >
{
    return static_cast< std::underlying_type_t< Axis > >( axis );
}
inline auto nextAxis( Axis axis ) -> Axis
{
    constexpr auto axes_values = magic_enum::enum_values< Axis >();
    static_assert( axes_values.size() > 0 );

    auto axis_index = magic_enum::enum_index( axis );
    assert( axis_index );
    return axes_values[ ( *axis_index + 1 ) % axes_values.size() ];
}
inline auto prevAxis( Axis axis ) -> Axis
{
    constexpr auto axes_values = magic_enum::enum_values< Axis >();
    static_assert( axes_values.size() > 0 );

    auto axis_index = magic_enum::enum_index( axis );
    assert( axis_index );
    return axes_values[ ( *axis_index + axes_values.size() - 1 ) % axes_values.size() ];
}

struct Direction {
    static auto fromVector( const std::array< int, 3 > & direction ) -> std::optional< Direction >
    {
        auto result = std::optional< Direction >();
        for ( unsigned i = 0; i < 3; i++ ) {
            switch ( direction[ i ] ) {
                case 0:
                    break;
                case -1:
                case 1:
                    if ( result ) {
                        return std::nullopt;
                    }
                    result = Direction{ .axis = toAxis( to_signed( i ) ),
                                        .is_positive = direction[ i ] > 0 };
                    break;
                default:
                    return std::nullopt;
            }
        }
        return result;
    }
    static auto fromVector( const rofi::configuration::matrices::Vector & direction )
            -> std::optional< Direction >
    {
        if ( auto dir = toIntegerArray( direction ) ) {
            return fromVector( *dir );
        }
        return std::nullopt;
    }

    auto movePosition( Position position ) const -> Position
    {
        position[ fromAxis( axis ) ] += is_positive ? 1 : -1;
        return position;
    }

    auto opposite() const -> Direction
    {
        return Direction{ .axis = axis, .is_positive = !is_positive };
    }

    auto operator<=>( const Direction & ) const = default;

    Axis axis = {};
    bool is_positive = {};
};

enum class JointPosition {
    Zero,
    Plus90,
    Minus90,
};
inline auto originalJointPosFromAngle( Angle angle ) -> std::optional< JointPosition >
{
    if ( std::abs( angle.rad() ) <= precision ) {
        return JointPosition::Zero;
    }
    if ( std::abs( ( 90_deg - angle ).rad() ) <= precision ) {
        return JointPosition::Plus90;
    }
    if ( std::abs( ( -90_deg - angle ).rad() ) <= precision ) {
        return JointPosition::Minus90;
    }
    return std::nullopt;
}
inline auto jointPositionToInteger( JointPosition joint_pos ) -> int8_t
{
    switch ( joint_pos ) {
        case JointPosition::Zero:
            return 0;
        case JointPosition::Minus90:
            return -90;
        case JointPosition::Plus90:
            return 90;
    }
    ROFI_UNREACHABLE( "JointPosition is out of bounds" );
}
inline auto jointPositionInverse( JointPosition value ) -> JointPosition
{
    switch ( value ) {
        case JointPosition::Plus90:
            return JointPosition::Minus90;
        case JointPosition::Minus90:
            return JointPosition::Plus90;
        case JointPosition::Zero:
            return JointPosition::Zero;
    }
    ROFI_UNREACHABLE( "JointPosition is out of bounds" );
}

/// This documentation is copy from Rust `voxel::body::VoxelBody`
///
/// Canonized representation:
/// - when `other_body_dir.is_dir_to_plus` is true, the VoxelBody is assumed a BodyA
///     otherwise it's assumed BodyB
/// - when `is_shoe_rotated` is `false`, X-connectors are in the next axis
///         - `other_body_dir.axis` is `Axis::X` => X-connectors are in Y axis
///         - `other_body_dir.axis` is `Axis::Y` => X-connectors are in Z axis
///         - `other_body_dir.axis` is `Axis::Z` => X-connectors are in X axis
///     otherwise X-connectors are in the previous axis
///         - `other_body_dir.axis` is `Axis::X` => X-connectors are in Z axis
///         - `other_body_dir.axis` is `Axis::Y` => X-connectors are in X axis
///         - `other_body_dir.axis` is `Axis::Z` => X-connectors are in Y axis
/// - when `joint_pos` is `Zero`, the shoe joint is in the zero position
///     when `joint_pos` is `Plus90` the Z-connector is towards the plus axis
///     otherwise the Z-connector is towards the minus axis
struct Voxel {
    /**
     * Rofi Module -> Voxel
     */

private:
    static auto getVoxelPos( const rofi::configuration::matrices::Matrix & bodyPos )
            -> atoms::Result< Position >
    {
        auto center = rofi::configuration::matrices::center( bodyPos );
        if ( auto voxelPos = toIntegerArray( center ) ) {
            return atoms::result_value( *voxelPos );
        }
        return atoms::result_error< std::string >( "Position is not on the grid" );
    }
    static auto getOtherBodyDir( const rofi::configuration::matrices::Matrix & bodyPos )
            -> atoms::Result< Direction >
    {
        auto dir = Direction::fromVector( bodyPos * rofi::configuration::matrices::Z );
        if ( !dir ) {
            return atoms::result_error< std::string >( "Module is not rotated orthogonaly" );
        }
        return atoms::result_value( *dir );
    }
    static auto getIsShoeRotated( const rofi::configuration::matrices::Matrix & bodyPos,
                                  Direction otherBodyDir ) -> atoms::Result< bool >
    {
        auto xDirOpt = Direction::fromVector( bodyPos * rofi::configuration::matrices::X );
        if ( !xDirOpt ) {
            return atoms::result_error< std::string >( "Module is not rotated orthogonaly" );
        }
        auto xDir = *xDirOpt;
        assert( otherBodyDir.axis != xDir.axis );

        return atoms::result_value( nextAxis( otherBodyDir.axis ) != xDir.axis );
    }
    static auto getJointPosition( const rofi::configuration::matrices::Matrix & bodyPos,
                                  Angle shoeJointAngle ) -> atoms::Result< JointPosition >
    {
        auto yDirOpt = Direction::fromVector( bodyPos * rofi::configuration::matrices::Y );
        if ( !yDirOpt ) {
            return atoms::result_error< std::string >( "Module is not rotated orthogonaly" );
        }
        auto yDir = *yDirOpt;

        auto originalJointPosOpt = originalJointPosFromAngle( shoeJointAngle );
        if ( !originalJointPosOpt ) {
            return atoms::result_error< std::string >( "Joint position is not a right angle" );
        }
        auto originalJointPos = *originalJointPosOpt;

        return atoms::result_value( yDir.is_positive ? originalJointPos
                                                     : jointPositionInverse( originalJointPos ) );
    }

    static auto fromBodyComponent( const rofi::configuration::Component & body, Angle shoeAngle )
            -> atoms::Result< Voxel >
    {
        assert( body.type == rofi::configuration::ComponentType::UmBody );
        auto bodyPos = body.getPosition();

        auto pos = getVoxelPos( bodyPos );
        if ( !pos ) {
            return std::move( pos ).assume_error_result();
        }
        auto otherBodyDir = getOtherBodyDir( bodyPos );
        if ( !otherBodyDir ) {
            return std::move( otherBodyDir ).assume_error_result();
        }
        auto isShoeRotated = getIsShoeRotated( bodyPos, *otherBodyDir );
        if ( !isShoeRotated ) {
            return std::move( isShoeRotated ).assume_error_result();
        }
        auto jointPos = getJointPosition( bodyPos, shoeAngle );
        if ( !jointPos ) {
            return std::move( jointPos ).assume_error_result();
        }

        return atoms::result_value( Voxel{ .pos = *pos,
                                           .other_body_dir = *otherBodyDir,
                                           .is_shoe_rotated = *isShoeRotated,
                                           .joint_pos = *jointPos } );
    }

public:
    static auto fromRofiModule( const rofi::configuration::UniversalModule & rofiModule )
            -> atoms::Result< std::array< Voxel, 2 > >
    {
        assert( rofiModule.type == rofi::configuration::ModuleType::Universal );
        auto shoeA = fromBodyComponent( rofiModule.getBodyA(), rofiModule.getAlpha() );
        if ( !shoeA ) {
            return std::move( shoeA ).assume_error_result();
        }
        auto shoeB = fromBodyComponent( rofiModule.getBodyB(), rofiModule.getBeta() );
        if ( !shoeA ) {
            return std::move( shoeB ).assume_error_result();
        }
        return atoms::result_value( std::array{ *shoeA, *shoeB } );
    }


    /**
     * Voxel -> Rofi Module
     */

public:
    auto getOtherBodyPos() const -> Position
    {
        return other_body_dir.movePosition( pos );
    }

    auto zConnDirection() const -> Direction
    {
        auto zConnAxis = is_shoe_rotated ? nextAxis( other_body_dir.axis )
                                         : prevAxis( other_body_dir.axis );
        switch ( joint_pos ) {
            case JointPosition::Zero:
                return other_body_dir.opposite();
            case JointPosition::Plus90:
                return Direction{ .axis = zConnAxis, .is_positive = true };
            case JointPosition::Minus90:
                return Direction{ .axis = zConnAxis, .is_positive = false };
        }
        ROFI_UNREACHABLE( "Joint Position is out of bounds" );
    }
    auto xPlusConnDirection() const -> Direction
    {
        if ( is_shoe_rotated ) {
            return Direction{ .axis = prevAxis( other_body_dir.axis ),
                              .is_positive = !other_body_dir.is_positive };
        } else {
            return Direction{ .axis = nextAxis( other_body_dir.axis ),
                              .is_positive = other_body_dir.is_positive };
        }
    }

    auto getXPlusConnMatrixRotation() const -> rofi::configuration::matrices::Matrix
    {
        namespace matrices = rofi::configuration::matrices;

        auto getYPlusDir = []( Direction xPlusDir, Direction zPlusDir ) {
            auto defaultIsPositive = xPlusDir.is_positive == zPlusDir.is_positive;
            switch ( zPlusDir.axis ) {
                case Axis::X:
                    switch ( xPlusDir.axis ) {
                        case Axis::Y:
                            return Direction{ .axis = Axis::Z, .is_positive = defaultIsPositive };
                        case Axis::Z:
                            return Direction{ .axis = Axis::Y, .is_positive = !defaultIsPositive };
                        case Axis::X:
                            ROFI_UNREACHABLE(
                                    "Z+ and X- connectors cannot have the same dir axis" );
                    }
                    ROFI_UNREACHABLE( "Invalid axis" );
                case Axis::Y:
                    switch ( xPlusDir.axis ) {
                        case Axis::Z:
                            return Direction{ .axis = Axis::X, .is_positive = defaultIsPositive };
                        case Axis::X:
                            return Direction{ .axis = Axis::Z, .is_positive = !defaultIsPositive };
                        case Axis::Y:
                            ROFI_UNREACHABLE(
                                    "Z+ and X- connectors cannot have the same dir axis" );
                    }
                    ROFI_UNREACHABLE( "Invalid axis" );
                case Axis::Z:
                    switch ( xPlusDir.axis ) {
                        case Axis::X:
                            return Direction{ .axis = Axis::Y, .is_positive = defaultIsPositive };
                        case Axis::Y:
                            return Direction{ .axis = Axis::X, .is_positive = !defaultIsPositive };
                        case Axis::Z:
                            ROFI_UNREACHABLE(
                                    "Z+ and X- connectors cannot have the same dir axis" );
                    }
                    ROFI_UNREACHABLE( "Invalid axis" );
            }
            ROFI_UNREACHABLE( "Invalid axis" );
        };

        auto xPlusDir = xPlusConnDirection();
        auto zPlusDir = zConnDirection().opposite();
        auto yPlusDir = getYPlusDir( xPlusDir, zPlusDir );

        auto plusDirs = std::array{ xPlusDir, yPlusDir, zPlusDir };

        auto result = matrices::identity;
        for ( size_t i = 0; i < 3; i++ ) {
            result.col( i ) = toMatrixVector( plusDirs[ i ].movePosition( {} ), 0. );
        }
        return result;
    }

    struct Connector {
        std::string_view name;
        Position pos;
        Direction dir;
        Direction oriVec;
    };

    static auto getConnectors( Voxel shoeA, Voxel shoeB ) -> std::array< Connector, 6 >
    {
        using namespace std::string_view_literals;

        auto xPlusConnDirShoeA = shoeA.xPlusConnDirection();
        auto xPlusConnDirShoeB = shoeB.xPlusConnDirection();

        auto zConnDirShoeA = shoeA.zConnDirection();
        auto zConnDirShoeB = shoeB.zConnDirection();

        return std::array{
                Connector{ .name = "A-X"sv,
                           .pos = shoeA.pos,
                           .dir = xPlusConnDirShoeA.opposite(),
                           .oriVec = zConnDirShoeA.opposite() },
                Connector{ .name = "A+X"sv,
                           .pos = shoeA.pos,
                           .dir = xPlusConnDirShoeA,
                           .oriVec = zConnDirShoeA },
                Connector{ .name = "A-Z"sv,
                           .pos = shoeA.pos,
                           .dir = zConnDirShoeA,
                           .oriVec = xPlusConnDirShoeA },
                Connector{ .name = "B-X"sv,
                           .pos = shoeB.pos,
                           .dir = xPlusConnDirShoeB.opposite(),
                           .oriVec = zConnDirShoeB.opposite() },
                Connector{ .name = "B+X"sv,
                           .pos = shoeB.pos,
                           .dir = xPlusConnDirShoeB,
                           .oriVec = zConnDirShoeB },
                Connector{ .name = "B-Z"sv,
                           .pos = shoeB.pos,
                           .dir = zConnDirShoeB,
                           .oriVec = xPlusConnDirShoeB },
        };
    }

    static auto getConnOrientation( Direction connDirection,
                                    Direction connOrientationVector,
                                    Direction otherOrientationVector )
            -> rofi::configuration::roficom::Orientation
    {
        if ( connOrientationVector.axis == otherOrientationVector.axis ) {
            if ( connOrientationVector.is_positive == otherOrientationVector.is_positive ) {
                return rofi::configuration::roficom::Orientation::North;
            } else {
                return rofi::configuration::roficom::Orientation::South;
            }
        } else {
            if ( connDirection.is_positive
                 ^ ( nextAxis( connDirection.axis ) == connOrientationVector.axis )
                 ^ ( connOrientationVector.is_positive == otherOrientationVector.is_positive ) )
            {
                return rofi::configuration::roficom::Orientation::West;
            } else {
                return rofi::configuration::roficom::Orientation::East;
            }
        }
    }

    static auto toRofiModule( Voxel shoeA, Voxel shoeB, rofi::configuration::ModuleId moduleId )
            -> rofi::configuration::UniversalModule
    {
        auto getGamma = [ & ] {
            if ( shoeA.is_shoe_rotated == shoeB.is_shoe_rotated ) {
                return 0_deg;
            }
            if ( shoeA.is_shoe_rotated ) {
                assert( !shoeB.is_shoe_rotated );
                return 90_deg;
            }
            assert( !shoeA.is_shoe_rotated );
            assert( shoeB.is_shoe_rotated );
            return -90_deg;
        };

        assert( shoeA.other_body_dir.movePosition( shoeA.pos ) == shoeB.pos );
        assert( shoeB.other_body_dir.movePosition( shoeB.pos ) == shoeA.pos );

        auto alpha = Angle::deg( jointPositionToInteger( shoeA.joint_pos ) );
        auto beta = Angle::deg( jointPositionToInteger( shoeB.joint_pos ) );
        auto gamma = getGamma();

        return rofi::configuration::UniversalModule( moduleId, alpha, beta, gamma );
    }

public:
    bool operator==( const Voxel & ) const = default;

    Position pos = {};
    Direction other_body_dir = {};
    bool is_shoe_rotated = {};
    JointPosition joint_pos = {};
};

struct VoxelWorld {
private:
    // Mapping (connectorPos, connectorDir) -> (component, orientationVector)
    using ConnectorMap = std::map< std::pair< Position, Direction >,
                                   std::pair< rofi::configuration::Component, Direction > >;

public:
    /**
     * \brief Converts `rofi::configuration::RofiWorld` to `VoxelWorld`.
     * Requires that:
     *  - all the modules are `rofi::configuration::UniversalModule`,
     *  - all joint values are multiple of 90 degrees,
     *  - all positions have to be whole integers.
     *
     * \note Does not check if the resulting `VoxelWorld` is connected.
    **/
    static auto fromRofiWorld( const rofi::configuration::RofiWorld & rofiWorld )
            -> atoms::Result< VoxelWorld >
    {
        if ( !rofiWorld.isPrepared() ) {
            return atoms::result_error< std::string >(
                    "RofiWorld has to be prepared to convert to voxel json" );
        }

        auto voxelBodies = std::vector< Voxel >();
        for ( const auto & rModule : rofiWorld.modules() ) {
            auto * universalModule = dynamic_cast< const rofi::configuration::UniversalModule * >(
                    &rModule );
            if ( !universalModule ) {
                return atoms::result_error< std::string >( "Module is not Universal module" );
            }
            auto voxelModule = Voxel::fromRofiModule( *universalModule );
            if ( !voxelModule ) {
                return std::move( voxelModule ).assume_error_result();
            }
            for ( auto voxel : *voxelModule ) {
                voxelBodies.push_back( std::move( voxel ) );
            }
        }

        return atoms::result_value( VoxelWorld{ .bodies = std::move( voxelBodies ) } );
    }

private:
    static auto isModuleRepr( Voxel voxelBody ) -> bool
    {
        return voxelBody.other_body_dir.is_positive;
    }

    auto posToBodiesMap() const -> atoms::Result< std::map< Position, Voxel > >
    {
        auto bodiesMap = std::map< Position, Voxel >();
        for ( const auto & voxelBody : bodies ) {
            auto inserted = bodiesMap.emplace( voxelBody.pos, voxelBody );
            if ( !inserted.second ) {
                return atoms::result_error< std::string >( "Multiple bodies at the same position" );
            }
        }
        return atoms::result_value( std::move( bodiesMap ) );
    }

    static void connectModules( ConnectorMap & connectorMap )
    {
        for ( const auto & [ connPosDir, connWithOri ] : connectorMap ) {
            const auto & [ connPos, connDir ] = connPosDir;
            if ( !connDir.is_positive ) {
                // Select each connection only once
                continue;
            }

            auto otherConnDirPos = std::pair{ connDir.movePosition( connPos ), connDir.opposite() };
            auto otherConnWithOri = connectorMap.find( otherConnDirPos );
            if ( otherConnWithOri == connectorMap.end() ) {
                continue;
            }

            const auto & [ conn, connOri ] = connWithOri;
            const auto & [ otherConn, otherConnOri ] = otherConnWithOri->second;

            assert( connDir.axis != connOri.axis );
            assert( connDir.axis != otherConnOri.axis );

            auto orientation = Voxel::getConnOrientation( connDir, connOri, otherConnOri );
            rofi::configuration::connect( conn, otherConn, orientation );
        }
    }

    auto fixInSpace( Voxel bodyToFixate, ConnectorMap & connectorMap ) const
            -> atoms::Result< std::monostate >
    {
        auto connIt = connectorMap.find(
                std::pair{ bodyToFixate.pos, bodyToFixate.xPlusConnDirection().opposite() } );
        if ( connIt == connectorMap.end() ) {
            return atoms::result_error< std::string >( "Couldn't find body's X- connector" );
        }
        auto conn = connIt->second;
        auto refPoint = toMatrixVector( bodyToFixate.pos );
        auto rotation = bodyToFixate.getXPlusConnMatrixRotation();

        rofi::configuration::connect< rofi::configuration::RigidJoint >( conn.first,
                                                                         refPoint,
                                                                         rotation );
        return atoms::result_value( std::monostate{} );
    }

public:
    /**
     * \brief Converts `VoxelWorld` to `rofi::configuration::RofiWorld`.
     * `VoxelWorld` should be connected and the resulting RofiWorld
     * will be fixed on one component to the world at the according position.
     *
     * If you want to convert `VoxelWorld` that isn't connected,
     * you can set \a fixateModulesByOne.
     *
     * \param fixateModulesByOne specifies whether to connect each module
     *      to the world separately by a `rofi::configuration::RigidJoint`
     *      instead of connecting modules together
     *      by `rofi::configuration::RoficomJoint`s
    **/
    auto toRofiWorld( bool fixateModulesByOne = false ) const
            -> atoms::Result< rofi::configuration::RofiWorld >
    {
        if ( bodies.empty() ) {
            return atoms::result_error< std::string >( "VoxelWorld cannot be empty" );
        }

        auto bodiesMapResult = posToBodiesMap();
        if ( !bodiesMapResult ) {
            return std::move( bodiesMapResult ).assume_error_result();
        }
        const auto bodiesMap = std::move( *bodiesMapResult );
        auto moduleId = rofi::configuration::ModuleId( 1 );
        auto connectorMap = ConnectorMap();

        auto rofiWorld = rofi::configuration::RofiWorld();
        for ( const auto & voxelBody : bodies ) {
            if ( !isModuleRepr( voxelBody ) ) {
                continue;
            }

            if ( !bodiesMap.contains( voxelBody.getOtherBodyPos() ) ) {
                return atoms::result_error< std::string >( "Invalid world" );
            }
            auto shoeA = voxelBody;
            auto shoeB = bodiesMap.at( voxelBody.getOtherBodyPos() );

            auto & mod = rofiWorld.insert( Voxel::toRofiModule( shoeA, shoeB, moduleId++ ) );

            for ( auto conn : Voxel::getConnectors( shoeA, shoeB ) ) {
                auto inserted = connectorMap.emplace( std::pair{ conn.pos, conn.dir },
                                                      std::pair{ mod.getConnector( conn.name ),
                                                                 conn.oriVec } );
                if ( !inserted.second ) {
                    return atoms::result_error< std::string >(
                            "Multiple connectors at the same position and direction" );
                }
            }
        }

        if ( fixateModulesByOne ) {
            for ( const auto & body : bodies ) {
                auto fixInSpaceResult = fixInSpace( body, connectorMap );
                if ( !fixInSpaceResult ) {
                    return std::move( fixInSpaceResult ).assume_error_result();
                }
            }
        } else {
            connectModules( connectorMap );

            assert( !bodies.empty() );
            auto fixInSpaceResult = fixInSpace( bodies.front(), connectorMap );
            if ( !fixInSpaceResult ) {
                return std::move( fixInSpaceResult ).assume_error_result();
            }
        }

        return atoms::result_value( std::move( rofiWorld ) );
    }

    bool operator==( const VoxelWorld & ) const = default;


public:
    std::vector< Voxel > bodies = {};
};


// Make Axis (de)serializable to nlohmann::json
inline void to_json( nlohmann::json & j, Axis axis )
{
    j = magic_enum::enum_name( axis );
}
inline void from_json( const nlohmann::json & j, Axis & axis )
{
    auto value = magic_enum::enum_cast< Axis >( j.get< std::string >() );
    axis = value ? *value : throw std::runtime_error( "Could not read Axis" );
}

// Make JointPosition (de)serializable to nlohmann::json
inline void to_json( nlohmann::json & j, const JointPosition & joint_pos )
{
    j = jointPositionToInteger( joint_pos );
}
inline void from_json( const nlohmann::json & j, JointPosition & joint_pos )
{
    auto value = j.get< int >();
    switch ( value ) {
        case 0:
            joint_pos = JointPosition::Zero;
            return;
        case -90:
            joint_pos = JointPosition::Minus90;
            return;
        case 90:
            joint_pos = JointPosition::Plus90;
            return;
    }
    throw std::runtime_error( "Could not read JointPosition" );
}

// Make Direction (de)serializable to nlohmann::json
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE( Direction, axis, is_positive )
// Make Voxel (de)serializable to nlohmann::json
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE( Voxel, pos, other_body_dir, is_shoe_rotated, joint_pos )
// Make VoxelWorld (de)serializable to nlohmann::json
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE( VoxelWorld, bodies )

} // namespace rofi::voxel
