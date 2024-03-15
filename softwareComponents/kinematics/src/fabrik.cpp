#include "manipulator.hpp"


using namespace rofi::configuration;
using namespace rofi::configuration::matrices;
using namespace rofi::geometry;
using namespace rofi::kinematics;

Angle rad_to_angle( double value ){
    return Angle::rad( static_cast< Angle::underlying_type >( value ) );
}

double normalize_angle( double rads ){
    if( rads < -M_PI - eps ){
        rads += 2 * M_PI;
    }
    if( rads > M_PI + eps ){
        rads -= 2 * M_PI;
    }
    return rads;
}


std::vector< Sphere > Manipulator::colliding_spheres( size_t joint_index, size_t body_index, const Vector& axis ) const {
    assert( arm.size() > body_index );

    Plane rotational( center( joint_position( body_index ) ), axis, 0.5 );

    auto spheres = collision_tree->colliding_leaves( rotational );
    
    auto colliding = std::vector< Sphere >();
    
    for( const auto& sphere : spheres ){
        Vector local = inverse( joint_position( joint_index ) ) * sphere.center;
        double radial_diff = std::fabs( radial( local ) - distance( center( joint_position( joint_index ) ), center( joint_position( body_index ) ) ) );
        //auto [ p, a ] = simplify( polar( local ), azimuth( local ) );
        if( radial_diff < 1 ){
            colliding.push_back( sphere );
        }
    }

    return colliding;
}

std::pair< Angle, Angle > Manipulator::find_angles( size_t joint_index, const Manipulator& other ) const {
    assert( joint_index + 1 < arm.size() );

    Matrix local = inverse( joint_position( joint_index ) );
    Vector target = center( local * other.joint_position( other.arm.size() - 2 - joint_index ) );

    Vector next = other.arm.size() - joint_index < 3 ? target :
        center( local * other.joint_position( other.arm.size() - 3 - joint_index ) );

    if( debug )
        std::cout << "next:\n" << next << '\n';

    if( arm[ joint_index ].id == arm[ joint_index + 1 ].id ){
        target = project( X, Vector{ 0, 0, 0, 1 }, target );
        auto [ p, a ] = simplify( -polar( target ), azimuth( target ) );

        return { rad_to_angle( p ), rad_to_angle( a ) };
    } else {
        auto* connection = get_connection( joint_index, joint_index + 1 );
        target = rotate( M_PI, Y ) * target;
        next = rotate( M_PI, Y ) * next;
        // Z-Z
        if( connection->sourceConnector % 3 == 2 && connection->destConnector % 3 == 2 ){
            if( connection->orientation == roficom::Orientation::North || connection->orientation == roficom::Orientation::South ){
                auto [ p, a ] = simplify( -polar( target ), azimuth( next ) );
                return { rad_to_angle( p ), rad_to_angle( a ) };
            } else {
                auto [ p, a ] = simplify( -polar( next ), azimuth( target ) );
                return { rad_to_angle( p ), rad_to_angle( a ) };
            }
        }
        // Z-X
        else if( connection->sourceConnector % 3 == 2 ){
            auto p = -polar(  target );
            auto a = azimuth( target );

            auto next_p = -polar( next - target );
            auto next_a = azimuth( next );

            Matrix expected = rotate( a, Z ) * rotate( -p, X ) * translate( Z ) * 
                rotate( roficom::orientationToAngle( connection->orientation ).rad(), -Z ) *
                translate( X );
            //std::cout << "expected:\n" << IO::toString( expected ) << "actual: " << next;
            auto exp_a = azimuth( center( expected ) );
            auto exp_p = -polar( center( expected ) - target );

            //std::cout << "p: " << p << " a: " << a << " next_p: " << next_p << " next_a: " << next_a << " exp: " << exp_a << ", " << exp_p << "\n";
            if( std::fabs( exp_a - next_a ) > M_PI_2 || std::fabs( exp_p - next_p ) > M_PI_2 
            ){
                a += M_PI;
                if( a > 2 * M_PI ){
                    a -= 2 * M_PI;
                }
                p *= -1;
            }
 
            if( connection->destConnector % 3 == 1 ){
                a += M_PI;
                p *= -1;
            }
            a = normalize_angle( a );
            
            return { rad_to_angle( p ), rad_to_angle( a ) };
        }
        // XZ
        else if( connection->destConnector % 3 == 2 )
        {
            auto multiplier = connection->sourceConnector % 3 == 0 ? -1.0f : 1.0f;

            auto a = azimuth( target ) + M_PI_2 * multiplier;
            a = normalize_angle( a );

            next += multiplier * ( rotate( a, Z ) * X );
            next = rotate( a, -Z ) * next;

            auto [ p, _ ] = simplify( polar( next ), azimuth( next ) );
            p -= M_PI_2;
            p -= roficom::orientationToAngle( connection->orientation ).rad() * multiplier;
            p = normalize_angle( p );

            if( p > M_PI_2 + eps ){
                p -= M_PI;
            }
            if( p < -M_PI_2 - eps ){
                p += M_PI;
            }

            return { rad_to_angle( -p ), rad_to_angle( a ) };
        }
        // XX
        else {
            auto multiplier = connection->sourceConnector % 3 == 0 ? -1.0f : 1.0f;
            auto a = azimuth( target ) + M_PI_2 * multiplier;
            //std::cout << "p: " << p << " a: " << a << '\n';

            return { rad_to_angle( 0.0 ), rad_to_angle( a ) };
        }
    }
}

