#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/errno.h>

#include "config.h"
#include "pid.h"

void pid_positional_init(struct pid_positional *ctrl, s32 kp, s32 ki, s32 kd)
{
	ctrl->type = PID_POSITIONAL;

	ctrl->kp = kp;
	ctrl->ki = ki;
	ctrl->kd = kd;

	ctrl->d_alpha     = SLAVE_PID_D_ALPHA;
	ctrl->d_filtered  = 0;

	ctrl->integral    = 0;
	ctrl->int_min     = SLAVE_PID_INTEGRAL_MIN;
	ctrl->int_max     = SLAVE_PID_INTEGRAL_MAX;

	ctrl->out_min     = SLAVE_PID_OUTPUT_MIN;
	ctrl->out_max     = SLAVE_PID_OUTPUT_MAX;
	ctrl->deadband    = SLAVE_PID_DEADBAND;
	ctrl->slew_rate   = SLAVE_PID_SLEW_RATE;

	ctrl->prev_meas   = 0;
	ctrl->prev_err    = 0;
	ctrl->prev_output = 0;

	ctrl->last_p      = 0;
	ctrl->last_i      = 0;
	ctrl->last_d      = 0;
	ctrl->last_err    = 0;
}

void pid_positional_reset(struct pid_positional *ctrl)
{
	ctrl->integral    = 0;
	ctrl->d_filtered  = 0;
	ctrl->prev_meas   = 0;
	ctrl->prev_err    = 0;
	ctrl->prev_output = 0;
	ctrl->last_p      = 0;
	ctrl->last_i      = 0;
	ctrl->last_d      = 0;
	ctrl->last_err    = 0;
}

/**
 * @brief Bumpless-transfer preload.
 *
 * Sets integral = current_output << PID_SCALE_BITS so the first update()
 * call continues smoothly from the given output.  Clamps integral to the
 * configured range.  Clears d_filtered to avoid a derivative spike on the
 * first measurement sample.
 */
void pid_positional_set_output(struct pid_positional *ctrl, s32 current_output)
{
	ctrl->integral = (s64)current_output << PID_SCALE_BITS;

	if (ctrl->integral > (s64)ctrl->int_max << PID_SCALE_BITS) {
		ctrl->integral = (s64)ctrl->int_max << PID_SCALE_BITS;
	}
	if (ctrl->integral < (s64)ctrl->int_min << PID_SCALE_BITS) {
		ctrl->integral = (s64)ctrl->int_min << PID_SCALE_BITS;
	}

	ctrl->prev_output = clamp(current_output, ctrl->out_min, ctrl->out_max);
	ctrl->d_filtered  = 0;
}

/**
 * @brief Run one PID iteration (positional form, D-on-measurement).
 *
 * Algorithm per tick (10 ms nominal):
 *   1. Compute error and deadband check
 *   2. D-on-M: d_raw = prev_meas - meas,  filter,  scale by kd
 *   3. P term: p = (kp * err) >> 8
 *   4. Pre-emptive anti-windup:  if P+D already saturating in the same
 *      direction as err, skip integral this tick
 *   5. Integral:  accumulate ki*err into s64,  clamp to int_min/max
 *   6. Sum output = p + d + (integral >> 8),  clamp to out_min/max
 *   7. Slew-rate limit output delta vs. prev_output
 *   8. Save diagnostics and history
 */
