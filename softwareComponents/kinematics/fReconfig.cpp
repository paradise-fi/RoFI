#include "fReconfig.hpp"

/* Helper functions  */
bool eq( const Matrix& a, const Matrix& b ){
    return arma::approx_equal( a, b, "absdiff", 0.02 );
}

bool eq( const Vector& a, const Vector& b ){
    return arma::approx_equal( a, b, "absdiff", 0.02 );
}

std::string toString( collisionStrategy coll ){
    switch( coll ){
        case( collisionStrategy::none ):
            return "none";

        case( collisionStrategy::naive ):
            return "naive";

        case( collisionStrategy::online ):
            return "online";

        default:
            return "";
    }
}

std::string toString( straightening str ){
    switch( str ){
        case( straightening::none ):
            return "none";

        case( straightening::onCollision ):
            return "onCollision";

        case( straightening::always ):
            return "always";

        default:
            return "";
    }
}

joint otherSide( joint j ){
    return { j.id, j.side == A ? B : A };
}

ID closestMass( const Configuration& init ){
    //std::cout << IO::toString( init );
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

    return bestID;
}

/* Use the symmetry of the modules to find the minimal amount of movement
   e.g. 90 polar 180 azimuth is the same as -90 polar 0 azimuth */
std::pair< double, double > simplify( double pol, double az ){
    while( ( az > M_PI || equals( az, M_PI ) ) ){
        pol *= -1;
        az -= M_PI;
    }
    while( ( az < -M_PI || equals( az, -M_PI ) ) ){
        pol *= -1;
        az += M_PI;
    }
    return { pol, az };
}


bool badConnection( const Edge& edge ){
    return edge.dock1() != ZMinus || edge.dock2() != ZMinus || edge.ori() != North;
}

reconfigurationStep setRotation( ID id, Joint j, double angle ){
    reconfigurationStep step;
    step.type = actionType::rotation;
    step.id = id;
    step.j = j;
    step.angle = angle;
    return step;
}

reconfigurationStep setConnection( Edge toConnect ){
    reconfigurationStep step;
    step.type = actionType::connection;
    step.edge = toConnect;
    return step;
}

reconfigurationStep setDisconnect( Edge toDisconnect ){
    reconfigurationStep step;
    step.type = actionType::disconnect;
    step.edge = toDisconnect;
    return step;
}

treeConfig::treeConfig( const std::string& path ){
    std::ifstream input( path );
    Configuration c;
    IO::readConfiguration( input, c );
    *this = treeConfig( c );
}

treeConfig::treeConfig( const std::string& path, ID r ){
    std::ifstream input( path );
    Configuration c;
    IO::readConfiguration( input, c );
    *this = treeConfig( c, r );
}

treeConfig::treeConfig( Configuration c ){
    c.computeMatrices();
    *this = treeConfig( c, closestMass( c ) );
}

treeConfig::treeConfig( Configuration c, ID r ) : config( c ), root( r ),
                                                  inspector( new treeConfigInspection())
{
    std::deque< std::pair< ID, int > > queue;
    std::unordered_set< ID > seen;
    int depth = 0;
    Configuration treed = config;
    treed.clearEdges();

    queue.emplace_back( root, depth );
    seen.insert( root );

    while( !queue.empty() ){
        auto [ id, depth ] = queue.front();
        depths[ id ] = depth;
        queue.pop_front();

        auto edges = config.getEdges( id, seen );
        for( auto edge : edges ){
            ID otherId = edge.id1() == id ? edge.id2() : edge.id1();
            if( treed.getEdges( otherId ).empty() ){
                treed.addEdge( edge );
            }
            queue.emplace_back( otherId, depth + 1 );
            seen.insert( otherId );
        }

    }
    treed.setFixed( root, A, identity );
    treed.computeMatrices();
    config = treed;
}

joints treeConfig::getFreeArm(){
    std::unordered_set< ID > seen = {};
    for( ID id : config.getIDs() ){
        for( ShoeId side : { A, B } ){
            if( id != root && connections( id, side, id ) == 0 ){
                return initArm( id, side, seen );
            }
        }
    }
    return {};
}

