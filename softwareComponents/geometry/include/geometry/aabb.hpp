#pragma once

#include <deque>
#include <memory>
#include <optional>

#include "shapes.hpp"

/*
  Standard implementation of Axis-Aligned Bounding Box trees. In order to store
  an object within this tree, collisions need to be defined between two objects
  of that type as well as between the object and boxes.

  AABBs are binary trees, where each node holds a bounding box of its children,
  with each child being another bounding box node or a leaf shape. It serves for
  efficient collision checking between geometric objects.
  */

namespace rofi::geometry {

using namespace rofi::configuration::matrices;

template< typename T >
concept geometric = requires ( const Box& box, const T& shape ){
    collide( box, shape );
    collide( shape, shape );
    shape.bounding_box();
};

template< geometric LeafShape >
struct AABB_Node;

template< geometric LeafShape >
struct AABB_Leaf {

    using leaf_t = AABB_Leaf< LeafShape >;
    using node_t = AABB_Node< LeafShape >;

    /* Holds a shape */
    LeafShape shape;
    /* Signals whether the object is a rigid obstacle or moving object */
    bool rigid = true;
    /* Bounding box of the object */
    Box bound;

    node_t* parent = nullptr;

    explicit AABB_Leaf( LeafShape shape, AABB_Node< LeafShape >* parent = nullptr )
        : shape( shape ), bound( shape.bounding_box() ), parent( parent ) {};

    leaf_t* find_leaf( const LeafShape& target ) {
        if( shape == target ){
            return this;
        }
        return nullptr;
    }

    bool collides( const LeafShape& target ){
        return collide( shape, target );
    }

    size_t size() const {
        return 1;
    }

    size_t depth() const {
        return 1;
    }

    bool is_left_child() const {
        return parent->children[ 0 ] && this == std::get_if< leaf_t >( parent->children[ 0 ].get() );
    }

};

// Leaves are expected to be spheres or polygons
template< geometric LeafShape >
struct AABB_Node {

    using leaf_t = AABB_Leaf< LeafShape >;
    using node_t = AABB_Node< LeafShape >;
    using var_t = std::variant< AABB_Node, AABB_Leaf< LeafShape > >;

    Box bound;

    std::array< std::unique_ptr< var_t >, 2 > children;

    AABB_Node* parent = nullptr;

    explicit AABB_Node( Box bound, AABB_Node* parent = nullptr ) : bound( bound ), parent( parent ) {}

    // resize so that both children fit inside the bounding box
    void fit(){
        if( !children[ 0 ] && !children[ 1 ] ){
            return;
        }
        auto get_bound = []( auto&& node ){ return node.bound; };
        for( auto idx : { 0, 1 } ){
            if( !children[ idx ] ){
                bound = std::visit( get_bound, *children[ 0 ] );
                return;
            }
        }

        // new corners
        bound = bounding_box( std::visit( get_bound, *children[ 0 ] ),
                              std::visit( get_bound, *children[ 1 ] ) );
    }

    leaf_t* find_leaf( const LeafShape& shape ) {
        auto find = [&]( auto&& node ) -> leaf_t* {
            if( collide( node.bound, shape ) ){
                return node.find_leaf( shape );
            }
            return nullptr;
        };
        for( auto idx : { 0, 1 } ){
            if( children[ idx ] ){
                auto* res = std::visit( find, *children[ idx ] );
                if( res ){
                    return res;
                }
            }
        }
        return nullptr;
    }

    bool collides( const LeafShape& shape ) const {
        auto coll = [&]( auto&& node ) {
            if( collide( node.bound, shape ) ){
                return node.collides( shape );
            }
            return false;
        };
        for( auto idx : { 0, 1 } ){
            if( children[ idx ] ){
                if( std::visit( coll, *children[ idx ] ) ){
                    return true;
                }
            }
        }
        return false;
    }

    size_t size() const {
        auto get_size = []( auto&& node ){ return node.size(); };
        return ( children[ 0 ] ? std::visit( get_size, *children[ 0 ] ) : 0 ) +
               ( children[ 1 ] ? std::visit( get_size, *children[ 1 ] ) : 0 );
    }

    size_t depth() const {
        auto get_depth = []( auto&& node ){ return node.depth(); };
        return 1 + std::max( children[ 0 ] ? std::visit( get_depth, *children[ 0 ] ) : 0,
                             children[ 1 ] ? std::visit( get_depth, *children[ 1 ] ) : 0 );
    }

