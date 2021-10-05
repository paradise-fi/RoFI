#include "dma.hpp"

std::array< Dma::ChannelsData, Dma::availablePeripherals.size() > Dma::_channelData;
