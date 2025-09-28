#pragma once

#include "messagingWrapper.hpp"
#include "functionRegistry.hpp"
#include "services/memoryService.hpp"
#include "services/workflowService.hpp"
#include "services/electionService.hpp"
#include "electionProtocolBase.hpp"
#include <boost/lockfree/queue.hpp>

using namespace rofi::hal;
using namespace rofi::net;
using namespace rofi::leadership;
using namespace std::chrono_literals;

class DistributedTaskManager
{
    const unsigned int METHOD_ID = 3;

    FunctionRegistry _functionRegistry;
    rofi::net::Ip6Addr _address;

    ElectionService _election;
    MessagingWrapper _messaging;
    DistributedMemoryService _memoryService;
    WorkFlowService _workFlowService;

    std::optional< std::function< bool(DistributedTaskManager& manager, rofi::net::Ip6Addr& requester ) > > _onTaskRequest;
    std::optional< std::function< void(DistributedTaskManager& manager, rofi::net::Ip6Addr& sender, int functionId ) > > _onTaskFailure;
    
    void onElectionSuccesful( const Ip6Addr& leader )
    {
        if ( _memoryService.isMemoryRegistered() )
        {
            _memoryService.setLeader( _election.getLeader() ); 
        }

        if ( _address == leader )
        {
            _functionRegistry.clearTasks();
            std::cout << "I am the leader." << std::endl;
            return;
        }

        std::cout << "I am the follower." << std::endl;
        requestTask();
    }

    void onMessage( Ip6Addr& sender, DistributionMessageType type, uint8_t* data, unsigned int size )
    {
        if ( type == DistributionMessageType::BlockingTaskRelease )
        {
            std::cout << "Blocking task release." << std::endl;
            _functionRegistry.unblockTaskSchedulers( true );
            return;
        }

        if ( type == DistributionMessageType::TaskRequest )
        {
            std::cout << "Received Task Request from " << sender << std::endl;

            // Should enqueing the task request be done by the user?
            if ( _onTaskRequest.has_value() && _onTaskRequest.value()( *this, sender ) )
            {
                return;
            }
            
            _functionRegistry.enqueueTaskRequest( sender );
            return;
        }

        if ( type == DistributionMessageType::DataStorageRequest )
        {
            std::cout << "Received Data Storage Request from " << sender << std::endl;
            if ( _memoryService.isMemoryRegistered() )
            {
                unsigned int offset = sizeof( DistributionMessageType ) + Ip6Addr::size();
                return _memoryService.onStorageMessage( sender, data + offset, size - offset, false );
            }
        }

        if ( type == DistributionMessageType::DataRemovalRequest )
        {
            if ( _memoryService.isMemoryRegistered() )
            {
                unsigned int offset = sizeof( DistributionMessageType ) + Ip6Addr::size();
                return _memoryService.onStorageMessage( sender, data + offset, size - offset, true );
            }
        }

        //TODO: Unused
        if ( type == DistributionMessageType::DataStorageSuccess )
        {
            std::cout << "Data storage succeeded." << std::endl;
            return;
        }

        int functionId = as< int >( data + sizeof( DistributionMessageType ) + sizeof( Ip6Addr ) );

        if ( type == DistributionMessageType::TaskFailed )
        {
            std::cout << "Received Task Failure from " << sender << std::endl;

            // TODO: Consider if this is necessary. Maybe there should be a break in the workflow instead?
            if ( _onTaskFailure.has_value() )
            {
                _onTaskFailure.value()( *this, sender, functionId );
            }
            return;
        }

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

        int taskId = task->id();

        if ( type == DistributionMessageType::TaskAssignment )
        {
            std::cout << "Received Task Assignment from " << sender << " for task with ID " << taskId << std::endl;
            if ( !_functionRegistry.enqueueTask( _address, std::move( task ), fn.value().get().completionType() ) )
            {
                std::cout << "Failed to register task assignment for function " << functionId << std::endl;
            }
            return;
        }

        if ( type == DistributionMessageType::TaskResult )
        {
            std::cout << "Received result from " << sender << " for Task with ID " << taskId << std::endl;
            if ( task->status() == TaskStatus::RepeatDistributed )
            {
                if ( !_functionRegistry.enqueueTask( sender, std::move( task ), fn.value().get().completionType() ) )
                {
                    std::cout << "Failed to register task for repeat for function " << functionId << std::endl;
                    return;
                }
                
                if ( !_functionRegistry.enqueueTaskRequest( sender ) )
                {
                    std::cout << "Failed to enqueue task request." << std::endl;
                }

                return;
            }

            if ( !_functionRegistry.enqueueTaskResult( std::move( task ), sender ) )
            {
                std::cout << "Failed to persist result from task " << taskId << " for function " << functionId << std::endl;
            }
        }
    }

public:
    static const int DISTRIBUTION_PORT = 7071;

    // ToDo: Move pcb ownership.
    DistributedTaskManager(
        std::unique_ptr< ElectionProtocolBase > election,
        Ip6Addr& address,
        MessageDistributor* distributor,
        std::unique_ptr< udp_pcb > pcb) 
    : _address( address ),
      _election( std::move( election ),
        [ this ]( const Ip6Addr& leader) {
            onElectionSuccesful( leader );
        }),
      _messaging( address, DISTRIBUTION_PORT, distributor,
        [ this ]( Ip6Addr sender, DistributionMessageType messageType, uint8_t* data, unsigned int size )
        {
            onMessage( sender, messageType, data, size );
        },
        std::move( pcb ) ),
      _memoryService( distributor, _messaging.sender(), address ),
      _workFlowService( _messaging.sender(), _functionRegistry, _memoryService )
    {
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
        return _election.registerLeaderFailureCallback( callback );
    }

    bool unregisterLeaderFailureCallback()
    {
        return _election.unregisterLeaderFailureCallback();
    }

    DistributedMemoryService& memoryService()
    {
        return _memoryService;
    }

    void doWork()
    {
        if ( !_election.isRunning() )
        {
            std::cout << "The election protocol is not running!" << std::endl;
            return;
        }

        if ( !_election.isElectionComplete() )
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
        return _messaging.sender();
    }

    void start( int moduleId )
    {
        _election.start( moduleId );
    }

    void broadcastUnblockSignal()
    {
        _messaging.sender().broadcastMessage(DistributionMessageType::BlockingTaskRelease, METHOD_ID);
    }

    bool requestTask()
    {
        if ( _election.getLeader() == _address )
        {
            return false;
        }

        auto emptyTask = Task< int >( 0 );
        _messaging.sender().sendMessage( DistributionMessageType::TaskRequest, emptyTask, _election.getLeader() );
        return true;
    }

    /// @brief Registers a callback that is called when a task request is received.
    /// @param callback Your custom callback. Returns true if the task request pipeline should not continue after this callback (e.g. to avoid double-scheduling of a task)
    void registerTaskRequestCallback( std::function< bool(DistributedTaskManager& manager, rofi::net::Ip6Addr& requester ) > callback )
    {
        _onTaskRequest = callback;
    }

    void registerTaskFailedCallback( std::function< void(DistributedTaskManager& manager, rofi::net::Ip6Addr& sender, int functionId ) > callback )
    {
        _onTaskFailure = callback;
    }
};