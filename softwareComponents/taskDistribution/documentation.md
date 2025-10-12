# Distributed RoFI Task Manager
The RoFI Task Manager is a tool to enable better distributed programming. The task manager is built on an election protocol and provides a centralized way to orchestration actions of the RoFIbot at runtime.

<hr>

## Examples
To find out how to use the RoFI task manager, consult examples provided in ~RoFI/examples/simulator/

1. distributionSimple - A simple example that shows how to get started with the basic functionality of the task manager.
2. distributionMemory - An extension of the simple example. Shows how to use distributed memory that the task manager can provide.
3. distributionBarrier - An extension of distributionMemory which shows how to use a barrier within the task manager.
4. distributionComplex - A complex example that demonstrates many aspects of the task manager, including custom serialization of complex objects that are not trivially copyable, and movement of a RoFIBot orchestrated by the leader module.
5. distributionSerializable - A simple example showcasing how to serialize custom, not trivially copyable data to be sent over the task manager.

<hr>

## Documentation
### Table of Contents

1. [DistributionTaskManager](#distributedtaskmanager)
2. [DistributionMemoryService](#distributedmemoryservice)
3. [LoggingService](#loggingservice)
4. [DistributedFunction](#distributedfunction)
5. [FunctionHandle](#functionhandle)
6. [DistributedMemoryBase](#distributedmemorybase)
7. [LoggerBase](#loggerbase)
8. [Serializable](#serializable)

<hr>

### DistributedTaskManager
The ``DistributedTaskManager`` class is the primary entry point into the functionality provided by the RoFI Task Manager. This class acts as a facade for the rest of the system, and in most scenarios (except for memory), you will not need to directly interact with the system underneath the task manager.

#### Constructor
```c++
DistributedTaskManager(
        std::unique_ptr< ElectionProtocolBase > election,
        Ip6Addr& address,
        MessageDistributor* distributor,
        std::unique_ptr< udp_pcb > pcb) 
```
Constructs a distributed task manager instance. It is strongly recommended that only one DistributedTaskManager instance exists per module.

``election`` - a RoFI Network Manager protocol that inherits the ``ElectionProtocolBase`` class. Election is necessary for the task manager to function, as it requires a leader module to exist within the RoFIBot.

``address`` - The address of this module. Should be connected to any routing protocols that are running.

``distributor`` - The MessageDistributor network protocol running on the RoFI Network Manager.

``pcb`` - A network pcb, used by the underlying network layers.

#### Methods

##### Logger Registration
```c++
template< std::derived_from< LoggerBase > Logger >
void useLogger( const Logger& logger )
```

Registers a logger instance. The logger instance can be custom made, but it must inherit the Logger abstract class. When a logger is provided, the task manager will automatically write messages for different events, warnings and errors into the logger.

##### Event Callbacks
```c++
bool registerLeaderFailureCallback( std::function< void() > callback )
bool unregisterLeaderFailureCallback()
```

Registers (and unregisters) a callback function for the event of leader failure (disconnect, crash, etc.). This is especially useful for failure recovery mechanisms.

```c++
void registerTaskRequestCallback( std::function< bool(DistributedTaskManager& manager, rofi::net::Ip6Addr& requester ) > callback )
void unregisterTaskRequestCallback()
```

Registers (and unregisters) a callback function for the event of task requests from follower modules. This allows for custom task request handling code. The callback should return true if the task request pipeline should be terminated after this callback.

```c++
void registerTaskFailedCallback( std::function< void(DistributedTaskManager& manager, rofi::net::Ip6Addr& sender, int functionId ) > callback )
void unregisterTaskFailedCallback()
```

Registers (and unregisters) a callback function for the event of a system failure when executing a function. This type of failure is not the same failure indicated by your own function defined in ``DistributedFunction``, but rather a failure of system configuration. For example, this could mean that a function was not registered on one of the modules, and thus the task manager failed to execute the function on that module.

```c++
void registerCustomMessageCallback( std::function< void( DistributedTaskManager& manager, rofi::net::Ip6Addr& sender, uint8_t* dataBuffer, unsigned int bufferSize ) > callback )
void unregisterCustomMessageCallback() {}
```

Registers (and unregisters) a callback function for the receiving of [custom messages](#sendcustommessage). Custom messages allow the user to implement and use their own messaging scheme within the task manager system. The user can also achieve this by using the lower level messaging API built over lwIP - this is a feature of convenience.

The data buffer received into the callback contains only the body of the message, headers are already parsed out at this point.

```c++
DistributedMemoryService& memoryService();
LoggingService& loggingService();
```

These methods provide direct access to some of the lower level subsystems that make up the task manager. 

- ``DistributedMemoryService`` provides a wrapper around custom memory implementations. It allows users to store and read data in the distributed system.
- ``LoggingService`` is concerned with logging. You can use this service to send log messages from your application, in which case they will end up in the same channels as the system logging.


##### Standard Workflow
```c++
void doWork();
```

Executes a single run of the standard workflow loop of the task manager. This method must be continuously invoked for the task manager to function properly.

```c++
void start( int moduleId );
```

Performs important initialization for the task manager. Namely, this method starts the election algorithm, which is necessary for the task manager to function correctly.

##### Unblock Queue Signals
```c++
void broadcastUnblockSignal();
void sendUnblockSignal( Ip6Addr& receiver );
```

Methods used in synchronization workflows. These methods allow users to unblock queues on other modules, allowing for the implementation mechanisms such as barriers.

```c++
std::optional< Ip6Addr > getLeader();
```

Returns the address of the current leader if the election is finished. Otherwise ``std::nullopt`` is returned.

##### SendCustomMessage

```c++
void sendCustomMessage( uint8_t* data, unsigned int dataSize, std::optional< Ip6Addr > target );
```
Sends a custom message to the target module. If ``target`` is ``std::nullopt``, the message is sent to all modules.

##### Manual Task Distribution
```c++
template< SerializableOrTrivial Result, SerializableOrTrivial... Arguments > 
bool sendFunctionExecutionOrder( int functionId, const Ip6Addr& target, int priority, bool setTopPriority, std::tuple< Arguments... >&& arguments );

template< SerializableOrTrivial Result, SerializableOrTrivial... Arguments > 
bool sendFunctionExecutionOrder( std::string functionName, const Ip6Addr& target, int priority, bool setTopPriority, std::tuple< Arguments... >&& arguments );

bool requestTask();
```
These methods allow for manual requests of task executions and task scheduling. These actions are typically performed as part of the task manager pipeline between leaders and followers, but they are needed when the user needs to manually start the pipeline of tasks again.

##### GetFunctionHandle
```c++
template< SerializableOrTrivial Result, SerializableOrTrivial... Arguments >
std::optional< FunctionHandle< Result, Arguments...> > getFunctionHandle( const std::string& functionName );

template< SerializableOrTrivial Result, SerializableOrTrivial... Arguments >
std::optional< FunctionHandle< Result, Arguments...> > getFunctionHandle( int functionId );
```

These functions allow the user to retrieve a handle to their custom-defined ``DistributedFunction``. The handle is a functional object which can be invoked with the ``()`` operator.

##### Register Function

```c++
template< SerializableOrTrivial Result, SerializableOrTrivial... Arguments,
              std::derived_from< DistributedFunction< Result, Arguments... > > Func >
bool registerFunction( const Func& function );
```

Registers a custom defined ``DistributedFunction`` into the task manager.

##### ClearAllTasks
```c++
void clearAllTasks();
```
Used to clear all tasks from the queues on this module. Can be useful for failure recovery.

<hr>

### DistributedMemoryService
Provides access to methods that work with a distributed memory implementation.

The ``DistributedMemoryService`` is created internally by the task manager. The constructor will thus not be discussed.

#### Methods

##### Memory Storage Callback
```c++
void registerOnMemoryStored( std::function< void( int memoryAddress, bool isLeaderMemory, DistributedMemoryService& memoryService ) > onMemoryStoredCb );
```

Registers a callback for the event that memory has been succesfully stored at this module.

##### Memory Implementation Manipulation Methods

```c++
template< std::derived_from< SharedMemoryBase > Memory >
bool useMemory( std::unique_ptr< Memory > memory );

bool isMemoryRegistered();

bool deleteMemory();
```

Methods for registering a memory implementation, checking whether a memory implementation is registered, and deregistering a memory implementation respectivelly. 

Any custom memory implementation must be derived from ``SharedMemoryBase``.

##### Data Access Methods

```c++
template < SerializableOrTrivial T >
bool saveData( T data, int address );
```

Used by the user to save data to the shared memory. Returns true if the data was stored succesfully, otherwise false.

If this module is a follower module, this method simply sends out a request for data storage to the leader and the actual data storage in this module is performed some time after the function finishes execution.


```c++
void removeData( int address );
```
Attempts to remove data from memory.

If this module is a follower module, this method simply sends out a request for data removal to the leader and the actual data removal in this module is performed some time after the function finishes execution.

```c++
MemoryReadResult readData( int address );
```

Attempts to read data from memory. If data is not read, the MemoryReadResult struct will have ``success`` set to false.


###### MemoryReadResult
```c++
struct MemoryReadResult
{
    bool success;
    std::vector< uint8_t > rawData;

    template< SerializableOrTrivial T >
    T data();
}
```
The structure used for representation of the result of memory reading. If the result was succesfull, the ``success`` flag is set to true. To read the data within the memory read result, you should use the ``data()`` function.

##### Memory Service Purge

```c++
void clearLocalMemory();
```

Removes all data from local memory.

```c++
void clearLocalQueue();
```
Removes all entries in the storage queue - all pending memory changes.

##### Metadata Access Methods

```c++
MemoryReadResult readMetadata( int address, const std::string& key );
bool saveMetadata( int address, const std::string& key, uint8_t* metadata, std::size_t metadataSize );
void removeMetadata( int address, const std::string& key );
```

Functions for storing just the metadata related to an address within memory. Analogous to functions for storing data. 

Note that functions for storing data should generally encompass metadata as well.

##### Internal Methods
```c++
void onStorageMessage( Ip6Addr sender, uint8_t* data, size_t size, bool isDeleteMessage );
```

**This method is used within the internal workflow of the task manager.** It is likely not useful to the end user. It is used for the processing of incoming memory-related messages that are passed to this service from the ``DistributedTaskManager`` instance that manages this memory service.

```c++
void processQueue();
```

**This method is used within the internal workflow of the task manager.** Calling the ``DistributedTaskManager.doWork()`` method already invokes this method. This method processes any pending memory-altering operation requests.

```c++
void setLeader( Ip6Addr leader );
```

**This method is used within the internal workflow of the task manager.** This method informs the task manager of who the current leader module is. You should generally not use this method.


<hr>

### LoggingService 
Provides a simple wrapper around a custom-defined logger, ensuring ease of use and managing internal logger status.

#### Methods

##### UseLogger
```c++
template< std::derived_from< LoggerBase > Logger >
void useLogger( const Logger& logger );
```

Registers a logger instance within the logging service. Only one logger instance can be registered at a time.

##### Logging

```c++
void logInfo( const std::string& message );
void logWarning( const std::string& message );
void logError( const std::string& message );
```

Instructs the underlying logger implementation to write log messages with the given log levels (Info, Warning, Error). It is up to the lower level implementation to handle the representation of these messages and their output.

<hr>

### DistributedFunction
The base abstract class for the development of user-defined functions within the RoFI task manager. A ``DistributedFunction`` implementation must be defined and registered on each module for it to work correctly.

<hr>

#### FunctionResult
The FunctionResult structure carries data about the execution of the function through the system.

##### Constructor
```c++
FunctionResult( std::optional< Result > value, FunctionResultType resultType );
```

##### Members
```c++
/// @brief The result value of the function.
std::optional< Result > result;
    
/// @brief Denotes whether the function is considered as a success.
FunctionResultType resultType;
```

##### FunctionResultType
```c++
enum FunctionResultType
{
    SUCCESS,
    FAILURE,
    TRY_AGAIN,
    TRY_AGAIN_LOCAL,
};
```

The ``SUCCESS`` or ``FAILURE`` values represent a natural flow of execution. The leader is then able to react to these values with functions defined in the DistributedFunction code.

The ``TRY_AGAIN`` value informs the leader that the task must be rescheduled again on the leader side.

The ``TRY_AGAIN_LOCAL`` value simply reschedules the task on the module's side. The task is placed at the front of the queue (result queue if on leader side), and will be invoked again. This can be, for example, useful for situations where busy wait is desireable.

<hr>

#### Methods

##### Execute
```c++
virtual FunctionResult< Result > execute( Arguments... args ) = 0;
```
The ``execute`` function is invoked on the follower module. It represents the primary function execution. For the return type semantics, see [FunctionResult](#functionresult).

**IMPORANT:** It is generally a bad idea to perform busy waiting in this function. Instead, the ``FunctionResultType::TRY_AGAIN_LOCAL`` should be used and the task manager loop should be allowed to cycle as normal.

##### OnFunctionSuccess
```c++
virtual bool onFunctionSuccess( std::optional< Result > result, const Ip6Addr& origin ) = 0;
```

The ``onFunctionSuccess`` method is invoked on the leader side. It is invoked when the leader receives a function result with result type set to ``FunctionResultType::SUCCESS``.

This method is generally used for processing the result and deciding what tasks the origin module should perform next.

The return value of this function indicates whether the function result processing should be attempted again. If the function returns true, the result is placed into an internal result queue again, and will be processed again immediately.

**IMPORANT:** It is generally a bad idea to perform busy waiting in this function. Instead, the function should return ``true`` and the task manager loop should be allowed to cycle as normal.

##### OnFunctionFailure
```c++
virtual bool onFunctionFailure( std::optional< Result > result, const Ip6Addr& origin ) = 0;
```

The ``onFunctionFailure`` method is invoked on the leader side. It is invoked when the leader receives a function result with result type set to ``FunctionResultType::FAILURE``.

This method is generally used for processing the possible result and deciding what tasks the origin module should perform next, or if the origin module should attempt this task again.

The return value of this function indicates whether the function result processing should be attempted again. If the function returns true, the result is placed into an internal result queue again, and will be processed again immediately.

**IMPORANT:** It is generally a bad idea to perform busy waiting in this function. Instead, the function should return ``true`` and the task manager loop should be allowed to cycle as normal.

##### Metadata Methods
```c++
virtual std::string functionName() const = 0;
virtual int functionId() const = 0;
virtual FunctionCompletionType completionType() const = 0;
virtual FunctionDistributionType distributionType() const = 0;
virtual FunctionType functionType() const = 0;
```

These functions are used as metadata for the task manager. The ``functionName()`` and ``functionId()`` functions must provide unique representation of the function.

The other three methods determine how the function is handled within the scheduling, distribution and registration pipelines of the task manager.

```c++
enum FunctionCompletionType
{
    Blocking, // Task waits for a general signal and does not get unblocked by a response from the leader
    NonBlocking, // Task does not wait.
};
```

```c++
enum FunctionDistributionType
{
    Unicast, // Function tasks are only sent to a specific module address
    Broadcast, // Function tasks are distributed to all modules at the same time
};
```

```c++
enum FunctionType
{
    Regular, // Registers the function as a regular function
    Barrier, // Registers the function as a barrier function
    Initial, // Registers the function as an initial function. There can only be one initial function.
};
```

<hr>

### FunctionHandle
The wrapper for ``DistributedFunction``. The ``FunctionHandle`` is a functional object that can be invoked by the user to begin the process of task scheduling for the given function invocation.

#### Methods
##### Invocation
```c++
bool operator()( const Ip6Addr& target, int priority, bool setTopPriority, std::tuple< Arguments... >&& arguments );
```
Used at the leader side. This call to the FunctionHandle queues the ``DistributedFunction`` execution for execution at the ``target`` follower.

- ``target`` - The module receiving the order to execute this ``DistributedFunction`` invocation.
- ``priority`` - The priority of the task. Higher priority tasks take are executed sooner than lower priority tasks.
- ``setTopPriority`` - Setting this flag ignores the priority setting. This task will be given the highest priority in the scheduler at the time of scheduling.
- ``arguments`` - An ``std::tuple`` representing the arguments for the function invocation.

The function returns true if the queueing process succeeded. Note that this only informs you of the success to queue this ``DistributedFunction`` invocation task on the leader's side. 

<hr>

### DistributedMemoryBase
The base abstract class for any distributed memory implementation.

#### Methods

##### Clear Memory
```c++
virtual void clear() = 0;
```
Instructs the memory implementation to completely clear its memory.

##### Data Writing
```c++
virtual MemoryWriteResult removeData( int address, bool isLeader ) = 0;
        
virtual MemoryWriteResult writeData( uint8_t* data, size_t size, int address, bool isLeader ) = 0;

virtual MemoryWriteResult writeMetadata( int address, const std::string& key, uint8_t* metadata, size_t metadataSize, bool isLeader ) = 0;

virtual MemoryWriteResult removeMetadata( int address, const std::string& key, bool isLeader ) = 0;
```

These functions are for writing into the data and metadata of the implementation. The return structure for these functions is used to instruct the ``MemoryService`` on how the memory should be propagated from the leader, what type of data was sent, and the success of the write operation.

###### MemoryPropagationType
```c++
enum MemoryPropagationType
{
    NONE,
    ONE_TARGET,
    SEND_TO_ALL,
};
```

Denotes the type of propagation for the memory entry. The entry either does not propagate at all (``NONE``), propagates to a single module (``ONE_TARGET``), or propagates via broadcast to all modules (``SEND_TO_ALL``).
###### MemoryWriteResult
```c++
struct MemoryWriteResult
{
    bool success;

    // Instructs the system to only write metadata.
    bool metadataOnly;

    // Instructs the system on how the data should be propagated.
    MemoryPropagationType propagationType;

    // Used only if the propagation is ONE_TARGET.
    std::optional< Ip6Addr > propagationTarget;

    MemoryWriteResult();
    MemoryWriteResult( bool success, bool metadataOnly, MemoryPropagationType propagationType, Ip6Addr target );
    MemoryWriteResult( bool success, bool metadataOnly, MemoryPropagationType propagationType );
};
```

This structure represents the result of writing into memory. It informs the upper layer (``DistributedMemoryService``) of the parameters of the write. This includes whether the write was succesful, whether it was a write of only metadata, what the ``PropagationType`` of this write operation should be, and if the target is only a single module, which module should receive this message. This information is used at the leader module to determine the further actions performed by ``MemoryService``.

##### Data Reading
```c++
virtual MemoryReadResult readData( int address ) const = 0;
virtual MemoryReadResult readMetadata( int address, const std::string& key) const = 0;
```

These methods serve for reading data from the memory implementation. For the return value, see [MemoryReadResult](#memoryreadresult)

##### Implementation-specific Serialization
```c++
virtual std::vector< uint8_t > serializeDataToMemoryFormat( uint8_t* data, size_t dataSize, int address, bool isMetadataOnly = false ) const = 0;
```

This function is used to serialize data for transport to other modules. This is because implementations may require different formats of data, and the memory service has no way of knowing how to format the data.

The function receives raw data to be stored in the ``data`` buffer, and its size in ``dataSize``. The purpose of this method is to append any possible metadata that may be required for the functioning of the memory. The ``address`` argument can be used for the retrieval of already existing metadata in the event that the data is being overwritten, otherwise this argument can be ignored as the address will be sent over the network as part of the memory storage header. Finally, the ``isMetadataOnly`` flag informs whether the data being stored is only metadata or not.

<hr>

### LoggerBase
This class is the base abstract class for any custom logger implementation that can be used within the RoFI task manager.

#### Methods
```c++
virtual void logInfo( const std::string& message ) = 0;
virtual void logWarning( const std::string& message ) = 0;
virtual void logError( const std::string& message ) = 0;
```

These methods are used for logging their respective log message levels.

<hr>

### Serializable
The abstract ``Serializable`` struct is used for the ``SerializableOrTrivial`` concept for universal serialization within the task manager task distribution workflow.

```c++
template< typename T >
concept SerializableOrTrivial = std::is_base_of_v< Serializable, T > || std::is_trivially_copyable_v< T >;
```

#### Methods

##### Serialize
```c++
virtual void serialize( uint8_t*& buffer ) const = 0;
```

Used to Serialize the data into the given buffer. The buffer will be of size atleast equal to the value returned by the ``size()`` function of ``Serializable``.

##### Deserialize

```c++
virtual void deserialize( const uint8_t*& buffer ) = 0;
```

Used to deserialize the data from a given buffer into this struct. The buffer wil be of size at least equal to the value returned by the ``size()`` function of ``Serializable``.

##### Size

```c++
virtual std::size_t size() const = 0;
```

The size that the data buffer for serialization of the data in this structure must have. Typically this includes sizes of individual items and some additional metadata, such as the size of a vector stored in the ``Serializable`` struct.