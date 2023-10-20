#pragma once

#include <array>
#include <cassert>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <vector>
#include <queue>

#include <armadillo>
#include <fmt/format.h>

#include <configuration/rofiworld.hpp>
#include <configuration/universalModule.hpp>
#include <parsing/parsing_lite.hpp>

#include <isoreconfig/isomorphic.hpp>
#include <isoreconfig/equality.hpp>
#include <isoreconfig/hashing.hpp>

#include <nlohmann/json.hpp>
namespace rofi::isoreconfig { // types and functions

using namespace rofi::configuration;

enum class Algorithm { BFStrict, BFShape, BFSEigen, ShapeStar };
enum class NodeType { World, Shape, Eigen, EigenCloud };

struct Node;

using NodeId = size_t;

class Reporter;

template < NodeType _NodeType >
std::vector< RofiWorld > bfs( 
    const RofiWorld& start, const RofiWorld& target,
    float step, Reporter& rep, size_t maxDepth = 0 );

template < NodeType _NodeType >
std::vector< RofiWorld > shapeStar( 
    const RofiWorld& start, const RofiWorld& target,
    float step, Reporter& rep );

template < NodeType _NodeType >
std::vector< Node > bfsTraverse( const RofiWorld& start, float step, size_t maxDepth = 0 );

struct Node
{
    NodeId nid;
    RofiWorld world;
    Cloud shape;
    std::array< int, 4 > eigenVals;
    size_t distFromStart;
    size_t predecessorId; 

    Node( NodeType nt, NodeId nid_, const RofiWorld& rw, size_t distFromStart_, size_t predecessorId_ ) :
        nid( nid_ ),
        world( rw ), 
        distFromStart( distFromStart_ ),
        predecessorId( predecessorId_ ) 
        {
            Vector cloudEigenVals;
            switch ( nt )
            {
            case NodeType::Shape:
                shape = rofiWorldToShape( rw );
                break;
            case NodeType::Eigen:
                eigenVals = rofiWorldToEigenValues( rw );
                break;
            case NodeType::EigenCloud:
                shape = rofiWorldToCloud( rw );
                cloudEigenVals = shape.eigenValues();
                eigenVals = { static_cast<int>( round( cloudEigenVals(0) / 0.0001 ) ), 
                              static_cast<int>( round( cloudEigenVals(1) / 0.0001 ) ),
                              static_cast<int>( round( cloudEigenVals(2) / 0.0001 ) ) };
                break;
            default:
                break;
            }  
        }
};

} // namespace rofi::isoreconfig

