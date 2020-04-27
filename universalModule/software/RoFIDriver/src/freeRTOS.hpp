#pragma once

#include <iostream>
#include <stdexcept>
#include <chrono>
#include <cstring>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/timers.h>
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
        if ( !_q.push( idx, c ) )
            std::cout << "Cannot push to queue! 1\n";
    }

    void push( T&& t, ExContext c = ExContext::Normal ) {
        IndexType idx = _idxs.pop( c );
        auto o = ObjectView< T >::create( object( idx ), std::move( t ) );
        o.release();
        if ( !_q.push( idx, c ) )
            std::cout << "Cannot push to queue! 2\n";
    }

    template < typename... Args >
    void emplace( Args&&... args ) {
        IndexType idx = _idxs.pop();
        auto o = ObjectView< T >::create( object( idx ), std::forward< Args >( args )... );
        o.release();
        if ( !_q.push( idx ) )
            std::cout << "Cannot push to queue! 3\n";
    }

    T pop( ExContext c = ExContext::Normal ) {
        IndexType idx = _q.pop( c );
        T t( std::move( ObjectView< T >::from( object( idx ) ).get() ) );
        memset( object( idx ), 0, sizeof( T ) );
        if ( !_idxs.push( idx, c ) )
            std::cout << "Cannot push to queue! 4\n";
        return t;
    }

private:
    void* object( IndexType i ) { return _objs + sizeof( T ) * i; }

    PrimitiveQueue< IndexType > _q, _idxs;
    uint8_t* _objs;
};

class Semaphore {
    class GiveGuard {
    public:
        GiveGuard( Semaphore& sem, ExContext c = ExContext::Normal )
            : _given( false ), _sem( sem ), _cont( c ) {}

        GiveGuard( const GiveGuard& ) = delete;
        GiveGuard& operator=( const GiveGuard& ) = delete;
        GiveGuard( GiveGuard&& ) = default;
        GiveGuard& operator=( GiveGuard&& ) = default;

        ~GiveGuard() {
            give();
        }

        void give() {
            if ( _given )
                return;
            _sem.give( _cont );
            _given = true;
        }
    private:
        bool _given;
        Semaphore& _sem;
        ExContext _cont;
    };
public:
    Semaphore( int max ) : _sem( xSemaphoreCreateCounting( max, max ) ) {
        if ( !_sem )
            throw std::runtime_error( "Cannot allocate" );
    }

    Semaphore( const Semaphore& ) = delete;
    Semaphore& operator=( const Semaphore& ) = delete;
    Semaphore( Semaphore&& o ) : _sem( o._sem ) { o._sem = nullptr; }
    Semaphore& operator=( Semaphore&& o ) { swap( o ); return *this; }

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

    GiveGuard giveGuard( ExContext c = ExContext::Normal ) {
        return GiveGuard( *this, c );
    }

    void swap( Semaphore& o ) {
        using std::swap;
        swap( _sem, o._sem );
    }

private:

    SemaphoreHandle_t _sem;
};

class IsrDeferrer {
    using Arg = void*;
    using Handler = void (*)( Arg );
public:
    IsrDeferrer( int size )
        : _q( xQueueCreate( size, sizeof( Handler ) + sizeof( Arg ) ) )
    {
        if ( !_q )
            throw std::runtime_error( "Cannot allocate queue" );
        auto res = xTaskCreate( _run, "IsrDeferrer", 2048, this, 15, nullptr );
        if ( res != pdPASS )
            throw std::runtime_error( "Cannot allocate task" );
    }
    IsrDeferrer( const IsrDeferrer& ) = delete;
    IsrDeferrer& operator=( const IsrDeferrer& ) = delete;
    IsrDeferrer( IsrDeferrer&& o ) : _q( o._q ) { o._q = nullptr; }
    IsrDeferrer& operator=( IsrDeferrer&& o ) { swap( o ); return *this; }

    ~IsrDeferrer() {
        if ( _q )
            vQueueDelete( _q );
    }

    void swap( IsrDeferrer& o ) {
        using std::swap;
        swap( _q, o._q );
    }

    void IRAM_ATTR isr( Arg a, Handler h ) {
        uint8_t data[ sizeof( Handler ) + sizeof( Arg ) ];
        *reinterpret_cast< Arg* >( data ) = a;
        *reinterpret_cast< Handler* >( data + sizeof( Arg ) ) = h;
        portBASE_TYPE higherPriorityTaskWoken = pdFALSE;
        xQueueSendToBackFromISR( _q, data, &higherPriorityTaskWoken );
        if( higherPriorityTaskWoken )
            portYIELD_FROM_ISR();
    }
private:
    static void _run(void* arg ) {
        auto* self = reinterpret_cast< IsrDeferrer* >( arg );
        while ( true ) {
            uint8_t data[ sizeof( Handler ) + sizeof( Arg ) ];
            xQueueReceive( self->_q, data, portMAX_DELAY );
            Arg& a = *reinterpret_cast< Arg* >( data );
            Handler& h = *reinterpret_cast< Handler* >( data + sizeof( Arg ) );
            (*h)( a );
        }
    }
    QueueHandle_t _q;
};

class Timer {
    using Routine = std::function< void() >;
public:
    enum class Type { OneShot, Periodic };

    Timer() : _t( nullptr ) {}
    Timer( std::chrono::milliseconds interval, Type t,  Routine r ) {
        auto* rp = new Routine( r );
        _t = xTimerCreate( "Timer name", pdMS_TO_TICKS( interval.count() ),
                           t == Type::Periodic, rp, _run );
    }

    Timer( const Timer& ) = delete;
    Timer& operator=( const Timer& ) = delete;
    Timer( Timer&& o ) : _t( o._t ) { o._t = nullptr; }
    Timer& operator=( Timer&& o ) { swap( o ); return *this; }

    ~Timer() {
        if ( !_t )
            return;
        auto routine = reinterpret_cast< Routine *>( pvTimerGetTimerID( _t ) );
        xTimerDelete( _t, portMAX_DELAY );
        delete routine;
    }

    void start() {
        xTimerStart( _t, portMAX_DELAY );
    }

    void stop() {
        if ( _t )
            xTimerStop( _t, portMAX_DELAY );
    }

    void release() {
        _t = nullptr;
    }

    void swap( Timer& o ) {
        using std::swap;
        swap( _t, o._t );
    }
private:
    static void _run( TimerHandle_t timer ) {
        auto routine = reinterpret_cast< Routine *>( pvTimerGetTimerID( timer ) );
        (*routine)();
    }

    TimerHandle_t _t;
};

} // namespace rtos