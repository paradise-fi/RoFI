#pragma once

#include <optional>

#include "shapes.hpp"


namespace rofi::geometry {

using namespace rofi::configuration::matrices;

template< typename T >
concept geometric = requires ( const Box& box, const T& shape ){
    collide( box, shape );
    shape.bounding_box();
};

// Leaves are expected to be spheres or polygons
template< geometric LeafShape >
struct AABB_Node {

    Box bound;

    // Node is either a leaf with a shape or has some children
    std::optional< LeafShape > shape;
    std::array< std::unique_ptr< AABB_Node >, 2 > children;
    AABB_Node* parent = nullptr;

    explicit AABB_Node( Box bound ) : bound( bound ) {}

    // resize so that both children fit inside the bounding box
    void fit(){
        if( !children[ 0 ] && !children[ 1 ] ){
            return;
        }
        for( auto idx : { 0, 1 } ){
            if( !children[ idx ] ){
                bound = children[ 1 - idx ]->bound;
                return;
            }
        }

        // new corners
        bound = bounding_box( children[ 0 ]->bound, children[ 1 ]->bound );
    }

    size_t objects() const {
        if( shape ){
            return 1;
        }
        return ( children[ 0 ] ? children[ 0 ]->objects() : 0 ) + ( children[ 1 ] ? children[ 1 ]->objects() : 0 );
    }

    size_t depth() const {
        return 1 + std::max( children[ 0 ] ? children[ 0 ]->depth() : 0, children[ 1 ] ? children[ 1 ]->depth() : 0 );
    }

    bool left_child(){
        return parent->children[ 0 ] && this == parent->children[ 0 ].get();
    }

    void print( const std::string& prefix = "" ){
        if( children[ 1 ] ){
            children[ 1 ]->print( prefix + "  " );
        }
        if( shape ){
            std::cout << prefix << "o\n";
        } else {
            std::cout << prefix << "B\n";
        }
        if( children[ 0 ] ){
            children[ 0 ]->print( prefix + "  " );
        }
    }
};

template< typename LeafShape >
class AABB {

    std::unique_ptr< AABB_Node< LeafShape > > _root = nullptr;

  public:

    AABB(){};

    AABB_Node< LeafShape >* insert( const LeafShape& shape ){
        if( !_root ){
            _root = std::make_unique< AABB_Node< LeafShape > >( shape.bounding_box() );
            _root->children[ 0 ] = std::make_unique< AABB_Node< LeafShape > >( shape.bounding_box() );
            _root->children[ 0 ]->shape = shape;
            _root->children[ 0 ]->parent = _root.get();
            return _root->children[ 0 ].get();
        }

        auto [ child, parent ] = find_relatives( shape );

        AABB_Node< LeafShape >* new_leaf = nullptr;

        if( !child ){
            new_leaf = append( shape, parent );
        } else {
            if( collide( child->shape.value(), shape ) ){
                return nullptr;
            }
            new_leaf = make_sibling( shape, child, parent );
        }

        // resize all boxes on the way to root
        while( parent ){
            parent->fit();
            parent = parent->parent;
        }

        return new_leaf;
    }

    size_t objects() const {
        if( !_root ){
            return 0;
        }
        return _root->objects();
    }

    size_t depth() const {
        if( !_root ){
            return 0;
        }
        return _root->depth();
    }

    AABB_Node< LeafShape >* root(){
        return _root.get();
    }

    bool erase( const LeafShape& shape ){
        auto* node = find_parent( shape );
        if( !node || !node->shape || node->shape.value() != shape ){
            return false;
        }
        if( node->parent ){
            if( node->left_child() ){
                node->parent->children[ 0 ] = nullptr;
            } else {
                node->parent->children[ 1 ] = nullptr;
            }
        } else {
            _root = nullptr;
        }
        return true;
    }


  private:

    AABB_Node< LeafShape >* append( const LeafShape& shape, AABB_Node< LeafShape >* parent ){
        auto new_leaf = std::make_unique< AABB_Node< LeafShape > >( shape.bounding_box() );
        new_leaf->shape = shape;
        for( auto idx : { 0, 1 } ){
            if( !parent->children[ idx ] ){
                new_leaf->parent = parent;
                parent->children[ idx ] = std::move( new_leaf );
                return parent->children[ idx ].get();
            }
        }
        return nullptr;
    }

    AABB_Node< LeafShape >* make_sibling( const LeafShape& shape, AABB_Node< LeafShape >* sibling, AABB_Node< LeafShape >* parent ){
        auto new_leaf = std::make_unique< AABB_Node< LeafShape > >( shape.bounding_box() );
        new_leaf->shape = shape;
        auto new_node = std::make_unique< AABB_Node< LeafShape > >( shape.bounding_box() );
        new_node->parent = parent;
        new_leaf->parent = new_node.get();

        for( auto idx : { 0, 1 } ){
            if( parent->children[ idx ] && parent->children[ idx ].get() == sibling ){
                parent->children[ idx ]->parent = new_node.get();
                new_node->children[ idx ] = std::move( parent->children[ idx ] );
                new_node->children[ 1 - idx ] = std::move( new_leaf );
                new_node->fit();
                parent->children[ idx ] = std::move( new_node );
                return parent->children[ idx ]->children[ 1 - idx ].get();
            }
        }
        return nullptr;
    }

    AABB_Node< LeafShape >* find_parent( const LeafShape& shape ){
        AABB_Node< LeafShape >* current = _root.get();
        AABB_Node< LeafShape >* last = nullptr;
        while( current != last ){
            last = current;
            for( auto idx : { 0, 1 } ){
                if( current->children[ idx ] && collide( current->children[ idx ]->bound, shape ) ){
                    current = current->children[ idx ].get();
                }
            }
        }
        return current;
    }

    // return pair ( sibling, parent )
    std::pair< AABB_Node< LeafShape >*, AABB_Node< LeafShape >* > find_relatives( const LeafShape& shape ){
        AABB_Node< LeafShape >* current = _root.get();
        assert( current );

        while( !current->shape ){
            for( auto idx : { 0, 1 } ){
                if( !current->children[ idx ] ){
                    return { nullptr, current };
                }
            }
            // standard heuristic for finding a sibling:
            //  enter the child that will lead to smaller resizing of bounding boxes
            std::array< double, 2 > costs;
            for( auto idx : { 0, 1 } ){
                costs[ idx ] = bounding_box( shape, current->children[ idx ]->bound ).volume()
                    + current->children[ 1 - idx ]->bound.volume();
            }
            current = costs[ 0 ] < costs[ 1 ] ? current->children[ 0 ].get() : current->children[ 1 ].get();
        }
        return { current, current->parent };
    }

};

} // namespace rofi::geometry
