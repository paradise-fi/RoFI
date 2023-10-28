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

#include <shapeReconfig/isomorphic.hpp>
#include <shapeReconfig/equality.hpp>
#include <shapeReconfig/hashing.hpp>

#include <nlohmann/json.hpp>
namespace rofi::shapereconfig { // types and functions

/**
 * BFStrict - BFS with strict RofiWorld equality
 * BFShape - BFS with complete shape equality
 * BFSEigen - BFS differentiating RoFIBots only by the eigenvalues of 
 * their PCA decompositions; considers reflected shapes to be equal
 * ShapeStar - A* using complete shape symmetry and a heuristic based on
 * recognition of the shapes of individual modules of the RoFIBot
 */
enum class Algorithm { BFStrict, BFShape, BFSEigen, ShapeStar };
/**
 * Node type describes what each node in the state space is defined by:
 * World - the corresponding RoFIWorld
 * Shape - the shape of the corresponding RoFIWorld in cannonical form ("largest" Cloud)
 * (unused, probably defines the same state space as EigenCloud, but is slower in experiments)
 * Eigen - the eigenvalues of the PCA decomposition of the RoFIWorld
 * EigenCloud - eigenvalues define the approximate shape, a (noncannonical) cloud
 * is used for comparing the definitive shape (quickly decides non-equality of two nodes)
 */
enum class NodeType { World, Shape, Eigen, EigenCloud };

struct Node;

using NodeId = size_t;

class Reporter;

template < NodeType _NodeType >
std::vector< rofi::configuration::RofiWorld > bfs( 
    const rofi::configuration::RofiWorld& start, 
    const rofi::configuration::RofiWorld& target,
    float step, Reporter& rep, size_t maxDepth = 0 );

template < NodeType _NodeType >
std::vector< rofi::configuration::RofiWorld > shapeStar( 
    const rofi::configuration::RofiWorld& start, 
    const rofi::configuration::RofiWorld& target,
    float step, Reporter& rep );

template < NodeType _NodeType >
std::vector< Node > bfsTraverse( 
    const rofi::configuration::RofiWorld& start, 
    float step, size_t maxDepth = 0 );

struct Node
{
    NodeId nid;
    rofi::configuration::RofiWorld world;
    Cloud shape;
    std::array< int, 4 > eigenVals;
    size_t distFromStart;
    size_t predecessorId; 

    Node( NodeType nt, NodeId nid_, const rofi::configuration::RofiWorld& rw, 
        size_t distFromStart_, size_t predecessorId_ ) :
        nid( nid_ ),
        world( rw ), 
        distFromStart( distFromStart_ ),
        predecessorId( predecessorId_ ) 
        {
            Vector cloudEigenVals;
            switch ( nt ) // each node type stores different information
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
                eigenVals = { static_cast<int>( round( cloudEigenVals(0) * 10000 ) ), 
                              static_cast<int>( round( cloudEigenVals(1) * 10000 ) ),
                              static_cast<int>( round( cloudEigenVals(2) * 10000 ) ) };
                break;
            default:
                break;
            }  
        }
};

} // namespace rofi::shapereconfig