joints treeConfig::initArm( ID id, ShoeId side, std::unordered_set< ID >& seen ){
    joints arm;
    int lastId = id;
    arm.push_front( { id, side } );
    side = side == A ? B : A;
    arm.push_front( { id, side } );
    while( connections( id, side, lastId ) == 1 ){
        joint next = getConnected( id, side, { lastId } );
        if( seen.count( next.id ) != 0 ){
            break;
        }
        arm.push_front( next );
        lastId = id;
        id = next.id;
        side = next.side;
        seen.insert( id );
        if( connections( id, side, lastId ) == 1 ){
            continue;
        }
        side = side == A ? B : A;
        arm.push_front( { id, side } );
    }
    return arm;
}

std::vector< joints > treeConfig::getFreeArms(){
    std::vector< joints > arms;
    std::unordered_set< ID > seen = { root };
    for( ID id : config.getIDs() ){
        for( ShoeId side : { A, B } ){
            if( connections( id, side, id ) == 0 && seen.count( id ) == 0 ){
                arms.emplace_back( joints{ { id, side == A ? B : A }, { id, side } } );
            }
        }
    }
    return arms;
}


int treeConfig::connections( ID id, ShoeId side, ID exclude ){
    auto edges = config.getEdges( id, { exclude } );
    int count = 0;
    for( auto edge : edges ){
        if( edge.id1() == id && edge.side1() == side ||
            edge.id2() == id && edge.side2() == side )
        {
            count++;
        }
    }
    return count;
}

joint treeConfig::getConnected( ID id, ShoeId side, std::unordered_set< ID > exclude ){
    auto edges = config.getEdges( id, exclude );
    for( auto edge : edges ){
        if( edge.id1() == id && edge.side1() == side ){
            return { edge.id2(), edge.side2() };
        }
        if( edge.id2() == id && edge.side2() == side ){
            return { edge.id1(), edge.side1() };
        }
    }
    return { -1, A };
}

bool treeConfig::tryConnections(){
    auto arms = getFreeArms();
    if( arms.size() <= 2 ){
        return fixConnections();
    }

    for( const auto& arm1 : arms ){
        for( const auto& arm2 : arms ){
            if( arm1 == arm2 )
                continue;

            size_t stepCount = reconfigurationSteps.size();
            Configuration oldConfig = config;
            ID oldRoot = root;
            std::map< ID, int > oldDepths = depths;
            if( connect( arm1, arm2, straight == straightening::always ) && tryConnections() ){
                return true;
            } else {
                reconfigurationSteps.resize( stepCount );
                config = oldConfig;
                root = oldRoot;
                depths = oldDepths;
            }
        }
    }
    return false;
}

bool treeConfig::goodConnections( const joints& arm ){
    for( size_t i = 1; i < arm.size(); ++i ){
        joint cur = arm[ i ];
        joint prev = arm[ i - 1 ];
        if( cur.id != prev.id && badConnection( edgeBetween( cur, prev ) ) ){
            return false;
        }
    }
    return true;
}

bool treeConfig::fixConnections(){
    auto arms = getFreeArms();
    auto arm = getFreeArm();

    if( goodConnections( arm ) ){
        straightenArm( arm );
        return true;
    }

    if( arms.size() == 1 ){
        Edge rootEdge = config.getEdges( root ).front();
        joints rootArm;
        rootArm.emplace_back( rootEdge.side1() == A ? joint{ root, A } : joint{ root, B } );
        rootArm.emplace_back( rootEdge.side1() == A ? joint{ root, B } : joint{ root, A } );

        if( connect( rootArm, arms.front(), false ) ){
            return fixConnections();
        }
        return false;
    }

    extend( arms[ 0 ], arms[ 1 ] );

    auto connectTwo = [&]( int current ){
        int other = current == 0 ? 1 : 0;
        auto rootArm = arms[ current ];
        Edge rootEdge = edgeBetween( rootArm.front(), { root, A } );

        rootArm.emplace_front( rootEdge.id1() != -1 ? joint{ root, A } : joint{ root, B } );
        rootArm.emplace_front( rootEdge.id1() != -1 ? joint{ root, B } : joint{ root, A } );

        if( connect( rootArm, arms[ other ], false ) ){
            return true;
        }
        return false;
    };

    if( !goodConnections( arms[ 1 ] ) && connectTwo( 0 ) ){
        return fixConnections();
    }

    if( !goodConnections( arms[ 0 ] ) && connectTwo( 1 ) ){
        return fixConnections();
    }

    return false;
}

