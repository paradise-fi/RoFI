/**
 * @brief Jaculus conversion traits for rofi::hal types.
 * @details This trait is used to convert between rofi::hal and JavaScript rofi interface.
 *
 */

// Descriptor
template<>
struct jac::ConvTraits<rofi::hal::RoFI::Descriptor> {
    static Value to(ContextRef ctx, rofi::hal::RoFI::Descriptor val) {
        auto obj = Object::create(ctx);
        obj.set<int>("jointCount", val.jointCount);
        obj.set<int>("connectorCount", val.connectorCount);
        return obj;
    }

    static rofi::hal::RoFI::Descriptor from(ContextRef ctx, ValueWeak val) {
        auto obj = val.to<Object>();
        return rofi::hal::RoFI::Descriptor{
            obj.get<int>("jointCount"),
            obj.get<int>("connectorCount"),
        };
    }
};


// Error
template<>
struct jac::ConvTraits<rofi::hal::Joint::Error> {
	static rofi::hal::Joint::Error from(ContextRef ctx, ValueWeak val) {
		auto str = val.to<std::string>();
		if (str == "Communication") {
			return rofi::hal::Joint::Error::Communication;
		} else if (str == "Hardware") {
			return rofi::hal::Joint::Error::Hardware;
		} else {
			throw std::runtime_error("Invalid Error");
		}
	}

	static Value to(ContextRef ctx, rofi::hal::Joint::Error val) {
		switch (val) {
			case rofi::hal::Joint::Error::Communication:
				return ConvTraits<std::string>::to(ctx, "Communication");
			case rofi::hal::Joint::Error::Hardware:
				return ConvTraits<std::string>::to(ctx, "Hardware");
		}
	}
};



// ConnectorEvent
template<>
struct jac::ConvTraits<rofi::hal::ConnectorEvent> {
    static rofi::hal::ConnectorEvent from(ContextRef ctx, ValueWeak val) {
        auto str = val.to<std::string>();
        if (str == "Connected") {
            return rofi::hal::ConnectorEvent::Connected;
        } else if (str == "Disconnected") {
            return rofi::hal::ConnectorEvent::Disconnected;
        } else if (str == "PowerChanged") {
            return rofi::hal::ConnectorEvent::PowerChanged;
        } else {
            throw std::runtime_error("Invalid ConnectorEvent");
        }
    }

    static Value to(ContextRef ctx, rofi::hal::ConnectorEvent val) {
        switch (val) {
            case rofi::hal::ConnectorEvent::Connected:
                return ConvTraits<std::string>::to(ctx, "Connected");
            case rofi::hal::ConnectorEvent::Disconnected:
                return ConvTraits<std::string>::to(ctx, "Disconnected");
            case rofi::hal::ConnectorEvent::PowerChanged:
                return ConvTraits<std::string>::to(ctx, "PowerChanged");
        }
    }
};

// ConnectorOrientation
template<>
struct jac::ConvTraits<rofi::hal::ConnectorOrientation> {
    static rofi::hal::ConnectorOrientation from(ContextRef ctx, ValueWeak val) {
        auto str = val.to<std::string>();
        if (str == "North") {
            return rofi::hal::ConnectorOrientation::North;
        } else if (str == "East") {
            return rofi::hal::ConnectorOrientation::East;
        } else if (str == "South") {
            return rofi::hal::ConnectorOrientation::South;
        } else if (str == "West") {
            return rofi::hal::ConnectorOrientation::West;
        } else {
            throw std::runtime_error("Invalid ConnectorOrientation");
        }
    }

    static Value to(ContextRef ctx, rofi::hal::ConnectorOrientation val) {
        switch (val) {
            case rofi::hal::ConnectorOrientation::North:
                return ConvTraits<std::string>::to(ctx, "North");
            case rofi::hal::ConnectorOrientation::East:
                return ConvTraits<std::string>::to(ctx, "East");
            case rofi::hal::ConnectorOrientation::South:
                return ConvTraits<std::string>::to(ctx, "South");
            case rofi::hal::ConnectorOrientation::West:
                return ConvTraits<std::string>::to(ctx, "West");
        }
    }
};

// ConnectorPosition
template<>
struct jac::ConvTraits<rofi::hal::ConnectorPosition> {
    static rofi::hal::ConnectorPosition from(ContextRef ctx, ValueWeak val) {
        auto str = val.to<std::string>();
        if (str == "Retracted") {
            return rofi::hal::ConnectorPosition::Retracted;
        } else if (str == "Extended") {
            return rofi::hal::ConnectorPosition::Extended;
        } else {
            throw std::runtime_error("Invalid ConnectorPosition");
        }
    }

    static Value to(ContextRef ctx, rofi::hal::ConnectorPosition val) {
        switch (val) {
            case rofi::hal::ConnectorPosition::Retracted:
                return ConvTraits<std::string>::to(ctx, "Retracted");
            case rofi::hal::ConnectorPosition::Extended:
                return ConvTraits<std::string>::to(ctx, "Extended");
        }
    }
};

// ConnectorLine
template<>
struct jac::ConvTraits<rofi::hal::ConnectorLine> {
	static rofi::hal::ConnectorLine from(ContextRef ctx, ValueWeak val) {
		auto str = val.to<std::string>();
		if (str == "Internal") {
			return rofi::hal::ConnectorLine::Internal;
		} else if (str == "External") {
			return rofi::hal::ConnectorLine::External;
		} else {
			throw std::runtime_error("Invalid ConnectorLine");
		}
	}

