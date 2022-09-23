#include <concepts>
#include <optional>
#include <vector>

#include <nlohmann/json.hpp>

#include <magic_enum.hpp>

#include "configuration/rofibot.hpp"


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

enum class Axis : int {
    X = 0,
    Y,
    Z
};
inline auto toAxis( std::underlying_type_t< Axis > i ) -> Axis
{
    auto axis = magic_enum::enum_cast< Axis >( i );
    return axis ? *axis : throw std::logic_error( "Axis index is out-of-bounds" );
}
inline auto nextAxis( Axis axis ) -> Axis
{
    constexpr auto axes_values = magic_enum::enum_values< Axis >();
    static_assert( axes_values.size() > 0 );

    auto axis_index = magic_enum::enum_index( axis );
    assert( axis_index );
    return axes_values[ ( *axis_index + 1 ) % axes_values.size() ];
}

struct Direction {
    Axis axis = {};
    bool is_positive = {};
};
inline auto getDirection( const rofi::configuration::matrices::Vector & direction )
        -> std::optional< Direction >
{
    auto dirOpt = toIntegerArray( direction );
    if ( !dirOpt ) {
        return std::nullopt;
    }
    auto dir = *dirOpt;

    auto result = std::optional< Direction >();
    for ( unsigned i = 0; i < 3; i++ ) {
        switch ( dir[ i ] ) {
            case 0:
                break;
            case -1:
            case 1:
                if ( result ) {
                    return std::nullopt;
                }
                result = Direction{ .axis = toAxis( to_signed( i ) ), .is_positive = dir[ i ] > 0 };
                break;
            default:
                return std::nullopt;
        }
    }
    return result;
}

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

using Position = std::array< int, 3 >;

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
    static auto getVoxelPos( const rofi::configuration::matrices::Matrix & bodyPos ) -> Position
    {
        auto center = rofi::configuration::matrices::center( bodyPos );
        if ( auto voxelPos = toIntegerArray( center ) ) {
            return *voxelPos;
        }
        throw std::runtime_error( "Position is not on the grid" );
    }
    static auto getOtherBodyDir( const rofi::configuration::matrices::Matrix & bodyPos )
            -> Direction
    {
        auto dir = getDirection( bodyPos * rofi::configuration::matrices::Z );
        if ( !dir ) {
            throw std::runtime_error( "Module is not rotated orthogonaly" );
        }
        return *dir;
    }
    static auto getIsShoeRotated( const rofi::configuration::matrices::Matrix & bodyPos,
                                  Direction otherBodyDir ) -> bool
    {
        auto xDirOpt = getDirection( bodyPos * rofi::configuration::matrices::X );
        if ( !xDirOpt ) {
            throw std::runtime_error( "Module is not rotated orthogonaly" );
        }
        auto xDir = *xDirOpt;
        assert( otherBodyDir.axis != xDir.axis );

        return nextAxis( otherBodyDir.axis ) != xDir.axis;
    }
    static auto getJointPosition( const rofi::configuration::matrices::Matrix & bodyPos,
                                  Angle shoeJointAngle ) -> JointPosition
    {
        auto yDirOpt = getDirection( bodyPos * rofi::configuration::matrices::Y );
        if ( !yDirOpt ) {
            throw std::runtime_error( "Module is not rotated orthogonaly" );
        }
        auto yDir = *yDirOpt;

        auto originalJointPosOpt = originalJointPosFromAngle( shoeJointAngle );
        if ( !originalJointPosOpt ) {
            throw std::runtime_error( "Joint position is not a right angle" );
        }
        auto originalJointPos = *originalJointPosOpt;

        return yDir.is_positive ? originalJointPos : jointPositionInverse( originalJointPos );
    }

    static auto fromBodyComponent( const rofi::configuration::Component & body, Angle shoeAngle )
            -> Voxel
    {
        assert( body.type == rofi::configuration::ComponentType::UmBody );
        auto bodyPos = body.getPosition();
        auto otherBodyDir = getOtherBodyDir( bodyPos );

        return Voxel{ .pos = getVoxelPos( bodyPos ),
                      .other_body_dir = otherBodyDir,
                      .is_shoe_rotated = getIsShoeRotated( bodyPos, otherBodyDir ),
                      .joint_pos = getJointPosition( bodyPos, shoeAngle ) };
    }
    static auto fromRofiModule( const rofi::configuration::UniversalModule & rofiModule )
            -> std::array< Voxel, 2 >
    {
        assert( rofiModule.type == rofi::configuration::ModuleType::Universal );
        return std::array{ fromBodyComponent( rofiModule.getBodyA(), rofiModule.getAlpha() ),
                           fromBodyComponent( rofiModule.getBodyB(), rofiModule.getBeta() ) };
    }

    Position pos = {};
    Direction other_body_dir = {};
    bool is_shoe_rotated = {};
    JointPosition joint_pos = {};
};

struct VoxelWorld {
    static auto fromRofiWorld( const rofi::configuration::Rofibot & rofiWorld ) -> VoxelWorld
    {
        if ( !rofiWorld.isPrepared() ) {
            throw std::runtime_error( "Rofibot has to be prepared to convert to voxel json" );
        }

        auto subtractPos = []( Position lhs, const Position & rhs ) -> Position {
            for ( size_t i = 0; i < lhs.size(); i++ ) {
                lhs[ i ] -= rhs[ i ];
            }
            return lhs;
        };

        auto minPos = Position();
        auto maxPos = Position();
        auto updateLimitPos = [ &minPos, &maxPos ]( const Position & pos ) {
            for ( size_t i = 0; i < pos.size(); i++ ) {
                minPos[ i ] = std::min( minPos[ i ], pos[ i ] );
                maxPos[ i ] = std::max( maxPos[ i ], pos[ i ] );
            }
        };

        auto modules = std::vector< Voxel >();
        for ( const auto & rofiModule : rofiWorld.modules() ) {
            auto universalModule = dynamic_cast< const rofi::configuration::UniversalModule * >(
                    rofiModule.module.get() );
            if ( !universalModule ) {
                throw std::runtime_error( "Module is not Universal module" );
            }
            for ( auto voxel : Voxel::fromRofiModule( *universalModule ) ) {
                updateLimitPos( voxel.pos );
                modules.push_back( std::move( voxel ) );
            }
        }
        for ( auto & voxel : modules ) {
            voxel.pos = subtractPos( std::move( voxel.pos ), minPos );
        }

        return VoxelWorld{ .modules = std::move( modules ),
                           .sizes = subtractPos( maxPos, minPos ) };
    }

    std::vector< Voxel > modules = {};
    Position sizes = {};
};


// Make Axis (de)serializable to nlohmann::json
void to_json( nlohmann::json & j, Axis axis )
{
    j = magic_enum::enum_name( axis );
}
void from_json( const nlohmann::json & j, Axis & axis )
{
    auto value = magic_enum::enum_cast< Axis >( j.get< std::string >() );
    axis = value ? *value : throw std::runtime_error( "Could not read Axis" );
}

// Make JointPosition (de)serializable to nlohmann::json
void to_json( nlohmann::json & j, const JointPosition & joint_pos )
{
    switch ( joint_pos ) {
        case JointPosition::Zero:
            j = 0;
            return;
        case JointPosition::Minus90:
            j = -90;
            return;
        case JointPosition::Plus90:
            j = 90;
            return;
    }
    throw std::runtime_error( "Could not read JointPosition" );
}
void from_json( const nlohmann::json & j, JointPosition & joint_pos )
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
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE( VoxelWorld, modules, sizes )

} // namespace rofi::voxel
