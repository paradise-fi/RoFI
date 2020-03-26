#include <catch2/catch.hpp>
#include "../MinMaxHeap.h"

class IntComp{
public:
    bool operator()(const int& a, const int& b) const {
        return a < b;
    }
};

TEST_CASE("Ints") {
    MinMaxHeap<int,IntComp> mmh(9);

    std::vector<int> ins = {5,3,8,1,2,9,4,6,7};
    std::vector<int> ins2 = {1,-1,10};
    REQUIRE(mmh.limit() == 9);
    for (int i = 0; i < ins.size(); ++i) {
        REQUIRE(mmh.size() == i);
        REQUIRE(mmh.push(ins[i]));
    }
    REQUIRE(mmh.size() == ins.size());
    
    auto min = mmh.popMin();
    

    auto max = mmh.popMax();
    REQUIRE(min == 1);
    REQUIRE(max == 9);
    

    REQUIRE(mmh.size() == ins.size() - 2);
    min = mmh.popMin();
    
    REQUIRE(min == 2);
    max = mmh.popMax();
    REQUIRE(max == 8);
    

    REQUIRE(mmh.size() == ins.size() - 4);
    min = mmh.popMin();
    
    REQUIRE(min == 3);
    max = mmh.popMax();
    REQUIRE(max == 7);
    

    REQUIRE(mmh.size() == ins.size() - 6);
    min = mmh.popMin();
    
    REQUIRE(min == 4);
    max = mmh.popMax();
    REQUIRE(max == 6);
    

    for (int i = 0; i < ins2.size(); ++i) {
        REQUIRE(mmh.size() == ins.size() - 8 + i);
        REQUIRE(mmh.push(ins2[i]));
        
    }

    min = mmh.popMin();
    
    REQUIRE(min == -1);
    min = mmh.popMin();
    
    REQUIRE(min == 1);
    min = mmh.popMin();
    
    REQUIRE(min == 5);
    min = mmh.popMin();
    
    REQUIRE(min == 10);
} //41 Asserts

class PIntComp{
public:
    bool operator()(const std::unique_ptr<int>& a, const std::unique_ptr<int>& b) const {
        if (!a || !b) {
            throw std::logic_error("xd");
        }
        return *a < *b;
    }
};

TEST_CASE("Unique Ptr to Ints") {
    MinMaxHeap<std::unique_ptr<int>,PIntComp> mmh(9);
    std::vector<int> ins = {5,3,8,1,2,9,4,6,7};
    std::vector<int> ins2 = {1,-1,10};
    REQUIRE(mmh.limit() == 9);
    for (int i = 0; i < ins.size(); ++i) {
        REQUIRE(mmh.size() == i);
        REQUIRE(mmh.push(std::make_unique<int>(ins[i])));
    }
    REQUIRE(mmh.size() == ins.size());
    
    auto min = mmh.popMin();
    

    auto max = mmh.popMax();
    REQUIRE(*min == 1);
    REQUIRE(*max == 9);
    

    REQUIRE(mmh.size() == ins.size() - 2);
    min = mmh.popMin();
    
    REQUIRE(*min == 2);
    max = mmh.popMax();
    REQUIRE(*max == 8);
    

    REQUIRE(mmh.size() == ins.size() - 4);
    min = mmh.popMin();
    
    REQUIRE(*min == 3);
    max = mmh.popMax();
    REQUIRE(*max == 7);
    

    REQUIRE(mmh.size() == ins.size() - 6);
    min = mmh.popMin();
    
    REQUIRE(*min == 4);
    max = mmh.popMax();
    REQUIRE(*max == 6);
    

    for (int i = 0; i < ins2.size(); ++i) {
        REQUIRE(mmh.size() == ins.size() - 8 + i);
        REQUIRE(mmh.push(std::make_unique<int>(ins2[i])));
        
    }

    min = mmh.popMin();
    
    REQUIRE(*min == -1);
    min = mmh.popMin();
    
    REQUIRE(*min == 1);
    min = mmh.popMin();
    
    REQUIRE(*min == 5);
    min = mmh.popMin();
    
    REQUIRE(*min == 10);
    REQUIRE(mmh.empty());
}