s32 pid_positional_update(struct pid_positional *ctrl, s32 set_point,
	s32 meas, u32 dt_ms)
{
	s32 err, p, d, d_raw, output;
	s32 pd_tentative;
	s32 delta;
	s64 integral_limit_lo, integral_limit_hi;

	/* ── 0. 误差计算 ────────────────────────────────────── */
	err = set_point - meas;

	/* ── 1. 死区检查 ────────────────────────────────────── */
	if (ctrl->deadband > 0 &&
		err >= -ctrl->deadband && err <= ctrl->deadband) {
		/* Freeze: keep integral, output, and diagnostic as last tick */
		ctrl->prev_err = err;
		/* prev_meas DOES update — we still need it once we leave deadband */
		ctrl->prev_meas = meas;
		ctrl->last_p   = 0;
		ctrl->last_i   = 0;
		ctrl->last_d   = 0;
		ctrl->last_err = err;
		return ctrl->prev_output;
	}

	/* ── 2. D-on-M + 一阶低通滤波 ────────────────────────── */
	d_raw = ctrl->prev_meas - meas;
	ctrl->prev_meas = meas;

	ctrl->d_filtered += ((d_raw - ctrl->d_filtered) * ctrl->d_alpha) >> PID_SCALE_BITS;

	d = (ctrl->kd * ctrl->d_filtered) >> PID_SCALE_BITS;

	/* ── 3. Proportional term ───────────────────────────── */
	p = (ctrl->kp * err) >> PID_SCALE_BITS;

	/* ── 4. 本拍预判抗饱和 ────────────────────────────────── */
	pd_tentative = p + d;

	integral_limit_lo = (s64)ctrl->int_min << PID_SCALE_BITS;
	integral_limit_hi = (s64)ctrl->int_max << PID_SCALE_BITS;

	/*
	 * Skip integral if the tentative P+D already exceeds output limits
	 * AND the error direction would push further into saturation.
	 * This eliminates the one-tick lag of traditional anti-windup.
	 */
	if (!((pd_tentative > ctrl->out_max && err > 0) ||
		(pd_tentative < ctrl->out_min && err < 0))) {
		/* Accumulate ki*err into s64 — NO right-shift here */
		ctrl->integral += (s64)ctrl->ki * (s64)err;

		/* Clamp integral (limits are in output-scale units) */
		if (ctrl->integral > integral_limit_hi) {
			ctrl->integral = integral_limit_hi;
		}
		if (ctrl->integral < integral_limit_lo) {
			ctrl->integral = integral_limit_lo;
		}
	}

	/* ── 5. 输出合成（积分右移 8 位） ────────────────────── */
	output = p + d + (s32)(ctrl->integral >> PID_SCALE_BITS);

	output = clamp(output, ctrl->out_min, ctrl->out_max);

	/* ── 6. 变化率限制 ───────────────────────────────────── */
	if (ctrl->slew_rate > 0) {
		delta = output - ctrl->prev_output;
		if (delta > ctrl->slew_rate) {
			output = ctrl->prev_output + ctrl->slew_rate;
		} else if (delta < -ctrl->slew_rate) {
			output = ctrl->prev_output - ctrl->slew_rate;
		}
	}

	/* ── 7. 分量诊断 ─────────────────────────────────────── */
	ctrl->last_p   = p;
	ctrl->last_i   = (s32)(ctrl->integral >> PID_SCALE_BITS);
	ctrl->last_d   = d;
	ctrl->last_err = err;

	ctrl->prev_err    = err;
	ctrl->prev_output = output;

	return output;
}

/* ═══════════════════════════════════════════════════════════════════════
 *  Relay-Feedback Auto-Tuning  (Aström-Hägglund)
 * ═══════════════════════════════════════════════════════════════════════ */

/** @brief Maximum samples before declaring tuning timeout (~10 s @ 10 ms) */
#define RELAY_TIMEOUT_SAMPLES  1000

/*
 * If bias == 0 it will be auto-captured from pid->prev_output
 * when pid_relay_start() is called.
 */
void pid_relay_init(struct pid_relay_tuner *t, struct pid_positional *pid,
	s32 relay_amp, s32 bias, s32 set_point, u32 min_cycles)
{
	t->pid            = pid;
	t->relay_amp      = relay_amp;
	t->bias           = bias;
	t->set_point      = set_point;
	t->min_cycles     = min_cycles;
	t->min_half_ticks = SLAVE_PID_RELAY_MIN_HALF_TICKS;

	t->state       = RELAY_IDLE;
	t->sample_count = 0;

	t->prev_err        = 0;
	t->last_relay_sign  = 1;   /* start with high output on first err sample */
	t->in_positive_half = false;
	t->peak_pos        = 0;
	t->peak_neg        = 0;
	t->half_ticks      = 0;
	t->last_half_ticks = 0;
	t->last_peak_pos   = 0;

	t->sum_period     = 0;
	t->sum_amplitude  = 0;
	t->cycle_count    = 0;

	t->ku = 0;
	t->tu = 0;
}

/*
 * Auto-captures bias from pid->prev_output if bias was set to 0 at init.
 * Resets accumulators and transitions to RELAY_RUNNING.
 */