	static Value to(ContextRef ctx, rofi::hal::ConnectorLine val) {
		switch (val) {
			case rofi::hal::ConnectorLine::Internal:
				return ConvTraits<std::string>::to(ctx, "Internal");
			case rofi::hal::ConnectorLine::External:
				return ConvTraits<std::string>::to(ctx, "External");
		}
	}
};

// LidarDistanceMode
template<>
struct jac::ConvTraits<rofi::hal::LidarDistanceMode> {
	static rofi::hal::LidarDistanceMode from(ContextRef ctx, ValueWeak val) {
		auto str = val.to<std::string>();
		if (str == "Autonomous") {
			return rofi::hal::LidarDistanceMode::Autonomous;
		} else if (str == "Short") {
			return rofi::hal::LidarDistanceMode::Short;
		} else if (str == "Long") {
			return rofi::hal::LidarDistanceMode::Long;
		} else {
			throw std::runtime_error("Invalid LidarDistanceMode");
		}
	}

	static Value to(ContextRef ctx, rofi::hal::LidarDistanceMode val) {
		switch (val) {
			case rofi::hal::LidarDistanceMode::Autonomous:
				return ConvTraits<std::string>::to(ctx, "Autonomous");
			case rofi::hal::LidarDistanceMode::Short:
				return ConvTraits<std::string>::to(ctx, "Short");
			case rofi::hal::LidarDistanceMode::Long:
				return ConvTraits<std::string>::to(ctx, "Long");
		}
	}
};

// LidarStatus
template<>
struct jac::ConvTraits<rofi::hal::LidarStatus> {
	static rofi::hal::LidarStatus from(ContextRef ctx, ValueWeak val) {
		auto str = val.to<std::string>();
		if (str == "Error") {
			return rofi::hal::LidarStatus::Error;
		} else if (str == "NotMeasured") {
			return rofi::hal::LidarStatus::NotMeasured;
		} else if (str == "OutsideRange") {
			return rofi::hal::LidarStatus::OutsideRange;
		} else if (str == "Valid") {
			return rofi::hal::LidarStatus::Valid;
		} else {
			throw std::runtime_error("Invalid LidarStatus");
		}
	}

	static Value to(ContextRef ctx, rofi::hal::LidarStatus val) {
		switch (val) {
			case rofi::hal::LidarStatus::Error:
				return ConvTraits<std::string>::to(ctx, "Error");
			case rofi::hal::LidarStatus::NotMeasured:
				return ConvTraits<std::string>::to(ctx, "NotMeasured");
			case rofi::hal::LidarStatus::OutsideRange:
				return ConvTraits<std::string>::to(ctx, "OutsideRange");
			case rofi::hal::LidarStatus::Valid:
				return ConvTraits<std::string>::to(ctx, "Valid");
		}
	}
};

// ConnectorState
template<>
struct jac::ConvTraits<rofi::hal::ConnectorState> {
	static rofi::hal::ConnectorState from(ContextRef ctx, ValueWeak val) {
		auto obj = val.to<Object>();
		rofi::hal::ConnectorState state;
		state.position = ConvTraits<rofi::hal::ConnectorPosition>::from(ctx, obj.get("position"));
		state.internal = ConvTraits<bool>::from(ctx, obj.get("internal"));
		state.external = ConvTraits<bool>::from(ctx, obj.get("external"));
		state.distanceMode = ConvTraits<rofi::hal::LidarDistanceMode>::from(ctx, obj.get("distanceMode"));
		state.connected = ConvTraits<bool>::from(ctx, obj.get("connected"));
		state.orientation = ConvTraits<rofi::hal::ConnectorOrientation>::from(ctx, obj.get("orientation"));
		state.internalVoltage = ConvTraits<float>::from(ctx, obj.get("internalVoltage"));
		state.internalCurrent = ConvTraits<float>::from(ctx, obj.get("internalCurrent"));
		state.externalVoltage = ConvTraits<float>::from(ctx, obj.get("externalVoltage"));
		state.externalCurrent = ConvTraits<float>::from(ctx, obj.get("externalCurrent"));
		state.lidarStatus = ConvTraits<rofi::hal::LidarStatus>::from(ctx, obj.get("lidarStatus"));
		state.distance = ConvTraits<int>::from(ctx, obj.get("distance"));
		return state;
	}

	static Value to(ContextRef ctx, rofi::hal::ConnectorState state) {
		auto obj = Object::create(ctx);
		obj.set("position", ConvTraits<rofi::hal::ConnectorPosition>::to(ctx, state.position));
		obj.set("internal", ConvTraits<bool>::to(ctx, state.internal));
		obj.set("external", ConvTraits<bool>::to(ctx, state.external));
		obj.set("distanceMode", ConvTraits<rofi::hal::LidarDistanceMode>::to(ctx, state.distanceMode));
		obj.set("connected", ConvTraits<bool>::to(ctx, state.connected));
		obj.set("orientation", ConvTraits<rofi::hal::ConnectorOrientation>::to(ctx, state.orientation));
		obj.set("internalVoltage", ConvTraits<float>::to(ctx, state.internalVoltage));
		obj.set("internalCurrent", ConvTraits<float>::to(ctx, state.internalCurrent));
		obj.set("externalVoltage", ConvTraits<float>::to(ctx, state.externalVoltage));
		obj.set("externalCurrent", ConvTraits<float>::to(ctx, state.externalCurrent));
		obj.set("lidarStatus", ConvTraits<rofi::hal::LidarStatus>::to(ctx, state.lidarStatus));
		obj.set("distance", ConvTraits<int>::to(ctx, state.distance));
		return obj;
	}
};
