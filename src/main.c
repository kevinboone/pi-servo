/*============================================================================
  
    pi-servo

    A demonstration of software PWM for Raspberry Pi, without libraries.
    
    main.c

    If using this program with the common SG90 micro-servo, the recommended
    PWM frequency is 50Hz (20000 usec cycles), and the acceptable input 
    range, expressed as a fraction, is .025 - 0.125. These values 
    correspond to pulse lengths from 0.5 - 2.5 msec. Note that, at the 
    recommended PWM frequency of 50Hz, that means that only the smallest part 
    of the available PWM range is used.

    Of course, if you're just setting the brightness of an LED, then the full
    output range can be used.

    CPU usage, of course, depends on the number of PWM cycles per second.
    There are typical figures for the Pi 3B+
    50 Hz   Too small to measure
    500 Hz  2-3% CPU
    5000 Hz 12-15% CPU

    Copyright (c)2020 Kevin Boone, GPL v3.0

============================================================================*/
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include "defs.h" 
#include "pwm.h" 

// This is the GPIO pin to connect the servo (or whatever) to
#define PIN 17

/*============================================================================
  main
 
  Note: in a real application, we'd trap signals and ensure that 
  pwm_stop gets called before exit. Otherwise, if the program stops
  unexpectedly, we could get stuck the output fully on.

============================================================================*/
int main (int argc, char **argv)
  {
  argc = argc; argv = argv; // Suppress warnings

  PWM *pwm = pwm_create (PIN);

  char *error = NULL;
  if (pwm_start (pwm, 20000, &error)) // 50 Hz
  //if (pwm_start (pwm, 2000, &error)) // 500 Hz
  //if (pwm_start (pwm, 200, &error)) // 5000 Hz
    {
    pwm_set_duty (pwm, 0.0); // Off

    // Loop, taking a fraction from the user. Set that fraction as the
    //  pwm duty cycle.
    double val = 1.0;
    while (val > 0.0)
      {
      printf ("Set on fraction (0.0-1.0) or a negative number to stop: ");
      fflush (stdout);
      fscanf (stdin, "%lf", &val);
      if (val > 0.0)
        {
        pwm_set_duty (pwm, val);
        printf ("Setting %lf\n", val);
        }
      }

    // Clean up. Important -- this stops the output, and leaves it int
    //   the low state.
    pwm_stop (pwm);
    }
  else
    {
    fprintf (stderr, "Can't start PWM: %s\n", error);
    free (error);
    }
  }


