//
// Created by martin on 8/1/24.
//

#pragma once

#include<cstdint>

/**
 * Possible jamming states which can be detected by jam monitor
 */
enum class JamStatus{
  Nominal, //No jam detected
  Jammed, //System was detected to be stuck, recovery can be attempted
  Recovery, //Monitor is watching for signs of recovery
  Fatal, //Permanently stuck, (result of timeout of recovery operation or, too many failures)
};


/**
 * Class monitors whether mechanism might be stuck based on provided timeouts and changes in positions.
 * If jam is detected, monitor should be informed about start of recovery action by user.
 */
class JamMonitor{
public:

  JamMonitor(uint32_t initStepTimeoutMs  = 150,
             uint32_t stepTimeoutMs = 100 ,
             float stepDistance = 0.0f,
             uint32_t recoveryTimeoutMs = 250,
             float recoveryDistance = 0.15f,
             int recoveryAttempts = 3 );

  void start(float pos, float goal); //start to monitor new move
  JamStatus startRecovery(); //tells to monitor that recovery started

  JamStatus update( float pos); //update status of monitor

private:
  const uint32_t _kInitStepTimeout;
  const uint32_t _kStepTimeout;
  const float _kMinStepDistance;

  const uint32_t _kRecoveryTimeout;
  const float _kRecoveryDistance;
  const int _kRecoveryAttempts;

  JamStatus _status = {};

  float _lastDistance = {};
  uint32_t _timeout = {}; //current timeout as absolute timepoint

  float _goal = {};
  int _attempt = {};
};
