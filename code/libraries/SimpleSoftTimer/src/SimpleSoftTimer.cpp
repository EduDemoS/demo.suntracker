#include <SimpleSoftTimer.h>

/**
 * @file
 * @brief       Simple, small software timer for simplified timeout handling
 * 
 * @author      Ueli Niederer, ueli.niederer@niederer-engineering.ch         
 * @copyright   Copyright (c) 2025 by Niederer Engineering GmbH
 */

using namespace HolisticSolutions;

#include <Arduino.h>

SimpleSoftTimer::SimpleSoftTimer(tSimpleSoftTimerTime timeout) : m_timeout(timeout) {
}

SimpleSoftTimer::~SimpleSoftTimer() {
}

/**
 * @brief Starts the timeout specified through @p timeout
 * 
 * @param timeout   [ms] timeout to wait for
 */
void SimpleSoftTimer::start(tSimpleSoftTimerTime timeout) {
    m_timeout = timeout;
    if (timeout > 0) {
        m_time_start = millis();
    }
}

/**
 * @brief Restarts the timeout with the timeout from the current time.
 * 
 * @note  Depending on the code between the detection of a timeout and the restart,
 *        periodicy of execution might jitter.
 */
void SimpleSoftTimer::restart() {
    m_time_start = millis();
}

/**
 * @return [ms] time since #StopWatch::start was called
 */
unsigned long SimpleSoftTimer::timePassed() {
    unsigned long now = millis();
    return now - m_time_start;
}

/**
 * @return true     if time since start is at least as long as specified during 
 *                  start
 * @return false    if timeout did not yet occur
 */
bool SimpleSoftTimer::isTimeout() {
    unsigned long timespan = timePassed();
    return timespan >= m_timeout;
}
