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
    virtual size_t size() const = 0;
    virtual int functionId() const = 0;
    virtual void setStatus( TaskStatus status ) = 0;

    virtual void copyToBuffer( PBuf& buffer, size_t start ) = 0;
    virtual void fillFromBuffer( const PBuf& buffer, size_t start ) = 0;
    
};

template < typename Result, typename... Arguments >
class Task : public TaskBase {
    int _id;
    TaskStatus _status;

    int _func_id;
    std::optional< Result > _result;
    std::tuple< Arguments... > _args;

    template< typename Arg >
    Arg readArgument( const uint8_t* buffer )
    {
        Arg argument;
        std::memcpy(&argument, buffer, sizeof( Arg ) );
        buffer += sizeof( Arg );
        return argument;
    }

    public:
    Task() {}
    Task( int functionId ) : _func_id( functionId ) {
        _status = TaskStatus::InProgress;
        _id = 0;
    }
    Task( int id, TaskStatus status, int functionId )
        : _id( id ), _status( status ), _func_id( functionId ) {}
    Task( int id, TaskStatus status, int functionId, int result )
        : _id( id ), _status( status ), _func_id( functionId ), _result( result ) {}
    Task( int id, TaskStatus status, int functionId, std::tuple< Arguments... > args)
        : _id( id ), _status( status ), _func_id( functionId ), _args( args ) {}

    int id() const override { return _id; }

    virtual size_t size() const override {
        using T = decltype( _args );
        return sizeof( _id ) + sizeof( _status ) + sizeof ( _func_id ) + sizeof( bool ) + sizeof ( Result ) + std::tuple_size< T >{};
    };

    virtual void copyToBuffer( PBuf& buffer, size_t idx ) override
    {
        auto internalBuff = buffer.payload();
        as< int >( internalBuff + idx ) = _func_id;
        idx += sizeof( _func_id );
        as< int >( internalBuff + idx ) = _id;
        idx += sizeof( _id );
        as< TaskStatus >( internalBuff + idx ) = _status;
        idx +=  sizeof( TaskStatus );
        bool hasValue = _result.has_value();
        as< bool >( internalBuff + idx ) = hasValue;
        idx += sizeof( bool );
        if ( _result.has_value() )
        {
            Result resultValue = _result.value();
            as< Result >( internalBuff + idx ) = resultValue;
            std::memcpy( internalBuff + idx, &resultValue, sizeof( Result ) );
        }
        idx += sizeof( Result );

        std::apply(
            [&]( Arguments&... args )
            {
                ((std::memcpy( internalBuff + idx, &args, sizeof( args ) ),
                idx += sizeof( args )), ...);
            }, 
            _args
        );
    }

    virtual void fillFromBuffer( const PBuf& buffer, size_t idx ) override
    {
        auto* internalBuff = buffer.payload();
        _id = as< int >( internalBuff + idx );
        idx += sizeof( int );
        _status = as< TaskStatus >( internalBuff + idx );
        idx += sizeof ( TaskStatus );
        bool hasValue = as< bool >( internalBuff + idx );
        idx += sizeof( bool );
        if ( hasValue )
        {
            Result value = as< Result >( internalBuff + idx );
            _result = std::make_optional< Result >( value );
        }
        else
        {
            _result.reset();
        }
        idx += sizeof( Result );
        _args = std::tuple< Arguments... >( readArgument< Arguments >( internalBuff )... );
    } 
    
    int functionId() const override { return _func_id; }

    std::optional< Result > result() const { return _result; }

    TaskStatus status() const { return _status; }

    const std::tuple< Arguments... >& arguments() const { return _args; }

    void setResult( Result result ) { _result = std::optional< Result >( result ); }

    void setStatus( TaskStatus status ) { _status = status; }

    bool operator< (const Task& rhs)
    {
        return _id < rhs.id();
    }

    bool operator== (const Task& rhs)
    {
        return _id == rhs.id();
    }
    
};