#pragma once
using namespace rofi::hal;
using namespace rofi::net;

#include <lwip++.hpp>
#include <lwip/udp.h>
#include <chrono>
#include <LRElect.hpp>
#include <atoms/units.hpp>
#include <vector>
#include <variant>

enum TaskStatus {
    Enqueued, // Still in Queue 
    InProgress, // Has been sent out
    Complete, // Returned with a success
    Failed, // Returned with a failure
    TimedOut // Timed Out.
};

class TaskBase 
{
public:
    virtual ~TaskBase() = default;

    virtual int id() const = 0;
    virtual size_t size() = 0;
    virtual int functionId() const = 0;
    virtual void setStatus( TaskStatus status ) = 0;

    virtual void copyToBuffer( uint8_t* buffer ) = 0;
    virtual void fillFromBuffer( const uint8_t* buffer ) = 0;

    virtual int getPriority() const = 0;
    virtual int getEffectivePriority() const = 0;
    virtual void incrementAge() = 0;
    virtual void setPriority( int priority ) = 0;

    virtual bool isQueuedToFront() = 0;
};

template < typename Result, typename... Arguments >
class Task : public TaskBase {
    int _id;
    TaskStatus _status;
    int _age = 0;
    int _func_id;
    int _priority;
    std::optional< Result > _result;
    bool _enqueueFront;
    std::tuple< Arguments... > _args;
    

    template< typename Arg >
    Arg readArgument( const uint8_t* buffer )
    {
        Arg argument;
        int agg = as< int  >( buffer );
        std::memcpy(&argument, buffer, sizeof( Arg ) );
        buffer += sizeof( Arg );
        return argument;
    }

    template< typename Arg >
    void writeArgument( uint8_t* buffer, unsigned int& idx, const Arg& arg )
    {
        std::memcpy( buffer + idx, &arg, sizeof( Arg ) );
        int agg = as< int >( buffer + idx );
        idx += sizeof( Arg );
    }

    public:
    Task() {}
    Task( int functionId ) : _func_id( functionId ) {
        _status = TaskStatus::InProgress;
        _id = 0;
    }
    Task( int id, TaskStatus status, int functionId, int priority, bool enqueueFront )
        : _id( id ), _status( status ), _func_id( functionId ), _priority( priority ), _enqueueFront( enqueueFront ) {}
    Task( int id, TaskStatus status, int functionId, int result, int priority, bool enqueueFront )
        : _id( id ), _status( status ), _func_id( functionId ), _result( result ), _priority( priority ), _enqueueFront( enqueueFront ) {}
    Task( int id, TaskStatus status, int functionId, int priority, bool enqueueFront, std::tuple< Arguments... > args)
        : _id( id ), _status( status ), _func_id( functionId ), _priority( priority ), _enqueueFront( enqueueFront ), _args( args ) {}

    int id() const override { return _id; }

    template< typename ArgTuple >
    constexpr std::size_t argumentsSize( const ArgTuple& ) {
        using T = std::remove_cvref_t<ArgTuple>;

        return []<std::size_t... Is>(std::index_sequence<Is...>) {
            return ( 0 + ... + sizeof(std::tuple_element_t<Is, T>) );
        }(std::make_index_sequence<std::tuple_size_v<T>>{});
    }

    virtual size_t size() override {
        using T = decltype( _args );

        return sizeof( _id ) + sizeof( _priority ) 
             + sizeof( _status ) + sizeof ( _func_id ) 
             + 2 * sizeof( bool ) + sizeof ( Result ) 
             + argumentsSize( _args );
    };

    virtual void copyToBuffer( uint8_t* buffer ) override
    {
        unsigned int idx = 0;
        as< int >( buffer + idx ) = _func_id;
        idx += sizeof( _func_id );
        as< int >( buffer + idx ) = _id;
        idx += sizeof( _id );
        as< int >( buffer + idx ) = _priority;
        idx += sizeof( _priority );
        as< bool >( buffer + idx ) = _enqueueFront;
        idx += sizeof( bool );
        as< TaskStatus >( buffer + idx ) = _status;
        idx +=  sizeof( TaskStatus );
        bool hasValue = _result.has_value();
        as< bool >( buffer + idx ) = hasValue;
        idx += sizeof( bool );
        
        if ( _result.has_value() )
        {
            Result resultValue = _result.value();
            as< Result >( buffer + idx ) = resultValue;
            std::memcpy( buffer + idx, &resultValue, sizeof( Result ) );
        }

        idx += sizeof( Result );

        std::apply(
            [&]( Arguments&... args )
            {
                ( writeArgument( buffer, idx, args), ...);
            }, 
            _args
        );
    }

    virtual void fillFromBuffer( const uint8_t* buffer ) override
    {
        unsigned int idx = 0;
        _func_id = as< int >( buffer + idx );
        idx += sizeof( int );
        _id = as< int >( buffer + idx );
        idx += sizeof( int );
        _priority = as< int >( buffer + idx );
        idx += sizeof( int );
        _enqueueFront = as< bool >( buffer + idx );
        idx += sizeof( bool );
        _status = as< TaskStatus >( buffer + idx );
        idx += sizeof ( TaskStatus );
        bool hasValue = as< bool >( buffer + idx );
        idx += sizeof( bool );

        if ( hasValue )
        {
            Result value = as< Result >( buffer + idx );
            _result = std::make_optional< Result >( value );
        }
        else
        {
            _result.reset();
        }

        idx += sizeof( Result );
        _args = std::tuple< Arguments... >( readArgument< Arguments >( buffer + idx )... );
    } 
    
    int functionId() const override { return _func_id; }

    std::optional< Result > result() const { return _result; }

    TaskStatus status() const { return _status; }

    const std::tuple< Arguments... >& arguments() const { return _args; }

    void setResult( std::optional< Result > result ) { _result = result; }

    void setStatus( TaskStatus status ) { _status = status; }

    bool operator< (const Task& rhs) { return _id < rhs.id(); }

    bool operator== (const Task& rhs) { return _id == rhs.id(); }
    
    virtual int getPriority() const override { return _priority; }

    virtual int getEffectivePriority() const override { return _priority + _age; }
    
    virtual void incrementAge() override { _age++; }

    virtual void setPriority( int priority ) override { _priority = priority; }

    virtual bool isQueuedToFront() override { return _enqueueFront; }
};