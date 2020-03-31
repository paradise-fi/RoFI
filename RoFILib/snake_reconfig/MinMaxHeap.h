#pragma once

#include <iostream>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <vector>

template<typename T, typename Comp>
class MinMaxHeap {
public:
    static_assert(std::is_invocable_r_v<bool, Comp, T, T>);

    MinMaxHeap(int cap)
    : _max_heap()
    , _min_heap()
    , _odd()
    , _capacity(0)
    , _occupied(0)
    , _comp() {
        resize(cap);
    };

    const T& max() const {
        if (_occupied == 0)
            throw std::logic_error("Empty heap");

        if (!_odd || _comp(_odd.value(), _max_heap[0]))
            return _max_heap[0];

        return _odd.value();
    }

    const T& min() const {
        if (_occupied == 0)
            throw std::logic_error("Empty heap");

        if (!_odd || !_comp(_odd.value(), _min_heap[0]))
            return _min_heap[0];

        return _odd.value();
    }

    T popMax() {
        if (_occupied == 0)
            throw std::logic_error("Empty heap");

        if (_occupied == 1) {
            --_occupied;
            T ret_val = std::move(_odd.value());
            _odd = std::nullopt;
            return std::move(ret_val);
        }
        if (!_odd) {
            T ret_val = std::move(_max_heap[0]);
            _odd = std::make_optional(std::move(_min_heap[_last()]));
            if (_occupied != 2) {
                _max_heap[0] = std::move(_max_heap[_last()]);
                --_occupied;
                _heapify_max_down(0);
            } else {
                --_occupied;
            }
            return std::move(ret_val);
        }
        if (_comp(_max_heap[0], _odd.value())) {
            --_occupied;
            T ret_val = std::move(_odd.value());
            _odd = std::nullopt;
            return std::move(ret_val);
        }
        T ret_val = std::move(_max_heap[0]);
        if (_occupied == 3) {
            --_occupied;
            _max_heap[0] = std::move(_odd.value());
            _odd = std::nullopt;
            return std::move(ret_val);
        }
        if (_comp(_max_heap[_last()], _odd.value())) {
            _max_heap[0] = std::move(_odd.value());
            --_occupied;
            _heapify_max_down(0);
        } else {
            _max_heap[0] = std::move(_max_heap[_last()]);
            _max_heap[_last()] = std::move(_odd.value());
            --_occupied;
            _heapify_max_down(0);
            if (_comp(_max_heap[_last()], _min_heap[_last()])) {
                std::swap(_max_heap[_last()], _min_heap[_last()]);
                _heapify_min_up(_last());
            }
        }
        _odd = std::nullopt;
        return std::move(ret_val);
    }

    T popMin() {
        if (_occupied == 0)
            throw std::logic_error("Empty heap");

        if (_occupied == 1) {
            --_occupied;
            T ret_val = std::move(_odd.value());
            _odd = std::nullopt;
            return std::move(ret_val);
        }
        if (!_odd) {
            T ret_val = std::move(_min_heap[0]);
            _odd = std::make_optional(std::move(_max_heap[_last()]));
            if (_occupied != 2) {
                _min_heap[0] = std::move(_min_heap[_last()]);
                --_occupied;
                _heapify_min_down(0);
            } else {
                --_occupied;
            }
            return std::move(ret_val);
        }
        if (_comp(_odd.value(), _min_heap[0])) {
            --_occupied;
            T ret_val = std::move(_odd.value());
            _odd = std::nullopt;
            return std::move(ret_val);
        }
        T ret_val = std::move(_min_heap[0]);
        if (_occupied == 3) {
            --_occupied;
            _min_heap[0] = std::move(_odd.value());
            _odd = std::nullopt;
            return std::move(ret_val);
        }
        if (_comp(_odd.value(), _min_heap[_last()])) {
            _min_heap[0] = std::move(_odd.value());
            --_occupied;
            _heapify_min_down(0);
        } else {
            _min_heap[0] = std::move(_min_heap[_last()]);
            _min_heap[_last()] = std::move(_odd.value());
            --_occupied;
            _heapify_min_down(0);
            if (_comp(_max_heap[_last()], _min_heap[_last()])) {
                std::swap(_min_heap[_last()], _max_heap[_last()]);
                _heapify_max_up(_last());
            }
        }
        _odd = std::nullopt;
        return std::move(ret_val);
    }

