#include <unity.h>
#include <Emulation.h>

#include <SimpleStateProcessor.h>

typedef enum eTestState {
    STATE_UNKNOWN = 0,
    STATE_A,
    STATE_B,

    STATE_COUNT
}tTestState;

static SSP_STATE_HANDLER(StateUnknown);
static SSP_STATE_HANDLER(StateA);
static SSP_STATE_HANDLER(StateB);

static const tSSP_State states[] = {
    SSP_STATE_DESCRIBE("Unknown", StateUnknown),
    SSP_STATE_DESCRIBE("A", StateA),
    SSP_STATE_DESCRIBE("B", StateB),

    SSP_STATE_LAST()
};

static int context = 0xdeadbeef;
static FunctionEmulator StateUnknownStub("StateUnknown");
static FunctionEmulator StateAStub("StateA");
static FunctionEmulator StateBStub("StateB");

void setUp(void) {
}

void tearDown(void) {
}

void test_init_to_NullState() {
    SimpleStateProcessor uut(STATE_UNKNOWN, states, &context);

    TEST_ASSERT_EQUAL_STRING("NullState", uut.CurrentStateNameGet());
} 

void test_switch_to_inital_state_upon_first_run() {
    SimpleStateProcessor uut(STATE_UNKNOWN, states, &context);
    
    StateUnknownStub.reset();
    StateAStub.reset();
    StateBStub.reset();
    
    uut.run();

    TEST_ASSERT_EQUAL(2, StateUnknownStub.timesCalled());

    TEST_ASSERT_EQUAL_STRING(states[STATE_UNKNOWN].name, uut.CurrentStateNameGet());
    TEST_ASSERT_EQUAL_PTR(&uut, StateUnknownStub.getArguments().resolve<SimpleStateProcessor *>(0, 0));
    TEST_ASSERT_EQUAL_PTR(SSP_REASON_ENTER, StateUnknownStub.getArguments().resolve<tSSP_Reason>(0, 1)); 
    TEST_ASSERT_EQUAL_PTR(&context, StateUnknownStub.getArguments().resolve<void *>(0, 2));

} 

void test_reason_and_context_are_correctly_set() {
    SimpleStateProcessor uut(STATE_UNKNOWN, states, &context);
    
    StateUnknownStub.reset();
    StateAStub.reset();
    StateBStub.reset();
    
    uut.run();
    uut.run();
    uut.NextStateSet(STATE_A);
    uut.run();

    TEST_ASSERT_EQUAL(4, StateUnknownStub.timesCalled());
    TEST_ASSERT_EQUAL(2, StateAStub.timesCalled());

    TEST_ASSERT_EQUAL_PTR(&uut, StateUnknownStub.getArguments().resolve<SimpleStateProcessor *>(0, 0));
    TEST_ASSERT_EQUAL_PTR(SSP_REASON_ENTER, StateUnknownStub.getArguments().resolve<tSSP_Reason>(0, 1)); 
    TEST_ASSERT_EQUAL_PTR(&context, StateUnknownStub.getArguments().resolve<void *>(0, 2));

    TEST_ASSERT_EQUAL_PTR(&uut, StateUnknownStub.getArguments().resolve<SimpleStateProcessor *>(1, 0));
    TEST_ASSERT_EQUAL_PTR(SSP_REASON_DO, StateUnknownStub.getArguments().resolve<tSSP_Reason>(1, 1)); 
    TEST_ASSERT_EQUAL_PTR(&context, StateUnknownStub.getArguments().resolve<void *>(1, 2));

    TEST_ASSERT_EQUAL_PTR(&uut, StateUnknownStub.getArguments().resolve<SimpleStateProcessor *>(3, 0));
    TEST_ASSERT_EQUAL_PTR(SSP_REASON_EXIT, StateUnknownStub.getArguments().resolve<tSSP_Reason>(3, 1)); 
    TEST_ASSERT_EQUAL_PTR(&context, StateUnknownStub.getArguments().resolve<void *>(3, 2));

    TEST_ASSERT_EQUAL_PTR(&uut, StateAStub.getArguments().resolve<SimpleStateProcessor *>(0, 0));
    TEST_ASSERT_EQUAL_PTR(SSP_REASON_ENTER, StateAStub.getArguments().resolve<tSSP_Reason>(0, 1)); 
    TEST_ASSERT_EQUAL_PTR(&context, StateAStub.getArguments().resolve<void *>(0, 2));

    TEST_ASSERT_EQUAL_PTR(&uut, StateAStub.getArguments().resolve<SimpleStateProcessor *>(1, 0));
    TEST_ASSERT_EQUAL_PTR(SSP_REASON_DO, StateAStub.getArguments().resolve<tSSP_Reason>(1, 1)); 
    TEST_ASSERT_EQUAL_PTR(&context, StateAStub.getArguments().resolve<void *>(1, 2));
} 

void test_switch_to_last_state(void) {
    SimpleStateProcessor uut(STATE_UNKNOWN, states, &context);
    
    StateUnknownStub.reset();
    StateAStub.reset();
    StateBStub.reset();
    
    uut.run();
    bool success = uut.NextStateSet(STATE_COUNT - 1);
    uut.run();

    TEST_ASSERT_EQUAL(true, success);
    TEST_ASSERT_EQUAL(2, StateBStub.timesCalled());
    TEST_ASSERT_EQUAL(SSP_REASON_ENTER, StateBStub.getArguments().resolve<tSSP_Reason>(0, 1));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_init_to_NullState);
    RUN_TEST(test_switch_to_inital_state_upon_first_run);
    RUN_TEST(test_reason_and_context_are_correctly_set);
    RUN_TEST(test_switch_to_last_state);

    UNITY_END();
}

static SSP_STATE_HANDLER(StateUnknown) {
    StateUnknownStub.recordFunctionCall();
    StateUnknownStub.captureArgs(fsm, reason, context);
    return 0;
}

static SSP_STATE_HANDLER(StateA) {
    StateAStub.recordFunctionCall();
    StateAStub.captureArgs(fsm, reason, context);
    return 0;
}

static SSP_STATE_HANDLER(StateB) {
    StateBStub.recordFunctionCall();
    StateBStub.captureArgs(fsm, reason, context);
    return 0;
}
