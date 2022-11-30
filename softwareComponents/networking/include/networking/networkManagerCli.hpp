#pragma once

#include <networking/networkManager.hpp>
#include "lwip++.hpp"

#include <sstream>
#include <string>

namespace rofi::net {

/**
 * Class for CLI management for NetworkManager class.
*/
class NetworkManagerCli {
    NetworkManager& _netManager;
    std::optional< Logger > _logger;

    void log( Logger::Level l, const std::string& msg ) {
        _netManager.log( l, "netcli", msg );
    }

    std::pair< Ip6Addr, uint8_t > parseIpMask( std::stringstream& ss ) {
        std::string ip; int mask;
        ss >> std::ws;
        std::getline( ss, ip, '/' );
        ss >> mask;
        if ( mask < 0 || mask > 128 ) {
            throw std::runtime_error( "parsed mask is outside of [0, 128] interval" );
        }
        return { Ip6Addr( ip.c_str() ), static_cast< uint8_t >( mask ) };
    }

    void parseAddress( const Interface& i, std::stringstream& ss ) {
        std::string cmd;
        ss >> cmd;

        if ( cmd == "show" ) {
            for ( const auto& [ ip, m ]  : i.getAddress() )
                std::cout << ip << "/" << static_cast< int >( m ) << "\n";
        } else if ( cmd == "get" ) {
            if ( ss.eof() )
                throw std::runtime_error( "address get [index] is missing index in the input" );
            int index;
            ss >> index;
            std::cout << i.getAddress( index ).first << "\n";
        } else if ( cmd == "add" || cmd == "rm" || cmd == "remove" ) {
            auto [ ip, mask ] = parseIpMask( ss );
            if ( cmd == "add" ? _netManager.addAddress( ip, mask, i ) : _netManager.removeAddress( ip, mask, i ) ) {
                log( Logger::Level::Info
                   , std::string( "address " ) + Logger::toString( ip )
                                               + "/" + Logger::toString( static_cast< int >( mask ) ) + cmd + " ok" );
            } else {
                log( Logger::Level::Info
                   , std::string( "address ") + cmd + " for " + Logger::toString( ip )
                                              + "/" + Logger::toString( static_cast< int >( mask ) ) + " failed" );
            }
        } else {
            throw std::runtime_error( cmd + " is not a valid command for address command" );
        }
    }

    // TODO: Do not leak function names in exceptions, just report what was parsed when error occurred
    void parseInterface( std::stringstream& ss ) {
        std::string name, cmd;
        ss >> name;
        auto interface = _netManager.findInterface( name );
        if ( !interface ) {
            throw std::runtime_error( "interface " + name + " not found" );
        }

        ss >> cmd;
        if ( cmd == "show" ) {
            std::cout << *interface << "\n";
        } else if ( cmd == "address" ) {
            parseAddress( interface->get(), ss );
        } else if ( cmd == "state" ) {
            std::cout << ( interface->get().isUp() ? "Up" : "Down" ) << "\n";
        } else if ( cmd == "stats" ) {
            std::cout << "sent: " << interface->get().dataSent() << " bytes / received: "
                                  << interface->get().dataReceived() << " bytes\n";
        } else {
            throw std::runtime_error( cmd + " is not a valid command for interface command" );
        }
    }

    void parseTable( std::stringstream& ss ) {
        std::string cmd;
        ss >> cmd;

        if ( cmd == "show" ) {
            std::cout << _netManager.routingTable() << "\n";
        } else if ( cmd == "add" || cmd == "rm" ) {
            parseRoute( cmd == "add", ss );
        } else {
            throw std::runtime_error( cmd + " is not a valid argument for table command" );
        }
    }

    void parseRoute( bool add, std::stringstream& ss ) {
        std::string cmd, ifname;
        int cost;
        
        auto [ ip, mask ] = parseIpMask( ss );
        ss >> ifname;
        ss >> cost;
        if ( add )
            _netManager.addRoute( ip, mask, ifname, cost );
        else
            _netManager.rmRoute( ip, mask, ifname );
    }

