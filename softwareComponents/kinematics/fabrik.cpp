#include "fabrik.hpp"
#include <optional>

void tentacleMonster::findTentacles(){
    std::vector< int > ids = config.getIDs();
    std::sort( ids.begin(), ids.end(), std::greater< int >() ); // quietly assume lower ids are closer to master
    std::unordered_set< int > exclude;

    for( int id : ids ){
        auto edges = config.getEdges( id, exclude );
        ID current = id;
        if( edges.size() == 1 ){
            tentacles.emplace_back( tentacle{ } );
            std::optional< Edge > lastEdge;
            while( ( edges = config.getEdges( current, exclude ) ).size() == 1 ){
                auto edge = edges.front();
                addJoints( current, edge, lastEdge );
                lastEdge = edge;
                exclude.insert( current );
                current = edges.front().id1() == current ?
                    edges.front().id2() : edges.front().id1();
            }
            joint j1, j2;
            j1.id = j2.id = current;
            j1.body = tentacles.back().front().prevEdge.value().side1();
            j2.body = j1.body == A ? B : A;
            j1.nextEdge = tentacles.back().front().prevEdge;
            tentacles.back().push_front( j1 );
            tentacles.back().push_front( j2 );
        }

        if( edges.empty() ){
            auto noExclude = config.getEdges( current );
            if( noExclude.size() == 0 ){
                tentacles.emplace_back( tentacle{ } );
                joint j1, j2;
                j1.id = j2.id = current;
                j1.body = A;
                j2.body = B;
                tentacles.back().push_back( j1 );
                tentacles.back().push_back( j2 );
            }

        }
    }

    config.computeMatrices();
    auto matrices = config.getMatrices();

    for( auto& t : tentacles ){
        for( auto& j : t ){
            j.trans = matrices[ j.id ][ j.body ];
        }
    }
}

Vector localPosition( tentacle& arm, int baseIndex, int posIndex ){
    return arm[ baseIndex ].getLocal( arm[ posIndex ].position() );
}

/* Use the symmetry of the modules to find the minimal amount of movement
   e.g. 90 polar 180 azimuth is the same as -90 polar 0 azimuth */
std::pair< double, double > simplify( double pol, double az ){
    while( !equals( pol, 0.0 ) &&
           ( az > M_PI || equals( az, M_PI ) ) ){
        pol *= -1;
        az -= M_PI;
    }
    while( !equals( pol, 0.0 ) &&
           ( az < -M_PI || equals( az, -M_PI ) ) ){
        pol *= -1;
        az += M_PI;
    }
    return { pol, az };
}

/* Adjust the module along the same joint ( rotate around X-axis ) */
void sameModule( tentacle& arm, int index, int direction = 1 /* forward */ ){
    Vector pos = project( arm[ index + direction ].trans * X,
                          arm[ index + direction ].position(),
                          arm[ index ].position() );

    pos = arm[ index + direction ].getLocal( pos );
    //std::cout << "pos:\n" << pos << '\n';

    auto [ p, _ ] = simplify( polar( pos ),
                              azimuth( pos ) );

     // if( direction == -1 )
     //    p = std::clamp( p, -M_PI_2, M_PI_2 );

    if( direction == 1 && index == arm.size() - 2 ){
        if( p < 0.0 )
            p = std::clamp( p, p, -M_PI_2 );
        if( p >= 0.0 )
            p = std::clamp( p, M_PI_2, p );
    } else {
        p = std::clamp( p, -M_PI_2, M_PI_2 );
    }

    //if( direction == -1 )
    arm[ index + direction ].xRot = p;

    arm[ index ].trans =
        arm[ index + direction ].trans *
        rotate( p, X ) *
        translate( Z );
}

/* Match joint rotation with target */
void rotateTo( joint& j, Matrix target ){
    double b = azimuth( inverse( j.trans ) * target * -Y );
    j.trans *= rotate( b, Z );

    auto [ p, a ] = simplify( polar( inverse( j.trans ) * target * Z ),
                              azimuth( inverse( j.trans ) * target * Z ) );

    p = std::clamp( p, -M_PI_2, M_PI_2 );

    j.trans *=
        rotate( a, Z ) *
        rotate( p, X );

    double c = azimuth( inverse( j.trans ) * target * -Y );

    j.xRot = p;
    j.zRot = a + b + c;

    j.trans *= rotate( c, Z );

}

