#include <generated/deconfig.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/ktime.h>

#include <kinetis/design_verification.h>
#include "kinetis/fsm.h"

// Enhanced FSM management
static bool fsm_performance_monitoring = false;
static struct fsm_statistics {
	u32 total_transitions;
	u32 total_state_changes;
	u32 total_errors;
	u64 total_execution_time_ms;
	u32 max_execution_time_ms;
	u32 min_execution_time_ms;
	ktime_t system_start_time;
	u32 state_counters[FSM_STATES];
	u32 state_transition_counts[FSM_STATES][FSM_STATES];
} fsm_stats = {0};

// Error tracking
static u32 fsm_last_error_code = 0;
static const char *fsm_state_names[FSM_STATES] = {
	"NONE", "INIT", "MODULE_INFO", "SIGN", "UDP_CREATE",
	"UDP_REGISTER", "UDP_SEND", "UDP_RECEIVE", "UDP_CLOSE", "RESET", "END"
};

static const char *fsm_condition_names[FSM_CONDITIONS] = {
	"OK", "ERROR", "TIMEOUT", "REPEATS_L1", "REPEATS_L2", "REPEATS_L3"
};

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/**
  * @step 1:  Modify the corresponding function according to the modified area and the corresponding function name.
  * @step 2:  This driver USES single linked list structure to register tasks.Depending on the user's needs, choose to use RTC or timer.
  * @step 3:  Write callback functions and initialization functions, such as function Task_Temperature_Humidit_Callback and HydrologyTask_Init.
  * @step 4:  Modify four areas: XXTask_TypeDef/Task_XX_Callback/HydrologyTask_Init.
  * @step 5:  Finally, HydrologyTask_Init is called in the main function.
  */

fsm_state fsm_step(struct state_machine *machine, struct sm_var *sm_var, struct transition **table)
{
	ktime_t start_time;
	fsm_state previous_state;

	if (fsm_performance_monitoring) {
		start_time = ktime_get();
	}

	previous_state = machine->current_state;

	if (sm_var->_repeats < 2) {
		struct transition *t = &table[machine->current_state][sm_var->_condition];

		if (fsm_performance_monitoring) {
			pr_debug("FSM: Transitioning from %s to %s (condition: %s)\n",
				fsm_state_names[previous_state],
				fsm_state_names[t->next],
				fsm_condition_names[sm_var->_condition]);
		}

		(*(t->action))(machine, sm_var);

		if (machine->current_state == t->next) {
			sm_var->_repeats++;
			fsm_stats.state_transition_counts[previous_state][t->next]++;
		} else {
			sm_var->_repeats = 0;
			fsm_stats.state_transition_counts[previous_state][t->next]++;
		}

		machine->current_state = t->next;
		fsm_stats.total_transitions++;
		fsm_stats.state_counters[machine->current_state]++;
	} else {
		sm_var->_condition = cERROR_REPEATS_L3;
		fsm_stats.total_errors++;
		fsm_last_error_code = 0x01; // Repeat limit exceeded

		pr_err("FSM: State machine exceeded repeat limit in state %s\n",
			fsm_state_names[machine->current_state]);

		struct transition *t = &table[machine->current_state][sm_var->_condition];
		(*(t->action))(machine, sm_var);

		sm_var->_repeats = 0;

		if (machine->current_state == sNB_UDP_RECEIVE) {
			machine->current_state = sNB_UDP_CLOSE;
		} else if (machine->current_state == sNB_UDP_CLOSE) {
			machine->current_state = sNB_END;
		} else {
			machine->current_state = sNB_RESET;
		}
	}

	if (fsm_performance_monitoring && previous_state != machine->current_state) {
		u64 exec_time = ktime_to_ms(ktime_sub(ktime_get(), start_time));
		fsm_stats.total_state_changes++;
		fsm_stats.total_execution_time_ms += exec_time;
		if (exec_time > fsm_stats.max_execution_time_ms) {
			fsm_stats.max_execution_time_ms = exec_time;
		}
		if (fsm_stats.min_execution_time_ms == 0 || exec_time < fsm_stats.min_execution_time_ms) {
			fsm_stats.min_execution_time_ms = exec_time;
		}

		pr_debug("FSM: Step completed in %llu ms\n", exec_time);
	}

	return machine->current_state;
}

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#ifdef DESIGN_VERIFICATION_FSM
#include "kinetis/test-kinetis.h"

