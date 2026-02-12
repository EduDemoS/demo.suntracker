#include "SimpleStateProcessor.h"

/**
 * @file
 * @brief   Simple state machine handler class
 * 
 *          This class provides a minimum state machine processor, that can run through
 *          an array of states. The states are injected through an array of state 
 *          descriptions. \see tSSP_State
 * 
 * @author    Ueli Niederer, ueli.niederer@niederer-engineering.ch         
 * @copyright Copyright (c) 2025 by Niederer Engineering GmbH
 */

static SSP_STATE_HANDLER(NullState);
static const tSSP_State NullState = SSP_STATE_DESCRIBE("NullState", NullState);

SimpleStateProcessor::SimpleStateProcessor(uintptr_t init, const tSSP_State *states, void *context) : m_states_len(0) {
    m_states = states;
    while (states->name != 0) {
        m_states_len++;
        states++;
    }

    m_state_init = &m_states[init];
    m_state_curr = &NullState;
    m_state_next = m_state_init;
    m_context = context;
}

SimpleStateProcessor::~SimpleStateProcessor() {

}

void SimpleStateProcessor::reset() {
    m_state_curr->handler(this, SSP_REASON_EXIT, m_context);
    m_state_curr = &NullState;
    m_state_next = m_state_init;
}

int SimpleStateProcessor::run() {
    if (m_state_next != m_state_curr) {
        m_state_curr->handler(this, SSP_REASON_EXIT, m_context);
        m_state_next->handler(this, SSP_REASON_ENTER, m_context);
        m_state_curr = m_state_next;
    }
    return m_state_curr->handler(this, SSP_REASON_DO, m_context);
}

bool SimpleStateProcessor::NextStateSet(uintptr_t nextState) {
    if (nextState >= m_states_len) {
        return false;
    }

    m_state_next = &m_states[nextState];
    return true;
}

uintptr_t SimpleStateProcessor::CurrentStateGet() {
    return m_state_curr - m_states;
}

const char *SimpleStateProcessor::CurrentStateNameGet() {
    return m_state_curr->name;
}

static SSP_STATE_HANDLER(NullState) {
    return 0;
}