    bool is_left_child(){
        return parent->children[ 0 ] && this == std::get_if< node_t >( parent->children[ 0 ].get() );
    }

    void erase_empty(){
        if( !children[ 0 ] && !children[ 1 ] && parent ){
            if( parent->children[ 0 ] && this == std::get_if< node_t >( parent->children[ 0 ].get() ) ){
                parent->children[ 0 ] = nullptr;
            } else {
                parent->children[ 1 ] = nullptr;
            }
            parent->erase_empty();
        }
    }
};

template< geometric LeafShape >
class AABB {

    using node_t = AABB_Node< LeafShape >;
    using leaf_t = AABB_Leaf< LeafShape >;
    using var_t = std::variant< node_t, leaf_t >;

    std::unique_ptr< var_t > _root = nullptr;

  public:

    AABB(){};

    leaf_t* insert( const LeafShape& shape ){
        if( !_root ){
            _root = std::make_unique< var_t >( node_t( shape.bounding_box() ) );
            auto* root = std::get_if< node_t >( _root.get() );
            root->children[ 0 ] = std::make_unique< var_t >( leaf_t( shape, std::get_if< node_t >( _root.get() ) ) );
            return std::get_if< leaf_t >( root->children[ 0 ].get() );
        }

        auto [ child, parent ] = find_relatives( shape );

        leaf_t* new_leaf = nullptr;

        if( !child ){
            new_leaf = append( shape, parent );
        } else {
            new_leaf = make_sibling( shape, child, parent );
        }

        // resize all boxes on the way to root
        while( parent ){
            parent->fit();
            parent = parent->parent;
        }

        return new_leaf;
    }

    size_t size() const {
        if( !_root ){
            return 0;
        }
        return std::visit( []( auto&& node ){ return node.size(); }, *_root );
    }

    size_t depth() const {
        if( !_root ){
            return 0;
        }
        return std::visit( []( auto&& node ){ return node.depth(); }, *_root );
    }

    var_t* root(){
        return _root.get();
    }

    void clear(){
        _root = nullptr;
    }

    bool collides( const LeafShape& shape ){
        if( !_root ){
            return false;
        }
        return std::visit( [&]( auto&& node ){ return node.collides( shape ); }, *_root );
    }

    leaf_t* find_leaf( const LeafShape& shape ){
        if( _root ){
            return std::visit( [&]( auto&& node ){ return node.find_leaf( shape ); }, *_root );
        }
        return nullptr;
    }

    template< typename Shape >
    std::vector< LeafShape > colliding_leaves( const Shape& shape ) const {
        auto get_bound = []( auto&& node ){ return node.bound; };

        std::deque< var_t* > queue;
        if( _root ){
            queue.push_back( _root.get() );
        }
        std::vector< LeafShape > colliding;

        while( !queue.empty() ){
            var_t* current = queue.front();
            queue.pop_front();
            if( std::holds_alternative< leaf_t >( *current ) ){
                leaf_t* leaf = std::get_if< leaf_t >( current );
                if( collide( leaf->shape, shape ) && leaf->rigid ){
                    colliding.push_back( leaf->shape );
                }
            } else {
                node_t* node = std::get_if< node_t >( current );
                for( auto idx : { 0, 1 } ){
                    if( node->children[ idx ] && collide( std::visit( get_bound, *node->children[ idx ] ), shape ) ){
                        queue.push_back( node->children[ idx ].get() );
                    }
                }
            }
        }
        return colliding;
    }

    bool erase( leaf_t* leaf ){
        if( !leaf ){
            return false;
        }

        if( leaf->parent ){
            if( leaf->is_left_child() ){
                 leaf->parent->children[ 0 ] = nullptr;
            } else {
                leaf->parent->children[ 1 ] = nullptr;
            }
            leaf->parent->erase_empty();
        }

        auto* root = std::get_if< node_t >( _root.get() );
        if( !root->children[ 0 ] && !root->children[ 1 ] ){
            _root = nullptr;
        }

        return true;
    }

    bool erase( const LeafShape& shape ){
        auto* node = find_leaf( shape );
        return erase( node );
    }

    auto begin() const {
        return Iterator( _root ? _root.get() : nullptr );
    }

