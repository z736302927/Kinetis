#ifndef __K_FSM_H
#define __K_FSM_H

#ifdef __cplusplus
extern "C" {
#endif

/* The following program is modified by the user according to the hardware device, otherwise the driver cannot run. */

/* Includes ------------------------------------------------------------------*/
#include <linux/types.h>

#define FSM_SUCCESS             0
#define FSM_FAILED              1
#define FSM_STATES              15
#define FSM_CONDITIONS          3

typedef int fsm_state;
typedef int fsm_condition;

struct sm_var {
	int _repeats;
	fsm_condition _condition;
};

struct state_machine {
	fsm_state current_state;
};

typedef int (*action_type)(struct state_machine *machine, struct sm_var *sm_var);

struct transition {
	fsm_state next;
	action_type action;
};

enum SState {
	sNB_NONE,
	sNB_INIT,
	sNB_MODULE_INFO,
	sNB_SIGN,
	sNB_UDP_CREATE,
	sNB_UDP_CLOSE,
	sNB_UDP_REGISTER,
	sNB_CERTIFICATE,
	sNB_UDP_SEND,
	sNB_UDP_RECEIVE,
	sNB_CoAP_SEVER,
	sNB_CoAP_SEND,
	sNB_CoAP_RECEIVE,
	sNB_RESET,
	sNB_END
};

enum CCondition {
	cOK,
	cERROR_REPEATS_S3, //Less than 3
	cERROR_REPEATS_L3, //greater than 3
};

void fsm_init(void);
fsm_state fsm_step(struct state_machine *machine, struct sm_var *sm_var,
	struct transition **table);

/* The above procedure is modified by the user according to the hardware device, otherwise the driver cannot run. */

#ifdef __cplusplus
}
#endif

#endif /* __K_FSM_H */
