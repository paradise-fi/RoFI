#include <networking/routingTable.hpp>
#include <networking/protocol.hpp>

namespace rofinet {

std::ostream& operator<<( std::ostream& o, const RoutingTable& rt ) {
    for ( auto& rec : rt._records ) {
        o << rec.ip() << "/" << static_cast< int >( rec.mask() ) << ":\n";
        bool first = true;
        for ( auto& g : rec.gateways() ) {
            o << "                        " << ( first ? "via " : "    " )
              << g.name() << " : " << static_cast< int >( g.cost() );
            if ( g.learnedFrom() )
                o << "\t[from " << g.learnedFrom()->name() << "]";
            o << "\n";
            first = false;
        }
    }

    return o;
}

} // namespace rofinet