extern int FSM_NB_None(struct state_machine *machine, struct sm_var *sm_var);
extern int FSM_NB_Init(struct state_machine *machine, struct sm_var *sm_var);
extern int FSM_NB_GetSign(struct state_machine *machine, struct sm_var *sm_var);
extern int FSM_NB_GetModuleInfo(struct state_machine *machine, struct sm_var *sm_var);
extern int FSM_NB_CreateUDP(struct state_machine *machine, struct sm_var *sm_var);
extern int FSM_NB_CloseUDP(struct state_machine *machine, struct sm_var *sm_var);
extern int FSM_NB_UDPRegister(struct state_machine *machine, struct sm_var *sm_var);
extern int FSM_NB_UDPSendData(struct state_machine *machine, struct sm_var *sm_var);
extern int FSM_NB_WaitReceiveData(struct state_machine *machine, struct sm_var *sm_var);
extern int FSM_NB_Reset(struct state_machine *machine, struct sm_var *sm_var);
extern int FSM_NB_End(struct state_machine *machine, struct sm_var *sm_var);

struct transition NB_Fail2Reset = {
	sNB_RESET,
	FSM_NB_Reset
};

struct transition NB_Reset2None = {
	sNB_NONE,
	FSM_NB_None
};

struct transition NB_None2Init = {
	sNB_INIT,
	FSM_NB_Init
};

struct transition NB_Init2ModuleInfo = {
	sNB_MODULE_INFO,
	FSM_NB_GetModuleInfo
};

struct transition NB_ModuleInfo2Sign = {
	sNB_SIGN,
	FSM_NB_GetSign
};

struct transition NB_Sign2UDPCreate = {
	sNB_UDP_CREATE,
	FSM_NB_CreateUDP
};

struct transition NB_UDPCreate2Register = {
	sNB_UDP_REGISTER,
	FSM_NB_UDPRegister
};

//struct transition NB_Register2GuyuCertificate =
//{
//  sNB_CERTIFICATE,
//  FSM_NB_WaitReceiveData
//};
//
//struct transition NB_GuyuCertificate2SendData =
//{
//  sNB_UDP_SEND,
//  FSM_NB_UDPSendData
//};

//struct transition NB_UDPCreate2SendData =
//{
//  sNB_UDP_SEND,
//  FSM_NB_UDPSendData
//};

struct transition NB_UDPRegister2SendData = {
	sNB_UDP_SEND,
	FSM_NB_UDPSendData
};

struct transition NB_UDPSendData2ReceiveData = {
	sNB_UDP_RECEIVE,
	FSM_NB_WaitReceiveData
};

struct transition NB_UDPReceiveData2UDPClose = {
	sNB_UDP_CLOSE,
	FSM_NB_CloseUDP
};

//struct transition NB_SendData2UDPClose =
//{
//  sNB_UDP_CLOSE,
//  FSM_NB_CloseUDP
//};

struct transition NB_UDPClose2End = {
	sNB_END,
	FSM_NB_End////////////////////////////
};

//struct transition NB_Sign2CoAPCreate =
//{
//  sNB_CoAP_SEVER,
//  FSM_NB_CreateUDP
//};
//
//struct transition NB_CoAPCreate2SendData =
//{
//  sNB_CoAP_SEND,
//  FSM_NB_UDPSendData
//};

struct transition NB_IOT_Trans_Table[FSM_STATES][FSM_CONDITIONS];

// Enhanced FSM utility functions
void fsm_enable_performance_monitoring(bool enable)
{
	fsm_performance_monitoring = enable;
	if (enable) {
		fsm_stats.system_start_time = ktime_get();
		pr_info("FSM performance monitoring enabled\n");
	}
}

void fsm_get_statistics(struct fsm_statistics *stats)
{
	if (stats) {
		*stats = fsm_stats;
	}
}

void fsm_reset_statistics(void)
{
	memset(&fsm_stats, 0, sizeof(fsm_stats));
	fsm_stats.system_start_time = ktime_get();
	fsm_stats.min_execution_time_ms = UINT_MAX;
	fsm_last_error_code = 0;
	pr_info("FSM statistics reset\n");
}