std::pair< Angle, Angle > Manipulator::find_end_angles( const Manipulator& other ) const {

    Matrix local_rotation = inverse( joint_position( arm.size() - 1 ) ) * other.joint_position( 0 );
    auto target_angles = matrix_to_angles( local_rotation );
    auto gamma = -rad_to_angle( target_angles[ 2 ] );
    
    return { rad_to_angle( std::clamp( target_angles[ 0 ], -M_PI_2, M_PI_2 ) ), gamma };
}

std::pair< Angle, Angle > Manipulator::find_limit( size_t joint_index, const Vector& axis, bool x ) const {
    /*
    1. Find obstacles intersecting the rotational plane
    2. Check polar and radial distance of each obstacle
    3. Return new bounds
    */
    
    //Plane rotational( body.leaf->shape.center, axis, body.leaf ? body.leaf->shape.radius : 0.0 );
    
    // std::cout << joint_index << "___________________\n";
    auto limits = x ? std::make_pair( -90_deg, 90_deg ) : std::make_pair( -360_deg, 360_deg );
    
    Matrix inv = inverse( joint_position( joint_index ) );
    size_t body_index = joint_index + 1;
    // for( size_t body_index = joint_index + 1; body_index < arm.size(); body_index++ ){
        auto colliding = colliding_spheres( joint_index, body_index, axis );
        for( const auto& sphere : colliding ){
            Plane rotational( center( joint_position( body_index ) ), axis, 0.5 );

            // std::cout << "sphere: " << sphere.center << "plane: " << rotational.origin << " ax:\n" << axis << '\n';
            Vector local = inv * project( axis, center( joint_position( joint_index ) ), sphere.center );
            local = arm[ joint_index ].id == arm[ joint_index + 1 ].id ? local : rotate( M_PI, Y ) * local;

            Vector local_body = inv * project( axis, center( joint_position( joint_index ) ), center( joint_position( body_index ) ) );
            local_body = arm[ joint_index ].id == arm[ joint_index + 1 ].id ? local_body : rotate( M_PI, Y ) * local_body;
            // Get polar angle of obstacle
            auto [ p, az ] = simplify( -polar( local ), azimuth( local ) );

            // ABC is the triangle between the rotational plane, closest collision-free point, and sphere center
            double a = std::sqrt( 1.0 - std::pow( dist( rotational, sphere ), 2 ) );
            double b = radial( local_body );
            double c = radial( local );
            double diff = std::acos( ( b * b + c * c - a * a ) / ( 2 * b * c ) );

            // subtract angle of point A to obtain nearest collision-free point
            //std::cout << "diff: " << diff << " a: " << a << " b: " << b  << " c: " << c << " p: " << p << '\n';
            
            //std::cout << "axis:\n" << axis;
            //pos = project( axis, Vector{ 0.0, 0.0, 0.0, 1.0 }, pos );
            auto [ current, idk ] = simplify( -polar( local_body ), 
                                             azimuth( local_body ) );
            //std::cout << "current: " << current << " j: " << joint_rotation( joint_index ).rad() << '\n';

            if( x ){
                double angle_diff = current - joint_rotation( joint_index ).rad();
                if( p < current && p + diff - angle_diff > limits.first.rad() ){
                    limits.first = rad_to_angle( p + diff - angle_diff );
                    //std::cout << "new lower: " << limits.first.deg() << '\n';
                } else if( p > current && p - diff - angle_diff < limits.second.rad() ){
                    limits.second = rad_to_angle( p - diff - angle_diff );
                    //std::cout << "new higher: " << limits.second.deg() << '\n';
                }
            } else {
                if( az - M_PI * 2 + diff * 2 > limits.first.rad() ){
                    limits.first = rad_to_angle( az + diff );
                    // std::cout << "new lower: " << limits.first.deg() << '\n';
                } else if( az - diff * 2 < limits.second.rad() ){
                    limits.second = rad_to_angle( az - diff );
                    // std::cout << "new higher: " << limits.second.deg() << '\n';
                }
            }
            

        }
    // }
    return limits;
}