bool treeConfig::connect( joints arm1, joints arm2, bool straighten ){
    Configuration oldConfig = link( arm1, arm2 );

    if( !config.connected() ){
        config = oldConfig;
        waitingConnections.clear();
        waitingDisconnects.clear();
        return false;
    }

    Matrix target = oldConfig.getMatrices().at( arm1.back().id ).at( arm1.back().side );

    bool result = fabrik( arm1, target );

    if( straight == straightening::onCollision ){
        auto arms = getFreeArms();
        for( auto j : arm1 ){
            auto colliding = collidingJoints( j );
            for( auto coll : colliding ){
                for( const auto& arm : arms ){
                    if( find( arm.begin(), arm.end(), coll ) != arm.end() ){
                        straightenArm( arm );
                    }
                }
            }
        }
    }

    if( collisions == collisionStrategy::naive && !config.isValid() ){
        result = false;
    }

    if( result ){
        for( auto [ id, side ] : arm1 ){
            for( auto j : { Alpha, Beta, Gamma } ){
                reconfigurationSteps.push_back( setRotation( id, j, config.getModule( id ).getJoint( j ) ) );
            }
        }
        for( const auto& connection : waitingConnections ){
            reconfigurationSteps.push_back( connection );
        }
        waitingConnections.clear();
        for( const auto& disconnect : waitingDisconnects ){
            reconfigurationSteps.push_back( disconnect );
        }
        waitingDisconnects.clear();
        if( straighten )
            straightenArm( arm1 );
    } else {
        config = oldConfig;
        waitingConnections.clear();
        waitingDisconnects.clear();
    }
    return result;
}

joint treeConfig::getPrevious( joint j ){
    auto edges = config.getEdges( j.id );

    for( const auto& edge : edges ){
        if( edge.side1() == j.side && depths[ edge.id2() ] < depths[ j.id ] ){
            return { edge.id2(), edge.side2() };
        }
    }
    return { -1, A };
}

void treeConfig::extend( joints& arm1, joints& arm2 ){
    auto pushPrevious = [&]( joints& current ){
        joint prev = getPrevious( current.front() );
        if( prev.id != -1 ){
            current.push_front( prev );
            if( getPrevious( prev ).id == -1 ){
                current.emplace_front( joint{ prev.id, prev.side == A ? B : A } );
            }
        }
    };

    auto extendArm = [&]( joints& current, joints& other ){
        while( depths[ current.front().id ] > depths[ other.front().id ]
            && getPrevious( current.front() ).id != -1 )
        {
            pushPrevious( current );
        }
    };
    extendArm( arm1, arm2 );
    extendArm( arm2, arm1 );

    while( getPrevious( arm1.front() ).id != getPrevious( arm2.front() ).id ){
        pushPrevious( arm1 );
        pushPrevious( arm2 );
    }
}

Configuration treeConfig::link( joints& arm1, joints& arm2 ){
    Configuration oldConfig = config;
    Edge newEdge = {
        arm1.back().id,
        arm1.back().side,
        ZMinus, North, ZMinus,
        arm2.back().side,
        arm2.back().id
    };
    extend( arm1, arm2 );

    size_t i = arm2.size() - 1;
    while( i != -1 ){
        arm1.push_back( arm2.back() );
        if( arm1.back().id != arm1[ arm1.size() - 2 ].id ){
            depths[ arm1.back().id ] = depths[ arm1[ arm1.size() - 2 ].id ] + 1;
        }
        if( i != 0 && arm2[ i ].id != arm2[ i - 1 ].id ){
            Edge current = edgeBetween( arm2[ i ], arm2[ i - 1 ] );
            if( badConnection( current ) ){
                arm2.pop_back();
                break;
            }
        }
        arm2.pop_back();
        --i;
    }

    auto edges = config.getEdges( arm1.back().id );
    for( auto edge : edges ){
        if( edge == edgeBetween( arm1[ arm1.size() - 2 ], arm1[ arm1.size() - 3 ] ) ){
            continue;
        }
        config.execute( Action( Action::Reconnect( false, edge ) ) );
        waitingDisconnects.emplace_back( setDisconnect( edge ) );
    }

    config.execute( Action( Action::Reconnect( true, newEdge ) ) );
    waitingConnections.emplace_back( setConnection( newEdge ) );

    config.setFixed( root, A, identity );
    config.computeMatrices();
    return oldConfig;
}