    bool push(T new_add) {
        if (_capacity <= _occupied)
            return false;

        ++_occupied;
        if (!_odd) {
            _odd = std::make_optional<T>(std::move(new_add));
            return true;
        }

        T other = std::move(_odd.value());
        _odd = std::nullopt;
        if (_comp(new_add, other)) {
            _min_heap[_last()] = std::move(new_add);
            _max_heap[_last()] = std::move(other);
        } else {
            _min_heap[_last()] = std::move(other);
            _max_heap[_last()] = std::move(new_add);
        }
        _heapify_max_up(_last());
        if (_comp(_max_heap[_last()], _min_heap[_last()])) {
            std::swap(_max_heap[_last()], _min_heap[_last()]);
            _heapify_max_up(_last());
        }
        _heapify_min_up(_last());
        if (_comp(_max_heap[_last()], _min_heap[_last()])) {
            std::swap(_max_heap[_last()], _min_heap[_last()]);
            _heapify_min_up(_last());
        }

        return true;
    }

    void resize(int cap) {
        if (cap < 0)
            throw std::logic_error("Trying to resize heap to have negative capacity");

        _max_heap.resize(cap/2);
        _min_heap.resize(cap/2);
        _capacity = cap;
        _occupied = std::min(_occupied, _capacity);
    }

    int size() const {
        return _occupied;
    }

    int limit() const {
        return _capacity;
    }

    bool empty() const {
        return _occupied == 0;
    }

    void debug_print() const {
        std::cout << "Max: ";
        for(const auto& e : _max_heap) {
            std::cout << e << " ";
        }
        std::cout << std::endl << "Min: ";
        for(const auto& e : _min_heap) {
            std::cout << e << " ";
        }
        std::cout << std::endl << "Odd: ";
        if (_odd) {
            std::cout << _odd.value();
        } else {
            std::cout << "no odd";
        }
        std::cout << "\nOccupied " << _occupied;
        std::cout << "\n%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%" << std::endl;
    }

private:

    int _l(int i) const {
        return 2*i+1;
    }

    int _r(int i) const {
        return 2*i+2;
    }

    int _p(int i) const {
        return (i+1)/2-1;
    }

    int _s(int i) const {
        return i & 1 ? i + 1 : i - 1;
    }

    bool _in(int i) const {
        return i >= 0 && i <= _last();
    }

    int _last() const {
        return _occupied/2 - 1;
    }

    void _heapify_max_down(int i) {
        int l = _l(i);
        int r = _r(i);
        if(!_in(l) && !_in(r)) {
            if (_comp(_max_heap[i], _min_heap[i])) {
                std::swap(_max_heap[i], _min_heap[i]);
                return _heapify_min_up(i);
            }
            return;
        }

        int largest;
        if (_in(l) && _comp(_max_heap[i], _max_heap[l])) {
            largest = l;
        } else {
            largest = i;
        }
        if (_in(r) && _comp(_max_heap[largest], _max_heap[r])) {
            largest = r;
        }
        if (largest != i) {
            std::swap(_max_heap[i], _max_heap[largest]);
            return _heapify_max_down(largest);
        }
    }

    void _heapify_min_down(int i) {
        int l = _l(i);
        int r = _r(i);
        if(!_in(l) && !_in(r)) {
            if(_comp(_max_heap[i], _min_heap[i])) {
                std::swap(_min_heap[i], _max_heap[i]);
                return _heapify_max_up(i);
            }
            return;
        }
        
        int smallest;
        if (_in(l) && _comp(_min_heap[l], _min_heap[i])) {
            smallest = l;
        } else {
            smallest = i;
        }
        if (_in(r) && _comp(_min_heap[r], _min_heap[smallest])) {
            smallest = r;
        }
        if (smallest != i) {
            std::swap(_min_heap[i], _min_heap[smallest]);
            return _heapify_min_down(smallest);
        }
    }

    void _heapify_max_up(int i) {
        if (i == 0)
            return;


        int p = _p(i);
        if (_comp(_max_heap[p], _max_heap[i])) {
            std::swap(_max_heap[i], _max_heap[p]);
            return _heapify_max_up(p);
        }
    }

    void _heapify_min_up(int i) {
        if (i == 0)
            return;


        int p = _p(i);
        if (_comp(_min_heap[i], _min_heap[p])) {
            std::swap(_min_heap[i], _min_heap[p]);
            return _heapify_min_up(p);
        }
    }

    std::vector<T> _max_heap;
    std::vector<T> _min_heap;
    std::optional<T> _odd;
    int _capacity;
    int _occupied;
    Comp _comp;
};