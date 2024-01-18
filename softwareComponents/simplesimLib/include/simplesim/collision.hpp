#pragma once

#include <configuration/rofiworld.hpp>
#include <atoms/result.hpp>

namespace rofi::simplesim {

enum class CollisionModel {
    None,
    Simple
};  

// Shared ptr to the specified collision model
atoms::Result< std::shared_ptr< rofi::configuration::Collision > > getCollisionPtr( CollisionModel collType ) {
    std::shared_ptr< rofi::configuration::Collision > collModel;
    switch (collType) {
    case CollisionModel::None:
        collModel = std::make_shared< rofi::configuration::NoCollision >();
        return atoms::result_value( collModel );
    case CollisionModel::Simple:
        collModel = std::make_shared< rofi::configuration::SimpleCollision >();
        return atoms::result_value( collModel );
    default:
        return atoms::result_error< std::string >( "Invalid collision model" );
    }
}

} // namespace rofi::simplesim
