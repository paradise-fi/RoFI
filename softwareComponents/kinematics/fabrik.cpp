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

void tentacleMonster::addJoints( ID id, Edge& edge, std::optional< Edge > lastEdge ){
    tentacle& a = tentacles.back();
    joint j1, j2;
    j1.id = j2.id = id;
    if( id == edge.id1() ){
        edge = reverse( edge );
    }
    if( !lastEdge ){
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


joint closestMass( const Configuration& init ){
    Vector mass = init.massCenter();
    ID bestID = 0;
    ShoeId bestShoe = A;
    double bestDist = std::numeric_limits<double>::max();
    for (const auto& [id, ms] : init.getMatrices()) {
        for (unsigned i = 0; i < 2; ++i) {
            const auto& matrix = ms[i];
            double currDist = sqDistVM(matrix, mass);
            if (currDist < bestDist) {
                bestDist = currDist;
                bestID = id;
                bestShoe = i ? B : A;
            }
        }
    }

    return { bestID, bestShoe };
}

void tentacleMonster::initialize(){
    config.computeMatrices();
    joint center = closestMass( config );
    leader = center.id;

    std::unordered_set< ID > seen;
    std::stack< joint > stack;

    stack.push( center );

    while( !stack.empty() ){
        // TODO: initialize and treefy rofibot
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
void sameModule( tentacle& arm, int& index, int direction = 1 /* forward */ ){
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

    if( direction == -1 )
        arm[ index + direction ].xRot = p;

    arm[ index ].trans =
        arm[ index + direction ].trans *
        rotate( p, X ) *
        translate( Z );

    index -= direction;
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

    j.xRot = p;
    j.zRot = a + b;
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

void computeTransformation( tentacle& arm, int& index, int direction = 1 ){
    Matrix transformation = identity;
    bool forward = direction == 1;
    int count = 0;
    bool flip = false;
    bool xz = false;
    bool zx = false;
    if( direction == 1 ){
        while( arm[ index + 1 - count ].prevEdge ){
            //TODO: deal with static joints
            ++count;
        }
        for( int i = 0; i < count; ++i ){
            Edge edge = arm[ index + 1 - i ].prevEdge.value();
            if( edge.dock2() == ZMinus ){
                transformation *= translate( Z );
            } else if( edge.dock2() == XPlus ){
                transformation *= rotate( M_PI, Z ) *
                     rotate( M_PI_2, Y );
                transformation *= translate( Z );
                //flip = !flip;
                zx = true;
            } else {
                transformation *= rotate( M_PI, Z ) *
                    rotate( -M_PI_2, X );
                transformation *= translate( Z );
                //flip = !flip;
                zx = true;
            }

            transformation *= rotate( oriToAngle( edge.ori() ), Z );

            if( edge.dock1() == XPlus ){
                transformation *= //rotate( M_PI, Z ) *
                    rotate( -M_PI_2, Y );
                xz = true;
            } else if( edge.dock1() == XMinus ){
                transformation *= //rotate( M_PI, Z ) *
                    rotate( M_PI_2, Y );
                xz = true;
            } // else if( edge.ori() == East || edge.ori() == West ){
            //     flip = !flip;
            // }
            //angle += oriToAngle( edge.ori() );
        }
    } else {
        while( arm[ index - 1 + count ].nextEdge ){
            //TODO: deal with static joints
            ++count;
        }
        for( int i = 0; i < count; ++i ){
            Edge edge = arm[ index - 1 + i ].nextEdge.value();
             if( edge.dock1() == XPlus ){
                transformation *= rotate( M_PI, Z ) *
                    rotate( M_PI_2, Y );
                zx = true;
            } else if( edge.dock1() == XMinus ){
                transformation *= rotate( M_PI, Z ) *
                    rotate( -M_PI_2, Y );
                //flip = !flip;
                zx = true;
            }

            transformation *= translate( Z );
            transformation *= rotate( oriToAngle( edge.ori() ), Z );

            if( edge.dock2() == XPlus ){
                transformation *= rotate( M_PI, Z ) *
                    rotate( M_PI_2, Y );
                //flip = !flip;
                xz = true;
                zx = false;
            } else if( edge.dock2() == XMinus ){
                transformation *= rotate( M_PI, Z ) *
                    rotate( -M_PI_2, Y );
                //flip = !flip;
                xz = true;
                zx = false;
            } else if( edge.ori() == East || edge.ori() == West ){
                flip = !flip;
            }

            // angle += oriToAngle( edge.ori() );
        }

    }
    arm[ index ].toNext[ forward ] = transformation;
    arm[ index ].nextIndex[ forward ] = count;
    arm[ index ].flip[ forward ] = flip;
    arm[ index ].zx[ forward ] = zx;
    arm[ index ].xz[ forward ] = xz;
    index -= direction * count;
}

/* Cover connections */
void connected( tentacle& arm, int& index, int direction = 1 ){
    bool forward = direction == 1;
    const Matrix& transformation = arm[ index ].toNext[ forward ];
    int count = arm[ index ].nextIndex[ forward ];

    const Matrix& expectedNext = transformation * translate( Vector{ 0.0, 0.0, 1.0, 0.0 } );

    Vector position = transformation * Vector{ 0.0, 0.0, 0.0, 1.0 };

    Vector expPos = expectedNext * Vector{ 0.0, 0.0, 0.0, 1.0 };

    Vector next = localPosition( arm, index + direction, index - direction * count );
    Vector current = localPosition( arm, index + direction, index + direction - direction * count );

    double pol = 0.0;
    double az = 0.0;

    if( !arm[ index ].flip[ forward ] ){
        pol = polar( current ) - polar( position );
        az = azimuth( next ) - azimuth( expPos );
    } else {
        pol = polar( next ) - polar( expPos );
        az = azimuth( current ) - azimuth( position );
    }
    if( arm[ index ].xz[ forward ] ){
        pol = polar( current ) - polar( position );
        az = azimuth( current ) - azimuth( position );
    }
    if( arm[ index ].zx[ forward ] ){
        pol = 0;
        az = azimuth( current ) - azimuth( position );
    }

    auto [ p, a ] = simplify( pol, az );

    p = std::clamp( p, -M_PI_2, M_PI_2 );

    arm[ index + direction ].xRot = p;
    arm[ index + direction ].zRot = a;

    arm[ index + direction - direction * count ].trans =
        arm[ index + direction ].trans *
        rotate( a, Z ) *
        rotate( p, X ) *
        transformation;

    index -= direction * arm[ index ].nextIndex[ forward ];
}

bool tentacleMonster::fabrik( tentacle& arm, Matrix target )
{
    Matrix base = arm.front().trans;
    int iterations = 0;
    Vector pos;
    double p;
    double a;

    for( int i = arm.size() - 1; i >= 0; ){
        if( arm[ i ].nextEdge ){
            computeTransformation( arm, i );
        } else {
            --i;
        }
    }
    for( int i = 0; i <= arm.size() - 1; ){
        if( arm[ i ].prevEdge ){
            computeTransformation( arm, i, -1 );
        } else {
            ++i;
        }
    }
    while( !equals( arm.back().trans, target ) ){

        /* Forward reaching */
        arm.back().trans = target;

        for( int i = arm.size() - 2; i >= 0; /*--i*/ ){
            if( arm[ i ].nextEdge ){
                connected( arm, i );
            } else {
                sameModule( arm, i );
            }
           if( debug ){
               printArm( 0 );
               int c = getchar();
               if( c == 'q' ){
                   setArm( arm, target );
                   return false;
               }
           }
        }

        /* Backward reaching */
        arm.front().trans = base;

        for( int i = 1; i <= arm.size() - 1; /*++i*/ ){
            if( arm[ i ].prevEdge ){
                connected( arm, i, -1 );
            } else {
                sameModule( arm, i, -1 );
            }

           if( debug ){
               //printArm( 0 );
               setArm( arm, target );
               config.computeMatrices();
               auto mats = config.getMatrices();
               std::cout << "xrot: " << arm[ i - 2 ].xRot << "zrot: " << arm[ i - 2 ].zRot <<'\n'
                         << "alpha " <<
                   config.getModule( ( i / 2 ) ).getJoint( Alpha ) << "beta: " <<
                   config.getModule( ( i / 2 ) ).getJoint( Beta ) << "gamma " <<
                   config.getModule( ( i / 2 ) ).getJoint( Gamma ) << '\n';
               std::cout << "actual:\n" << IO::toString( mats[ (i + 1)/ 2 ][ (1 + i ) % 2 ] ) <<
                   "internal:\n" << IO::toString( arm[ i - 1 ].trans );

               int c = getchar();
               if( c == 'q' ){
                   setArm( arm, target );
                   return false;
               }
           }
        }

        rotateTo( arm.back(), target );

        // TODO: number of iterations should adapt to shoulder length and connections
        if( ++iterations == 3000 ){
            setArm( arm, target );
            if( debug ){
                std::cout << "wanted:\n" << IO::toString( target )
                          << "got:\n";
                printArm( 0 );
            }
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


tentacle tentacleMonster::link( size_t arm1, size_t arm2 ){
    tentacle result = tentacles[ arm1 ];
    Edge edge = { tentacles[ arm1 ].back().id,
                  tentacles[ arm1 ].back().body,
                  ZMinus, South, ZMinus,
                  tentacles[ arm2 ].back().body,
                  tentacles[ arm2 ].back().id
                };
    result.back().nextEdge = edge;
    tentacle reversed = tentacles[ arm2 ];
    std::reverse( reversed.begin(), reversed.end() );
    result.front().prevEdge = edge;
    result.insert( result.end(), reversed.begin(), reversed.end() );
    return result;
}

void tentacleMonster::detach( size_t arm ){
    std::unordered_set< ID > exclude;
    for( auto joint : tentacles[ arm ] ){
        exclude.insert( joint.id );
    }
    auto edges = config.getEdges( tentacles[ arm ].front().id, exclude );
    for( auto edge : edges ){
        config.execute( Action( Action::Reconnect( false, edge ) ) );
    }
    findTentacles();
}

bool tentacleMonster::connectArms( size_t arm1, size_t arm2 ){
    tentacle arm = link( arm1, arm2 );
    config.computeMatrices();
    auto mats = config.getMatrices();
    Matrix target = tentacles[ arm2 ].front().body == A ?
        mats[ tentacles[ arm2 ].front().id ][ 0 ] :
        mats[ tentacles[ arm2 ].front().id ][ 1 ] * rotate( M_PI, Z ) * rotate( M_PI, X );

    std::cout << IO::toString( target );
    return fabrik( arm, target );
}
bool tentacleMonster::reach( size_t arm, Vector position, double xRot,
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

void tentacleMonster::printArm( size_t arm ){
    for( auto& j : tentacles[ arm ] ){
        std::cout << "\nID: " << j.id << " Body: " << j.body << "\n" << IO::toString( j.trans );
    }
}
