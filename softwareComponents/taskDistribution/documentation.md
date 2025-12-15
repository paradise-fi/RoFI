# Distributed RoFI Task Manager
The RoFI Task Manager is a tool intended to enable better distributed programming. The task manager is built on an election protocol and provides a centralized way to orchestration actions of the RoFIbot at runtime.

Within the system, there is one privileged module that we refer to the leader module. This module is chosen by an election round that takes place before any task distribution can happen. This module is intended to act as an orchestrator, and task distribution should only happen within this module.

<hr>

## Examples
To find out how to use the RoFI task manager, consult examples provided in ~RoFI/examples/simulator/

1. distributionSimple - A simple example that shows how to get started with the basic functionality of the task manager.
2. distributionMemory - An extension of the simple example. Shows how to use distributed memory that the task manager can provide.
3. distributionBarrier - An extension of distributionMemory which shows how to use a barrier within the task manager.
4. distributionComplex - A complex example that demonstrates many aspects of the task manager, including custom serialization of complex objects that are not trivially copyable, and movement of a RoFIBot orchestrated by the leader module.
5. distributionSerializable - A simple example showcasing how to serialize custom, not trivially copyable data to be sent over the task manager.
6. distributionBlockingTasks - A simple example for two modules that showcases blocking task semantics and priorities.
7. distributionBlockingMemory - A simple example showing off blocking read operations on a custom memory implementation.

<hr>

## Documentation
### Table of Contents

