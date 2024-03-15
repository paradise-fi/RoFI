#pragma once

#include <configuration/Matrix.h>

namespace rofi::geometry {

using namespace rofi::configuration;
using namespace rofi::configuration::matrices;

/* Implementation of a visibility graph between sphere shaped obstacles */

struct Visibility {

    /*
      Each vertex is equipped with a baseline `cost` which influences the
      weight of the resulting points for the NURBS curves, as well as a
      `tax` which does not affect the shape but gets raised when near an
      unsuccessful path.
    */
    std::vector< Vector > vertices;
    std::vector< double > costs;
    std::vector< double > tax;

    std::vector< size_t > last_path;

    /*
     * The graph is an adjacency matrix because it's simple, we expect a lot of
     * edges, and we want constant time lookup. The matrix is symmetrical, since
     * edges are not oriented.
     */
    arma::mat adjacency_matrix;

    Visibility() = default;

    Visibility( const std::vector< Vector >& vertices, const std::vector< double >& costs, const AABB< Sphere >& obstacles ) :
        vertices( vertices ), costs( costs ), tax( costs.size(), 1.0 )
    {
        adjacency_matrix = arma::mat( vertices.size() + 2, vertices.size() + 2, arma::fill::zeros );
        for( size_t i = 0; i < vertices.size(); i++ ){
            for( size_t j = 0; j < i; j++ ){
                connect_vertices( i, j, obstacles );
            }
        }
    }

    /* Connect vertices at given indices if there are no obstacles in the way */
    void connect_vertices( size_t i, size_t j, const AABB< Sphere >& obstacles ){
        if( distance( vertices[ i ], vertices[ j ] ) < 2.0 ){
        if( obstacles.colliding_leaves( Segment( vertices[ i ], vertices[ j ] ) ).size() == 0 ){
            adjacency_matrix.at( i, j ) = distance( vertices[ i ], vertices[ j ] );
            adjacency_matrix.at( j, i ) = adjacency_matrix.at( i, j );
        }
        }
    }

    /* Standard Djikstra, returns points where the last component serves as
       the weight */
    std::vector< Vector > shortest_path( size_t src_idx, size_t target_idx ){
        using point_cost = std::pair< size_t, double >;
        auto distances = std::vector< double >( vertices.size(), std::numeric_limits<double>::infinity() );
        auto pred = std::vector< size_t >( vertices.size(), std::numeric_limits< size_t >::max() );

        auto comp = [&]( const point_cost& a, const point_cost& b ){
            return a.second > b.second;
        };
        std::priority_queue< point_cost, std::vector< point_cost >, decltype( comp ) > queue( comp );
        queue.push( { src_idx, 0 } );
        distances[ src_idx ] = 0.0;

        while( !queue.empty() ){
            auto [ u, old_cost ] = queue.top();
            queue.pop();
            if( distances[ u ] < old_cost ){
                continue;
            }

            for( size_t row = 0; row < adjacency_matrix.n_rows; row++ ){
                if( row == u ){
                    continue;
                }
                if( adjacency_matrix.at( row, u ) == 0.0 ){
                    continue;
                }

                double cost = distances[ u ] //+ costs[ row ]
                    + tax[ row ] + adjacency_matrix.at( row, u );
                if( cost < distances[ row ] ){
                    distances[ row ] = cost;
                    pred[ row ] = u;
                    queue.push( { row, cost } );
                }
            }
        }
        std::vector< Vector > path;
        std::vector< size_t > idxs;
        size_t idx = target_idx;

        while( idx != src_idx ){
            path.push_back( vertices[ idx ] );
            idxs.push_back( idx );
            path.back().at( 3 ) = costs[ idx ];
            if( pred[ idx ] == std::numeric_limits< size_t >::max() ){
                return {};
            }
            idx = pred[ idx ];
        }
        path.push_back( vertices[ src_idx ] );
        idxs.push_back( src_idx );
        std::reverse( path.begin(), path.end() );
        last_path = idxs;

        return path;
    }

    /*
      Raise cost near the last found path. If the path leads directly from
      source to target and is evaluated as unsuccessful, delete the edge.
    */
    void raise_cost(){
        if( last_path.size() == 2 ){
            adjacency_matrix.at( vertices.size() - 1, vertices.size() - 2 ) = 0.0;
            adjacency_matrix.at( vertices.size() - 2, vertices.size() - 1 ) = 0.0;
            return;
        }
        for( size_t i = 1; i < last_path.size(); i++ ){
            adjacency_matrix.at( last_path[ i ], last_path[ i - 1 ] ) *= 2;
            adjacency_matrix.at( last_path[ i - 1 ], last_path[ i ] ) *= 2;

            if( last_path[ i ] != vertices.size() - 2 ){
                for( size_t j = 0; j < vertices.size() - 2; j++ ){
                    double dist = distance( vertices[ last_path[ i ] ], vertices[ j ] );
                    // Constants chosen ad-hoc, just to make sure different
                    // paths get explored first
                    if( std::fabs( dist ) < 1.0 ){
                        tax[ j ] += 1000.0;
                    } else {
                        tax[ j ] += 1000.0 / ( std::pow( dist, 10 ) );
                    }
                }
            }
        }
    }
};

} // namespace rofi::geometry