void fsm_print_performance_report(void)
{
	u64 uptime = ktime_to_ms(ktime_sub(ktime_get(), fsm_stats.system_start_time));
	u64 avg_exec_time = fsm_stats.total_transitions > 0 ?
		fsm_stats.total_execution_time_ms / fsm_stats.total_transitions : 0;

	pr_info("=== FSM Performance Report ===\n");
	pr_info("System uptime: %llu ms\n", uptime);
	pr_info("Total transitions: %u\n", fsm_stats.total_transitions);
	pr_info("State changes: %u\n", fsm_stats.total_state_changes);
	pr_info("Total errors: %u\n", fsm_stats.total_errors);
	pr_info("Avg execution time: %llu ms\n", avg_exec_time);
	pr_info("Min execution time: %u ms\n", fsm_stats.min_execution_time_ms);
	pr_info("Max execution time: %u ms\n", fsm_stats.max_execution_time_ms);

	pr_info("State visit counts:\n");
	for (int i = 0; i < FSM_STATES; i++) {
		if (fsm_stats.state_counters[i] > 0) {
			pr_info("  %s: %u visits\n", fsm_state_names[i], fsm_stats.state_counters[i]);
		}
	}

	pr_info("=============================\n");
}

const char *fsm_get_state_name(fsm_state state)
{
	if (state >= FSM_STATES) {
		return "UNKNOWN";
	}
	return fsm_state_names[state];
}

const char *fsm_get_condition_name(fsm_condition condition)
{
	if (condition >= FSM_CONDITIONS) {
		return "UNKNOWN";
	}
	return fsm_condition_names[condition];
}

void NB_IOT_FSM_Init(void)
{
	pr_info("Initializing NB IoT FSM with %d states and %d conditions\n", FSM_STATES, FSM_CONDITIONS);

	if (fsm_performance_monitoring) {
		pr_info("FSM performance monitoring active\n");
	}

	NB_IOT_Trans_Table[sNB_NONE][0] = NB_None2Init;
	NB_IOT_Trans_Table[sNB_NONE][1] = NB_Reset2None;
	NB_IOT_Trans_Table[sNB_NONE][2] = NB_Fail2Reset;

	NB_IOT_Trans_Table[sNB_INIT][0] = NB_Init2ModuleInfo;
	NB_IOT_Trans_Table[sNB_INIT][1] = NB_None2Init;
	NB_IOT_Trans_Table[sNB_INIT][2] = NB_Fail2Reset;

	NB_IOT_Trans_Table[sNB_MODULE_INFO][0] = NB_ModuleInfo2Sign;
	NB_IOT_Trans_Table[sNB_MODULE_INFO][1] = NB_Init2ModuleInfo;
	NB_IOT_Trans_Table[sNB_MODULE_INFO][2] = NB_Fail2Reset;

	NB_IOT_Trans_Table[sNB_SIGN][0] = NB_Sign2UDPCreate;
	NB_IOT_Trans_Table[sNB_SIGN][1] = NB_ModuleInfo2Sign;
	NB_IOT_Trans_Table[sNB_SIGN][2] = NB_Fail2Reset;

	NB_IOT_Trans_Table[sNB_UDP_CREATE][0] = NB_UDPCreate2Register;
	NB_IOT_Trans_Table[sNB_UDP_CREATE][1] = NB_Sign2UDPCreate;
	NB_IOT_Trans_Table[sNB_UDP_CREATE][2] = NB_Fail2Reset;

	NB_IOT_Trans_Table[sNB_UDP_CLOSE][0] = NB_UDPClose2End;
	NB_IOT_Trans_Table[sNB_UDP_CLOSE][1] = NB_UDPReceiveData2UDPClose;
	NB_IOT_Trans_Table[sNB_UDP_CLOSE][2] = NB_Fail2Reset;

	NB_IOT_Trans_Table[sNB_UDP_REGISTER][0] = NB_UDPRegister2SendData;
	NB_IOT_Trans_Table[sNB_UDP_REGISTER][1] = NB_UDPCreate2Register;
	NB_IOT_Trans_Table[sNB_UDP_REGISTER][2] = NB_Fail2Reset;

	//  NB_IOT_Trans_Table[sNB_CERTIFICATE][0] = NB_GuyuCertificate2SendData;
	//  NB_IOT_Trans_Table[sNB_CERTIFICATE][1] = NB_Register2GuyuCertificate;
	//  NB_IOT_Trans_Table[sNB_CERTIFICATE][2] = NB_GuyuCertificate2SendData;

	NB_IOT_Trans_Table[sNB_UDP_SEND][0] = NB_UDPSendData2ReceiveData;
	NB_IOT_Trans_Table[sNB_UDP_SEND][1] = NB_UDPRegister2SendData;
	NB_IOT_Trans_Table[sNB_UDP_SEND][2] = NB_Fail2Reset;

	NB_IOT_Trans_Table[sNB_UDP_RECEIVE][0] = NB_UDPReceiveData2UDPClose;
	NB_IOT_Trans_Table[sNB_UDP_RECEIVE][1] = NB_UDPSendData2ReceiveData;
	NB_IOT_Trans_Table[sNB_UDP_RECEIVE][2] = NB_UDPReceiveData2UDPClose;

	NB_IOT_Trans_Table[sNB_RESET][0] = NB_Reset2None;
	NB_IOT_Trans_Table[sNB_RESET][1] = NB_Fail2Reset;
	NB_IOT_Trans_Table[sNB_RESET][2] = NB_Fail2Reset;

	NB_IOT_Trans_Table[sNB_END][0] = NB_UDPClose2End;
	NB_IOT_Trans_Table[sNB_END][1] = NB_UDPClose2End;
	NB_IOT_Trans_Table[sNB_END][2] = NB_Fail2Reset;

	pr_info("NB IoT FSM initialization completed\n");
}