namespace rofi::shapereconfig::detail { // auxiliary structs and adjacent configurations generators

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
        // Comparing eigenvalues is very quick but not complete (reflections have same eigenvalues), 
        // isometry of clouds is costly but completely precise
        // Most nodes are not equal, so isometry does not have to be used most of the time (lazy evaluation)
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

inline void generateParametersRec( std::vector< std::vector< float > >& result, std::vector< float >& current,
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

/**
 * @brief A generic function to generate all possible joint rotations (parameters) 
 * for an arbitrary module with any amount of degrees of freedom
 */
inline std::vector< std::vector< float > > generateParameters( size_t degreeOfFreedom, float step ) 
{
    std::vector< std::vector< float > > result;
    std::array< float, 3 > possChange { -step, step, 0 };
    std::vector< float > current;

    generateParametersRec( result, current, possChange, degreeOfFreedom );
    assert( result.back() == std::vector< float >( degreeOfFreedom, 0 ) );
    result.pop_back(); // Remove identity
    return result;
}

/**
 * @brief Descendants generated by changing joint parameters (e. g. rotation of a module)
 */
inline std::vector< rofi::configuration::RofiWorld > getDescendantsRotate( 
    const rofi::configuration::RofiWorld& current, float step )
{
    std::vector< rofi::configuration::RofiWorld > result;

    for ( const Module& rModule : current.modules() )
        for ( size_t j = 0; j < rModule.joints().size(); ++j )
        {
            auto& currJoint = rModule.joints()[j].joint;
            // Possible optimalization: generate rotations for all reocurring dogs in advance
            // For only universal modules, should not be too expensive
            for ( auto& possRot : generateParameters( currJoint->positions().size(), step ) )
            {                
                rofi::configuration::RofiWorld newBot = current;

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

/**
 * @brief Descendants generated by disconnecting already connected roficoms
 */
inline std::vector< rofi::configuration::RofiWorld > getDescendantsDisconnect(
    const rofi::configuration::RofiWorld& current ) 
{
    std::vector< rofi::configuration::RofiWorld > result;
    auto allConnects = current.roficomConnections();

    assert( allConnects.size() + 1 >= current.modules().size() ); // rofiworld must be connected
    if ( allConnects.size() + 1 == current.modules().size() )
        return result;
    
    for ( auto start = allConnects.begin(); start != allConnects.end(); ++start )
    {
        rofi::configuration::RofiWorld newBot = current;
        newBot.disconnect( start.get_handle() );

        if ( newBot.prepare().has_value() && newBot.isValid() )
            result.push_back( newBot );
    }

    return result;
}

inline std::unordered_set< std::pair< int, int >, HashPairIntInt > occupiedRoficoms( 
    const rofi::configuration::RofiWorld& world )
{
    std::unordered_set< std::pair< int, int >, HashPairIntInt > result;

    for ( const auto& rofiJoint : world.roficomConnections() )
    {
        result.insert( std::make_pair( rofiJoint.getSourceModule(world).getId(), rofiJoint.sourceConnector ) );
        result.insert( std::make_pair( rofiJoint.getDestModule(world).getId(), rofiJoint.destConnector ) );
    }

    return result;
}

/**
 * @brief Descendants generated by connecting adjacent roficoms
*/ 
inline std::vector< rofi::configuration::RofiWorld > getDescendantsConnect(
    const rofi::configuration::RofiWorld& parentWorld ) 
{
    if ( parentWorld.modules().size() <= 1 )
        return {};

    std::vector< rofi::configuration::RofiWorld > result;

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

    // By sorting unoccupied roficoms by their position, only those adjacent 
    // in the container can possibly make a connection - only size()-1 connections to consider
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

        if ( !equals( currPos, nextPos ) )
            continue; // roficoms are not adjacent

        const Matrix& currCompAbsPos = parentWorld.getModulePosition( currModId ) * parentWorld.getModule( currModId )->getComponentRelativePosition( currCompId );
        const Matrix& nextCompAbsPos = parentWorld.getModulePosition( nextModId ) * parentWorld.getModule( nextModId )->getComponentRelativePosition( nextCompId );
        
        for ( roficom::Orientation o : allOrientations ) 
        {
            if ( !equals( currCompAbsPos * orientationToTransform( o ), nextCompAbsPos ) ) 
                continue; // current orientation does not fit

            rofi::configuration::RofiWorld nextWorld = parentWorld;
            connect( 
                nextWorld.getModule( currModId )->components()[ currCompId ], 
                nextWorld.getModule( nextModId )->components()[ nextCompId ],
                o );

            auto prepared = nextWorld.prepare();
            // roficoms are adjacent and have the right orientation;
            // if the world was valid before, it should be valid now
            assert( prepared );
            result.push_back( nextWorld );
            ++i; // each roficom is adjacent to at most one other roficom
            break; // only one orientation fits per possible connection
        }
    }

    return result;
}

inline std::vector< rofi::configuration::RofiWorld > getDescendants(
    const rofi::configuration::RofiWorld& current, float step ) 
{
    std::vector< rofi::configuration::RofiWorld > rotatedWorlds = getDescendantsRotate( current, step );
    std::vector< rofi::configuration::RofiWorld > disconnectedWorlds = getDescendantsDisconnect( current );
    std::vector< rofi::configuration::RofiWorld > connectedWorlds = getDescendantsConnect( current );

    rotatedWorlds.insert( rotatedWorlds.end(), 
        std::make_move_iterator( disconnectedWorlds.begin() ),
        std::make_move_iterator( disconnectedWorlds.end() ) );

    rotatedWorlds.insert( rotatedWorlds.end(), 
        std::make_move_iterator( connectedWorlds.begin() ),
        std::make_move_iterator( connectedWorlds.end() ) );

    return rotatedWorlds;
}

/**
 * @brief Create a vector of rofiworlds using predecessors, from start to target
 */
inline std::vector< rofi::configuration::RofiWorld > getPredecessors( 
    const NodeId& targetId, const NodeId& startId, 
    const std::vector< std::unique_ptr< Node > >& nodePtrs )
{
    assert( startId < nodePtrs.size() && nodePtrs[ startId ]->nid == startId );
    assert( targetId < nodePtrs.size() && nodePtrs[ targetId ]->nid == targetId );

    std::vector<rofi::configuration::RofiWorld> plan;
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

} // namespace rofi::shapereconfig::detail

namespace rofi::shapereconfig {

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

    void onUpdateQueue( const std::priority_queue< std::pair< NodeId, size_t >, std::vector< std::pair< NodeId, size_t > >, detail::PriorityPairComparator< NodeId > >& priorQueue )
    {
        _maxQueueSize = std::max( _maxQueueSize, priorQueue.size() );
    }

    void onGenerateDescendants( const std::vector< rofi::configuration::RofiWorld >& desc )
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

/**
 * @brief Classic BFS search from initial to target RoFIWorlds.
 * Explored state space is defined by the starting node and the "getDescendants" function.
 * Returns the found path in the form of a sequence of RofiWorlds, where adjacent
 * worlds are "one step away" (adjacent in the state space).
 * 
 * @tparam _NodeType defines how to store, compare, and hash the explored nodes.
 */
template < NodeType _NodeType >
std::vector< rofi::configuration::RofiWorld > bfs( 
    const rofi::configuration::RofiWorld& start, const rofi::configuration::RofiWorld& target,
    float step, Reporter& rep, size_t maxDepth )
{
    using namespace rofi::shapereconfig::detail;

    std::vector< std::unique_ptr< Node > > nodePtrs;
    std::unordered_set< Node*, HashNodePtr< _NodeType >, EqualNodePtr< _NodeType > > visitedNodes;

    NodeId startId = nodePtrs.size();
    nodePtrs.push_back( std::make_unique< Node >( _NodeType, startId, start, 0, startId ) );
    visitedNodes.insert( nodePtrs[ startId ].get() ); 
    rep.onNewNode( *nodePtrs[ startId ] );

    // Avoids repeatedly creating the same node for comparison, which might be costly
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

        std::vector< rofi::configuration::RofiWorld > descendants = getDescendants( currentNode.world, step );
        rep.onGenerateDescendants( descendants );

        for ( const rofi::configuration::RofiWorld& child : descendants ) {
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

// Complete BFS graph traversal which returns all unique nodes in the state space
// Used for the convTable heuristic to find all possible shapes of one module
// Not intended to be used with anything else than shapes of one module with 90 degree steps
template < NodeType _NodeType >
std::vector< Node > bfsTraverse( 
    const rofi::configuration::RofiWorld& start, float step, size_t maxDepth )
{
    using namespace rofi::shapereconfig::detail;

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

        std::vector< rofi::configuration::RofiWorld > descendants = getDescendants( currentNode.world, step );

        for ( const rofi::configuration::RofiWorld& child : descendants ) {
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

/**
 * @brief Given nodes which form a shape state space (result of bfsTraverse), 
 * generates a table of shortest paths between all of the shapes
 * @return First: Table of shortest paths
 * @return Second: Mapping from shapes in cannonical form to ids used in the table
*/
inline std::pair< arma::Mat< size_t >, std::unordered_map< Cloud, size_t, HashCloud > > 
    createConversionTable( const std::vector< Node >& nodes, float step )
{
    assert( nodes.size() > 0 );
    assert( nodes.front().world.modules().size() == 1 ); // conversion table for single modules

    // Assign IDs to shapes of nodes
    std::unordered_map< Cloud, size_t, HashCloud > shapeIds;
    for ( size_t i = 0; i < nodes.size(); ++i )
        shapeIds.insert( { nodes[ i ].shape, i } );

    arma::Mat< size_t > convTable( shapeIds.size(), shapeIds.size(), arma::fill::zeros );

    for ( size_t i = 0; i < nodes.size(); ++i )
    {
        for ( const Node& reachedNode : bfsTraverse< NodeType::Shape >( nodes[ i ].world, step ) )
        {
            assert( shapeIds.contains( reachedNode.shape ) ); // Input nodes should form a shape state space
            convTable( i, shapeIds[ reachedNode.shape ] ) = reachedNode.distFromStart;
        }
    }

    assert( convTable.is_symmetric() );
    return std::make_pair( convTable, shapeIds );
}

/**
 * @brief Creates a histogram for shapes of individual modules in a given RoFIWorld.
 * Last number is the number of RoFICoM connections in the RoFIWorld.
 * Shape IDs are given by <cloudIds> (second returned value of the createConversionTable function).
 */
inline std::vector< size_t > countModShapes( 
    const rofi::configuration::RofiWorld& rw, 
    const std::unordered_map< Cloud, size_t, HashCloud >& cloudIds )
{
    std::vector< size_t > modShapeCounts( cloudIds.size() );

    for ( const std::vector< Vector >& decomposedModule : decomposeRofiWorldModules( rw ) )
    {
        Cloud modShape = canonCloud( Cloud( decomposedModule ) );
        assert( cloudIds.contains( modShape ) );
        assert( cloudIds.at( modShape ) < modShapeCounts.size() );
        ++modShapeCounts[ cloudIds.at( modShape ) ];
    }

    modShapeCounts.push_back( rw.roficomConnections().size() );
    return modShapeCounts;
}

/**
 * @brief (Admissible) Heuristic function, estimating the minimal number of steps a RoFIWorld
 * with <shapeCounts1> histogram of module shapes must make to reach some RoFIWorld with <shapeCounts2>.
 */
inline size_t modShapesDistance( const std::vector< size_t >& shapeCounts1, const std::vector< size_t >& shapeCounts2 )
{
    assert( shapeCounts1.size() == shapeCounts2.size() );
    assert( std::accumulate( shapeCounts1.begin(), shapeCounts1.end(), 0 ) - shapeCounts1.back()
         == std::accumulate( shapeCounts2.begin(), shapeCounts2.end(), 0 ) - shapeCounts2.back() );
    
    size_t result = 0;

    for ( size_t i = 0; i < shapeCounts1.size() - 1; ++i )
    {
        if ( shapeCounts1[ i ] > shapeCounts2[ i ] )
            result += shapeCounts1[ i ] - shapeCounts2[ i ];
    }
    result += std::abs( int(shapeCounts1.back()) - int(shapeCounts2.back()));

    return result;
}

/**
 * @brief (Non-admissible) Heuristic function, estimating the minimal number of steps a RoFIWorld
 * with <shapeCounts1> histogram of module shapes must make to reach some RoFIWorld with <shapeCounts2>.
 * Uses a table <convTable> of shortest paths between the module shapes  
 * (first returned argument of createConversionTable).
 * Not every shape can be transformed to another by just one simple rotation;
 * might return higher estimates than modShapesDistance, sometimes even overestimating
 * (counts the number of steps greedily - cheapest possible transformations first).
 * Assumes the IDs correspond between <convTable> and <shapeCounts>.
 */
inline size_t modShapesDistanceWithTable( std::vector< size_t > shapeCounts1, const std::vector< size_t >& shapeCounts2, 
    const arma::Mat< size_t >& convTable )
{
    using namespace rofi::shapereconfig::detail;

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
    {
        for ( size_t reqShapeId : reqShapes )
        {
            TransPair tp = std::make_pair( extraShapeId, reqShapeId );
            size_t conversionPrice = convTable( extraShapeId, reqShapeId );
            possConversions.push_back( std::make_pair( tp, conversionPrice ) );
        }
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

// Experimental eigenvalues heuristics for Eigen and EigenCloud node types
inline size_t eigenDistance( const Cloud& n1, const Cloud& n2 )
{
    Vector eigDiff = n1.eigenValues() - n2.eigenValues();
    return size_t( ( std::abs( eigDiff(0) ) + std::abs( eigDiff(1) ) + std::abs( eigDiff(2) ) ) / matrices::precision );
}

inline size_t eigenVecDistance( const Cloud& n1, const Cloud& n2 )
{
    return size_t( std::round( matrices::distance( n1.eigenValues(), n2.eigenValues() ) / matrices::precision ) );
}

/**
 * @brief A* algorithm exploring the state space defined by 
 * the starting node and the "getDescendants" function.
 * Uses the modShapesDistanceWithTable heuristic
 * (not effective for node types different from Shape or EigenCloud).
 * Assumes the RoFIWorld is composed of UniversalModules only, 
 * with each of their parameters divisible by <step> (so they can be reached by
 * <step> rotations from a module in the standard position).
 * 
 * @tparam _NodeType defines how to store, compare, and hash the explored nodes.
 * 
 * @returns the found path in the form of a sequence of RofiWorlds, where adjacent
 * worlds are "one step away" (adjacent in the state space).
 */
template < NodeType _NodeType >
std::vector< rofi::configuration::RofiWorld > shapeStar(
    const rofi::configuration::RofiWorld& start, 
    const rofi::configuration::RofiWorld& target,
    float step,
    Reporter& rep )
{
    using namespace rofi::shapereconfig::detail;
    
    // We are not able to gain or lose modules while reconfiguring
    if ( start.modules().size() != target.modules().size() )
        return {};

    rofi::configuration::RofiWorld oneModRofi;
    oneModRofi.insert( UniversalModule( 0 ) ); 
    rofi::parsing::fixateRofiWorld( oneModRofi );
    oneModRofi.prepare().get_or_throw_as< std::logic_error >();
    auto [ convTable, modShapeIds ] = createConversionTable( bfsTraverse< NodeType::Shape >( oneModRofi, step ), step );

    // Node ID is the position in the vector
    std::vector< std::unique_ptr< Node > > nodePtrs;
    std::unordered_set< Node*, HashNodePtr< _NodeType >, EqualNodePtr< _NodeType > > expanded, seen;
    std::unordered_map< NodeId, size_t > distanceToTarget; // estimated distance to target configuration

    NodeId startId = 0;
    nodePtrs.push_back( std::make_unique< Node >( _NodeType, startId, start, 0, startId ) );
    rep.onNewNode( *nodePtrs[ startId ] );

    // Avoids repeatedly creating the same node for comparison, which might be costly
    Node targetNode( _NodeType, 0, target, 0, 0 );
    std::vector< size_t > targetModShapes = countModShapes( target, modShapeIds );

    distanceToTarget.insert( { 
        startId, modShapesDistanceWithTable( countModShapes( start, modShapeIds ), targetModShapes, convTable )
    } );

    std::priority_queue< 
        std::pair< NodeId, size_t >,
        std::vector< std::pair< NodeId, size_t > >, 
        PriorityPairComparator< NodeId > > 
    priorQueue;
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

        if ( EqualNode< _NodeType >{}( currentNode, targetNode ) )
        {
            rep.onPathFound( currentNode );
            return getPredecessors( currentId, startId, nodePtrs );
        }
            
        assert( seen.contains( &currentNode ) );
        auto [ _, inserted ] = expanded.insert( &currentNode );
        if( !inserted )
            continue;

        std::vector< rofi::configuration::RofiWorld > descendants = getDescendants( currentNode.world, step );
        rep.onGenerateDescendants( descendants );

        for ( const rofi::configuration::RofiWorld& childWorld : descendants ) {

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

            size_t estimatedDistance = modShapesDistanceWithTable( 
                countModShapes( childWorld, modShapeIds ), 
                targetModShapes, 
                convTable );
            size_t newDistance = currentNode.distFromStart + 1 + estimatedDistance;

            // Node is already in queue, and has a shorter path to target than we found now - skip it
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

            // Possibly faster: check if the newly generated node is the target
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

} // namespace rofi::shapereconfig
