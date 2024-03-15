#include "treeReconfig.hpp"

#include <isoreconfig/isomorphic.hpp>

using namespace rofi::geometry;
using namespace rofi::configuration;
using namespace rofi::configuration::matrices;
using namespace rofi::reconfiguration;

roficom::Orientation rofi::reconfiguration::mirrored( roficom::Orientation ori ){
    switch( ori ){
        case roficom::Orientation::North:
            return roficom::Orientation::South;
        case roficom::Orientation::East:
            return roficom::Orientation::West;
        case roficom::Orientation::West:
            return roficom::Orientation::East;
        default:
            return roficom::Orientation::North;
    }
};

bool rofi::reconfiguration::symmetric( const RoficomJoint& joint, const RoficomJoint& other ){
    
    // equal
    if( ( joint.sourceConnector % 3 == other.sourceConnector % 3 && joint.destConnector % 3 == other.destConnector % 3 )
     || ( joint.destConnector % 3 == other.sourceConnector % 3 && joint.sourceConnector % 3 == other.destConnector % 3 ) )
    {
        if( joint.orientation == other.orientation ){
            return true;
        }
    }
    auto matching = [&](){
        return ( other.destConnector % 3 == joint.destConnector % 3 && other.sourceConnector % 3 == joint.sourceConnector % 3 ) ||
               ( other.sourceConnector % 3 == joint.destConnector % 3 && other.destConnector % 3 == joint.sourceConnector % 3 );
    };

    // -Z-Z
    if( joint.sourceConnector % 3 == 2 && joint.destConnector % 3 == 2 ){
        if( other.sourceConnector % 3 != 2 || other.destConnector % 3 != 2 ){
            return false;
        }
        if( joint.orientation == roficom::Orientation::North || joint.orientation == roficom::Orientation::South ){
            return other.orientation == joint.orientation || other.orientation == mirrored( joint.orientation );
        }
    }
    // -ZX
    else if( joint.sourceConnector % 3 == 2 ){
        // std::cout << "zx\n";
        if( other.sourceConnector % 3 != 2 && other.destConnector % 3 != 2 ){
            return false;
        }
        if( other.sourceConnector % 3 == 2 && other.destConnector % 3 == 2 ){
            return false;
        }
        return ( matching() && other.orientation == joint.orientation ) || ( !matching() && other.orientation == mirrored( joint.orientation ) );
    }
    // -X
    if( joint.sourceConnector % 3 < 2 ){
        if( other.sourceConnector % 3 == 2 && other.destConnector % 3 == 2 ){
            return false;
        }
        if( joint.destConnector % 3 == 2 ){
            // std::cout << "xz\n";
            if( other.sourceConnector % 3 != 2 && other.destConnector % 3 != 2 ){
                return false;
            }
            return other.orientation == joint.orientation || other.orientation == mirrored( joint.orientation );
        }
        
        if( other.sourceConnector % 3 < 2 && other.destConnector < 2 ){
            if( matching() || ( other.sourceConnector % 3 == other.destConnector % 3 && joint.sourceConnector % 3 == joint.destConnector % 3 ) ){
                return other.orientation == joint.orientation;
            }
            else {
                return !matching() && other.sourceConnector % 3 != other.destConnector % 3 && other.orientation == mirrored( joint.orientation );
            }
        }
        // if( other.destConnector % 3 == 1 && other.sourceConnector < 2 ){
        //     return ( other.sourceConnector % 3 == joint.destConnector % 3 && other.orientation == joint.orientation ) ||
        //             ( other.orientation == mirrored( joint.orientation ) );
        // }
    }
    return false;
}

int size( Tree& tree ){
    int s = 0;
    tree.map( [&]( const auto& ){ s++; } );
    return s;
}

std::pair< int, int > rofi::reconfiguration::mcs_naive( const RofiWorld& source, const RofiWorld& target ){
    // std::map< int, int > mapping;
    // TreeConfiguration t( target );

    std::map< std::pair< int, int >, int > matches;
    // std::vector< std::shared_ptr< Tree > > trees;
    for( const auto& m : target.modules() ){
        for( const auto& m2 : source.modules() ){
            // auto* node = &s.tree;
            TreeConfiguration s( source, m2.getId() );
            TreeConfiguration t( target, m.getId() );
            std::cout << "target: " << serialization::toJSON( t.world ) << '\n';
            std::deque< std::vector< int > > correct = { {} };
            int maps = 0;
            while( !correct.empty() ){
                auto path = correct.front();
                correct.pop_front();
                maps++;
                auto* current = get_node( &s.tree, path );

                for( int i = 0; i < 4; i++ ){
                    // path.push_back( i );
                    auto* t_node = get_node( &t.tree, path );
                    // if( current->children[ i ] && t_node && t_node->in_connector == current->children[ i ]->in_connector
                    //     && current->children[ i ]->orientation == t_node->orientation )
                    if( t_node && current->children[ i ] && t_node->children[ i ] && ( i == 3 || 
                        symmetric( get_connection( source, current->id, current->side * 3 + i ),
                                get_connection( target, t_node->id, t_node->side * 3 + i ) ) ) )
                    {
                        path.push_back( i );
                        correct.push_back( path );
                        path.pop_back();
                        // t_node->children[ i ]->orientation = current->children[ i ]->orientation;
                        // t_node->children[ i ]->in_connector = current->children[ i ]->in_connector;
                    }
                }
            }
            matches[ { m2.getId(), m.getId() } ] = maps;
            std::cout << "matched: " << maps << '\n';
        }
        // trees.push_back( std::make_shared( t.tree ) );
        // }
    }
    // getchar();

    std::pair< int, int > res;
    std::vector< std::pair< int, int > > best;
    int max = 0;
    for( auto [ idxs, count ] : matches ){
        if( count > max ){
            best = { idxs };
            max = count;
        } else if( count == max ){
            best.push_back( idxs );
        }
    }
    std::sort( best.begin(), best.end(), [&]( auto p1, auto p2 ){
        return distance( center( target.getModule( p1.first )->components().front().getPosition()), rofi::isoreconfig::centroid( source ) ) <
                distance( center( target.getModule( p2.first )->components().front().getPosition()), rofi::isoreconfig::centroid( source ) );
    });
    return best.front();
    // return std::distance(matched.begin(), std::max_element(matched.begin(), matched.end()));
    
}