void pid_relay_start(struct pid_relay_tuner *t)
{
	if (!t->pid) {
		return;
	}

	/* Auto-capture bias from last known output */
	if (t->bias == 0) {
		t->bias = t->pid->prev_output;
	}

	t->state        = RELAY_RUNNING;
	t->sample_count = 0;

	t->prev_err         = 0;
	t->last_relay_sign   = 1;   /* start with high output on first err sample */
	t->in_positive_half  = false;
	t->peak_pos         = 0;
	t->peak_neg         = 0;
	t->half_ticks       = 0;
	t->last_half_ticks  = 0;
	t->last_peak_pos    = 0;

	t->sum_period    = 0;
	t->sum_amplitude = 0;
	t->cycle_count   = 0;

	pr_info("relay_tuner: started, bias=%d, amp=%d, set_point=%d, min_cycles=%u\n",
		t->bias, t->relay_amp, t->set_point, t->min_cycles);
}

/*
 * Compute Ku/Tu → Kp/Ki/Kd via Ziegler-Nichols and write to PID.
 * Uses conservative Ki factor 1000 (Ti=Tu) for slow motor plants.
 */
static void relay_finish_tuning(struct pid_relay_tuner *t)
{
	s32 avg_half_period, avg_amplitude;
	s32 ku_q8, tu_ticks;
	s32 kp_q8, ki_q8, kd_q8;

	avg_half_period = (s32)(t->sum_period / (2 * t->cycle_count));
	avg_amplitude   = (s32)(t->sum_amplitude / t->cycle_count);

	if (avg_half_period <= 0 || avg_amplitude <= 0) {
		pr_err("relay_tuner: invalid averages half_period=%d amplitude=%d\n",
			avg_half_period, avg_amplitude);
		t->state = RELAY_FAILED;
		return;
	}

	/*
	 * Critical gain:  Ku = 4d / (π * A)
	 *
	 * Using rational approximation π ≈ 355/113:
	 *   Ku = (4 * d * 113) / (355 * A)
	 *
	 * In Q8.8:
	 *   Ku_q8 = (4 * relay_amp * 113 * 256) / (355 * A)
	 */
	ku_q8 = (s32)(((s64)4 * (s64)t->relay_amp * 113 * PID_SCALE) / (355 * (s64)avg_amplitude));

	/*
	 * Critical period in ticks:  Tu = sum_period / cycle_count
	 */
	tu_ticks = (s32)(t->sum_period / t->cycle_count);

	/*
	 * Ziegler-Nichols with conservative factor 0.5:
	 *
	 *   Kp = 0.3 * Ku * 0.5  = 0.15 * Ku
	 *   Ki = Kp / Ti with Ti = Tu (full-period, factor 1000 for slow motors)
	 *   Kd = (Kp * Tu) / 8
	 */
	kp_q8 = (ku_q8 * 15) / 100; /* 0.15 * Ku */

	if (tu_ticks > 0 && SLAVE_PID_PERIOD_MS > 0) {
		ki_q8 = (s32)(((s64)1000 * kp_q8) / ((s64)tu_ticks * SLAVE_PID_PERIOD_MS));
	} else {
		ki_q8 = 0;
	}

	if (SLAVE_PID_PERIOD_MS > 0) {
		kd_q8 = (s32)(((s64)kp_q8 * tu_ticks * SLAVE_PID_PERIOD_MS) / 8000);
	} else {
		kd_q8 = 0;
	}

	pr_info("relay_tuner: done  Ku=%d(Q8.8)  Tu=%d ticks\n", ku_q8, tu_ticks);
	pr_info("relay_tuner: Z-N  Kp=%d  Ki=%d  Kd=%d (Q8.8)\n", kp_q8, ki_q8, kd_q8);

	/* Write gains and reset PID state */
	t->pid->kp = kp_q8;
	t->pid->ki = ki_q8;
	t->pid->kd = kd_q8;

	/* Store results for caller inspection */
	t->ku = ku_q8;
	t->tu = tu_ticks;

	pid_positional_reset(t->pid);

	t->state = RELAY_DONE;
}

