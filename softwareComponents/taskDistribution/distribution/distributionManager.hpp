#pragma once

#include "messageSender.hpp"
#include "functionRegistry.hpp"
#include "LRElect.hpp"
#include "../memory/sharedMemoryBase.hpp"
#include "../memory/replicatedMemory.hpp"
#include <boost/lockfree/queue.hpp>

using namespace rofi::hal;
using namespace rofi::net;
using namespace rofi::leadership;
using namespace std::chrono_literals;

class DistributionManager
{
    const unsigned int METHOD_ID = 3;

    FunctionRegistry _functionRegistry;
    NetworkManager& _net_manager;
    Ip6Addr _address;

    LRElect _election;
    MessageSender _sender;
    std::unique_ptr< SharedMemoryBase > _memory;
    std::unique_ptr< udp_pcb > _pcb;
    
    int _elected_count = 0;

    static void recv_message( void* manager, 
        struct udp_pcb* pcb,
        struct pbuf * p,
        const ip6_addr_t * addr,
        u16_t port )
    {
        DistributionManager* self = static_cast< DistributionManager* >( manager );
        if ( self )
        {
            self->receiveMessage( nullptr, pcb, p, addr, port );
        }   
    }

    void onLeaderElected()
    {
        if ( _elected_count == 3 )
        {
            if ( _memory != nullptr )
            {
                _memory->setLeader( _election.getLeader() ); 
            }

            if ( _address == _election.getLeader() )
            {
                _functionRegistry.clearTasks();
                std::cout << "I am the leader." << std::endl;
            }
            else 
            {
                std::cout << "I am a follower." << std::endl;
                auto emptyTask = Task< int >(0);
                _sender.sendMessage( DistributionMessageType::TaskRequest, emptyTask, _election.getLeader() );
            }
            _elected_count++;
        }
        if (_elected_count < 3)
        {
            _elected_count++;
        }
    }

    void onLeaderFailed()
    {
        _elected_count = 0;
    }

    void onMessage( Ip6Addr& sender, DistributionMessageType type, uint8_t* data, unsigned int size )
    {
        if ( type == DistributionMessageType::BlockingTaskRelease )
        {
            _functionRegistry.unblockTaskSchedulers();
            return;
        }

        if ( type == DistributionMessageType::TaskRequest )
        {
            std::cout << "Received Task Request from " << sender << std::endl;
            _functionRegistry.enqueueTaskRequest( sender );
            return;
        }

        if ( type == DistributionMessageType::TaskFailed )
        {
            std::cout << "Received Task Failure from " << sender << std::endl;
            // How to react?
            return;
        }

        if ( type == DistributionMessageType::DataStorageRequest )
        {
            std::cout << "Received Data Storage Request from " << sender << std::endl;
            if ( _memory != nullptr )
            {
                unsigned int offset = sizeof( DistributionMessageType ) + Ip6Addr::size();
                return _memory->onStorageMessage( sender, data + offset, size - offset );
            }
        }

        if ( type == DistributionMessageType::DataStorageSuccess )
        {
            std::cout << "Data storage succeeded." << std::endl;
            return;
        }

        int functionId = as< int >( data + sizeof( DistributionMessageType ) + sizeof( Ip6Addr ) );

        std::optional<std::reference_wrapper< FunctionConcept > > fn = _functionRegistry.getFunction( functionId );

        if ( !fn )
        {
            std::cout << "Function with ID " << functionId << " not found." << std::endl;
            return;
        }

        auto task = _functionRegistry.getTaskFromBuffer(
            data + sizeof( DistributionMessageType ) + sizeof( Ip6Addr ), 
            functionId );

        if ( task == nullptr )
        {
            // Something went wrong.
            return;
        }

        if ( type == DistributionMessageType::TaskAssignment )
        {
            std::cout << "Received Task Assignment from " << sender << " for task with ID " << task->id() << std::endl;
            _functionRegistry.enqueueTask( _address, std::move( task ), fn.value().get().completionType() );
            return;
        }

        if ( type == DistributionMessageType::TaskResult )
        {
            std::cout << "Received result from " << sender << " for Task with ID " << task->id() << std::endl;
            if ( !_functionRegistry.invokeFunctionReaction( sender, *task.get() ) )
            {
                std::cout << "Error invoking reaction for function " << task->functionId() << std::endl;
            }
        }
    }