double oriToAngle( unsigned int ori ){
    switch( ori ){
        case South:
            return 0.0;
        case East:
            return M_PI_2;
        case North:
            return M_PI;
        default:
            return M_PI + M_PI_2;
    }
}

/* Cover -Z to -Z connections */
void connected( tentacle& arm, int index, int direction = 1 ){
    Matrix transformation = identity;
    Matrix expectedNext = identity;
    bool flip = false;
    int count = 1;
    double angle = 0.0;

    // TODO: remove duplication
    if( direction == 1 ){
        while( arm[ index - ( direction * ( count - 1 ) ) ].prevEdge ){
            //TODO: deal with static joints
            ++count;
        }
        for( int i = 0; i < count; ++i ){
            Edge edge = arm[ index - i ].nextEdge.value();
            if( edge.dock2() == ZMinus ){
                transformation *= translate( Vector{ 0.0, 0.0, 1.0, 0.0 } );
            } else if( edge.dock2() == XPlus ){
                transformation *= translate( Vector{ -1.0, 0.0, 0.0, 0.0 } );
                transformation *= rotate( M_PI, Z ) * rotate( M_PI_2, Y );
                flip = !flip;
            } else {
                transformation *= translate( Vector{ 1.0, 0.0, 0.0, 0.0 } );
                transformation *= rotate( M_PI_2, Y );
                flip = !flip;
            }

            if( edge.dock1() == XPlus ){
                transformation *= rotate( M_PI, Z ) * rotate( M_PI_2, Y );
            } else if( edge.dock1() == XMinus ){
                transformation *= rotate( M_PI_2, Y );
            }//  else if( edge.ori() == East || edge.ori() == West ){
            //     flip = !flip;
            // }

            transformation *= rotate( oriToAngle( edge.ori() ), Z );
            //angle += oriToAngle( edge.ori() );
        }
    } else {
        while( arm[ index - ( direction * ( count - 1 ) ) ].nextEdge ){
            //TODO: deal with static joints
            ++count;
        }
        for( int i = 0; i < count; ++i ){
            Edge edge = arm[ index + i ].prevEdge.value();
            if( edge.dock1() == ZMinus ){
                transformation *= translate( Vector{ 0.0, 0.0, 1.0, 0.0 } );
            } else if( edge.dock1() == XPlus ){
                transformation *= translate( Vector{ 1.0, 0.0, 0.0, 0.0 } );
                transformation *= rotate( M_PI, Z ) * rotate( M_PI_2, Y );
                flip = !flip;
            } else {
                transformation *= translate( Vector{ -1.0, 0.0, 0.0, 0.0 } );
                transformation *= rotate( M_PI_2, Y );
                flip = !flip;
            }

            if( edge.dock2() == XPlus ){
                transformation *= rotate( M_PI, Z ) * rotate( M_PI_2, Y );
            } else if( edge.dock2() == XMinus ){
                transformation *= rotate( M_PI_2, Y );
            } else if( edge.ori() == East || edge.ori() == West ){
                flip = !flip;
            }

            transformation *= rotate( oriToAngle( edge.ori() ), Z );
            angle += oriToAngle( edge.ori() );
        }
    }
    expectedNext = transformation * translate( Vector{ 0.0, 0.0, 1.0, 0.0 } );

    Vector position = transformation * Vector{ 0.0, 0.0, 0.0, 1.0 };
    double pChange = polar( position );
    double aChange = azimuth( position );

    Vector expPos = expectedNext * Vector{ 0.0, 0.0, 0.0, 1.0 };
    double pExp = polar( expectedNext * Vector{ 0.0, 0.0, 0.0, 1.0 } );
    double aExp = azimuth( expectedNext * Vector{ 0.0, 0.0, 0.0, 1.0 } );


    Vector next = localPosition( arm, index + direction, index - direction );
    Vector current = localPosition( arm, index + direction, index );

    double pol = 0.0;
    double az = 0.0;

    if( !flip ){
        pol = polar( current ) - pChange;
        az = azimuth( next ) - aExp;
    } else {
        pol = polar( next ) - pExp;
        az = azimuth( current ) - aChange;
    }

    auto [ p, a ] = simplify( pol, az );

    p = std::clamp( p, -M_PI_2, M_PI_2 );

    arm[ index + direction ].xRot = p;
    arm[ index + direction ].zRot = a;

    arm[ index ].trans =
        arm[ index + direction ].trans *
        rotate( a, Z ) *
        rotate( p, X ) *
        transformation;

}

