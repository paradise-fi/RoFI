#include "taskManager.hpp"
#include "functionManager.hpp"
#include "LRElect.hpp"
#include "../memory/sharedMemoryBase.hpp"
#include "../memory/replicatedMemory.hpp"
#include "../messageService/messageService.hpp"
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
    std::unique_ptr< MessageService > _messageService;
    std::unique_ptr< SharedMemoryBase > _memory;
    
    int _elected_count = 0;

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
                _messageService->sendMessage( METHOD_ID, DistributionMessageType::TaskRequest, emptyTask, _election.getLeader() );
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

        auto task = _task_manager.popTask(requester, true);
        if ( !task.has_value() )
        {
            auto initial = _task_manager.getInitialTask();
            if ( !initial.has_value() )
            {
                std::cout << "Task distribution failed: No initial task given.";
                return;
            }
            _messageService->sendMessage( METHOD_ID, DistributionMessageType::TaskAssignment, initial.value(), requester );
            return;
        }
        _messageService->sendMessage( METHOD_ID, DistributionMessageType::TaskAssignment, task.value().get(), requester );
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
            _messageService->sendMessage( METHOD_ID, DistributionMessageType::TaskFailed, task.get(), _election.getLeader() );
            _task_manager.finishActiveTask( _address );
            return;
        }

        _messageService->sendMessage( METHOD_ID, DistributionMessageType::TaskResult, task.get(), _election.getLeader() );
        _task_manager.finishActiveTask( _address );
    }

    void onMessage( Ip6Addr sender, DistributionMessageType messageType, uint8_t* data, size_t size )
    {
        std::cout << "Received message from " << sender << std::endl;
        if ( messageType == DistributionMessageType::BlockingTaskRelease )
        {
            _task_manager.unblockSchedulers();
            return;
        }

        if ( messageType == DistributionMessageType::TaskRequest )
        {
            std::cout << "Received Task Request from" << sender << std::endl;
            _taskRequests.push( sender );
            return;
        }

        if ( messageType == DistributionMessageType::TaskFailed )
        {
            std::cout << "Received Task Failure from " << sender << std::endl;
            // How to react?
            return;
        }

        if ( messageType == DistributionMessageType::DataStorageRequest )
        {
            std::cout << "Received Data Storage Request from " << sender << std::endl;
            if (_memory != nullptr)
            {
                return _memory->onStorageMessage( sender, data, size );
            }
            // TODO: Exception if memory is nullptr?
        }

        int functionId = as< int >( data );

        std::optional<std::reference_wrapper<FunctionConcept>> fn = _function_manager.getFunction( functionId );

        if (!fn)
        {
            std::cout << "Function with ID " << functionId << " not found." << std::endl;
            return;
        }

        auto task = getTaskFromBuffer( data + sizeof( int ), functionId );
        
        if ( task == nullptr )
        {
            // Something went wrong.
            return;
        }

        if ( messageType == DistributionMessageType::TaskAssignment )
        {
            std::cout << "Received Task Assignment from " << sender << " for task with ID " << task->id() << std::endl;
            _task_manager.enqueueTask( _address, std::move( task ), fn.value().get().getCompletionType() );
            return;
        }

        if ( messageType == DistributionMessageType::TaskResult )
        {
            std::cout << "Received result from " << sender << " for Task with ID " << task->id() << std::endl;
            if ( !_function_manager.invokeReaction( sender, *task.get() ) )
            {
                std::cout << "Error invoking reaction for function " << task->functionId() << std::endl;
            }
        }
    }
public:
    static const int DISTRIBUTION_PORT = 7071;

    DistributionManager(NetworkManager& netmg, Ip6Addr& address, MessageDistributor* distributor, std::unique_ptr< udp_pcb > pcb, int port) 
    : _net_manager( netmg ), _address( address ),
      _taskRequests( boost::lockfree::queue< ip6_addr_t >( 1024 ) ),
      _election( netmg, distributor, address, 5,
        [ this ] {
            onLeaderElected();
        },
        [ this ] {
            onLeaderFailed();
        } ),
      _messageService( std::make_unique< MessageService >( address, port, distributor, std::move( pcb ) ) )
    {
        _messageService->registerOnMessageCallback( 
            METHOD_ID,
            [&]( Ip6Addr sender, DistributionMessageType messageType, uint8_t* data, size_t size ) 
            {
                onMessage( sender, messageType, data, size );
            });
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
    bool registerFunction( int id, 
        FunctionType< Result, Arguments... > function, 
        ReactionType< Result, Arguments... > reaction,
        CompletionType completionType = CompletionType::NonBlocking )
    {
        return _function_manager.addFunction< Result, Arguments... >( id, function, reaction, completionType );
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
    bool pushTask( const Ip6Addr& addr, int functionId, int priority, bool enqueueFront, std::tuple< Arguments... >&& arguments )
    {
        auto fn = _function_manager.getFunction( functionId );

        if (!fn)
        {
            return false;
        }

        bool result = _task_manager.enqueueTask< Result >( addr, functionId, priority, enqueueFront, fn.value().get().getCompletionType(), std::move( arguments ) );
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

    MessageService& messageService()
    {
        return *(_messageService.get());
    }
    
    void broadcastUnblockSignal()
    {
        _messageService->broadcastMessage( DistributionMessageType::BlockingTaskRelease, METHOD_ID );
    }

};