bool treeConfig::fabrik( const joints& arm, const Matrix& target ){
    config.computeMatrices();
    Configuration backArm = initBackArm( arm, target );

    const auto& matrices = config.getMatrices();
    Matrix base = matrices.at( arm.front().id ).at( arm.front().side );

    size_t iterations = 0;
    while( !eq( config.getMatrices().at( arm.back().id ).at( arm.back().side ), target )
           || ( collisions == collisionStrategy::online && !config.isValid() ) )
    {
        reaching( arm, backArm, target );
        if( ++iterations == 1000 ){
            return false;
        }
    }
    return true;
}

void treeConfig::rotateTowards( const joints& arm, size_t currentJoint, const Matrix& target ){
    Joint j = arm[ currentJoint ].side == A ? Alpha : Beta;
    rotateJoints( arm, currentJoint, config, -to_rad( config.getModule( arm[ currentJoint ].id ).getJoint( j ) ), 0 );

    Matrix mat = config.getMatrices().at( arm[ currentJoint ].id ).at( arm[ currentJoint ].side );
    Vector targetX = inverse( mat ) * target * Vector{ -1.0, 0.0, 0.0, 1.0 };

    rotateJoints( arm, currentJoint, config, 0, -( azimuth( targetX ) - azimuth( -X ) ) );

    mat = config.getMatrices().at( arm[ currentJoint ].id ).at( arm[ currentJoint ].side );

    Vector targetZ = inverse( mat ) * target * Vector{ 0.0, 0.0, 1.0, 1.0 };
    targetZ = project( X, Vector{ 0.0, 0.0, 0.0, 1.0 }, targetZ );

    auto [ p, a ] = simplify( polar( targetZ ), azimuth( targetZ ) );

    rotateJoints( arm, currentJoint, config, -p, 0 );

    if( debug ){
        std::cout << "target:\n" << target << "act:\n" << config.getMatrices().at( arm.back().id ).at( arm.back().side );
    }
}

void treeConfig::reaching( const joints& arm, Configuration& backArm, const Matrix& target ){
    for( size_t i = arm.size(); i --> 1; ){
        auto [ p, a ] = computeJoints( arm, i, backArm, config, true );
        rotateJoints( arm, i, backArm, p, a );
        if( debug ){
            std::cout << "p: " << p << " a: " << a << " i: " << i << '\n';
            getchar();
        }
    }
    if( debug ){
        std::cout << IO::toString( backArm );
    }

    for( size_t i = 0; i < arm.size() - 1; ++i ){
        auto [ p, a ] = computeJoints( arm, i, config, backArm, false );
        rotateJoints( arm, i, config, p, a );
        if( collisions == collisionStrategy::online ){
            fixCollisions( arm, i );
        }

        if( debug ){
            std::cout << "p: " << p << " a: " << a << '\n';
            getchar();
        }

    }
    rotateTowards( arm, arm.size() - 1, target );
    if( debug )
        std::cout << IO::toString( config );
}

Configuration treeConfig::initBackArm( const joints& arm, const Matrix& target ){
    Configuration backArm;

    for( size_t i = 0; i < arm.size(); ++i ){
        if( i == 0 || arm[ i - 1 ].id != arm[ i ].id )
            backArm.addModule( 0, 0, 0, arm[ i ].id );
        if( i != 0 && arm[ i ].id != arm[ i - 1 ].id ){
            backArm.addEdge( edgeBetween( arm[ i - 1 ], arm[ i ] ) );
        }
    }

    backArm.setFixed( arm.back().id, arm.back().side, target );
    backArm.computeMatrices();
    return backArm;
}

Matrix treeConfig::nextPosition( const joints& arm, size_t currentJoint,
                                 Configuration& currentConfig,
                                 bool forward )
{
    int direction = forward ? -1 : 1;

    if( currentJoint >= arm.size() || currentJoint == arm.size() - 1 && direction == 1 ){
        return currentConfig.getMatrices().at( arm.back().id ).at( arm.back().side );
    }
    if( currentJoint == 0 && direction == -1 ){
        return currentConfig.getMatrices().at( arm.front().id ).at( arm.front().side );
    }
    if( arm[ currentJoint ].id == arm[ currentJoint + direction ].id ){
        return currentConfig.computeOtherSideMatrix( arm[ currentJoint ].id,
                                                     arm[ currentJoint ].side );
    }

    while( currentJoint > 0 && currentJoint < arm.size() - 1 &&
           arm[ currentJoint ].id != arm[ currentJoint + direction ].id )
    {
        currentJoint += direction;
    }
    currentConfig.computeMatrices();
    const auto& matrices = currentConfig.getMatrices();

    return matrices.at( arm[ currentJoint ].id ).at( arm[ currentJoint ].side );
}