bool tentacleMonster::fabrik( tentacle& arm, Matrix target )
{
    Matrix base = arm.front().trans;
    int iterations = 0;
    Vector pos;
    double p;
    double a;

    while( !equals( arm.back().trans, target ) ){

        /* Forward reaching */
        arm.back().trans = target;

        for( int i = arm.size() - 2; i >= 0; --i ){
            if( arm[ i ].nextEdge ){
                if( arm[ i ].nextEdge->dock1() == ZMinus ){
                    connected( arm, i );
                } else {
                    assert( false );
                }
            } else {
                sameModule( arm, i );
            }
           if( debug ){
               printArm( 0 );
               getchar();
           }
        }

        /* Backward reaching */
        arm.front().trans = base;

        for( size_t i = 1; i <= arm.size() - 1; ++i ){
            if( arm[ i ].prevEdge ){
                if( arm[ i ].prevEdge->dock1() == ZMinus ){
                    connected( arm, i, -1 );
                } else {
                    assert( false );
                }
            } else {
                sameModule( arm, i, -1 );
            }

           if( debug ){
               printArm( 0 );
               getchar();
           }
        }

        rotateTo( arm.back(), target );

        if( ++iterations == 2000 ){
            setArm( arm, target );
            std::cout << "wanted:\n" << IO::toString( target )
                << "got:\n";
            printArm( 0 );
            return false;
        }
    }

    setArm( arm, target );
    return true;
}

bool tentacleMonster::setJoint( ID id, Joint j, double rad ){
    double current = config.getModule( id ).getJoint( j );
    config.execute( Action( Action::Rotate( id, j, -current ) ) );
    double deg = to_deg( rad );
    if( j != Gamma )
        deg = std::clamp( deg, -90.0, 90.0 );
    return config.execute( Action( Action::Rotate( id, j, deg ) ) );
}

void tentacleMonster::setArm( tentacle& arm, Matrix target ){
    int direction = 1;
    for( auto& j : arm ){
        Joint current = j.body == A ? Alpha : Beta;
        //if( j.prevEdge && j.prevEdge.value().ori() == North )
            //direction *= -1;
        setJoint( j.id, current, j.xRot * direction );
        setJoint( j.id, Gamma, j.zRot * direction );
    }

    config.computeMatrices();
}

bool tentacleMonster::reach( int arm, Vector position, double xRot,
                             double yRot, double zRot)
{
    Matrix target =
        translate( position ) *
        rotate( zRot, Z ) *
        rotate( yRot, Y ) *
        rotate( xRot, X ) *
        identity;

    return fabrik( tentacles[ arm ], target );
}

void tentacleMonster::printArm( int arm ){
    for( auto& j : tentacles[ arm ] ){
        std::cout << "\nID: " << j.id << " Body: " << j.body << "\n" << IO::toString( j.trans );
    }
}


void reposition( tentacle& arm, int index, Vector previous, double length = 1.0 ){
   // Vector previous = forward ? arm[ index + 1 ].position : arm[ index - 1 ].position;
    double lambda = length / distance( arm[ index ].position(), previous );
    Vector pos = ( length - lambda ) * previous + lambda * arm[ index ].position();
    arm[ index ].setPosition( pos );
};


void tentacleMonster::addJoints( ID id, Edge edge, std::optional< Edge > lastEdge ){
    tentacle& a = tentacles.back();
    joint j1, j2;
    j1.id = j2.id = id;
    if( !lastEdge ){
        if( id == edge.id1() ){
            edge = reverse( edge );
        }
        j1.body = edge.side2() == A ? B : A;
        j2.body = j1.body == A ? B : A;

        j2.prevEdge = edge;

        a.push_front( j1 );
        a.push_front( j2 );
    } else {
        Edge last = lastEdge.value();
        j1.body = last.id1() == id ? last.side1() : last.side2();
        j2.body = edge.id1() == id ? edge.side1() : edge.side2();
        j1.nextEdge = lastEdge;
        if( j1.body == j2.body ){
            j1.prevEdge = edge;
            a.push_front( j1 );
        } else {
            j2.prevEdge = edge;
            a.push_front( j1 );
            a.push_front( j2 );
        }
    }
}