1. [DistributionTaskManager](#distributedtaskmanager)
2. [CallbackFacade](#callbackfacade)
3. [MemoryFacade](#memoryfacade)
4. [LoggingService](#loggingservice)
5. [DistributedFunction](#distributedfunction)
6. [FunctionHandle](#functionhandle)
7. [DistributedMemoryBase](#distributedmemorybase)
8. [LoggerBase](#loggerbase)
9. [Serializable](#serializable)

<hr>

### DistributedTaskManager
The ``DistributedTaskManager`` class is the primary entry point into the functionality provided by the RoFI Task Manager. This class acts as a facade for the rest of the system, and in most scenarios (except for memory), you will not need to directly interact with the system underneath the task manager.

#### Constructor
```c++
DistributedTaskManager(
        std::unique_ptr< ElectionProtocolBase > election,
        Ip6Addr& address,
        MessageDistributor& distributor,
        std::unique_ptr< udp_pcb > pcb,
        int blockingMessageTimeoutMs = 300 );
```
Constructs a distributed task manager instance. It is strongly recommended that only one DistributedTaskManager instance exists per module.

``election`` - a RoFI Network Manager protocol that inherits the ``ElectionProtocolBase`` class. Election is necessary for the task manager to function, as it requires a leader module to exist within the RoFIBot.

``address`` - The address of this module. Should be connected to any routing protocols that are running.

``distributor`` - The MessageDistributor network protocol running on the RoFI Network Manager, this is used by internal functionality to send out and receive multicast messages.

``pcb`` - A network pcb, used by the underlying network layers.

``blockingMessageTimeoutMs`` - Setting for the timeout of blocking messages within the task manager.

#### Methods

```c++
CallbackFacade& callbacks();
MemoryFacade memory();
FunctionFacade functionRegistry()
LoggingService& loggingService();
```

These methods provide direct access to some of the lower level subsystems that make up the task manager. 

- ``CallbackFacade`` provides a wrapper around callback management within the distributed task manager. It allows users to register callbacks to specified events.
- ``MemoryFacade`` provides a wrapper around custom memory implementations. It allows users to store and read data in the distributed system. It is important that this facade does not outlive the ``DistributedTaskManager``!
- ``FunctionFacade`` provides a wrapper around the function and task subsystem, allowing users to register functions, retrieve function handles and manipulate task queues in a limited manner. It is important that this facade does not outlive the ``DistributedTaskManager``!
- ``LoggingService`` is concerned with logging. You can use this service to send log messages from your application, in which case they will end up in the same channels as the system logging.


##### Standard Workflow
```c++
void doWork( unsigned int messageProcessingBatch = 5 );
```

Executes a single run of the standard workflow loop of the task manager. This method must be continuously invoked for the task manager to function properly.

The **messageProcessingBatch** argument is used to configure the **maximum** number of incoming messages that will be processed and dispatched to subsystems during a single iteration.

```c++
void start( int initialElectionDelay = 1, int electionCyclesBeforeStabilization = 3 )
```

Performs important initialization for the task manager. Namely, this method starts the election algorithm, which is necessary for the task manager to function correctly.
The arguments are used to modify the behaviour of the election algorithm (namely its initial delay in seconds before election starts) and the number of election "cycles" (leader liveness checks) that must pass before we consider the election stable and the distributed task manager can begin to do work.

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

##### SendCustomMessageBlocking
```c++
struct MessagingResult
{
    bool success;
    std::string statusMessage;
    std::vector< uint8_t > rawData;
};

MessagingResult sendCustomMessageBlocking( uint8_t* data, size_t dataSize, Ip6Addr& target )
```

Sends a custom message that stalls program execution until a response is received or the timeout window expires. This type of message can only be sent to a specific target module.

If an error has occured during sending or receiving of the message, success is set to false and the reason message can be found in ``statusMessage``.


##### Manual Task Request
```c++
bool requestTask();
```
This method allows for manual requests of task. This is typically performed as part of the task manager pipeline between leaders and followers, but manual requests may be needed when the user needs to manually start the pipeline of tasks again.

##### Cleanup
```c++
void cleanUp( bool cleanSchedulers = true, bool cleanMemory = false, bool cleanMessages = true );
```

Use this method to clean up selected parts of the task manager after leader failure.

<hr>

### CallbackFacade
Provides access to registration of callback mechanisms within the distributed task manager.

#### Callback Types
This is a list of all the callback types available within the task manager. Note that these usings are not actually present within the code and serve simply to group all available callbacks together in a convenient manner.
```c++
// onTaskRequestCallback
std::function< bool( DistributedTaskManager& manager, const rofi::hal::Ip6Addr& requester ) >;

// onTaskFailureCallback
std::function< void( DistributedTaskManager& manager, const rofi::hal::Ip6Addr& sender, const int functionId ) >;

// onCustomMessageCallback
std::function< void( DistributedTaskManager& manager, const rofi::hal::Ip6Addr& sender, uint8_t* data, const size_t size ) >;

// onCustomMessageBlockingCallback
std::function< MessagingResult( DistributedTaskManager& manager, const rofi::hal::Ip6Addr& sender, uint8_t* data, const size_t size ) >;
```

#### Leader Failure Callback
```c++
/// @return True if the registration succeeded.
virtual bool registerLeaderFailureCallback( std::function< void() >&& callback ) = 0;

/// @return True if the function was unregistered.
virtual bool unregisterLeaderFailureCallback() = 0;
```

These methods are used to register (unregister) a callback that is called in the event that a module detects its connection to the leader has been severed. This is best used for failure recovery purposes.

#### Task Request Callback
```c++
virtual void registerTaskRequestCallback( 
    std::function< bool( DistributedTaskManager& manager,
                         const rofi::hal::Ip6Addr& requester ) >&& callback ) = 0;
```

Used to register a callback for the event of a task request message being processed by the module. The boolean return value of the OnTaskRequestCallback function informs the distributed task manager about whether it should still continue with the task request pipeline, namely whether it should put the task into its task scheduler or not. Task scheduling is **not** performed if OnTaskRequestCallback returns **true**.

#### Task Failed Callback
```c++
virtual void registerTaskFailedCallback( 
    std::function< void( DistributedTaskManager& manager,
                         const rofi::hal::Ip6Addr& sender,
                         const int functionId ) >&& callback ) = 0;
```

Used to register a callback for the event of a task failure message being processed by the module. Note that this callback is invoked for exceptional failure, namely the event of a function not being registered at the executing module. This callback can be, for example, used to log the issue at the leader side and then perform a graceful shutdown of the whole system.

#### Custom Message Callbacks
```c++
virtual void registerNonBlockingCustomMessageCallback( 
    std::function< void( DistributedTaskManager& manager,
                         const rofi::hal::Ip6Addr& sender,
                         uint8_t* data,
                         const size_t size ) >&& callback ) = 0;

virtual void registerBlockingCustomMessageCallback( 
    std::function< MessagingResult( DistributedTaskManager& manager,
                                    const rofi::hal::Ip6Addr& sender,
                                    uint8_t* data,
                                    const size_t size ) >&& callback ) = 0;
```

These callbacks are used for non-blocking and blocking custom messages respectively. Custom messages serve as a mechanism for implementing a user's own type of messaging without having to handle low-level details of networking.

<hr>

### MemoryFacade
Provides access to methods that work with a distributed memory implementation.

#### Methods

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
bool saveData( T&& data, int address );
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
    bool requestRemoteRead;
    // Used when a remote read is required. Leave nullopt to send message to leader.
    std::optional< rofi::hal::Ip6Addr > readTarget;
    std::vector< uint8_t > rawData;

    template< SerializableOrTrivial T >
    T data();
}
```
The structure used for representation of the result of memory reading. If the result was succesfull, the ``success`` flag is set to true. To read the data within the memory read result, you should use the ``data()`` function.

Special attention should be paid to ``bool requestRemoteRead`` and ``std::optional< rofi::hal::Ip6Addr > readTarget`` as these two attributes are used to configure remote reading from memory. Should a Distributed Memory implementation need to read from a remote module, the read method should return requestRemoteRead as ``true`` and provide either the source of information in readTarget, or null if the message is to be sent to the leader who can then route the request to the data holder. Either way, if ``requestRemoteRead`` is set to true, the resulting read will be blocking. It will either return once the read is complete, or when the timeout window expires.

##### Memory Service Purge

```c++
void clearLocalMemory();
```

Removes all data from local memory.

##### Metadata Access Methods

```c++
MemoryReadResult readMetadata( int address, const std::string& key );
bool saveMetadata( int address, const std::string& key, uint8_t* metadata, std::size_t metadataSize );
void removeMetadata( int address, const std::string& key );
```

Functions for storing just the metadata related to an address within memory. Analogous to functions for storing data. 

Note that functions for storing data should generally encompass metadata as well.

##### Access to user-created memory implementation
```c++
std::optional< std::reference_wrapper< DistributedMemoryBase > > memory()
```

Used to fetch the DistributedMemoryBase implementation provided by the user. This method should only be used if a the user requires to retrieve the memory implementation for very specific workflows. This method should not be necessary for regular memory-related workflows as the internals of the task manager handle all the details.

<hr>

### FunctionFacade
Provides access to methods that allow for the registration of functions and task scheduler cleanup.

##### GetFunctionHandle
```c++
template< SerializableOrTrivial Result, SerializableOrTrivial... Arguments >
std::optional< FunctionHandle< Result, Arguments...> > getFunctionHandle( const std::string& functionName );

template< SerializableOrTrivial Result, SerializableOrTrivial... Arguments >
std::optional< FunctionHandle< Result, Arguments...> > getFunctionHandle( int functionId );
```

These functions allow the user to retrieve a handle to their custom-defined ``DistributedFunction``. The handle is a functional object which can be invoked with the ``()`` operator.

**IMPORTANT: The function handle should never be used outside of the leader module, as it can lead to unexpected and undefined behaviours within the task manager.**

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
Removes all tasks from all schedulers on this module. Also clears all pending task requests and task results.
This function is called as part of ``DistrbutedTaskManager::cleanUp()``.

##### Unblock Task Schedulers
```c++
void unblockTaskSchedulers( bool hardUnblock = false )
```

Used to unblock all task schedulers on the module from blocking tasks. The ``hardUnblock`` argument, if true, ensures that barrier tasks are also cleared.

<hr>

### LoggingService 
Provides a simple wrapper around a custom-defined logger, ensuring ease of use and managing internal logger status.

#### Verbosity
Verbosity is used for messages of the Information log level. You may set verbosity to limit the number of informative messages written to the logger.

```c++
enum class LogVerbosity
{
    Low = 1,
    Medium = 2,
    High = 3,
};
```

#### Methods

##### UseLogger
```c++
template< std::derived_from< LoggerBase > Logger >
void useLogger( const Logger& logger, LogVerbosity verbosity );
```

Registers a logger instance within the logging service. Only one logger instance can be registered at a time.
The verbosity level passed to this function configures the logging service to use that verbosity level for informational messages.

##### Logging

```c++
void logInfo( const std::string& message, LogVerbosity messageVerbosityLevel = LogVerbosity::Medium );
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
    Blocking, // Task waits for a general signal and its scheduler does not get unblocked until that signal is received
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
    Barrier, // Registers the function as a barrier function - no task, regardless of priority, put after this function into a scheduler will be queued before this task is cleared.
    Initial, // Registers the function as an initial function. There can only be one initial function.
};
```

**IMPORTANT!** Barrier functions also require ``FunctionCompletionType`` to be set to Blocking, otherwise they shall not be registered as barrier functions with schedulers. This is because the semantics ``FunctionCompletionType`` affect what happens *after* a task is chosen to leave the queue, while ``FunctionType::Barrier`` provides guarantees on priority-scheduling blocking.

<hr>

### FunctionHandle
The wrapper for ``DistributedFunction``. The ``FunctionHandle`` is a functional object that can be invoked by the user to begin the process of task scheduling for the given function invocation.

#### Methods
##### Invocation
```c++
bool operator()( const Ip6Addr& target, unsigned int priority, bool setTopPriority, std::tuple< Arguments... >&& arguments );
```
Used at the leader side. This call to the FunctionHandle queues the ``DistributedFunction`` execution for execution at the ``target`` follower.

- ``target`` - The module receiving the order to execute this ``DistributedFunction`` invocation.
- ``priority`` - The priority of the task. Higher priority tasks take are executed sooner than lower priority tasks. The maximum that can be set here is 100. Otherwise, the function will not be executed.
- ``setTopPriority`` - Setting this flag ignores the priority setting. This task will be given the highest priority in the scheduler at the time of scheduling.
- ``arguments`` - An ``std::tuple`` representing the arguments for the function invocation.

The function returns true if the queueing process succeeded. Note that this only informs you of the success to queue this ``DistributedFunction`` invocation task on the leader's side. 
**Important:** Modules are expected to work at a one-thread workflow loop. Attempting to execute functions under ``FunctionHandle`` from multiple threads may lead to undefined behaviour.

<hr>

### DistributedMemoryBase
The base abstract class for any distributed memory implementation.

#### Methods

##### Clear Memory
```c++
virtual void clear() = 0;
```
Instructs the memory implementation to completely clear its memory.

###### Memory Storage Behavior
```c++
enum MemoryStorageBehavior
{
    LocalFirstStorage,
    LeaderFirstStorage,
};

virtual MemoryStorageBehavior storageBehavior() const = 0;
```

The return value of this method informs the memory service that manages your memory implementation about how to proceed with data storage. 

``MemoryStorageBehavior::LocalFirstStorage`` ensures that data is first stored locally, and then sent to the leader for processing. Leader contact can be ommited by setting ``MemoryPropagationType`` in ``MemoryWriteResult`` to ``MemoryPropagationType::None``.
``MemoryStorageBehavior::LeaderFirstStorage`` ensures that data is first processed at the leader and does not touch the local data storage at all unless the leader instructs local memory changes.

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
    // Instructs the system that the memory storage pipeline should continue as normal.
    bool success;

    // Instructs the system to only write metadata.
    bool metadataOnly;

    // Instructs the system on how the data should be propagated.
    MemoryPropagationType propagationType;

    // Used only if the propagation is ONE_TARGET. If null and propagation is ONE_TARGET, this will target the leader.
    std::optional< Ip6Addr > propagationTarget;

    MemoryWriteResult() = default;

    MemoryWriteResult( bool success, bool stored, bool metadataOnly, MemoryPropagationType propagationType, Ip6Addr target )
    : success( success ), stored( stored ), metadataOnly( metadataOnly ), propagationType( propagationType ), propagationTarget( target ) {}

    MemoryWriteResult( bool success, bool stored, bool metadataOnly, MemoryPropagationType propagationType )
    : success( success ), stored( stored ), metadataOnly( metadataOnly ), propagationType( propagationType ) {}
};
```

This structure represents the result of writing into memory. It informs the upper layer (``DistributedMemoryService``) of the parameters of the write. This includes whether the write was succesful, whether it was a write of only metadata, what the ``PropagationType`` of this write operation should be, and if the target is only a single module, which module should receive this message. This information is used at the leader module to determine the further actions performed by ``MemoryService``.

##### Data Reading
```c++
virtual MemoryReadResult readData( int address, bool isLeader ) const = 0;
virtual MemoryReadResult readMetadata( int address, const std::string& key, bool isLeader) const = 0;
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