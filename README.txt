Pitch Modification/Autotune

Authors: Leo Wylonis and Izzy Farrow

Overview
This Arduino sketch captures live audio at roughly 19 kHz, analyzes its fundamental pitch over short buffers, and immediately adjusts playback speed to bring the signal onto the nearest musical semitone. It combines a lightweight pitch-detection module with an 8-bit DAC so you can hear the output in real time.

User Interface
A three-color LED acts as your tuning meter: red when the input is flat, blue when it’s sharp, and green when the pitch is locked on target. A simple slide switch toggles between manual” mode—where a rotary encoder directly shifts playback speed—and automatic “autotune” mode—where the sketch captures a block of samples, computes the closest musical note, and then continually nudges playback speed until you’re in tune. Rotating the encoder gives you fine, real-time control over speed and tuning bias.

Setup & Wiring
Audio from a microphone preamp or line‐level source feeds the Arduino’s analog input. The digital I/O pins form an 8-bit DAC that drives your speaker or amplifier. The three LEDs share PWM pins and common power/ground rails. The encoder’s two signals and built-in button use pin-change interrupts, and a simple toggle switch on an analog pin flips between modes. All you need is common 5 V, ground, and the wiring diagram in the comments to get started.

How to Build
Copy sketch into a single folder, wire circuit according to schematic, and run with your choice of audio input!

What Happens Under the Hood
– Continuous Sampling: The ADC fires an interrupt at ~38 kHz, but only the top 8 bits are used to sample at ~19 kHz.
– Pitch Detection: A time-domain based difference function runs over 512 samples to find the signal’s period.
– Speed Adjustment: In manual mode, encoder turns adjust a speed multiplier. In autotune mode, the detected frequency is compared to a semitone scale, and the speed multiplier is incrementally changed until the LEDs indicate on-pitch.