namespace rofi::isoreconfig::detail { // auxiliary structs and adjacent configurations generators

using namespace rofi::configuration;
using namespace rofi::isoreconfig;

template < NodeType _NodeType >
struct EqualNode;

template <>
struct EqualNode< NodeType::World >
{
    bool operator()( const Node& n1, const Node& n2 ) const
    {
        return equalConfiguration( n1.world, n2.world );
    }
};

template <>
struct EqualNode< NodeType::Shape >
{
    bool operator()( const Node& n1, const Node& n2 ) const
    {
        return n1.shape == n2.shape;
    }
};

template <>
struct EqualNode< NodeType::Eigen >
{
    bool operator()( const Node& n1, const Node& n2 ) const
    {
        return n1.eigenVals == n2.eigenVals;
    }
};

template <>
struct EqualNode< NodeType::EigenCloud >
{
    bool operator()( const Node& n1, const Node& n2 ) const
    {
        return n1.eigenVals == n2.eigenVals && isometric( n1.shape, n2.shape );
    }
};

template < NodeType _NodeType >
struct EqualNodePtr
{
    bool operator()( const Node* n1, const Node* n2 ) const
    {
        return EqualNode< _NodeType >{}( *n1, *n2 );
    }
};

template < NodeType _NodeType >
struct HashNodePtr;

template <>
struct HashNodePtr< NodeType::World >
{
    size_t operator()( const Node* nodePtr ) const
    {
        return RofiWorldHash{}( nodePtr->world );
    }
};

template <>
struct HashNodePtr< NodeType::Shape >
{
    size_t operator()( const Node* nodePtr ) const
    {
        return HashCloud{}( nodePtr->shape );
    }
};

template <>
struct HashNodePtr< NodeType::Eigen >
{
    size_t operator()( const Node* nodePtr ) const
    {
        return HashArray< int, 4 >{}( nodePtr->eigenVals );
    }
};

template <>
struct HashNodePtr< NodeType::EigenCloud >
{
    size_t operator()( const Node* nodePtr ) const
    {
        return HashArray< int, 4 >{}( nodePtr->eigenVals );
    }
};

template < typename _Type >
struct PriorityPairComparator
{
    bool operator() ( const std::pair< _Type, size_t >& l, const std::pair< _Type, size_t >& r ) const 
    { 
        return l.second > r.second;
    }
};

void generateParametersRec( std::vector< std::vector< float > >& result, std::vector< float >& current,
     const std::array< float, 3 >& possChange, size_t toBeAdded )
{
    if ( toBeAdded == 0 ) 
    {
        result.push_back( current );
        return;
    }

    for ( float change : possChange )
    {
        current.push_back( change );
        generateParametersRec( result, current, possChange, toBeAdded - 1 );
        assert( current.size() > 0 );
        current.pop_back();
    }
}

std::vector< std::vector< float > > generateParameters( size_t degreeOfFreedom, float step ) 
{
    std::vector< std::vector< float > > result;
    std::array< float, 3 > possChange { -step, step, 0 };
    std::vector< float > current;

    generateParametersRec( result, current, possChange, degreeOfFreedom );
    assert( result.back() == std::vector< float >( degreeOfFreedom, 0 ) );
    result.pop_back(); // Remove identity
    return result;
}

// Descendants generated by changing joint parameters (e. g. rotation of a module)
std::vector< RofiWorld > getDescendantsRotate( const RofiWorld& current, float step )
{
    std::vector< RofiWorld > result;

    for ( const Module& rModule : current.modules() )
        for ( size_t j = 0; j < rModule.joints().size(); ++j )
        {
            auto& currJoint = rModule.joints()[j].joint;
            // TODO generate rotations for all reocurring dogs in advance
            for ( auto& possRot : generateParameters( currJoint->positions().size(), step ) )
            {                
                RofiWorld newBot = current;

                // Shouldnt work - is const
                // newBot.getModule(  modInf.module->getId() )->joints()[j].joint->changePositions( possRot );

                // Skip rotation if it does not respect joint bounds
                if ( !newBot.getModule( rModule.getId() )->changeJointPositionsBy( int(j), possRot ).has_value() )
                    continue;

                if ( newBot.prepare().has_value() && newBot.isValid() )
                    result.push_back( newBot );
            }
        }

    return result;
}

// Descendants generated by disconnecting already connected roficoms
std::vector< RofiWorld > getDescendantsDisconnect(
    const RofiWorld& current ) 
{
    std::vector< RofiWorld > result;
    auto allConnects = current.roficomConnections();

    assert( allConnects.size() + 1 >= current.modules().size() ); // rofiworld must be connected
    if ( allConnects.size() + 1 == current.modules().size() )
        return result;
    
    for ( auto start = allConnects.begin(); start != allConnects.end(); ++start )
    {
        RofiWorld newBot = current;
        newBot.disconnect( start.get_handle() );

        if ( newBot.prepare().has_value() && newBot.isValid() )
            result.push_back( newBot );
    }

    return result;
}

std::unordered_set< std::pair< int, int >, HashPairIntInt > occupiedRoficoms( const RofiWorld& world )
{
    std::unordered_set< std::pair< int, int >, HashPairIntInt > result;

    for ( const auto& rofiJoint : world.roficomConnections() )
    {
        result.insert( std::make_pair( rofiJoint.getSourceModule(world).getId(), rofiJoint.sourceConnector ) );
        result.insert( std::make_pair( rofiJoint.getDestModule(world).getId(), rofiJoint.destConnector ) );
    }

    return result;
}

// Descendants generated by connecting adjacent roficoms
std::vector< RofiWorld > getDescendantsConnect(
    const RofiWorld& parentWorld ) 
{
    if ( parentWorld.modules().size() <= 1 )
        return {};

    std::vector< RofiWorld > result;

    using ModCompPos = std::tuple< int, int, Vector >;

    std::vector< ModCompPos > roficomPositions;
    std::unordered_set< std::pair< int, int >, HashPairIntInt > occupied = occupiedRoficoms( parentWorld );
    auto connections = parentWorld.roficomConnections();

    for ( const Module& rModule : parentWorld.modules() )
        for ( const Component& rConnector : rModule.connectors() )
        {
            assert( rConnector.type == ComponentType::Roficom );

            int modId = rModule.getId();
            int compId = rModule.componentIdx( rConnector );

            if ( occupied.contains( std::make_pair( modId, compId ) ) )
                continue;

            Vector position = arma::round( ( rConnector.getPosition() * matrices::translate( { -0.5, 0, 0 } ) ).col(3) / ERROR_MARGIN ) * ERROR_MARGIN;
            // Vector position = ( rConnector.getPosition() * matrices::translate( { -0.5, 0, 0 } ) ).col(3);
            roficomPositions.push_back( std::make_tuple( modId, compId, position ) );
        }

    auto compareModCompPos = []( const ModCompPos& modCompPos1, const ModCompPos& modCompPos2 )
    {
        const Vector& pos1 = std::get<2>( modCompPos1 );
        const Vector& pos2 = std::get<2>( modCompPos2 );

        return pos1(0) < pos2(0) || 
            ( pos1(0) == pos2(0) && ( pos1(1) < pos2(1) || 
            ( pos1(1) == pos2(1) && pos1(2) < pos2(2) ) ) );
    };

    std::sort( roficomPositions.begin(), roficomPositions.end(), compareModCompPos );

    static constexpr auto allOrientations = std::array{ 
        roficom::Orientation::North,
        roficom::Orientation::East,
        roficom::Orientation::South,
        roficom::Orientation::West 
    };

    assert( roficomPositions.size() > 0 );
    for ( size_t i = 0; i < roficomPositions.size() - 1; ++i ) 
    {
        auto& [ currModId, currCompId, currPos ] = roficomPositions[ i ];
        auto& [ nextModId, nextCompId, nextPos ] = roficomPositions[ i + 1 ];

        // assert( !isConnected( parentWorld, currModId, currCompId, nextModId, nextCompId ) );

        if ( !equals( currPos, nextPos ) )
            continue; // roficoms are not adjacent

        const Matrix& currCompAbsPos = parentWorld.getModulePosition( currModId ) * parentWorld.getModule( currModId )->getComponentRelativePosition( currCompId );
        const Matrix& nextCompAbsPos = parentWorld.getModulePosition( nextModId ) * parentWorld.getModule( nextModId )->getComponentRelativePosition( nextCompId );
        
        for ( roficom::Orientation o : allOrientations ) 
        {
            if ( !equals( currCompAbsPos * orientationToTransform( o ), nextCompAbsPos ) ) 
                continue; // current orientation does not fit

            RofiWorld nextWorld = parentWorld;
            connect( 
                nextWorld.getModule( currModId )->components()[ currCompId ], 
                nextWorld.getModule( nextModId )->components()[ nextCompId ],
                o );

            auto prepared = nextWorld.prepare();
            // roficoms are adjacent and have the right orientation;
            // if the world was valid before, it should be valid now
            assert( prepared );
            // assert( nextWorld.isValid() );
            result.push_back( nextWorld );
            ++i; // each roficom is adjacent to at most one other roficom
            break; // only one orientation fits per possible connection
        }
    }

    return result;
}

// Get possible configurations made from the current one
// "1 step" away, (TODO ignoring configurations of identical classes? or leave it for bfs?)
std::vector< RofiWorld > getDescendants(
    const RofiWorld& current, float step ) 
{
    std::vector< RofiWorld > rotatedWorlds = getDescendantsRotate( current, step );
    std::vector< RofiWorld > disconnectedWorlds = getDescendantsDisconnect( current );
    std::vector< RofiWorld > connectedWorlds = getDescendantsConnect( current );

    rotatedWorlds.insert( rotatedWorlds.end(), 
        std::make_move_iterator( disconnectedWorlds.begin() ),
        std::make_move_iterator( disconnectedWorlds.end() ) );

    rotatedWorlds.insert( rotatedWorlds.end(), 
        std::make_move_iterator( connectedWorlds.begin() ),
        std::make_move_iterator( connectedWorlds.end() ) );

    return rotatedWorlds;
}

// Create a vector of rofiworlds using predecessors, from start to target
std::vector< RofiWorld > getPredecessors( const NodeId& targetId, const NodeId& startId, 
    const std::vector< std::unique_ptr< Node > >& nodePtrs )
{
    assert( startId < nodePtrs.size() && nodePtrs[ startId ]->nid == startId );
    assert( targetId < nodePtrs.size() && nodePtrs[ targetId ]->nid == targetId );

    std::vector<RofiWorld> plan;
    NodeId currentId = targetId;
    
    while ( currentId != startId ) {
        plan.push_back( nodePtrs[ currentId ]->world );
        assert( currentId < nodePtrs.size() && nodePtrs[ currentId ]->nid == currentId );
        currentId = nodePtrs[ currentId ]->predecessorId;
    }

    plan.push_back( nodePtrs[ currentId ]->world );
    std::reverse( plan.begin(), plan.end() );
    return plan;
}

} // namespace rofi::isoreconfig::detail