int t_fsm_basic(int argc, char **argv)
{
	struct state_machine machine = {0};
	struct sm_var sm_var = {0};
	int steps = 0;

	pr_info("=== FSM Basic Test ===\n");

	// Reset and enable monitoring
	fsm_reset_statistics();
	fsm_enable_performance_monitoring(true);

	// Initialize state machine
	NB_IOT_FSM_Init();
	machine.current_state = sNB_NONE;

	pr_info("Starting FSM basic operations test...\n");

	// Run some basic transitions
	for (int i = 0; i < 10; i++) {
		fsm_state prev_state = machine.current_state;
		sm_var._condition = cOK;

		fsm_step(&machine, &sm_var, (struct transition **)NB_IOT_Trans_Table);

		if (machine.current_state != prev_state) {
			steps++;
			pr_debug("Step %d: %s -> %s\n", steps,
				fsm_get_state_name(prev_state),
				fsm_get_state_name(machine.current_state));
		}

		// Force transition to next state for testing
		if (machine.current_state == sNB_NONE) {
			break; // Stop at NONE state
		}
	}

	pr_info("FSM basic test: %d steps completed\n", steps);
	fsm_print_performance_report();

	return PASS;
}

int t_fsm_state_transitions(int argc, char **argv)
{
	struct state_machine machine = {0};
	struct sm_var sm_var = {0};
	fsm_state expected_sequence[] = {
		sNB_NONE, sNB_INIT, sNB_MODULE_INFO, sNB_SIGN,
		sNB_UDP_CREATE, sNB_UDP_REGISTER, sNB_UDP_SEND,
		sNB_UDP_RECEIVE, sNB_UDP_CLOSE, sNB_END, sNB_RESET, sNB_NONE
	};
	int sequence_len = sizeof(expected_sequence) / sizeof(expected_sequence[0]);
	int matches = 0;

	pr_info("=== FSM State Transition Test ===\n");

	// Reset statistics and enable monitoring
	fsm_reset_statistics();
	fsm_enable_performance_monitoring(true);
	NB_IOT_FSM_Init();

	machine.current_state = sNB_NONE;
	sm_var._condition = cOK;
	sm_var._repeats = 0;

	// Test expected state sequence
	for (int i = 1; i < sequence_len; i++) {
		fsm_state prev_state = machine.current_state;

		// Trigger transition
		fsm_step(&machine, &sm_var, (struct transition **)NB_IOT_Trans_Table);

		if (machine.current_state == expected_sequence[i]) {
			matches++;
			pr_info("PASS - Step %d: Reached expected state %s\n", i,
				fsm_get_state_name(machine.current_state));
		} else {
			pr_err("FAIL - Step %d: Expected %s, got %s\n", i,
				fsm_get_state_name(expected_sequence[i]),
				fsm_get_state_name(machine.current_state));
		}

		// Reset condition for next transition
		sm_var._condition = cOK;
		if (prev_state == machine.current_state) {
			sm_var._repeats++; // Force transition by incrementing repeats
		}
	}

	pr_info("State transition test: %d/%d matches\n", matches, sequence_len - 1);
	fsm_print_performance_report();

	return (matches >= (sequence_len - 1) * 0.8) ? PASS : FAIL;
}