std::pair< double, double > treeConfig::computeJoints( const joints& arm, size_t currentJoint,
                                                       Configuration& currentConfig,
                                                       Configuration& otherConfig,
                                                       bool forward )
{
    int direction = forward ? -1 : 1;

    Joint j = arm[ currentJoint ].side == A ? Alpha : Beta;
    double p = currentConfig.getModule( arm[ currentJoint ].id ).getJoint( j );
    rotateJoints( arm, currentJoint, currentConfig, -to_rad( p ), 0 );

    currentConfig.computeMatrices();
    Matrix local = currentConfig.getMatrices().at( arm[ currentJoint ].id ).at( arm[ currentJoint ].side );

    Matrix current = inverse( local ) *
        nextPosition( arm, currentJoint, currentConfig, forward );
    Matrix other = inverse( local ) *
        nextPosition( arm, currentJoint, otherConfig, forward );
    Matrix next = inverse( local ) *
        nextPosition( arm, currentJoint + direction, otherConfig, forward );

    if( debug )
        std::cout << "current\n" << IO::toString( current ) << "other\n" << IO::toString( other )
                  << "next\n" << IO::toString( next ) << '\n';
    return computeAngles( arm, currentJoint, currentConfig, otherConfig, current, other, next, forward );
}

std::pair< double, double > treeConfig::computeAngles( const joints& arm, size_t currentJoint,
                                                       Configuration& currentConfig,
                                                       Configuration& otherConfig,
                                                       Matrix currentPos, Matrix otherPos,
                                                       Matrix nextPos, bool forward )
{
    Vector Zero = { 0.0, 0.0, 0.0, 1.0 };

    int direction = forward ? -1 : 1;

    if( arm[ currentJoint ].id == arm[ currentJoint + direction ].id ){
        if( currentJoint != arm.size() - 1 && currentJoint != 0 ){
            // TODO: check correctness
            Edge edge = edgeBetween( arm[ currentJoint - direction ], arm[ currentJoint ] );
            if( edge.dock1() != ZMinus && edge.dock2() == ZMinus ){
                double a = azimuth( otherPos * Zero );

                if( a > M_PI_2 ){
                    a -= M_PI;
                }
                if( a < -M_PI_2 ){
                    a += M_PI;
                }
                rotateJoints( arm, currentJoint - direction, currentConfig, a, 0 );
                Matrix local = currentConfig.getMatrices().at( arm[ currentJoint ].id ).at( arm[ currentJoint ].side );

                currentPos = inverse( local ) * nextPosition( arm, currentJoint, currentConfig, forward );
                otherPos = inverse( local ) * nextPosition( arm, currentJoint, otherConfig, forward );
            }
        }
        Matrix cur = currentConfig.getMatrices().at( arm[ currentJoint ].id ).at( arm[ currentJoint ].side );
        Vector pos = project( X,
                              Zero,
                              otherPos * Zero );
        auto [ p1, a1 ] = simplify( polar( pos ), azimuth( pos ) );
        auto [ p2, a2 ] = simplify( polar( currentPos * Zero ), azimuth( currentPos * Zero ) );
        return { p1 - p2, 0 };
    }

    Edge edge = edgeBetween( arm[ currentJoint ], arm[ currentJoint + direction ] );
    if( edge.dock1() == ZMinus && edge.dock2() == ZMinus ){
        if( edge.ori() == North || edge.ori() == South ){
            auto [ p1, _ ] = simplify( polar( rotate( M_PI, Y )  * otherPos * Zero ),
                                       azimuth( rotate( M_PI, Y ) * otherPos * Zero ) );
            auto [ ee, a1 ] = simplify( polar( rotate( M_PI, Y )  * otherPos * Zero ),
                                        azimuth( rotate( M_PI, Y ) * nextPos * Zero ) );

            return { p1, a1 };
        } else {
            Vector next = rotate( M_PI, Y ) * nextPos * Zero;
            next = project( X, Zero, next );
            auto [ p1, _ ] = simplify( polar( next ),
                                        azimuth( next ) );
            auto [ ee, a1 ] = simplify( polar( rotate( M_PI, Y )  * otherPos * Zero ),
                                        azimuth( rotate( M_PI, Y ) * otherPos * Zero ) );

            return { p1, a1 };
        }
    }
    if( edge.dock1() == ZMinus ){
        auto [ p1, a1 ] = simplify( polar( rotate( M_PI, Y ) * otherPos * Zero ),
                                    azimuth( rotate( M_PI, Y ) * otherPos * Zero ) );
        double a2 = azimuth( rotate( M_PI, Y ) * nextPos * Zero );
        // TODO: adjust rotation based on orientation and angle, to make further points reachable
        return { p1, a1 };
    }
    // TODO: adjust rotation for X-X connections
    auto [ _, a1 ] = simplify( polar( rotate( M_PI, Y )  * otherPos * Zero ),
                               azimuth( rotate( M_PI, Y ) * otherPos * Zero ) );
    double a2 = edge.dock1() == XPlus ? -M_PI_2 : M_PI_2;

    return { 0, a1 - a2 };
}

