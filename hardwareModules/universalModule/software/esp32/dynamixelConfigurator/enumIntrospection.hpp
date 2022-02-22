#include <algorithm>
#include <cctype>

#include <peripherals/dynamixel.hpp>

// Magic Enums are broken for GCC 8.4; let's make things simple, stupid...

using namespace rofi::hal::dynamixel;

inline const Register RegisterValues[] = {
    Register::ModelNumber,
    Register::ModelInformation,
    Register::FirmwareVersion,
    Register::ID,
    Register::BaudRate,
    Register::ReturnDelayTime,
    Register::DriveMode,
    Register::OperatingMode,
    Register::SecondaryID,
    Register::ProtocolType,
    Register::HomingOffset,
    Register::MovingThreshold,
    Register::TemperatureLimit,
    Register::MaxVoltageLimit,
    Register::MinVoltageLimit,
    Register::PWMLimit,
    Register::VelocityLimit,
    Register::MaxPositionLimit,
    Register::MinPositionLimit,
    Register::StartupConfiguration,
    Register::Shutdown,
    Register::TorqueEnable,
    Register::LED,
    Register::StatusReturnLevel,
    Register::RegisteredInstruction,
    Register::HardwareErrorStatus,
    Register::VelocityIGain,
    Register::VelocityPGain,
    Register::PositionDGain,
    Register::PositionIGain,
    Register::PositionPGain,
    Register::Feedforward2ndGain,
    Register::Feedforward1stGain,
    Register::BusWatchdog,
    Register::GoalPWM,
    Register::GoalVelocity,
    Register::ProfileAcceleration,
    Register::ProfileVelocity,
    Register::GoalPosition,
    Register::RealtimeTick,
    Register::Moving,
    Register::MovingStatus,
    Register::PresentPWM,
    Register::PresentLoad,
    Register::PresentVelocity,
    Register::PresentPosition,
    Register::VelocityTrajectory,
    Register::PositionTrajectory,
    Register::PresentInputVoltage,
    Register::PresentTemperature,
    Register::BackupReady,
};

inline std::string to_string( Register r ) {
    switch( r ) {
        case Register::ModelNumber: return "ModelNumber";
        case Register::ModelInformation: return "ModelInformation";
        case Register::FirmwareVersion: return "FirmwareVersion";
        case Register::ID: return "ID";
        case Register::BaudRate: return "BaudRate";
        case Register::ReturnDelayTime: return "ReturnDelayTime";
        case Register::DriveMode: return "DriveMode";
        case Register::OperatingMode: return "OperatingMode";
        case Register::SecondaryID: return "SecondaryID";
        case Register::ProtocolType: return "ProtocolType";
        case Register::HomingOffset: return "HomingOffset";
        case Register::MovingThreshold: return "MovingThreshold";
        case Register::TemperatureLimit: return "TemperatureLimit";
        case Register::MaxVoltageLimit: return "MaxVoltageLimit";
        case Register::MinVoltageLimit: return "MinVoltageLimit";
        case Register::PWMLimit: return "PWMLimit";
        case Register::VelocityLimit: return "VelocityLimit";
        case Register::MaxPositionLimit: return "MaxPositionLimit";
        case Register::MinPositionLimit: return "MinPositionLimit";
        case Register::StartupConfiguration: return "StartupConfiguration";
        case Register::Shutdown: return "Shutdown";
        case Register::TorqueEnable: return "TorqueEnable";
        case Register::LED: return "LED";
        case Register::StatusReturnLevel: return "StatusReturnLevel";
        case Register::RegisteredInstruction: return "RegisteredInstruction";
        case Register::HardwareErrorStatus: return "HardwareErrorStatus";
        case Register::VelocityIGain: return "VelocityIGain";
        case Register::VelocityPGain: return "VelocityPGain";
        case Register::PositionDGain: return "PositionDGain";
        case Register::PositionIGain: return "PositionIGain";
        case Register::PositionPGain: return "PositionPGain";
        case Register::Feedforward2ndGain: return "Feedforward2ndGain";
        case Register::Feedforward1stGain: return "Feedforward1stGain";
        case Register::BusWatchdog: return "BusWatchdog";
        case Register::GoalPWM: return "GoalPWM";
        case Register::GoalVelocity: return "GoalVelocity";
        case Register::ProfileAcceleration: return "ProfileAcceleration";
        case Register::ProfileVelocity: return "ProfileVelocity";
        case Register::GoalPosition: return "GoalPosition";
        case Register::RealtimeTick: return "RealtimeTick";
        case Register::Moving: return "Moving";
        case Register::MovingStatus: return "MovingStatus";
        case Register::PresentPWM: return "PresentPWM";
        case Register::PresentLoad: return "PresentLoad";
        case Register::PresentVelocity: return "PresentVelocity";
        case Register::PresentPosition: return "PresentPosition";
        case Register::VelocityTrajectory: return "VelocityTrajectory";
        case Register::PositionTrajectory: return "PositionTrajectory";
        case Register::PresentInputVoltage: return "PresentInputVoltage";
        case Register::PresentTemperature: return "PresentTemperature";
        case Register::BackupReady: return "BackupReady";
    }
    assert( false && "Unknown value" );
}