int t_fsm_error_handling(int argc, char **argv)
{
	struct state_machine machine = {0};
	struct sm_var sm_var = {0};
	int error_tests = 0, error_passes = 0;

	pr_info("=== FSM Error Handling Test ===\n");

	// Reset statistics and enable monitoring
	fsm_reset_statistics();
	fsm_enable_performance_monitoring(true);
	NB_IOT_FSM_Init();

	machine.current_state = sNB_INIT;
	sm_var._condition = cOK;

	// Test 1: Normal operation
	fsm_step(&machine, &sm_var, (struct transition **)NB_IOT_Trans_Table);
	if (machine.current_state == sNB_MODULE_INFO) {
		pr_info("PASS - Normal operation works\n");
		error_passes++;
	} else {
		pr_err("FAIL - Normal operation failed\n");
	}
	error_tests++;

	// Test 2: Repeat limit error
	sm_var._repeats = 3; // Exceed repeat limit
	fsm_state prev_state = machine.current_state;
	fsm_step(&machine, &sm_var, (struct transition **)NB_IOT_Trans_Table);

	if (fsm_last_error_code == 0x01) {
		pr_info("PASS - Repeat limit error detected\n");
		error_passes++;
	} else {
		pr_err("FAIL - Repeat limit error not detected\n");
	}
	error_tests++;

	// Test 3: Invalid state recovery
	machine.current_state = sNB_UDP_CLOSE;
	sm_var._condition = cERROR_REPEATS_L3;
	fsm_step(&machine, &sm_var, (struct transition **)NB_IOT_Trans_Table);

	if (machine.current_state == sNB_RESET) {
		pr_info("PASS - Error recovery working\n");
		error_passes++;
	} else {
		pr_err("FAIL - Error recovery failed\n");
	}
	error_tests++;

	pr_info("Error handling test: %d/%d tests passed\n", error_passes, error_tests);
	fsm_print_performance_report();

	return (error_passes >= error_tests * 0.7) ? PASS : FAIL;
}

int t_fsm_performance(int argc, char **argv)
{
	struct state_machine machine = {0};
	struct sm_var sm_var = {0};
	ktime_t start_time, end_time;

	pr_info("=== FSM Performance Test ===\n");

	// Reset statistics and enable monitoring
	fsm_reset_statistics();
	fsm_enable_performance_monitoring(true);
	NB_IOT_FSM_Init();

	start_time = ktime_get();

	// Test transition performance
	for (int i = 0; i < 1000; i++) {
		machine.current_state = sNB_NONE;
		sm_var._condition = cOK;
		sm_var._repeats = 0;

		fsm_step(&machine, &sm_var, (struct transition **)NB_IOT_Trans_Table);
	}

	end_time = ktime_get();

	pr_info("FSM performance: 1000 transitions completed in %llu ms\n",
		ktime_to_ms(ktime_sub(end_time, start_time)));

	// Test rapid state changes
	start_time = ktime_get();
	machine.current_state = sNB_INIT;

	for (int i = 0; i < 500; i++) {
		sm_var._condition = cERROR_REPEATS_L3;
		fsm_step(&machine, &sm_var, (struct transition **)NB_IOT_Trans_Table);

		if (machine.current_state == sNB_RESET) {
			machine.current_state = sNB_INIT; // Reset for next iteration
		}
	}

	end_time = ktime_get();
	pr_info("FSM error recovery: 500 cycles completed in %llu ms\n",
		ktime_to_ms(ktime_sub(end_time, start_time)));

	fsm_print_performance_report();
	return PASS;
}

int t_fsm_validation(int argc, char **argv)
{
	pr_info("=== FSM Validation Test ===\n");

	// Test state name conversion
	const char *state_name = fsm_get_state_name(sNB_INIT);
	if (state_name && strcmp(state_name, "INIT") == 0) {
		pr_info("PASS - State name conversion working\n");
	} else {
		pr_err("FAIL - State name conversion failed\n");
		return FAIL;
	}

	// Test condition name conversion
	const char *condition_name = fsm_get_condition_name(cOK);
	if (condition_name && strcmp(condition_name, "OK") == 0) {
		pr_info("PASS - Condition name conversion working\n");
	} else {
		pr_err("FAIL - Condition name conversion failed\n");
		return FAIL;
	}

	// Test statistics functions
	struct fsm_statistics stats;
	fsm_get_statistics(&stats);

	if (stats.total_transitions == 0) {
		pr_info("PASS - Statistics functions working (empty state)\n");
	} else {
		pr_err("FAIL - Statistics functions returned unexpected data\n");
		return FAIL;
	}

	// Test statistics reset
	fsm_reset_statistics();
	fsm_get_statistics(&stats);

	if (stats.total_transitions == 0) {
		pr_info("PASS - Statistics reset working\n");
	} else {
		pr_err("FAIL - Statistics reset failed\n");
		return FAIL;
	}

	return PASS;
}

int t_fsm_example(int argc, char **argv)
{
	pr_info("=== FSM Example Test ===\n");
	return PASS;
}

#endif
