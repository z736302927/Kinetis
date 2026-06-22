#ifndef PROPORTIONAL_INTEGRAL_DERIVATIVE_H
#define PROPORTIONAL_INTEGRAL_DERIVATIVE_H

#include <linux/types.h>

/**
 * @file pid.h
 * @brief Product-grade positional PID controller with relay auto-tuning
 *
 * Features:
 * - Derivative-on-measurement (D-on-M) — eliminates setpoint-kick
 * - First-order low-pass filter on D term (d_alpha, Q8.8)
 * - s64 integral accumulator for sub-LSB error retention
 * - Pre-emptive anti-windup: tests P+D before deciding whether to integrate
 * - Dual clamping: integral limits + output limits
 * - Deadband: freezes integral and holds output when |err| <= threshold
 * - Slew-rate limiting on output delta
 * - Bumpless transfer (pid_positional_set_output)
 * - Component diagnostics: last_p / last_i / last_d / last_err
 * - Relay-feedback auto-tuning (Astrom-Hägglund method)
 */

/** @brief Q8.8 fixed-point scaling for PID gains */
#define PID_SCALE_BITS    8
#define PID_SCALE         (1 << PID_SCALE_BITS)

enum pid_type {
	PID_POSITIONAL,
	PID_INCREMENTAL,
};

/**
 * @brief Positional-form PID controller
 *
 * Implements the discrete-time positional algorithm:
 *   u(k) = Kp*e(k) + Ki*Σe(k) + Kd*(meas(k-1) - meas(k))
 *
 * All gains are Q8.8 fixed-point.  The integral is accumulated as
 * ki*err (no right-shift) into a s64 to preserve sub-LSB information;
 * the >>8 happens only at output summation.
 */
struct pid_positional {
	enum pid_type type;           /**< Must be first member; set to PID_POSITIONAL */

	/* ── Gains (Q8.8) ───────────────────────────────── */
	s32 kp;                         /**< Proportional gain */
	s32 ki;                         /**< Integral gain */
	s32 kd;                         /**< Derivative gain */

	/* ── D-term low-pass filter (Q8.8) ──────────────── */
	s32 d_alpha;                    /**< Filter coefficient (0=freeze, 256=bypass) */
	s32 d_filtered;                 /**< Filtered derivative value */

	/* ── Integral (s64, raw Q8.8 units) ─────────────── */
	s64 integral;                   /**< Integral accumulator (ku*err units, no right-shift) */
	s32 int_min;                    /**< Integral lower clamp (output-scale) */
	s32 int_max;                    /**< Integral upper clamp (output-scale) */

	/* ── Output limits & extra constraints ──────────── */
	s32 out_min;                    /**< Output lower clamp */
	s32 out_max;                    /**< Output upper clamp */
	s32 deadband;                   /**< |err| <= deadband → freeze integral & hold output */
	s32 slew_rate;                  /**< Max output delta per tick (0=disabled), Q8.8 */

	/* ── Previous-cycle state ───────────────────────── */
	s32 prev_meas;                  /**< Previous measurement (for D-on-M) */
	s32 prev_err;                   /**< Previous error (diagnostics / deadband) */
	s32 prev_output;                /**< Previous output (deadband hold + slew-rate limiting) */

	/* ── Per-cycle diagnostics (read-only to caller) ── */
	s32 last_p;                     /**< Proportional component of last output */
	s32 last_i;                     /**< Integral component of last output (post >>8) */
	s32 last_d;                     /**< Derivative component of last output */
	s32 last_err;                   /**< Error of last cycle */
};

/**
 * @brief Initialise a positional PID controller from config constants.
 * @param ctrl Controller instance
 * @param kp   Proportional gain (Q8.8)
 * @param ki   Integral gain (Q8.8)
 * @param kd   Derivative gain (Q8.8)
 */
void pid_positional_init(struct pid_positional *ctrl, s32 kp, s32 ki, s32 kd);

/**
 * @brief Reset all dynamic state (integral, filters, diagnostics) to zero.
 * @param ctrl Controller instance
 */
void pid_positional_reset(struct pid_positional *ctrl);

/**
 * @brief Bumpless-transfer preload.
 *
 * Sets integral = current_output << PID_SCALE_BITS so the first
 * update() call produces a smooth continuation from the given output.
 *
 * @param ctrl           Controller instance
 * @param current_output Current actuator output value
 */
void pid_positional_set_output(struct pid_positional *ctrl, s32 current_output);

