/*==========================================================================
  
    pwm.c

    Simple software PWM for Raspberry Pi GPIO pins. Create an instance
    of this "class" for each pin to be controlled. Each instance
    will create its own thread to do the timing. 

    Typical calling sequence is:

    PWM *pwm = pwm_create ()
    pwm_start (pwm, ...)
    pmw_set_duty (pwm, val1)
    pmw_set_duty (pwm, val2)
    ...
    pwm_stop (pwm)
    pwm_destroy (pwm)


    Copyright (c)2020 Kevin Boone, GPL v3.0

============================================================================*/
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <pthread.h>
#include <fcntl.h>
#include "defs.h" 
#include "pwm.h" 

// PWM structure -- stores all internal data related to this
//  PWM instance
struct _PWM
  {
  int pin; // GPIO pin number
  pthread_t pthread; // Reference to the running thread
  BOOL stop; // Set when pwm_stop() is called, to stop the PWM thead
  int f_value; // Saved file handle for the 'value' pseudo-file
  int cycle_usec; // PWM cycle-length, equals on_usec + off_usec
  int on_usec; // "On" time in usec
  int off_usec; // "Off" time in usec
  };

// Forward references
static int pwm_write_to_file (const char *filename, const char *text);
int pwm_setup_pin (PWM *self);
int pwm_unsetup_pin (PWM *self);
int pwm_set_pin (PWM *self, int value);

/*============================================================================
  pwm_create
============================================================================*/
PWM *pwm_create (int pin)
  {
  PWM *self = malloc (sizeof (PWM));
  memset (self, 0, sizeof (PWM));
  self->pin = pin;
  self->f_value = -1;
  return self;
  }

/*============================================================================
  pwm_destroy
============================================================================*/
void pwm_destroy (PWM *self)
  {
  if (self)
    {
    pwm_stop (self);
    free (self);
    }
  }

/*============================================================================
  pwm_loop

  This is the method that does the real work. It runs continuously until
  pwm_stop is called. All this method does, really, is turn on the GPIO
  pin, wait a bit, turn it off, wait a bit. It's important that the operations
  carried out by this method have the lowest possible overheads, as the
  loop time might be milliseconds, or even microseconds.

  Since writing the value pseudo-file involves a kernel trap, there will
  always be some overhead. For that reason, we handle the "fully on"
  and "fully off" situations differently, and don't try to write a value
  that we'll have to overwrite a millisecond later.

============================================================================*/
void *pwm_loop (void *arg)
  {
  PWM *self = (PWM *)arg;
  while (!self->stop)
    {
    if (self->on_usec != 0)
      pwm_set_pin (self, 1); 
    usleep (self->on_usec);
    if (!self->stop)
      {
      if (self->off_usec != 0)
        pwm_set_pin (self, 0); 
      usleep (self->off_usec);
      }
    }
  return NULL;
  }

/*============================================================================
  pwm_set_duty
============================================================================*/
void pwm_set_duty (PWM *self, double duty)
  {
  assert (self != NULL);
  int on_usec = (int) (self->cycle_usec * duty); 
  int off_usec = self->cycle_usec - on_usec;
  self->on_usec = on_usec;
  self->off_usec = off_usec;
  }

/*============================================================================
  pwm_start
============================================================================*/
BOOL pwm_start (PWM *self, int cycle_usec, char **error)
  {
  assert (self != NULL);
  BOOL ret = FALSE;
  if (pwm_setup_pin (self) == 0)
    {
    self->stop = FALSE;
    self->cycle_usec = cycle_usec;
    self->on_usec = 0;
    self->off_usec = cycle_usec;
    pthread_t pt;
    pthread_create (&pt, NULL, pwm_loop, self);
    pthread_detach (pt); // This doesn't seem to prevent the small mem leak,
                       //  perhaps because the application closes before the
                       //  loop has time to complete nicely.
    self->pthread = pt;
    ret = TRUE;
    }
  else
    {
    if (error)
      asprintf (error, "Can't set up pin: %s", strerror (errno));
    }
  return ret;
  }

/*============================================================================
  pwm_stop
============================================================================*/
void pwm_stop (PWM *self)
  {
  assert (self != NULL);
  self->stop = TRUE;
  pwm_unsetup_pin (self);
  }

/*============================================================================
  pwm_write_to_file
============================================================================*/
static int pwm_write_to_file (const char *filename, const char *text)
  {
  int ret = -1;
  FILE *f = fopen (filename, "w");
  if (f)
    {
    fprintf (f, text);
    fclose (f);
    ret = 0;
    }
  return ret;
  }

/*============================================================================
  pwm_set_pin
============================================================================*/
int pwm_set_pin (PWM *self, int value)
  {
  assert (self != NULL);
  char v[1];
  if (value)
    v[0] = '1';
  else
    v[0] = '0';
  write (self->f_value, &v, 1); 
  return 0;
  }

/*============================================================================
  pwm_setup_pin
============================================================================*/
int pwm_setup_pin (PWM *self)
  {
  assert (self != NULL);
  char s[50];
  snprintf (s, sizeof(s), "%d", self->pin);
  int ret = pwm_write_to_file ("/sys/class/gpio/export", s);
  if (ret == 0)
    {
    char s[50];
    snprintf (s, sizeof(s), "/sys/class/gpio/gpio%d/direction", self->pin);
    pwm_write_to_file (s, "out");
    snprintf (s, sizeof(s), "/sys/class/gpio/gpio%d/value", self->pin);
    self->f_value = open (s, O_WRONLY);
    if (self->f_value >= 0) ret = 0;
    }
  return ret;
  }

/*============================================================================
  pwm_unsetup_pin
============================================================================*/
int pwm_unsetup_pin (PWM *self)
  {
  assert (self != NULL);
  char s[50];
  snprintf (s, sizeof(s), "%d", self->pin);
  int ret = pwm_write_to_file ("/sys/class/gpio/unexport", s);
  if (self->f_value >= 0)
    close (self->f_value);
  self->f_value = -1;
  return ret;
  }


