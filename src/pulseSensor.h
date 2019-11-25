#ifndef PULSE_SENSOR_H
#define PULSE_SENSOR_H

#include <stdbool.h>
#include <stdio.h>

// Pulse detection output variables.
// Volatile because our pulse detection code could be called from an Interrupt



volatile int BPM;                // int that holds raw Analog in 0. updated every call to readSensor()
volatile int Signal;             // holds the latest incoming raw data (0..1023)
volatile int IBI;                // int that holds the time interval (ms) between beats! Must be seeded!
volatile bool Pulse;          // "True" when User's live heartbeat is detected. "False" when not a "live beat".
volatile bool QS;             // The start of beat has been detected and not read by the Sketch.
volatile int threshSetting;      // used to seed and reset the thresh variable
volatile int amp;                         // used to hold amplitude of pulse waveform, seeded (sample value)
volatile unsigned long lastBeatTime;      // used to find IBI. Time (sampleCounter) of the previous detected beat start.



#endif