namespace rofi::isoreconfig {

using namespace rofi::isoreconfig::detail;

class Reporter
{
    std::vector< size_t > _layerNodes;
    size_t _maxQueueSize = 0;
    size_t _descendantsGenerated = 0;
    size_t _nodesTotal = 0;
    size_t _maxDescendants = 0;
    bool _pathFound = false;
    size_t _pathLength = 0;

public:
    Reporter() = default;

    void onNewNode( const Node& n )
    {
        ++_nodesTotal;
        if ( n.distFromStart >= _layerNodes.size() )
            _layerNodes.resize( n.distFromStart + 1 );
        
        ++_layerNodes[ n.distFromStart ];
    }

    void onUpdateDistance( size_t newDist, size_t oldDist )
    {
        assert( oldDist < _layerNodes.size() );
        assert( _layerNodes[ oldDist ] > 0 );

        if ( newDist >= _layerNodes.size() )
            _layerNodes.resize( newDist + 1 );
        
        ++_layerNodes[ newDist ];
        _layerNodes[ oldDist ] -= 1;        
    }

    void onUpdateQueue( const std::queue< NodeId >& bfsQueue )
    {
        _maxQueueSize = std::max( _maxQueueSize, bfsQueue.size() );
    }

    void onUpdateQueue( const std::priority_queue< std::pair< NodeId, size_t >, std::vector< std::pair< NodeId, size_t > >, PriorityPairComparator< NodeId > >& priorQueue )
    {
        _maxQueueSize = std::max( _maxQueueSize, priorQueue.size() );
    }

