#pragma once

#include <condition_variable>  // std::condition_variable
#include <cstddef>  // std::size_t
#include <mutex>  // std::mutex
#include <optional>  // std::optional
#include <queue>  // std::queue
#include <utility>  // std::forward


template < typename T, std::size_t CAPACITY >
class BoundedQueue {
public:
    void enqueue( T&& t ) {
        std::lock_guard< std::mutex > lock( _mtx );
        if ( _queue.size() >= CAPACITY ) {
            _queue.pop();
        }
        _queue.push( std::forward< T >( t ) );
        _cv.notify_one();
    }

    template< typename... Args >
    void emplace( Args&&... args ) {
        std::lock_guard< std::mutex > lock( _mtx );
        if ( _queue.size() >= CAPACITY ) {
            _queue.pop();
        }
        _queue.emplace( std::forward< Args >( args )... );
        _cv.notify_one();
    }

    T dequeue() {
        std::unique_lock< std::mutex > lock( _mtx );
        while( _queue.empty() ) {
            _cv.wait( lock );
        }

        T val = std::move( _queue.front() );
        _queue.pop();

        return val;
    }

    std::optional< T > dequeue( std::stop_token stoken ) {
        std::unique_lock< std::mutex > lock( _mtx );
        if ( _cv.wait( lock, stoken, [ this ] { return ! _queue.empty(); } ) ) {
            std::optional< T > val = { std::move( _queue.front() ) };
            _queue.pop();
            return val;
        }

        return {};
    }

    template < typename Rep, typename Period >
    std::optional< T > dequeue( std::stop_token stoken, const std::chrono::duration< Rep, Period >& timeout ) {
        std::unique_lock< std::mutex > lock( _mtx );
        if ( _cv.wait_for( lock, stoken, timeout, [ this ] { return ! _queue.empty(); } ) ) {
            std::optional< T > val = { std::move( _queue.front() ) };
            _queue.pop();
            return val;
        }

        return {};
    }

    std::size_t size() {
        std::lock_guard< std::mutex > lock( _mtx );
        return _queue.size();
    }

private:
    std::queue< T > _queue{};
    mutable std::mutex _mtx{};
    std::condition_variable_any _cv{};
};