    void parseProtocol( std::stringstream& ss ) {
        std::string cmd, name;
        ss >> cmd;
        if ( cmd == "show" ) {
            for ( auto& proto : _netManager.protocols() ) {
                std::cout << proto->name() << " ";
            }
            std::cout << std::endl;
        } else {
            name = cmd;
            auto* proto = _netManager.getProtocol( name );
            if ( !proto )
                throw std::runtime_error( name + " is an invalid name for protocol" );
            ss >> cmd;
            if ( cmd == "show"  ) {
                std::cout << proto->info() << std::endl;
            } else if ( cmd == "manage" ) {
                ss >> cmd;
                _netManager.setProtocol( *proto, _netManager.interface( cmd ) );
            } else if ( cmd == "ignore" ) {
                ss >> cmd;
                _netManager.removeProtocol( *proto, _netManager.interface( cmd ) );
            } else {
                throw std::runtime_error( cmd + " is not a valid argument for protocol command" );
            }
        }
    }

    void parseLog( std::stringstream& ss ) const {
        std::string cmd, name;
        ss >> cmd;
        if ( cmd == "show" ) {
            for ( auto& l : _netManager.logs() )
                Logger::printLog( std::cout, l );
        } else {
            name = cmd;
            ss >> cmd;
            auto interface = _netManager.findInterface( name );
            if ( cmd == "show" && interface ) {
                for ( auto l : _netManager.logsFrom( *interface ) ) {
                    Logger::printLog( std::cout, l );
                }
            } else {
                throw std::runtime_error( cmd + " and " + name + " are invalid input for log command" );
            }
        }
    }

    public:
        NetworkManagerCli( NetworkManager& netManager ) : _netManager( netManager ) {}

        /**
         * \brief Process given command and makes appropriate changes to underlying NetworkManager.
         * 
         * \return Returns @true if the command was succesfully parsed.
        */
        bool command( const std::string& cmd ) {
            std::stringstream ss( cmd );
            return command( ss );
        }

        /**
         * \brief Overloaded variant of the previous `command` function.
        */
        bool command( std::stringstream& ss ) {
            std::string cmd;
            ss >> cmd;
            if ( cmd == "netmg" ) {
                if ( ss.eof() ) { // empty
                    help();
                    return true;
                }

                ss >> cmd;
                if ( cmd == "help" ) {
                    help();
                } else if ( cmd == "if" || cmd == "interface" ) {
                    parseInterface( ss );
                } else if ( cmd == "table" ) {
                    parseTable( ss );
                } else if ( cmd == "forwarding-table" ) {
                    ss >> cmd;
                    if ( cmd == "show" ) {
                        ip_print_table();
                    } else {
                        throw std::runtime_error( cmd + " is not a valid argument for forwarding-table command" );
                    }
                } else if ( cmd == "proto" || cmd == "protocol" ) {
                    parseProtocol( ss );
                } else if ( cmd == "log" || cmd == "logs" ) {
                    parseLog( ss );
                }
            }

            if ( !ss.eof() )
                ss >> std::ws;

            if ( !ss.eof() ) {
                throw std::runtime_error( "bad input: it has some trailing words" );
            }

            return ss.eof();
        }

        /**
         * \brief Prints out help message containing the usage description.
        */
        void help() const {
            const char* helpmsg = 
                "netmg  [ if | interface ]    <name>  show\n"
                "                                     address show\n"                   // show addresses
                "                                             get            <index>\n" // get address on given index
                "                                             add            <ip/mask>\n" // add address
                "                                             [rm | remove]  <ip/mask>\n" // remove address
                "\n"
                "       table                 show\n"
                "                             route   add  <ip/mask>  <interface-name> cost"
                "                                     rm   <ip/mask>  <interface-name> cost"
                "\n"
                "       forwarding-table      show\n"
                "\n"
                "       [ proto | protocol ]  show\n"
                "                             <name>  show\n"                    // shows managed interfaces
                "                                     manage <interface-name>\n" // set-up management
                "                                     ignore <interface-name>\n" // set-down management
                "\n"
                "       [ log | logs ]        show\n"
                "                             <name>  show\n"; // logs from the interface
            std::cout << helpmsg;
        }
    };
} // namespace rofi::net