    void doWorkLeader()
    {
        if ( !_functionRegistry.anyTaskRequests() )
        {
            return;
        }

        auto requester = _functionRegistry.getTaskRequester();
        if ( !requester.has_value() )
        {
            return;
        }

        auto task = _functionRegistry.popTaskForAddress( requester.value(), true );
        if ( !task.has_value() )
        {
            std::cout << "Task distribution failed: No initial task given.";
            return;
        }

        auto function = _functionRegistry.getFunction( task.value().get().functionId() );
        if ( !function.has_value() )
        {
            std::cout << "Task distribution failed: Given task has associated no function.";
            return;
        }

        switch ( function.value().get().distributionType() )
        {
            case FunctionDistributionType::Broadcast: 
            {
                _sender.broadcastMessage( DistributionMessageType::TaskAssignment, task.value().get(), METHOD_ID );
                return;
            }

            case FunctionDistributionType::Unicast:
            {
                _sender.sendMessage( DistributionMessageType::TaskAssignment, task.value().get(), requester.value() );
                return;
            }

            default:
            {
                std::cout << "Undefined Function Distribution Type found." << std::endl;
                return;
            }
        }
    }

    void doWorkFollower()
    {
        auto taskCandidate = _functionRegistry.popTaskForAddress( _address );

        if ( !taskCandidate.has_value() )
        {
            return;
        }

        auto task = std::move( taskCandidate.value() );
        
        if  ( !_functionRegistry.invokeFunction( task.get() ) )
        {
            _sender.sendMessage( DistributionMessageType::TaskFailed, task.get(), _election.getLeader() );
            _functionRegistry.finishActiveTask( _address );
            return;
        }

        _sender.sendMessage( DistributionMessageType::TaskResult, task.get(), _election.getLeader() );
        _functionRegistry.finishActiveTask( _address );
    }

public:
    static const int DISTRIBUTION_PORT = 7071;

    DistributionManager(NetworkManager& netmg, Ip6Addr& address, MessageDistributor* distributor, std::unique_ptr< udp_pcb > pcb) 
    : _net_manager( netmg ), _address( address ),
      _election( netmg, distributor, address, 5,
        [ this ] {
            onLeaderElected();
        },
        [ this ] {
            onLeaderFailed();
        } ),
      _sender( MessageSender( address, DISTRIBUTION_PORT, pcb.get(), distributor ) )
    { 
        LOCK_TCPIP_CORE();
        _pcb = std::move( pcb );
        UNLOCK_TCPIP_CORE();

        if ( !_pcb )
        {
            std::cout << "PCB Null" << std::endl;
        }

        LOCK_TCPIP_CORE();
        udp_bind( _pcb.get(), IP6_ADDR_ANY, DISTRIBUTION_PORT );
        udp_recv( _pcb.get(), recv_message, this);
        UNLOCK_TCPIP_CORE();

        distributor->registerMethod( METHOD_ID, 
            [ this ] ( Ip6Addr sender, uint8_t* data, unsigned int size ) 
            {
                DistributionMessageType type = as< DistributionMessageType >( data );
                onMessage( sender, type, data, size ); 
            },
            [] () { return; } );
    }

    template< std::derived_from< SharedMemoryBase > Memory >
    bool useMemory( std::unique_ptr< Memory > memory )
    {
        if (_memory != nullptr )
        {
            return false;
        }

        _memory = std::move( memory );
        return true;
    } 

    // ToDo: Consider moving these functions out of here and giving access to memory by .memory()
    bool saveData( uint8_t* data, int size, int address )
    {
        if ( _memory == nullptr )
        {
            return false;
        }
        return _memory->store( data, size, address );
    }

    template< typename T >
    bool readData( int address, T& out )
    {
        std::vector< uint8_t > data = _memory->read( address );

            if ( data.size() < sizeof( T ) )
            {
                return false;    
            }

            std::memcpy( &out, data.data(), sizeof( T ) );
            return true;
    }

    void doWork()
    {
        if ( _elected_count <= 3 )
        {
            return;
        }

        if ( _address == _election.getLeader() )
        {
            return doWorkLeader();
        }
        doWorkFollower();
    }

    FunctionRegistry& functionRegistry()
    {
        return _functionRegistry;
    }

    void start( int moduleId )
    {
        _election.start( moduleId );
    }

    void receiveMessage( void*,
        struct udp_pcb*,
        struct pbuf* p,
        const ip6_addr_t*, 
        u16_t )
    {
        if ( !p )
        {
            return;
        }
        
        auto packet = rofi::hal::PBuf::own( p );
        DistributionMessageType type = as< DistributionMessageType >( packet.payload() );
        Ip6Addr sender = as< Ip6Addr >( packet.payload() + sizeof( DistributionMessageType ) );

        onMessage( sender, type, packet.payload(), packet.size() );
    }

    MessageSender& getSender()
    {
        return _sender;
    }

    void broadcastUnblockSignal()
    {
        _sender.broadcastMessage(DistributionMessageType::BlockingTaskRelease, METHOD_ID);
    }

};