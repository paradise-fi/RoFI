//
// Created by martin on 8/1/24.
//
#include "jammonitor.hpp"

#include <drivers/hal.hpp>
#include <system/dbg.hpp>

#include <cmath>

/**
 * @param initStepTimeoutMs  Timeout of the initial move
 * @param stepTimeoutMs  Timeout for progress in position
 * @param stepDistance Minimal change in position to be considered move
 * @param recoveryTimeoutMs Timeout for recovery move
 * @param recoveryDistance Distance of the recovery move
 * @param recoveryAttempts Maximum allowed recovery moves in single taks before
 * fatal jam
 */
JamMonitor::JamMonitor(uint32_t initStepTimeoutMs,
                       uint32_t stepTimeoutMs,
                       float stepDistance,
                       uint32_t recoveryTimeoutMs,
                       float recoveryDistance,
                       int recoveryAttempts)
    : _kInitStepTimeout{initStepTimeoutMs},
      _kStepTimeout{stepTimeoutMs},
      _kMinStepDistance{stepDistance},
      _kRecoveryTimeout{recoveryTimeoutMs},
      _kRecoveryDistance{recoveryDistance},
      _kRecoveryAttempts{recoveryAttempts} {}

/**
 * Tells monitor that the new task was started.
 * Resets internal timers even from Fatal error.
 * @param pos Current position
 * @param goal Goal position
 */
void JamMonitor::start(float pos, float goal) {
  _status = JamStatus::Nominal;
  _attempt = 0;
  _timeout = HAL_GetTick() + _kInitStepTimeout;

  _lastDistance = std::abs(_goal - pos);
  _goal = goal;
}

/**
 * Tell monitor that recovery move will be attempted and to look for the signs
 * of recovery.
 */
JamStatus JamMonitor::startRecovery() {
  if (_status != JamStatus::Fatal) {
    _timeout = HAL_GetTick() + _kRecoveryTimeout;
    _status = JamStatus::Recovery;
  }
  return _status;
}

/**
 * Check whether given timeout has passed, i.e. timeout < current_time.
 * Takes into account overflows
 * @param timeout
 * @param current_time
 * @return
 */
bool isExpired( uint32_t timeout, uint32_t current_time){
    return (int32_t)(timeout - current_time) < 0;
}

/**
 *
 * @param position current position of motor
 * @return true if jam was detected false otherwise
 */


JamStatus JamMonitor::update(float pos) {
  auto remainingDistance = std::abs(_goal - pos);
  switch (_status) {
  case JamStatus::Nominal:
    if (remainingDistance < _lastDistance - _kMinStepDistance) {
      _lastDistance = remainingDistance;
      _timeout = HAL_GetTick() + _kStepTimeout;
      break;
    }

    if (isExpired(_timeout, HAL_GetTick())) { // timeout has already expired

      if (++_attempt <= _kRecoveryAttempts) {
        _status = JamStatus::Stuck;
        break;
      }
      _status = JamStatus::Fatal; // no more attempts
      break;
    }
    break;

  case JamStatus::Recovery:
    if (remainingDistance > _lastDistance + _kRecoveryDistance) {
      // sucessfull recovery -> resume normal operation
      _status = JamStatus::Nominal;
      _lastDistance = remainingDistance;
      _timeout = HAL_GetTick() + _kInitStepTimeout;
      break;
    }

    if (isExpired(_timeout, HAL_GetTick())) {
      // coudl not recover in time
      _status = JamStatus::Fatal;
    }

  default:
    break;
  }

  return _status;
}