void treeConfig::rotateJoints( const joints& arm, size_t currentJoint, Configuration& currentConfig,
                            double polar, double azimuth )
{

    double az = to_deg( azimuth );
    double gamma = currentConfig.getModule( arm[ currentJoint ].id ).getJoint( Gamma );
    if( gamma + az > 180 ){
        az -= 360;
    }
    if( gamma + az < -180 ){
        az += 360;
    }

    if( arm[ currentJoint ].side == A ){
        double pol = to_deg( polar );

        double cur = currentConfig.getModule( arm[ currentJoint ].id ).getJoint( Alpha );
        pol = std::clamp( pol , -90.0 - cur, 90.0 - cur );

        currentConfig.execute( Action( Action::Rotate( arm[ currentJoint ].id, Alpha, pol ) ) );
        currentConfig.execute( Action( Action::Rotate( arm[ currentJoint ].id, Gamma, az ) ) );
    } else {
        double pol = to_deg( polar );
        double cur = currentConfig.getModule( arm[ currentJoint ].id ).getJoint( Beta );
        pol = std::clamp( pol, -90.0  - cur, 90.0  - cur );

        currentConfig.execute( Action( Action::Rotate( arm[ currentJoint ].id, Beta, pol ) ) );
        currentConfig.execute( Action( Action::Rotate( arm[ currentJoint ].id, Gamma, az ) ) );
    }
    currentConfig.computeMatrices();
}

Edge treeConfig::edgeBetween( const joint& j1, const joint& j2 ){
    auto edges = config.getEdges( j1.id );
    for( auto edge : edges ){
        if( edge.id1() == j1.id && edge.id2() == j2.id ){
            return edge;
        } else if( edge.id2() == j1.id && edge.id1() == j2.id ){
           return reverse( edge );
        }
    }
    return { -1, A, ZMinus, North, ZMinus, A, -1 };
}

void treeConfig::straightenArm( const joints& arm ){
    for( auto [ id, side ] : arm ){
        Joint j = side == A ? Alpha : Beta;
        config.getModule( id ).setJoint( j, 0 );
        config.getModule( id ).setJoint( Gamma, 0 );
        reconfigurationSteps.emplace_back( setRotation( id, j, 0 ) );
        reconfigurationSteps.emplace_back( setRotation( id, Gamma, 0 ) );
    }
}

std::set< joint > treeConfig::collidingJoints( joint j ){
    auto matrices = config.getMatrices();
    Vector position = matrices.at( j.id ).at( j.side ) * Vector{ 0, 0, 0, 1 };
    std::set< joint > collidingJoints;

    for( auto [ id, ar ] : matrices ){
        for( auto side : { A, B } ){
            if( id == j.id && side == j.side )
                continue;

            Vector current = ar[ side ] * Vector{ 0, 0, 0, 1 };
            if( std::fabs( radial( position - current ) ) < 0.99 ){
                collidingJoints.insert( { id, side } );
            }
        }
    }
    return collidingJoints;
}

std::pair< Vector, double > treeConfig::intersectionCircle( joint current, joint colliding ){
    Matrix mat = config.getMatrices().at( current.id ).at( current.side );
    Vector collidingPos = inverse( mat ) * config.getMatrices().at( colliding.id ).at( colliding.side ) * Vector{ 0, 0, 0, 1 };

    Vector center = project( X, Vector{ 0, 0, 0, 1 }, collidingPos );
    double radius = 1 - radial( collidingPos - center ) * radial( collidingPos - center );

    return { center, radius };
}

