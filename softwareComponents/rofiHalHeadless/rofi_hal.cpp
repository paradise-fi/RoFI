#include <rofi_hal.hpp>

#include <chrono>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {

using namespace rofi::hal;

class EmptyPartitionImpl : public UpdatePartition::Implementation {
public:
    std::size_t getSize() override
    {
        return 0;
    }

    void read( std::size_t offset, std::size_t size, const std::span< unsigned char > & buffer ) override
    {
        if ( offset != 0 || size != 0 || buffer.size() != 0 ) {
            throw std::out_of_range( "Headless HAL exposes only an empty partition" );
        }
    }

    void write( std::size_t offset, const std::span< const unsigned char > & data ) override
    {
        if ( offset != 0 || !data.empty() ) {
            throw std::out_of_range( "Headless HAL exposes only an empty partition" );
        }
    }

    void commit() override {}
    void abort() override {}
};

class DummyRoFIImpl : public RoFI::Implementation {
public:
    explicit DummyRoFIImpl( RoFI::Id id ) : _id( id ) {}

    RoFI::Id getId() const override
    {
        return _id;
    }

    Joint getJoint( int ) override
    {
        throw std::logic_error( "Headless HAL does not provide joint access" );
    }

    Connector getConnector( int ) override
    {
        throw std::logic_error( "Headless HAL does not provide connector access" );
    }

    RoFI::Descriptor getDescriptor() const override
    {
        return {};
    }

    Partition getRunningPartition() override
    {
        return Partition( _partition );
    }

    UpdatePartition initUpdate() override
    {
        return UpdatePartition( _partition );
    }

    void reboot() override {}

private:
    RoFI::Id _id;
    std::shared_ptr< EmptyPartitionImpl > _partition = std::make_shared< EmptyPartitionImpl >();
};

std::shared_ptr< RoFI::Implementation > getLocalImpl()
{
    static auto impl = std::make_shared< DummyRoFIImpl >( 0 );
    return impl;
}

} // namespace

namespace rofi::hal {

RoFI RoFI::getLocalRoFI()
{
    return RoFI( getLocalImpl() );
}

RoFI RoFI::getRemoteRoFI( Id remoteId )
{
    return RoFI( std::make_shared< DummyRoFIImpl >( remoteId ) );
}

void RoFI::sleep( int ms )
{
    if ( ms > 0 ) {
        std::this_thread::sleep_for( std::chrono::milliseconds( ms ) );
    }
}

void RoFI::postpone( int ms, std::function< void() > callback )
{
    if ( !callback ) {
        return;
    }

    std::thread( [ ms, callback = std::move( callback ) ]() mutable {
        RoFI::sleep( ms );
        callback();
    } ).detach();
}

} // namespace rofi::hal
