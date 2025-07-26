#pragma once

#include "messageSender.hpp"
#include "../tasks/taskManager.hpp"
#include "../functions/functionManager.hpp"
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

    TaskManager _task_manager;
    FunctionManager _function_manager;
    NetworkManager& _net_manager;
    Ip6Addr _address;

    boost::lockfree::queue< ip6_addr_t > _taskRequests;

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

    std::unique_ptr< TaskBase > getTaskFromBuffer( uint8_t* buffer, int functionId )
    {
        auto fnOptional = _function_manager.getFunction( functionId );

        if ( !fnOptional.has_value() )
        {
            // Something is wrong!
            return nullptr;
        }

        auto fun = fnOptional.value();
        auto task = fun.get().createTask();
        task->fillFromBuffer( buffer );
        return task;
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
                _task_manager.clearTasks();
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

    void doWorkLeader()
    {
        if ( _taskRequests.empty() )
        {
            std::cout << "No task requests" << std::endl;
            return;
        }

        Ip6Addr requester(1);
        if (!_taskRequests.pop( requester ))
        {
            std::cout << "No requester for task found." << std::endl;
            return;
        }

        auto task = _task_manager.popTask(requester, true);
        if ( !task.has_value() )
        {
            auto initial = _task_manager.getInitialTask();
            if ( !initial.has_value() )
            {
                std::cout << "Task distribution failed: No initial task given.";
                return;
            }
            _sender.sendMessage( DistributionMessageType::TaskAssignment, initial.value(), requester );
            return;
        }

        _sender.sendMessage( DistributionMessageType::TaskAssignment, task.value().get(), requester );
    }

    void doWorkFollower()
    {
        auto taskCandidate = _task_manager.popTask( _address );

        if ( !taskCandidate.has_value() )
        {
            return;
        }

        auto task = std::move( taskCandidate.value() );
        
        if  ( !_function_manager.invokeFunction( task.get() ) )
        {
            _sender.sendMessage( DistributionMessageType::TaskFailed, task.get(), _election.getLeader() );
            _task_manager.finishActiveTask( _address );
            return;
        }

        _sender.sendMessage( DistributionMessageType::TaskResult, task.get(), _election.getLeader() );
        _task_manager.finishActiveTask( _address );
    }

public:
    static const int DISTRIBUTION_PORT = 7071;

    DistributionManager(NetworkManager& netmg, Ip6Addr& address, MessageDistributor* distributor, std::unique_ptr< udp_pcb > pcb) 
    : _net_manager( netmg ), _address( address ),
      _taskRequests( boost::lockfree::queue< ip6_addr_t >( 1024 ) ),
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

    template < typename Result, typename... Arguments >
    bool registerFunction( std::unique_ptr< DistributedFunction< Result, Arguments... > > userFunction,
        CompletionType completionType = CompletionType::NonBlocking )
    {
        return _function_manager.addFunction< Result, Arguments... >( std::move( userFunction ), completionType );
    }

    bool unregisterFunction( int id )
    {
        return _function_manager.removeFunction( id );
    }

    template< typename Result, typename... Arguments >
    bool executeFunction( const Ip6Addr& executorAddress, int priority, bool giveTopPriority, int functionId, std::tuple< Arguments... >&& arguments )
    {
        auto fn = _function_manager.getFunction( functionId );

        if ( !fn.has_value() )
        {
            return false;
        }

        bool result = _task_manager.enqueueTask< Result >( executorAddress, functionId, priority,
            giveTopPriority, fn.value().get().completionType(), std::move( arguments ) );

        if ( result )
        {
            _taskRequests.push( executorAddress );
        }

        return result;
    }

    template< typename Result, typename... Arguments >
    bool executeFunction( const Ip6Addr& executorAddress, int priority, bool giveTopPriority, std::string functionName, std::tuple< Arguments... >&& arguments )
    {    
        auto fn = _function_manager.getFunction( functionName );

        if ( !fn.has_value() )
        {
            return false;
        }
        
        bool result = _task_manager.enqueueTask< Result >( executorAddress, fn.value().get().functionId(), priority,
            giveTopPriority, fn.value().get().completionType(), std::move( arguments ) );

        if ( result )
        {
            _taskRequests.push( executorAddress );
        }

        return result;
    }

    /*
    * Takes existing functionId and creates a task for it that is given at initialization to every requesting module.
    */
    bool setInitialTask( int functionId )
    {
        auto fn = _function_manager.getFunction( functionId );
        if ( !fn.has_value() )
        {
            return false;
        }
        
        return _task_manager.setInitialTask( fn.value() );
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

        // Add one more field for taskId

        if ( type == DistributionMessageType::BlockingTaskRelease )
        {
            _task_manager.unblockSchedulers();
            return;
        }

        if ( type == DistributionMessageType::TaskRequest )
        {
            std::cout << "Received Task Request from " << sender << std::endl;
            _taskRequests.push( sender );
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
            if (_memory != nullptr)
            {
                unsigned int offset = sizeof( DistributionMessageType ) + Ip6Addr::size();
                return _memory->onStorageMessage( sender, packet.payload() + offset, packet.size() - offset );
            }
            // TODO: Exception if memory is nullptr?
        }

        if ( type == DistributionMessageType::DataStorageSuccess )
        {
            std::cout << "Data storage succeeded." << std::endl;
            return;
        }

        int functionId = as< int >( packet.payload() + sizeof( DistributionMessageType ) + sizeof( Ip6Addr ) );

        std::optional<std::reference_wrapper< FunctionConcept > > fn = _function_manager.getFunction( functionId );

        if (!fn)
        {
            std::cout << "Function with ID " << functionId << " not found." << std::endl;
            return;
        }

        auto task = getTaskFromBuffer(
            packet.payload() + sizeof( DistributionMessageType ) + sizeof( Ip6Addr ), 
            functionId );

        if ( task == nullptr )
        {
            // Something went wrong.
            return;
        }

        if ( type == DistributionMessageType::TaskAssignment )
        {
            std::cout << "Received Task Assignment from " << sender << " for task with ID " << task->id() << std::endl;
            _task_manager.enqueueTask( _address, std::move( task ), fn.value().get().completionType() );
            return;
        }

        if ( type == DistributionMessageType::TaskResult )
        {
            std::cout << "Received result from " << sender << " for Task with ID " << task->id() << std::endl;
            if ( !_function_manager.invokeReaction( sender, *task.get() ) )
            {
                std::cout << "Error invoking reaction for function " << task->functionId() << std::endl;
            }
        }
    }

    MessageSender& getSender()
    {
        return _sender;
    }

    void BroadcastUnblockSignal()
    {
        _sender.broadcastMessage(DistributionMessageType::BlockingTaskRelease, METHOD_ID);
    }

};