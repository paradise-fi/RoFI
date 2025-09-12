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
#include "serializable.hpp"

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
    virtual TaskStatus status() const = 0;

    virtual void copyToBuffer( uint8_t* buffer ) = 0;
    virtual void fillFromBuffer( const uint8_t* buffer ) = 0;

    virtual int getPriority() const = 0;
    virtual int getEffectivePriority() const = 0;
    virtual void incrementAge() = 0;
    virtual void setPriority( int priority ) = 0;

    virtual bool isQueuedToFront() = 0;
};

template < SerializableOrTrivial Result, SerializableOrTrivial... Arguments >
class Task : public TaskBase {
    int _id;
    TaskStatus _status;
    int _age = 0;
    int _func_id;
    int _priority;
    std::optional< Result > _result;
    bool _enqueueFront;
    std::tuple< Arguments... > _args;

    template< SerializableOrTrivial T >
    T readFromBuffer( const uint8_t*& buffer )
    {
        T argument;
        if constexpr ( std::is_base_of_v< Serializable, T > )
        {
            argument.deserialize( buffer );
            return argument;
        }
        else
        {
            std::memcpy(&argument, buffer, sizeof( T ) );
            buffer += sizeof( T );   
            return argument;
        }   
    }

    template< SerializableOrTrivial T >
    void writeToBuffer( uint8_t*& buffer, const T& value )
    {
        if constexpr( std::is_base_of_v< Serializable, T > )
        {
            value.serialize( buffer );
        }
        else
        {
            std::memcpy( buffer, &value, sizeof( T ) );
            buffer += sizeof( T );
        }
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

    template< SerializableOrTrivial T >
    std::size_t getSerializableOrTrivialSize( const T& argument )
    {
        if constexpr ( std::is_base_of_v< Serializable, T > )
        {
            return argument.size();
        }

        return sizeof( std::decay_t< T > );
    }

    template< SerializableOrTrivial... ArgTuple >
    std::size_t calculateArgumentsSize( std::tuple< ArgTuple... >& tuple)
    {
        if constexpr ( sizeof...( ArgTuple ) == 0 )
        {
            return 0;
        }

        return std::apply
        (
            [ & ]( ArgTuple const&... arg )
            {
                // Fold expression -> iterate over all of ArgTuple and sum the sizes together together.
                return ( 0 + ... + getSerializableOrTrivialSize( arg ) );
            },
            tuple
        );
    }

    virtual size_t size() override {
        size_t size =  sizeof( _id ) + sizeof( _priority ) 
             + sizeof( _status ) + sizeof ( _func_id ) 
             + 2 * sizeof( bool ) + sizeof ( Result ) 
             + calculateArgumentsSize( _args );

        if constexpr( !std::is_void_v< Result > )
        {
            if constexpr ( std::is_base_of_v< Serializable, Result > )
            {
                return size + _result.size();
            }
        }

        return size + sizeof( Result );
    };

    virtual void copyToBuffer( uint8_t* buffer ) override
    {
        writeToBuffer( buffer, _func_id );
        writeToBuffer( buffer, _id );
        writeToBuffer( buffer, _priority );
        writeToBuffer( buffer, _enqueueFront );
        writeToBuffer( buffer, _status );
        writeToBuffer( buffer, _result.has_value() );
        // unsigned int idx = 0;
        // as< int >( buffer + idx ) = _func_id;
        // idx += sizeof( _func_id );
        // as< int >( buffer + idx ) = _id;
        // idx += sizeof( _id );
        // as< int >( buffer + idx ) = _priority;
        // idx += sizeof( _priority );
        // as< bool >( buffer + idx ) = _enqueueFront;
        // idx += sizeof( bool );
        // as< TaskStatus >( buffer + idx ) = _status;
        // idx +=  sizeof( TaskStatus );
        // bool hasValue = _result.has_value();
        // as< bool >( buffer + idx ) = hasValue;
        // idx += sizeof( bool );
        
        if ( _result.has_value() )
        {
            // Parse using proper serialization
            // Result sresultValue = _result.value();
            writeToBuffer( buffer, _result.value() );
            // as< Result >( buffer + idx ) = resultValue;
            // std::memcpy( buffer + idx, &resultValue, sizeof( Result ) );
        }

        // idx += sizeof( Result );

        std::apply(
            [&]( Arguments&... args )
            {
                ( writeToBuffer( buffer, args ), ... );
            }, 
            _args
        );
    }

    virtual void fillFromBuffer( const uint8_t* buffer ) override
    {
        _func_id = readFromBuffer< int >( buffer );
        _id = readFromBuffer< int >( buffer );
        _priority = readFromBuffer< int >( buffer );
        _enqueueFront = readFromBuffer< bool >( buffer );
        _status = readFromBuffer< TaskStatus >( buffer );
        bool hasValue = readFromBuffer< bool >( buffer );
        // unsigned int idx = 0;
        // _func_id = as< int >( buffer + idx );
        // idx += sizeof( int );
        // _id = as< int >( buffer + idx );
        // idx += sizeof( int );
        // _priority = as< int >( buffer + idx );
        // idx += sizeof( int );
        // _enqueueFront = as< bool >( buffer + idx );
        // idx += sizeof( bool );
        // _status = as< TaskStatus >( buffer + idx );
        // idx += sizeof ( TaskStatus );
        // bool hasValue = as< bool >( buffer + idx );
        // idx += sizeof( bool );

        if ( hasValue )
        {
            // Parse using proper serialization
            // Result value = as< Result >( buffer + idx );
            // _result = std::make_optional< Result >( value );
            _result = std::make_optional< Result >( readFromBuffer< Result >( buffer ) );
        }
        else
        {
            _result.reset();
        }

        // idx += sizeof( Result );
        _args = std::tuple< Arguments... >( readFromBuffer< Arguments >( buffer )... );
    } 
    
    int functionId() const override { return _func_id; }

    std::optional< Result > result() const { return _result; }

    virtual TaskStatus status() const override { return _status; }

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