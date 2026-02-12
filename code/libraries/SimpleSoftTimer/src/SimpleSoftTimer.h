#pragma once

/**
 * @file
 * @brief       Simple, small software timer for simplified timeout handling
 * 
 * @author      Ueli Niederer, ueli.niederer@niederer-engineering.ch         
 * @copyright   Copyright (c) 2025 by Niederer Engineering GmbH
 */
 
namespace HolisticSolutions{
    typedef unsigned long tSimpleSoftTimerTime;

    class SimpleSoftTimer {
        public: 
            SimpleSoftTimer(tSimpleSoftTimerTime timeout);
            virtual ~SimpleSoftTimer();

            void start(tSimpleSoftTimerTime timeout = 0);
            void restart();
            
            tSimpleSoftTimerTime timePassed();
            bool                 isTimeout();

        private:
            tSimpleSoftTimerTime m_time_start;
            tSimpleSoftTimerTime m_timeout;
    };
};
