#pragma once

#include <iostream>
#include <stdexcept>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <typeinfo>

namespace rtos {

/* View memory as an object */
template < typename T >
class ObjectView {
public:
    ObjectView( const ObjectView& ) = delete;
    ObjectView& operator=( const ObjectView& ) = delete;
    ObjectView( ObjectView&& o ): _o( o._o ) { o._o = nullptr; }
    ObjectView& operator=( ObjectView&& o ) { swap( o ); return *this; }
    ~ObjectView() { if ( _o )  _o->~T(); }

    static ObjectView from( void *p ) {
        return ObjectView( p );
    }

    template < typename... Args >
    static ObjectView create( void* p, Args&&... args ) {
        ObjectView o( p );
        new ( o._o ) T( std::forward< Args >( args )... );
        return o;
    }

    void swap( ObjectView& o ) {
        using std::swap;
        swap( _o, o._o );
    }

    T& get() { return *_o; }
    operator bool() const { return _o; }
    void release() { _o = nullptr; }

private:
    ObjectView( void* p): _o( reinterpret_cast< T* >( p ) ) { }

    T* _o;
};

enum class ExContext { Normal, ISR };

template < typename T >
class PrimitiveQueue {
public:
    PrimitiveQueue( int size )
        : _q( xQueueCreate( size, sizeof( T ) ) )
    {
        if ( !_q )
            throw std::runtime_error( "Cannot allocate queue" );
    }
    PrimitiveQueue( const PrimitiveQueue& ) = delete;
    PrimitiveQueue& operator=( const PrimitiveQueue& ) = delete;
    PrimitiveQueue( PrimitiveQueue&& o ) : _q( o._q ) { o._q = nullptr; }
    PrimitiveQueue& operator=( PrimitiveQueue&& o ) { swap( o ); return *this; }

    ~PrimitiveQueue() {
        if ( _q )
            vQueueDelete( _q );
    }

    void swap( PrimitiveQueue& o ) {
        using std::swap;
        swap( _q, o._q );
    }

    int freeSpace() const { return uxQueueSpacesAvailable( _q ); }
    bool push( const T& t, ExContext c = ExContext::Normal ) {
        if ( c == ExContext::ISR )
            return xQueueSendToBackFromISR( _q, &t, nullptr );
        return xQueueSendToBack( _q, &t, portMAX_DELAY ) == pdTRUE;
    }

    T pop( ExContext c = ExContext::Normal ) {
        T t;
        if ( c == ExContext::ISR )
            return xQueueReceiveFromISR( _q, &t, nullptr );
        else
            xQueueReceive( _q, &t, portMAX_DELAY );
        return t;
    }

private:
    QueueHandle_t _q;
};

template < typename T >
class Queue {
public:
    using IndexType = int;
    Queue( int size )
        : _q( size ), _idxs( size ),
          _objs( new uint8_t[ sizeof( T ) * size ] )
    {
        for ( IndexType i = 0; i != size; i++ )
            _idxs.push( i );
    }
    Queue( const Queue& ) = delete;
    Queue& operator=( const Queue& ) = delete;
    Queue( Queue&& o ):
        _q( std::move( o._q ) ),
        _idxs( std::move( o._idxs ) ),
        _objs( o._objs )
    {
        o._objs = nullptr;
    }
    Queue& operator=( Queue&& o ) { swap( o ); return *this; }

    ~Queue() {
        if ( _objs )
            delete _objs;
    }

    void swap( Queue& o ) {
        using std::swap;
        swap( _q, o._q );
        swap( _idxs, o._idxs );
        swap( _objs, o._objs );
    }

    int freeSpace() const { return _q.freeSpace(); }

    void push( const T& t, ExContext c = ExContext::Normal ) {
        IndexType idx = _idxs.pop( c );
        auto o = ObjectView< T >::create( object( idx ), t );
        o.release();
        _q.push( idx, c );
    }

    void push( T&& t, ExContext c = ExContext::Normal ) {
        IndexType idx = _idxs.pop( c );
        auto o = ObjectView< T >::create( object( idx ), std::move( t ) );
        o.release();
        _q.push( idx, c );
    }

    template < typename... Args >
    void emplace( Args&&... args ) {
        IndexType idx = _idxs.pop();
        auto o = ObjectView< T >::create( object( idx ), std::forward< Args >( args )... );
        o.release();
        _q.push( idx );
    }

    T pop( ExContext c = ExContext::Normal ) {
        IndexType idx = _q.pop( c );
        T t( std::move( ObjectView< T >::from( object( idx ) ).get() ) );
        _idxs.push( idx, c );
        return t;
    }

private:
    void* object( IndexType i ) { return _objs + sizeof( T ) * i; }

    PrimitiveQueue< IndexType > _q, _idxs;
    uint8_t* _objs;
};

class Semaphore {
public:
    Semaphore( int max ) : _sem( xSemaphoreCreateCounting( max, max ) ) {
        if ( !_sem )
            throw std::runtime_error( "Cannot allocate" );
    }

    bool tryTake( ExContext c = ExContext::Normal ) {
        if ( c == ExContext::ISR )
            return xSemaphoreTakeFromISR( _sem, nullptr ) == pdTRUE;
        return xSemaphoreTake( _sem, 0 ) == pdTRUE;
    }

    bool take() {
        return xSemaphoreTake( _sem, portMAX_DELAY ) == pdTRUE;
    }

    bool give( ExContext c = ExContext::Normal ) {
        if ( c == ExContext::ISR )
            return xSemaphoreGiveFromISR( _sem, nullptr ) == pdTRUE;
        return xSemaphoreGive( _sem ) == pdTRUE;
    }

    bool count() const {
        return uxSemaphoreGetCount( _sem );
    }
private:
    SemaphoreHandle_t _sem;
};
} // namespace rtos