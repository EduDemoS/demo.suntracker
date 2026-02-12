#pragma once

/**
 * @file
 * @brief Simple state machine handler class
 * 
 * @author Ueli Niederer, ueli.niederer@niederer-engineering.ch         
 * @copyright Copyright (c) 2025 by Niederer Engineering GmbH
 */
 
#include <stdint.h>

class SimpleStateProcessor;

/**
    \brief  Represents the potential reasons to enter a state handler 
*/
typedef enum eSSP_Reason {
    SSP_REASON_ENTER = 0,   /**< Handler is called to execute "enter" action */
    SSP_REASON_DO,          /**< Handler is called to execute "do" action */
    SSP_REASON_EXIT,        /**< Handler is called to execute "exit" action */

    SSP_REASON_COUNT
}tSSP_Reason;

typedef bool (*tSSP_StateHandler)(SimpleStateProcessor *SSM, tSSP_Reason reason, void *context);
#define SSP_STATE_HANDLER(stateName) bool stateName ## Handler(SimpleStateProcessor *fsm, tSSP_Reason reason, void *context)

/**
    \brief  Describes a single state for the FSM. For maximum portability, 
            use #SSM_STATE_DESCRIBE to create instances rather than 
            creating the instances yourself.
*/
typedef struct sSSM_State {
    const char *name;               /**< Printable name of the state */
    tSSP_StateHandler handler;      /**< State handler */
}tSSP_State;
#define SSP_STATE_DESCRIBE(stateName, stateHandler)    { stateName, stateHandler ## Handler }
#define SSP_STATE_LAST()                               { 0, 0 }

/**
    \brief  Simple finite state machine library
*/
class SimpleStateProcessor
{
    public:
                        SimpleStateProcessor(uintptr_t init, const tSSP_State *states, void *context);
        virtual         ~SimpleStateProcessor();

        int             run();
        void            reset();

        bool            NextStateSet(uintptr_t nextState);
        uintptr_t       CurrentStateGet();
        const char *    CurrentStateNameGet();

    private:
    void             *m_context;

    const tSSP_State *m_states;
    uintptr_t         m_states_len;

    const tSSP_State *m_state_init; /**< Initial state */
    const tSSP_State *m_state_curr; /**< Current state running */
    const tSSP_State *m_state_next; /**< Next state to enter */
};
