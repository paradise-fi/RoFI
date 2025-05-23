#include "messageSender.hpp"
#include "taskManager.hpp"
#include "functionManager.hpp"
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
    static const int DISTRIBUTION_PORT = 7071;

    TaskManager _task_manager;
    FunctionManager _function_manager;
    NetworkManager& _net_manager;
    MessageSender _sender;
    std::unique_ptr< SharedMemoryBase > _memory;
    Ip6Addr _address;
    LRElect _election;

    boost::lockfree::queue< ip6_addr_t > _taskRequests;

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

    std::unique_ptr< TaskBase > getTaskFromBuffer(PBuf& buffer, int functionId, int start)
    {
        auto fnOptional = _function_manager.getFunction( functionId );

        if ( !fnOptional.has_value() )
        {
            // Something is wrong!
            return nullptr;
        }

        auto fun = fnOptional.value();
        auto task = fun.get().createTask();
        task->fillFromBuffer(buffer, start);
        return task;
    }

    void onLeaderElected()
    {
        if ( _elected_count == 3 )
        {
            std::cout << "Election stabilized." << std::endl;
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
            return;
        }

        Ip6Addr requester(1);
        if (!_taskRequests.pop( requester ))
        {
            return;
        }

        auto task = _task_manager.popTask(requester);
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
        _sender.sendMessage( DistributionMessageType::TaskAssignment, *(task.value().get()), requester );
    }

    void doWorkFollower()
    {
        auto taskCandidate = _task_manager.popTask( _address );

        if ( !taskCandidate.has_value() )
        {
            return;
        }

        auto task = std::move( taskCandidate.value() );
        
        if  ( !_function_manager.invokeFunction( *task.get() ) )
        {
            _sender.sendMessage( DistributionMessageType::TaskFailed, *task.get(), _election.getLeader() );
            return;
        }

        _sender.sendMessage( DistributionMessageType::TaskResult, *task.get(), _election.getLeader() );
    }

public:
    DistributionManager(NetworkManager& netmg, Ip6Addr& address, MessageDistributor* distributor) 
    : _net_manager( netmg ), _address( address ),
      _taskRequests( boost::lockfree::queue< ip6_addr_t >( 1024 ) ),
      _election( netmg, distributor, address, 5,
        [ this ] {
            onLeaderElected();
        },
        [ this ] {
            onLeaderFailed();
        } ),
      _sender( MessageSender( address, DISTRIBUTION_PORT ) )
    { 
        LOCK_TCPIP_CORE();
        _pcb = std::make_unique< udp_pcb >( *udp_new() );
        UNLOCK_TCPIP_CORE();

        if ( !_pcb )
        {
            std::cout << "PCB Null" << std::endl;
        }

        LOCK_TCPIP_CORE();
        udp_bind( _pcb.get(), IP6_ADDR_ANY, DISTRIBUTION_PORT );
        udp_recv( _pcb.get(), recv_message, this);
        UNLOCK_TCPIP_CORE();

        _sender.setPcb( _pcb.get() );
    }

    // TODO: Add possibility to use any memory manager.
    bool useDistributedMemory(MessageDistributor* distributor)
    {
        _memory = std::make_unique< ReplicatedMemoryManager >( _net_manager, distributor, _address, _sender );
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
    bool registerFunction( int id, 
        FunctionType< Result, Arguments... > function, 
        ReactionType< Result, Arguments... > reaction )
    {
        return _function_manager.addFunction< Result, Arguments... >( id, function, reaction );
    }

    bool unregisterFunction( int id )
    {
        return _function_manager.removeFunction( id );
    }

    /// @brief Pushes task into manager. This task is to be distributed to another module.
    /// @tparam Result The type of the task result.
    /// @tparam ...Arguments The types of task arguments. 
    /// @param addr The address of the receiver / delegate module.
    /// @param functionId The ID of the function to be invoked at receiver.
    /// @param arguments The arguments for the function.
    /// @return True if the push was succesful. Otherwise false.
    template< typename Result, typename... Arguments >
    bool pushTask( const Ip6Addr& addr, int functionId, std::tuple< Arguments... >&& arguments )
    {
        bool result = _task_manager.enqueueTask< Result >( addr, functionId, std::move( arguments ) );
        _taskRequests.push(addr);
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
        const ip6_addr_t* addr, 
        u16_t )
    {
        if ( !p )
        {
            return;
        }
        
        auto packet = rofi::hal::PBuf::own( p );
        DistributionMessageType type = static_cast< DistributionMessageType >( packet[ 0 ] );
        Ip6Addr sender = as< Ip6Addr >( packet.payload() + sizeof( DistributionMessageType ) );

        if ( type == DistributionMessageType::TaskRequest )
        {
            std::cout << "Received Task Request from" << sender << std::endl;
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
                return _memory->onStorageMessage( sender, packet.payload(), sizeof( DistributionMessageType ) + Ip6Addr::size(), packet.size() );
            }
            // TODO: Exception if memory is nullptr?
        }

        
        int functionId = as< int >( packet.payload() + sizeof( DistributionMessageType ) + sizeof( Ip6Addr ) ); //static_cast< int >( packet[ sizeof( DistributionMessageType ) ] );

        auto task = getTaskFromBuffer(
            packet, 
            functionId, 
            sizeof( DistributionMessageType ) + sizeof( int ) + sizeof( Ip6Addr ) );
        
        if ( task == nullptr )
        {
            // Something went wrong.
            return;
        }

        if ( type == DistributionMessageType::TaskAssignment )
        {
            std::cout << "Received Task Assignment from " << sender << " for task with ID " << task->id() << std::endl;
            _task_manager.enqueueTask( _address, std::move( task ) );
            return;
        }

        if ( type == DistributionMessageType::TaskResult )
        {
            const TaskBase& test = *task.get();
            std::cout << "Received result from " << sender << " for Task with ID " << task->id() << std::endl;
            if ( !_function_manager.invokeReaction( sender, *task.get() ) )
            {
                std::cout << "Error invoking reaction for function " << task->functionId() << std::endl;
            }
        }
    }

};