    void onGenerateDescendants( const std::vector< RofiWorld >& desc )
    {
        _maxDescendants = std::max( _maxDescendants, desc.size() );
        _descendantsGenerated += desc.size();
    }

    void onPathFound( const Node& finalNode )
    {
        _pathFound = true;
        _pathLength = finalNode.distFromStart + 1;
    }

    nlohmann::json toJSON() const
    {
        nlohmann::json res;
        res[ "foundPath" ] = _pathFound;
        res[ "pathLength" ] = _pathLength;
        res[ "maxDistFromStart" ] = _layerNodes.size();
        res[ "totalNodes" ] = _nodesTotal;
        res[ "maxQSize" ] = _maxQueueSize;
        res[ "maxDescendants" ] = _maxDescendants;
        res[ "generatedDescendants" ] = _descendantsGenerated;
        res[ "layerNodes" ] = nlohmann::json::array();
        for ( size_t i = 0; i < _layerNodes.size(); ++i ) 
            res[ "layerNodes" ].push_back( _layerNodes[ i ] );
        
        return res;
    }
};

template < NodeType _NodeType >
std::vector< RofiWorld > bfs( const RofiWorld& start, const RofiWorld& target,
    float step, Reporter& rep, size_t maxDepth )
{
    std::vector< std::unique_ptr< Node > > nodePtrs;
    std::unordered_set< Node*, HashNodePtr< _NodeType >, EqualNodePtr< _NodeType > > visitedNodes;

    NodeId startId = nodePtrs.size();
    nodePtrs.push_back( std::make_unique< Node >( _NodeType, startId, start, 0, startId ) );
    visitedNodes.insert( nodePtrs[ startId ].get() ); 
    rep.onNewNode( *nodePtrs[ startId ] );

    // Placeholder node for comparison
    Node targetNode( _NodeType, 0, target, 0, 0 );

    if ( EqualNode< _NodeType >{}( *nodePtrs[ startId ], targetNode ) )
    {
        rep.onPathFound( *nodePtrs[ startId ] );
        return { start };
    }

    std::queue< NodeId > bfsQueue;
    bfsQueue.push( startId ); 
    rep.onUpdateQueue( bfsQueue );

    while ( !bfsQueue.empty() ) 
    {
        Node& currentNode = *nodePtrs[ bfsQueue.front() ];
        bfsQueue.pop(); 
        rep.onUpdateQueue( bfsQueue );

        if ( maxDepth > 0 && currentNode.distFromStart >= maxDepth )
            break;
        
        assert( visitedNodes.contains( &currentNode ) );
        assert( currentNode.distFromStart >= 0 );

        std::vector< RofiWorld > descendants = getDescendants( currentNode.world, step );
        rep.onGenerateDescendants( descendants );

        for ( const RofiWorld& child : descendants ) {
            NodeId childId = nodePtrs.size();
            Node childNode( _NodeType, childId, child, currentNode.distFromStart + 1, currentNode.nid );

            if ( visitedNodes.contains( &childNode ) )
                continue;

            nodePtrs.push_back( std::make_unique< Node >( childNode ) );
            rep.onNewNode( *nodePtrs[ childId ] );
            visitedNodes.insert( nodePtrs[ childId ].get() );

            if ( EqualNode< _NodeType >{}( *nodePtrs[ childId ], targetNode ) )
            {
                rep.onPathFound( *nodePtrs[ childId ] );
                return getPredecessors( childId, startId, nodePtrs );
            }
            
            bfsQueue.push( childNode.nid );
            rep.onUpdateQueue( bfsQueue );
        }
    }
    // Target is not reachable from start using <step> rotations and dis/connections
    return {};
}

template < NodeType _NodeType >
std::vector< Node > bfsTraverse( const RofiWorld& start, float step, size_t maxDepth )
{
    std::vector< std::unique_ptr< Node > > nodePtrs;
    std::unordered_set< const Node*, HashNodePtr< _NodeType >, EqualNodePtr< _NodeType > > visitedNodes;

    NodeId startId = nodePtrs.size();
    nodePtrs.push_back( std::make_unique< Node >( _NodeType, startId, start, 0, startId ) );
    visitedNodes.insert( nodePtrs[ 0 ].get() ); 

    std::queue< NodeId > bfsQueue;
    bfsQueue.push( startId );

    while ( !bfsQueue.empty() ) 
    {
        const Node& currentNode = *nodePtrs[ bfsQueue.front() ];
        bfsQueue.pop();

        if ( maxDepth > 0 && currentNode.distFromStart == maxDepth )
            break;

        assert( visitedNodes.contains( &currentNode ) );
        assert( currentNode.distFromStart >= 0 );

        std::vector< RofiWorld > descendants = getDescendants( currentNode.world, step );

        for ( const RofiWorld& child : descendants ) {
            NodeId childNodeId = nodePtrs.size();
            Node childNode( _NodeType, childNodeId, child, currentNode.distFromStart + 1, currentNode.predecessorId );

            if ( visitedNodes.contains( &childNode ) )
                continue;

            nodePtrs.push_back( std::make_unique< Node >( childNode ) );
            visitedNodes.insert( nodePtrs[ childNodeId ].get() );
            bfsQueue.push( childNode.nid );
        }
    }

    std::vector< Node > result;
    for ( const Node* nodePtr : visitedNodes )
        result.push_back( *nodePtr );
    return result;
}

// Given nodes which form a shape state space, generates a table of shortest paths from one shape to another
// (second returned argument is a mapping from shapes to ids, which are used to address the table).
std::pair< arma::Mat< size_t >, std::unordered_map< Cloud, size_t, HashCloud > > 
    createConversionTable( const std::vector< Node >& nodes, float step )
{
    assert( nodes.size() > 0 );
    assert( nodes.front().world.modules().size() == 1 ); // conversion table for single modules

    // Assign IDs to nodes (their shapes) - use positions in nodes
    std::unordered_map< Cloud, size_t, HashCloud > shapeIds;
    for ( size_t i = 0; i < nodes.size(); ++i )
        shapeIds.insert( { nodes[ i ].shape, i } );

    arma::Mat< size_t > convTable( shapeIds.size(), shapeIds.size(), arma::fill::zeros );

    for ( size_t i = 0; i < nodes.size(); ++i )
        for ( const Node& reachedNode : bfsTraverse< NodeType::Shape >( nodes[ i ].world, step ) )
        {
            assert( shapeIds.contains( reachedNode.shape ) ); // Input nodes should form a shape state space
            convTable( i, shapeIds[ reachedNode.shape ] ) = reachedNode.distFromStart;
        }

    assert( convTable.is_symmetric() );
    return std::make_pair( convTable, shapeIds );
}

std::vector< size_t > countModShapes( const RofiWorld& rw, const std::unordered_map< Cloud, size_t, HashCloud >& cloudIds )
{
    std::vector< size_t > modShapeCounts( cloudIds.size() );

    for ( const std::vector< Vector >& decomposedModule : decomposeRofiWorldModules( rw ) )
    {
        Cloud modShape = canonCloud( Cloud( decomposedModule ) );
        assert( cloudIds.contains( modShape ) );
        assert( cloudIds.at( modShape ) < modShapeCounts.size() );
        ++modShapeCounts[ cloudIds.at( modShape ) ];
    }

    // Add number of connections as the last metric
    modShapeCounts.push_back( rw.roficomConnections().size() );
    return modShapeCounts;
}

size_t modShapesDistance( const std::vector< size_t >& shapeCounts1, const std::vector< size_t >& shapeCounts2 )
{
    assert( shapeCounts1.size() == shapeCounts2.size() );
    assert( std::accumulate( shapeCounts1.begin(), shapeCounts1.end(), 0 ) - shapeCounts1.back() == std::accumulate( shapeCounts2.begin(), shapeCounts2.end(), 0 ) - shapeCounts2.back() );
    
    size_t result = 0;

    for ( size_t i = 0; i < shapeCounts1.size() - 1; ++i )
    {
        if ( shapeCounts1[ i ] > shapeCounts2[ i ] )
            result += shapeCounts1[ i ] - shapeCounts2[ i ];
    }
    result += std::abs( int(shapeCounts1.back()) - int(shapeCounts2.back()));

    return result;
}

size_t modShapesDistanceWithTable( std::vector< size_t > shapeCounts1, const std::vector< size_t >& shapeCounts2, 
    const arma::Mat< size_t >& convTable )
{
    assert( shapeCounts1.size() == shapeCounts2.size() );
    assert( shapeCounts1.size() > 0 );
    assert( std::accumulate( shapeCounts1.begin(), shapeCounts1.end(), 0 ) - shapeCounts1.back() == std::accumulate( shapeCounts2.begin(), shapeCounts2.end(), 0 ) - shapeCounts2.back() );
    
    std::unordered_set< size_t > extraShapes, reqShapes; 

    // Last element is number of connections; price is at least their difference
    size_t result = std::abs( int(shapeCounts1.back()) - int(shapeCounts2.back()));

    int reqTransactions = 0;
    std::vector< int > shapeCountsDiff( shapeCounts1.size() - 1, 0 );

    for ( size_t i = 0; i < shapeCounts1.size() - 1; ++i )
    {
            shapeCountsDiff[ i ] = int( shapeCounts1[ i ] ) - int( shapeCounts2[ i ] );
            if ( shapeCountsDiff[ i ] > 0 )
            {
                extraShapes.insert( i );
                reqTransactions += shapeCountsDiff[ i ];
            }
            else if ( shapeCountsDiff[ i ] < 0 )
                reqShapes.insert( i );
    }

    using TransPair = std::pair< size_t, size_t >;
    std::vector< std::pair< TransPair, size_t > > possConversions;

    for ( size_t extraShapeId : extraShapes )
        for ( size_t reqShapeId : reqShapes )
        {
            TransPair tp = std::make_pair( extraShapeId, reqShapeId );
            size_t conversionPrice = convTable( extraShapeId, reqShapeId );
            possConversions.push_back( std::make_pair( tp, conversionPrice ) );
        }
    
    std::sort( possConversions.begin(), possConversions.end(), PriorityPairComparator< TransPair >{} );
    std::reverse( possConversions.begin(), possConversions.end() );

    for ( auto [ transPair, price ] : possConversions )
    {
        auto [ extraShapeId, reqShapeId ] = transPair;
        if ( reqTransactions <= 0 )
        {
            assert( reqTransactions == 0 );
            break;
        }
        
        int madeTransactions = std::max( 0, shapeCountsDiff[ extraShapeId ] );
        shapeCountsDiff[ extraShapeId ] -= madeTransactions;
        shapeCountsDiff[ reqShapeId ] += madeTransactions;
        reqTransactions -= madeTransactions;
        result += madeTransactions * price;
    }

    assert( reqTransactions == 0 );
    return result;
}

/* size_t eigenDistance( const Cloud& n1, const Cloud& n2 )
{
    Vector eigDiff = n1.eigenValues() - n2.eigenValues();
    return size_t( ( std::abs( eigDiff(0) ) + std::abs( eigDiff(1) ) + std::abs( eigDiff(2) ) ) / matrices::precision );
}

size_t eigenVecDistance( const Cloud& n1, const Cloud& n2 )
{
    return size_t( std::round( matrices::distance( n1.eigenValues(), n2.eigenValues() ) / matrices::precision ) );
} */

template < NodeType _NodeType >
std::vector< RofiWorld > shapeStar(
    const RofiWorld& start, 
    const RofiWorld& target,
    float step,
    Reporter& rep )
{
    // We are not able to gain or lose modules while reconfiguring
    if ( start.modules().size() != target.modules().size() )
        return {};

    RofiWorld oneModRofi;
    // Assume all parameters of universal modules in given rofiworlds are divisible by <step>,
    // so their shapes can be generated from a module in standard position (all params are 0).
    oneModRofi.insert( UniversalModule( 0 ) ); 
    rofi::parsing::fixateRofiWorld( oneModRofi );
    oneModRofi.prepare().get_or_throw_as< std::logic_error >();
    // Assign ids to all universal module shapes that can be created using <step> rotations
    // and create a table of shortest paths between them
    auto [ convTable, modShapeIds ] = createConversionTable( bfsTraverse< NodeType::Shape >( oneModRofi, step ), step );

    // Position in vector is ID of node (rofiworld and its shape)
    std::vector< std::unique_ptr< Node > > nodePtrs;
    std::unordered_set< Node*, HashNodePtr< _NodeType >, EqualNodePtr< _NodeType > > expanded, seen;
    std::unordered_map< NodeId, size_t > distanceToTarget;

    // Create start node
    NodeId startId = 0;
    nodePtrs.push_back( std::make_unique< Node >( _NodeType, startId, start, 0, startId ) );
    rep.onNewNode( *nodePtrs[ startId ] );

    // Placeholder node and metric for comparison, actual rofiworld might be different
    Node targetNode( _NodeType, 0, target, 0, 0 );
    std::vector< size_t > targetModShapes = countModShapes( target, modShapeIds );

    /* eigenVecDistance( nodePtrs[ startId ]->shape, targetNode.shape ) */
    distanceToTarget.insert( { startId, modShapesDistanceWithTable( countModShapes( start, modShapeIds ), targetModShapes, convTable ) } );

    std::priority_queue< std::pair< NodeId, size_t >, std::vector< std::pair< NodeId, size_t > >, PriorityPairComparator< NodeId > > priorQueue;
    assert( distanceToTarget.contains( startId ) );
    priorQueue.push( std::make_pair( startId, distanceToTarget[ startId ] ) ); 
    rep.onUpdateQueue( priorQueue );
    seen.insert( nodePtrs[ startId ].get() );

    while ( !priorQueue.empty() ) 
    {
        auto [ currentId, priority ] = priorQueue.top();
        priorQueue.pop(); 
        rep.onUpdateQueue( priorQueue );
        Node& currentNode = *nodePtrs[ currentId ];
        
        // assert( distanceToTarget.contains( currentId ) );

        if ( EqualNode< _NodeType >{}( currentNode, targetNode ) )
        {
            rep.onPathFound( currentNode );
            return getPredecessors( currentId, startId, nodePtrs );
        }
            
        assert( seen.contains( &currentNode ) );
        auto [ _, inserted ] = expanded.insert( &currentNode );
        if( !inserted )
            continue;

        std::vector< RofiWorld > descendants = getDescendants( currentNode.world, step );
        rep.onGenerateDescendants( descendants );

        for ( const RofiWorld& childWorld : descendants ) {

            // if node with same shape was already expanded, skip it
            NodeId nextNodeId = nodePtrs.size();
            Node nextNode( _NodeType, nextNodeId, childWorld, currentNode.distFromStart + 1, currentId );
            if ( expanded.contains( &nextNode ) )
                continue;

            // find the node in queue, or create a new one
            NodeId childId = nextNodeId;
            auto childIter = seen.find( &nextNode );
            if ( childIter == seen.end() )
            {
                nodePtrs.push_back( std::make_unique<Node>( Node( nextNode ) ) );
                rep.onNewNode( *nodePtrs[ nextNodeId ] );
            } else 
                childId = (*childIter)->nid;

            /* eigenVecDistance( nodePtrs[ childId ]->shape, targetNode.shape ) */
            size_t newDistance = currentNode.distFromStart + 1 + modShapesDistanceWithTable( countModShapes( childWorld, modShapeIds ), targetModShapes, convTable );

            // Node is already in queue, and has a shorter path to target than we found now
            if ( distanceToTarget.contains( childId ) && distanceToTarget[ childId ] <= newDistance )
            {
                assert( seen.contains( nodePtrs[ childId ].get() ) );
                continue;
            }

            // Node is in queue, but we found a better path from current node
            nodePtrs[ childId ]->predecessorId = currentId;
            nodePtrs[ childId ]->distFromStart = currentNode.distFromStart + 1;
            rep.onUpdateDistance( currentNode.distFromStart + 1, nodePtrs[ childId ]->distFromStart );
            distanceToTarget[ childId ] = newDistance;

            priorQueue.push( std::make_pair( childId, newDistance ) ); 
            rep.onUpdateQueue( priorQueue );
            assert( !expanded.contains( nodePtrs[ childId ].get() ) );
            seen.insert( nodePtrs[ childId ].get() );

            // does this help?
            /* if ( nodePtrs[ childId ]->shape == targetShape )
            {
                rep.onReturn( true );
                return getPredecessors( predecessor, childId, startId, nodePtrs );
            } */
        }
    }

    // Target is not reachable from start using step rotations and dis/connections
    return {};
}

} // namespace rofi::isoreconfig