inline std::string to_string( Error r ) {
    switch( r ) {
        case Error::Nothing: return "Nothing";
        case Error::ResultFail: return "ResultFail";
        case Error::Instruction: return "Instruction";
        case Error::CRC: return "CRC";
        case Error::DataRange: return "DataRange";
        case Error::DataLength: return "DataLength";
        case Error::DataLimit: return "DataLimit";
        case Error::Access: return "Access";
    }
    assert( false && "Unknown value" );
}

inline Register readRegisterName( std::string name ) {
    std::transform( name.begin(),  name.end(),  name.begin(),
                    [](unsigned char c){ return std::tolower(c); });

    if ( name == "modelnumber" )
        return Register::ModelNumber;
    if ( name == "modelinformation" )
        return Register::ModelInformation;
    if ( name == "firmwareversion" )
        return Register::FirmwareVersion;
    if ( name == "id" )
        return Register::ID;
    if ( name == "baudrate" )
        return Register::BaudRate;
    if ( name == "returndelaytime" )
        return Register::ReturnDelayTime;
    if ( name == "drivemode" )
        return Register::DriveMode;
    if ( name == "operatingmode" )
        return Register::OperatingMode;
    if ( name == "secondaryid" )
        return Register::SecondaryID;
    if ( name == "protocoltype" )
        return Register::ProtocolType;
    if ( name == "homingoffset" )
        return Register::HomingOffset;
    if ( name == "movingthreshold" )
        return Register::MovingThreshold;
    if ( name == "temperaturelimit" )
        return Register::TemperatureLimit;
    if ( name == "maxvoltagelimit" )
        return Register::MaxVoltageLimit;
    if ( name == "minvoltagelimit" )
        return Register::MinVoltageLimit;
    if ( name == "pwmlimit" )
        return Register::PWMLimit;
    if ( name == "velocitylimit" )
        return Register::VelocityLimit;
    if ( name == "maxpositionlimit" )
        return Register::MaxPositionLimit;
    if ( name == "minpositionlimit" )
        return Register::MinPositionLimit;
    if ( name == "startupconfiguration" )
        return Register::StartupConfiguration;
    if ( name == "shutdown" )
        return Register::Shutdown;
    if ( name == "torqueenable" )
        return Register::TorqueEnable;
    if ( name == "led" )
        return Register::LED;
    if ( name == "statusreturnlevel" )
        return Register::StatusReturnLevel;
    if ( name == "registeredinstruction" )
        return Register::RegisteredInstruction;
    if ( name == "hardwareerrorstatus" )
        return Register::HardwareErrorStatus;
    if ( name == "velocityigain" )
        return Register::VelocityIGain;
    if ( name == "velocitypgain" )
        return Register::VelocityPGain;
    if ( name == "positiondgain" )
        return Register::PositionDGain;
    if ( name == "positionigain" )
        return Register::PositionIGain;
    if ( name == "positionpgain" )
        return Register::PositionPGain;
    if ( name == "feedforward2ndgain" )
        return Register::Feedforward2ndGain;
    if ( name == "feedforward1stgain" )
        return Register::Feedforward1stGain;
    if ( name == "buswatchdog" )
        return Register::BusWatchdog;
    if ( name == "goalpwm" )
        return Register::GoalPWM;
    if ( name == "goalvelocity" )
        return Register::GoalVelocity;
    if ( name == "profileacceleration" )
        return Register::ProfileAcceleration;
    if ( name == "profilevelocity" )
        return Register::ProfileVelocity;
    if ( name == "goalposition" )
        return Register::GoalPosition;
    if ( name == "realtimetick" )
        return Register::RealtimeTick;
    if ( name == "moving" )
        return Register::Moving;
    if ( name == "movingstatus" )
        return Register::MovingStatus;
    if ( name == "presentpwm" )
        return Register::PresentPWM;
    if ( name == "presentload" )
        return Register::PresentLoad;
    if ( name == "presentvelocity" )
        return Register::PresentVelocity;
    if ( name == "presentposition" )
        return Register::PresentPosition;
    if ( name == "velocitytrajectory" )
        return Register::VelocityTrajectory;
    if ( name == "positiontrajectory" )
        return Register::PositionTrajectory;
    if ( name == "presentinputvoltage" )
        return Register::PresentInputVoltage;
    if ( name == "presenttemperature" )
        return Register::PresentTemperature;
    if ( name == "backupready" )
        return Register::BackupReady;

    throw std::runtime_error( "Unknown register name " + name );
}