void treeConfig::fixCollisions( const joints& arm, size_t currentJoint )
{
    Vector Zero = { 0, 0, 0, 1 };
    size_t nextJoint = currentJoint + 1;
    while( nextJoint < arm.size() - 1 && arm[ nextJoint - 1 ].id != arm[ nextJoint ].id
           && arm[ nextJoint + 1 ].id != arm[ nextJoint ].id )
    {
        nextJoint++;
    }

    if( arm[ currentJoint ].id != arm[ nextJoint ].id &&
        edgeBetween( arm[ currentJoint ], arm[ currentJoint + 1 ] ).dock1() != ZMinus )
    {
        return;
    }

    auto colliding = collidingJoints( arm[ nextJoint ] );

    if( colliding.empty() ){
        return;
    }

    Joint j = arm[ currentJoint ].side == A ? Alpha : Beta;
    double p = config.getModule( arm[ currentJoint ].id ).getJoint( j );
    rotateJoints( arm, currentJoint, config, -to_rad( p ), 0 );

    std::pair< double, double > outer = { -M_PI_2, M_PI_2 };
    std::pair< double, double > inner = { 0.0, 0.0 };
    for( auto coll : colliding ){
        auto [ center, radius ] = intersectionCircle( arm[ currentJoint ], coll );
        auto intersections = xCircleIntersections( Zero, center, 1, radius + 0.5 );
        auto [ p1, a1 ] = simplify( polar( intersections[ 0 ] ), azimuth( intersections[ 0 ] ) );
        auto [ p2, a2 ] = simplify( polar( intersections[ 1 ] ), azimuth( intersections[ 1 ] ) );

        double lower = p1 < p2 ? p1 : p2;
        double higher = p1 < p2 ? p2 : p1;
        if( lower < -M_PI_2 ){
            outer.first = std::max( outer.first, higher );
        } else if( higher > M_PI_2 ){
            outer.second = std::min( outer.second, lower );
        } else {
            inner.first = std::min( inner.first, lower );
            inner.second = std::max( inner.second, higher );
        }
    }

    if( p >= outer.second ){
        p = outer.second;
    } else if( p <= outer.first ){
        p = outer.first;
    } else {
        if( std::fabs( inner.first - p ) < std::fabs( inner.second - p ) ){
            p = inner.first;
        } else {
            p = inner.second;
        }
    }
    rotateJoints( arm, currentJoint, config, to_rad( p ), 0 );

}

void toVideo( std::string outputFile, std::string fromConfig, std::string toConfig ){
    std::ofstream output;
    output.open( outputFile, std::ofstream::out | std::ofstream::trunc );

    Configuration c;
    Configuration last;

    std::ifstream input( fromConfig );
    IO::readConfiguration( input, c );

    output << IO::toString( c );

    treeConfig to( toConfig );
    treeConfig from( fromConfig );

    c = from.config;
    output << IO::toString( c );

    if( !to.tryConnections() || !from.tryConnections() ){
        return;
    }

    size_t counter = 0;
    last = c;
    for( const auto& step : from.reconfigurationSteps ){
        if( step.type == actionType::rotation ){
            c.getModule( step.id ).setJoint( step.j, step.angle );
        }
        if( step.type == actionType::connection ){
            c.execute( Action( Action::Reconnect( true, step.edge ) ) );
        }
        if( step.type == actionType::disconnect ){
            c.execute( Action( Action::Reconnect( false, step.edge ) ) );
        }
        if( c != last )
            output << IO::toString( c );
        last = c;
        counter++;
    }

    c = to.config;
    last = c;
    for( size_t i = to.reconfigurationSteps.size(); i --> 0; ){
        auto step = to.reconfigurationSteps[ i ];

        if( step.type == actionType::rotation ){
            c.getModule( step.id ).setJoint( step.j, step.angle );
        }
        if( step.type == actionType::connection ){
            c.execute( Action( Action::Reconnect( true, step.edge ) ) );
        }
        if( step.type == actionType::disconnect ){
            c.execute( Action( Action::Reconnect( false, step.edge ) ) );
        }

        if( c != last )
            output << IO::toString( c );

        last = c;
        counter++;
    }
    std::cout << "ctr; " << counter << '\n';

}