    auto end() const {
        return Iterator( nullptr );
    }


  private:

    node_t* to_node( var_t* var ){
        return std::get_if< node_t >( var );
    }
    leaf_t* to_leaf( var_t* var ){
        return std::get_if< leaf_t >( var );
    }
    leaf_t* append( const LeafShape& shape, node_t* parent ){
        auto new_leaf = std::make_unique< var_t >( leaf_t( shape, parent ) );
        leaf_t* leaf = to_leaf( new_leaf.get() );
        for( auto idx : { 0, 1 } ){
            if( !parent->children[ idx ] ){
                parent->children[ idx ] = std::move( new_leaf );
                return leaf;
            }
        }
        return nullptr;
    }

    leaf_t* make_sibling( const LeafShape& shape, leaf_t* sibling, node_t* parent ){
        auto new_node = std::make_unique< var_t >( node_t( shape.bounding_box(), parent ) );
        auto new_leaf = std::make_unique< var_t >( leaf_t( shape, std::get_if< node_t >( new_node.get() ) ) );
        node_t* node = to_node( new_node.get() );
        leaf_t* leaf = to_leaf( new_leaf.get() );

        for( auto idx : { 0, 1 } ){
            if( sibling->is_left_child() != idx ){
                sibling->parent = node;
                node->children[ idx ] = std::move( parent->children[ idx ] );
                node->children[ 1 - idx ] = std::move( new_leaf );
                node->fit();
                parent->children[ idx ] = std::move( new_node );
                return leaf;
            }
        }
        return nullptr;
    }


    // return pair ( sibling, parent )
    std::pair< leaf_t*, node_t* > find_relatives( const LeafShape& shape ){
        var_t* current = _root.get();
        assert( current );

        while( !std::holds_alternative< leaf_t >( *current ) ){
            node_t* node = to_node( current );
            for( auto idx : { 0, 1 } ){
                if( !node->children[ idx ] ){
                    return { nullptr, node };
                }
            }
            // standard heuristic for finding a sibling:
            //  enter the child that will lead to smaller resizing of bounding boxes
            std::array< double, 2 > costs;
            auto get_bound = []( auto&& node ){ return node.bound; };
            for( auto idx : { 0, 1 } ){
                costs[ idx ] = bounding_box( shape.bounding_box(),
                                             std::visit( get_bound, *node->children[ idx ] ) ).volume()
                    + std::visit( get_bound, *node->children[ 1 - idx ] ).volume();
            }

            current = costs[ 0 ] < costs[ 1 ] ? node->children[ 0 ].get() : node->children[ 1 ].get();
        }
        leaf_t* leaf = std::get_if< leaf_t >( current );
        return { leaf, leaf->parent };
    }

    struct Iterator {

        var_t* current;

        Iterator( var_t* node ) : current( node ){
            descend();
        }

        const auto& operator++(){
            next();
            return *this;
        }

        const auto& operator*(){
            assert( std::holds_alternative< leaf_t >( *current ) );
            return std::get_if< leaf_t >( current )->shape;
        }

        bool operator==( const Iterator& other ) const {
            return current == other.current;
        }

        bool operator!=( const Iterator& other ) const = default;

      private:
        /* Descend tree to the leftmost leaf */
        void descend(){
            while( current && !std::holds_alternative< leaf_t >( *current ) ){
                node_t* node = std::get_if< node_t >( current );
                for( auto idx : { 0, 1 } ){
                    if( node->children[ idx ] ){
                        current = node->children[ idx ].get();
                        break;
                    }
                }
            }
        }
        /* Traverse to the next leaf on the right */
        void next(){
            if( !current ){
                return;
            }
            node_t* parent = std::visit( []( auto&& node ){ return node.parent; }, *current );
            while( parent && ( current == parent->children[ 1 ].get() ) ){
                if( parent->parent && !parent->is_left_child() ){
                    current = parent->parent->children[ 1 ] ? parent->parent->children[ 1 ].get() : nullptr;
                }
                parent = parent->parent;
            }
            while( parent && !parent->children[ 1 ] ){
                parent = parent->parent;
            }
            if( !parent ){
                current = nullptr;
                return;
            }
            current = parent->children[ 1 ].get();
            descend();
        }
    };
};

} // namespace rofi::geometry
