#include <algorithm>
#include <cassert>
#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <vector>
#include <configuration/rofiworld.hpp>
#include <configuration/serialization.hpp>

namespace rofi::reconfiguration {

using namespace rofi::configuration;

struct Tree {
    int id = 0;
    bool side = 0;
    int in_connector = 0;
    roficom::Orientation orientation = roficom::Orientation::North;
    Tree* parent = nullptr;
    std::array< std::shared_ptr< Tree >, 4 > children = { { nullptr, nullptr, nullptr, nullptr } };

    bool operator==( const Tree& other ) const {
        if( in_connector != other.in_connector || orientation != other.orientation ){
            return false;
        }
        for( int i = 0; i < 4; i++ ){
            if( ( children[ i ] == nullptr ) != ( other.children[ i ] == nullptr ) ){
                return false;
            }
            if( children[ i ] == nullptr ){
                continue;
            }
            if( *children[ i ] != *other.children[ i ] ){
                return false;
            }
        }
        return true;
    }

    bool operator!=( const Tree& other ) const { return !( *this == other ); }

    template< typename function >
    void map( function f ){
        f( *this );
        for( auto& child : children ){
            if( child != nullptr ){
                child->map( f );
            }
        }
    }

    std::shared_ptr< Tree > copy( Tree* parent = nullptr ) const 
    {
        auto c = std::make_shared< Tree >( id, side );
        c->parent = parent;
        for( int i = 0; i < 4; i++ ){
            if( children[ i ] ){
                c->children[ i ] = children[ i ]->copy( c.get() );
            }
        }
        return c;
    }
};

inline Tree* get_shoulder( Tree* node, int max_length = 5 ){
    int ctr = 0;
    if( !node->parent ){
        return node;
    }
    while( node->parent && ctr++ < max_length &&
             std::count_if( node->parent->children.begin(), node->parent->children.end(), 
                        []( const auto& child ){ return child != nullptr; } ) <= 1 )
    {
        node = node->parent;
    } 
    if( node->parent && node->parent->id == node->id ){
        node = node->parent;
    }
    return node;
}

inline int arm_length( Tree* node ){
    Tree* shoulder = get_shoulder( node, INT32_MAX );
    int length = 0;
    while( node && node != shoulder ){
        node = node->parent;
        length++;
    }
    return length;
}

inline std::vector< int > get_path( Tree* node ){
    std::vector< int > path;
    while( node->parent ){
        for( int i = 0; i < 4; i++ ){
            if( node->parent->children[ i ] && node->parent->children[ i ].get() == node ){
                path.insert( path.begin(), i );
            }
        }
        node = node->parent;
    }
    return path;
}

inline Tree* get_node( Tree* root, const std::vector< int >& path ){
    for( auto idx : path ){
        if( root )
            root = root->children[ idx ].get();
    }
    return root;
};

inline Tree* get_node( Tree* root, int id, bool side ){
    if( root->id == id && root->side == side ){
        return root;
    }
    for( int i = 0; i < 4; i++ ){
        if( root->children[ i ] ){
            auto* result = get_node( root->children[ i ].get(), id, side );
            if( result ){
                return result;
            }
        }
    }
    return nullptr;
}

inline bool is_parent( Tree* node, Tree* other ){
    while( other->parent ){
        if( other->parent->id == node->id ){
            return true;
        }
        other = other->parent;
    }
    return false;
}

inline int depth( Tree* node ){
    int ctr = 0;
    while( node->parent ){
        node = node->parent;
        ctr++;
    }
    return ctr;
}

inline int node_distance( Tree* node, Tree* other ){
    auto find_common = []( Tree* first, Tree* second ){
        while( depth( first ) > depth( second ) ){
            first = first->parent;
        }
        return first;
    };
    auto* node_parent = find_common( node, other );
    auto* other_parent =  find_common( other, node );
    if( node_parent == other_parent ){
        return std::max( depth( node ), depth( other ) ) - std::min( depth( node ), depth( other ) );
    }
    return depth( node ) + depth( other );
}

inline std::vector< Tree* > get_leaves( Tree* root ){
    std::vector< Tree* > leaves;
    bool leaf = true;
    for( int i = 4; i-- > 0; ){
        if( root->children[ i ] ){
            auto res = get_leaves( root->children[ i ].get() );
            leaves.insert( leaves.end(), res.begin(), res.end() );
            leaf = false;
        }
    }
    if( leaf ){
        return { root };
    }
    return leaves;
}

inline std::string serialize( Tree& root ){
    std::string result;
    auto ori_to_str = []( const auto& ori ){
        switch( ori ){
            case roficom::Orientation::North:
                return "n";
            case roficom::Orientation::East:
                return "e";
            case roficom::Orientation::West:
                return "w";
            default:
                return "s";
        }
    };
    root.map( [&]( auto& node ){
        result.append( ori_to_str( node.orientation ) );
        auto path = ( get_path( &node ) );
        for( auto i : path ){
            result.append( std::to_string( i ) );
        }
    });
    return result;
}
}