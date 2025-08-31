#pragma once

#include "messageReceiver.hpp"
#include "messageSender.hpp"
#include "functionRegistry.hpp"
#include "memoryService.hpp"
#include "workflowService.hpp"
#include "LRElect.hpp"
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
    MessageReceiver _receiver;
    MessageSender _sender;
    MemoryService _memoryService;
    WorkFlowService _workFlowService;
    std::unique_ptr< udp_pcb > _pcb;

    std::optional< std::function< void() > > _onLeaderFailed;
    
    int _elected_count = 0;

    // ToDo: Move elsewhere.
    void onLeaderElected()
    {
        if ( _elected_count == 3 )
        {
            if ( _memoryService.isMemoryRegistered() )
            {
                _memoryService.setLeader( _election.getLeader() ); 
            }

            if ( _address == _election.getLeader() )
            {
                _functionRegistry.clearTasks();
                std::cout << "I am the leader." << std::endl;
            }
            else 
            {
                std::cout << "I am a follower." << std::endl;
                requestTask();
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

        if ( _onLeaderFailed.has_value() )
        {
            _onLeaderFailed.value()();
        }
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
            if ( _memoryService.isMemoryRegistered() )
            {
                // ToDo: Move packet payload past Type and Sender before this function
                unsigned int offset = sizeof( DistributionMessageType ) + Ip6Addr::size();
                return _memoryService.onStorageMessage( sender, data + offset, size - offset );
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

public:
    static const int DISTRIBUTION_PORT = 7071;

    // ToDo: Move pcb ownership.
    DistributionManager(
        NetworkManager& netmg,
        Ip6Addr& address,
        MessageDistributor* distributor,
        std::unique_ptr< udp_pcb > pcb) 
    : _net_manager( netmg ), _address( address ),
      _election( netmg, distributor, address, 5,
        [ this ] {
            onLeaderElected();
        },
        [ this ] {
            onLeaderFailed();
        } ),
      _receiver(
        DISTRIBUTION_PORT, pcb.get(),
        [ this ]( Ip6Addr sender, DistributionMessageType messageType, uint8_t* data, unsigned int size )
        {
            onMessage( sender, messageType, data, size );
        }),
      _sender( address, DISTRIBUTION_PORT, pcb.get(), distributor ),
      _workFlowService( _sender, _functionRegistry )
    { 
        LOCK_TCPIP_CORE();
        _pcb = std::move( pcb );
        UNLOCK_TCPIP_CORE();

        distributor->registerMethod( METHOD_ID, 
            [ this ] ( Ip6Addr sender, uint8_t* data, unsigned int size ) 
            {
                DistributionMessageType type = as< DistributionMessageType >( data );
                onMessage( sender, type, data, size ); 
            },
            [] () { return; } );
    }

    bool registerLeaderFailureCallback( std::function< void() > callback )
    {
        if ( _onLeaderFailed.has_value() )
        {
            return false;
        }

        _onLeaderFailed = callback;
    }

    bool unregisterLeaderFailureCallback()
    {
        if ( !_onLeaderFailed.has_value() )
        {
            return false;
        }

        _onLeaderFailed.reset();
        return true;
    }

    MemoryService& memoryService()
    {
        return _memoryService;
    }

    void doWork()
    {
        if ( _elected_count <= 3 )
        {
            return;
        }

        if ( _address == _election.getLeader() )
        {
            return _workFlowService.doWorkLeader( METHOD_ID );
        }
        _workFlowService.doWorkFollower( _address, _election.getLeader() );
    }

    FunctionRegistry& functionRegistry()
    {
        return _functionRegistry;
    }

    MessageSender& getSender()
    {
        return _sender;
    }

    void start( int moduleId )
    {
        _election.start( moduleId );
    }

    void broadcastUnblockSignal()
    {
        _sender.broadcastMessage(DistributionMessageType::BlockingTaskRelease, METHOD_ID);
    }

    bool requestTask()
    {
        if ( _election.getLeader() == _address )
        {
            return false;
        }

        auto emptyTask = Task< int >(0);
        _sender.sendMessage( DistributionMessageType::TaskRequest, emptyTask, _election.getLeader() );
        return true;
    }
};