/**
 * @brief Run one PID iteration.
 * @param ctrl      Controller instance
 * @param set_point Desired value (e.g. target RPM)
 * @param meas      Current measurement (e.g. actual RPM)
 * @param dt_ms     Control period in ms (reserved for future variable-rate use)
 * @return          Actuator output (clamped to [out_min, out_max])
 */
s32 pid_positional_update(struct pid_positional *ctrl, s32 set_point,
			  s32 meas, u32 dt_ms);

/*
 * RELAY_IDLE    — not started
 * RELAY_RUNNING — bang-bang active, collecting oscillation data
 * RELAY_DONE    — enough cycles collected, gains written to pid
 * RELAY_FAILED  — timeout or invalid data
 */
enum pid_relay_state {
	RELAY_IDLE,
	RELAY_RUNNING,
	RELAY_DONE,
	RELAY_FAILED,
};

/**
 * @brief Relay-feedback auto-tuner (Astrom-Hägglund method)
 *
 * Applies ±relay_amp bang-bang around bias to induce sustained oscillation,
 * measures Ku and Tu, then computes Kp/Ki/Kd via Ziegler-Nichols with
 * a conservative 0.5 safety factor.
 */
struct pid_relay_tuner {
	struct pid_positional *pid;     /**< Target PID controller to tune */
	s32 relay_amp;                  /**< Relay amplitude (bang-bang step size) */
	s32 bias;                       /**< Operating-point bias (auto-captured from pid->prev_output) */
	s32 set_point;                  /**< Regulation setpoint for tuning */

	u32 min_cycles;                 /**< Minimum oscillation cycles to collect */
	u32 min_half_ticks;             /**< Minimum half-cycle ticks (avoids switching faster than plant can respond) */

	/* ── State ──────────────────────────────────────── */
	enum pid_relay_state state;     /**< Current tuner state */
	u32 sample_count;               /**< Samples since start (timeout guard) */

	/* ── Zero-crossing detection ────────────────────── */
	s32 prev_err;                   /**< Error from previous sample */
	s32 last_relay_sign;            /**< Sign of actual relay output (+1 high, -1 low), for min-half hold */
	bool in_positive_half;          /**< Currently in positive half-cycle */
	s32 peak_pos;                   /**< Peak error in current positive half-cycle */
	s32 peak_neg;                   /**< Peak error in current negative half-cycle */
	u32 half_ticks;                 /**< Tick count of current half-cycle */
	u32 last_half_ticks;            /**< Duration of previous half-cycle */
	s32 last_peak_pos;              /**< Peak of previous positive half-cycle */

	/* ── Accumulated statistics ─────────────────────── */
	s64 sum_period;                 /**< Accumulated full-period ticks */
	s64 sum_amplitude;              /**< Accumulated peak-to-peak amplitude */
	u32 cycle_count;                /**< Number of complete cycles collected */

	/* ── Results ────────────────────────────────────── */
	s32 ku;                         /**< Critical gain (Q8.8) */
	s32 tu;                         /**< Critical period (ticks) */
};

/**
 * @brief Initialise the relay tuner.
 * @param t          Tuner instance
 * @param pid        Target PID controller
 * @param relay_amp  Relay amplitude (bang-bang step)
 * @param bias       Operating bias (0 = auto-capture on start)
 * @param set_point  Regulation setpoint
 * @param min_cycles Minimum cycles to collect before stopping
 */
void pid_relay_init(struct pid_relay_tuner *t, struct pid_positional *pid,
		    s32 relay_amp, s32 bias, s32 set_point, u32 min_cycles);

/**
 * @brief Start the relay tuning process.
 *
 * Captures pid->prev_output as bias if bias==0, resets internal accumulators,
 * and transitions state to RELAY_RUNNING.
 *
 * @param t Tuner instance
 */
void pid_relay_start(struct pid_relay_tuner *t);

/**
 * @brief Feed one measurement sample to the tuner.
 *
 * When RELAY_RUNNING, returns the relay control output (±relay_amp + bias).
 * When RELAY_DONE / RELAY_FAILED, returns 0.
 * Caller should apply this output directly to the actuator while tuning.
 *
 * @param t    Tuner instance
 * @param meas Current measurement
 * @return     Relay output (RELAY_RUNNING), or 0 otherwise
 */
s32 pid_relay_update(struct pid_relay_tuner *t, s32 meas);

#endif /* PROPORTIONAL_INTEGRAL_DERIVATIVE_H */
