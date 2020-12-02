/*============================================================================
  
  pwm.h

  Functions to do simple software-based PWM control.

  Copyright (c)1990-2020 Kevin Boone. Distributed under the terms of the
  GNU Public Licence, v3.0

  ==========================================================================*/
#pragma once

struct PWM;
typedef struct _PWM PWM;

BEGIN_DECLS

/** Create a PWM instance. This method only initializes and allocates 
    memory, so it will always succed. Call pwm_destroy() when finished. */
PWM     *pwm_create (int pin);

/** Tidy up this PWM instance. Implicitly calls pwm_stop(). */
void     pwm_destroy (PWM *pwm);

/** Initialize the GPIO and start the PWM thread. This method can fail,
    because it accesses hardware. If it does, it fills in *error. Caller
    must free *error if it is set. The caller must set the PWM cycle
    length, in microseconds. */
BOOL     pwm_start (PWM *self, int cycle_usec, char **error);

/** Stop the PWM thread, and uninitialze the GPIO. */
void     pwm_stop (PWM *self);

/** Set the PWM output level, as a fraction from 0.0 (off) to 1.0 (high). */
void     pwm_set_duty (PWM *self, double duty);

END_DECLS