void Manipulator::set_joint( size_t joint_index, Angle polar, Angle azimuth ) {
    auto* mod = static_cast< UniversalModule* >( world.getModule( arm[ joint_index ].id ) );
    if( arm[ joint_index ].side == A ){
        mod->setAlpha( polar );
    } else {
        mod->setBeta( polar );
    }
    mod->setGamma( azimuth + mod->getGamma() );
    
    auto res = world.prepare();
    assert( res );
}

void Manipulator::reaching( const Manipulator& other, bool check_collisions ) {
    assert( arm.size() != 0 );

    if( check_collisions ){
        for( size_t index = 1; index < arm.size(); ++index ){
            if( arm[ index ].leaf ){
                collision_tree->erase( arm[ index ].leaf );
                arm[ index ].leaf = nullptr;
            }
        }
    }

    for( size_t i = 0; i < arm.size() - 1; i++ ){
        auto [ polar, azimuth ] = find_angles( i, other );

        bool check = true;
        if( i == arm.size() - 2 && distance( center( joint_position( arm.size() - 1 ) ), center( other.joint_position( 0 ) ) ) < 0.1 ){
            check = false;
        }
        if( check_collisions && check ){
            AABB_Leaf< Sphere >* end = nullptr;
            if( i != arm.size() - 2 ){
                end = collision_tree->insert( Sphere( center( other.joint_position( 0 ) ) ) );
            }
            Vector z = arm[ i ].id == arm[ i + 1 ].id ? Z : -Z;

            auto [ zl, zh ] = find_limit( i, joint_position( i ) * Z, false );
            azimuth = rad_to_angle( std::clamp( azimuth.rad(), zl.rad(), zh.rad() ) );
            
            auto [ xl, xh ] = find_limit( i, joint_position( i ) * rotate( azimuth.rad(), z ) * X, true );
            polar = rad_to_angle( std::clamp( polar.rad(), xl.rad(), xh.rad() ) );

            if( end ){
                collision_tree->erase( end );
            }
        }
        polar = rad_to_angle( std::clamp( polar.rad(), -M_PI_2f, M_PI_2f ) );
        set_joint( i, polar, azimuth );

        if( check_collisions ){
            arm[ i + 1 ].leaf = collision_tree->insert( Sphere( center( joint_position( i + 1 ) ) ) );
        }
    }
    auto [ polar, azimuth ] = find_end_angles( other );
    set_joint( arm.size() - 1, polar, azimuth );
}

Manipulator Manipulator::copy( const Matrix& target ) const {
    RofiWorld other_world;
    for( const auto& j : joints ){
        const auto& src = j.getSourceModule( world );
        const auto& dest = j.getDestModule( world );
        Module* new_src = other_world.getModule( src.getId() );
        Module* new_dest = other_world.getModule( dest.getId() );
        if( !new_src ){
            new_src = &other_world.insert( src );
        }
        if( !new_dest ){
            new_dest = &other_world.insert( dest );
        }
        connect( new_dest->components()[ j.destConnector ], new_src->components()[ j.sourceConnector ], j.orientation );
    }
    if( joints.empty() ){
        for( const auto& module : world.modules() ){
            other_world.insert( module );
        }
    }
    std::string conn = arm.back().side == A ? "A-X" : "B-X";
    connect< RigidJoint >( static_cast< UniversalModule* >( other_world.getModule( arm.back().id ) )->getConnector( conn ), Vector{ 0, 0, 0 }, target );
    auto res = other_world.prepare();
    assert( res );
    auto reversed = arm;
    for( auto& j : reversed ){
        j.leaf = nullptr;
    }
    std::reverse( reversed.begin(), reversed.end() );
    Manipulator copy( other_world, reversed, nullptr, false );

    return copy;
}

bool Manipulator::fabrik( Matrix target, double prec_position, double prec_rotation, bool check_collisions ){
    /* 
    1. Copy manipulator
    2. Root copy in target
    3. Reach forward and backward
    4. Finish if stuck or hits limit
    */

    Manipulator other = copy( target );
    size_t iterations = 0;
    
    if( debug ) other.debug = true;
    std::string conn = arm.back().side == A ? "A-X" : "B-X";
    auto& um = static_cast< UniversalModule* >( world.getModule( arm.back().id ) )->getConnector( conn );
    while( !eq( um.getPosition(), target, prec_rotation ) || !eq( center( um.getPosition() ), center( target ), prec_position )
    ){
        other.reaching( *this );
        reaching( other, check_collisions );
        if( iterations++ == 40 ){
            //std::cout << "other:" << serialization::toJSON( other.world ) << '\n';
            // std::cout << "target:\n" << IO::toString(target) << "\nreached:\n" << IO::toString( um.getPosition() );
            // std::cout << serialization::toJSON( world ) << '\n';
            // getchar();
            return false;
        }
    }
    return true;
}