/**
 * @brief Feed one measurement sample to the tuner.
 *
 * When RELAY_RUNNING:
 *   - Detect zero-crossings to identify half-cycle boundaries
 *   - On positive→negative crossing: save peak_pos + half_ticks
 *   - On negative→positive crossing: accumulate cycle stats
 *   - Return relay control output: bias ± relay_amp
 *
 * When RELAY_DONE or RELAY_FAILED: return 0.
 *
 * When the required number of cycles is collected, computes Ku/Tu
 * via the Astrom-Hägglund formula and writes Kp/Ki/Kd to the PID
 * controller using Ziegler-Nichols rules with a conservative 0.5 factor.
 */
s32 pid_relay_update(struct pid_relay_tuner *t, s32 meas)
{
	s32 err, relay_output;
	s32 amplitude;         /* peak-to-peak amplitude of a full cycle */
	u32 period_ticks;      /* period of a full cycle in ticks */
	s32 desired_sign;      /* +1 = high (bias+amp), -1 = low (bias-amp) */
	bool do_switch;        /* Relay actually changes output this tick */

	if (t->state == RELAY_IDLE) {
		return t->bias;
	}

	if (t->state == RELAY_DONE || t->state == RELAY_FAILED) {
		return 0;
	}

	if (!t->pid) {
		return 0;
	}

	t->sample_count++;
	if (t->sample_count > RELAY_TIMEOUT_SAMPLES) {
		pr_warn("relay_tuner: timeout after %u samples\n", t->sample_count);
		t->state = RELAY_FAILED;
		return 0;
	}

	err = t->set_point - meas;

	/* ── Determine desired relay output from error sign ──────── */
	desired_sign = (err >= 0) ? 1 : -1;

	/*
	 * Enforce minimum half-cycle: the relay must hold its current
	 * output for at least min_half_ticks before being allowed to
	 * flip.  This prevents the relay from chattering faster than
	 * the plant can respond, which would produce tiny oscillation
	 * amplitude and inflated Ku.
	 */
	if (t->half_ticks && t->half_ticks < t->min_half_ticks) {
		desired_sign = t->last_relay_sign;
	}

	do_switch = (desired_sign != t->last_relay_sign);

	/* ── Compute actual relay output ─────────────────────────── */
	if (desired_sign >= 0) {
		relay_output = t->bias + t->relay_amp;
	} else {
		relay_output = t->bias - t->relay_amp;
	}

	/* ── Track peaks within the current half-cycle ───────────── */
	if (err >= 0) {
		if (err > t->peak_pos) {
			t->peak_pos = err;
		}
	} else {
		if (err < t->peak_neg) {
			t->peak_neg = err;
		}
	}
	t->half_ticks++;

	/* ── Relay switching → treat as half-cycle boundary ──────── */
	if (do_switch) {
		if (t->last_relay_sign >= 0) {
			/*
			 * Relay was HIGH, now switching to LOW.
			 * Positive half-cycle → negative half.
			 */
			t->last_peak_pos   = t->peak_pos;
			t->last_half_ticks = t->half_ticks - 1;

			t->peak_pos   = 0;
			t->half_ticks = 1;   /* start at 1 so min-hold applies immediately */

		} else {
			/*
			 * Relay was LOW, now switching to HIGH.
			 * Negative half-cycle → positive half.
			 * A full cycle has just completed.
			 */
			period_ticks = t->last_half_ticks + (t->half_ticks - 1);
			amplitude    = t->last_peak_pos - t->peak_neg;

			if (period_ticks > 0 && amplitude > 0) {
				t->sum_period    += period_ticks;
				t->sum_amplitude += amplitude;
				t->cycle_count++;

				pr_debug("relay_tuner: cycle %u  period=%u ticks  amplitude=%d\n",
					t->cycle_count, period_ticks, amplitude);
			}

			t->peak_neg   = 0;
			t->half_ticks = 1;   /* start at 1 so min-hold applies immediately */
		}

		/* Start new half-cycle peak tracking */
		if (err >= 0) {
			t->peak_pos = err;
		} else {
			t->peak_neg = err;
		}

		t->last_relay_sign = desired_sign;
		t->in_positive_half = (desired_sign >= 0);
	}

	t->prev_err = err;

	/* ── Enough cycles collected? ────────────────────────── */
	if (t->cycle_count >= t->min_cycles) {
		relay_finish_tuning(t);
		return 0;
	}

	return relay_output;
}
