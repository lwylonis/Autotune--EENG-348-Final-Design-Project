#include <Arduino.h>
#include <math.h>
#include <stdint.h>
#include "pitch_detection.h"

// Pin definitions
#define LED_RED_PIN 11
#define LED_GREEN_PIN 13
#define LED_BLUE_PIN 10
#define ENCODER_CLK 8
#define ENCODER_DT 9
#define ENCODER_BUTTON 12
#define AUDIO_PIN A5
#define MODE_SWITCH_PIN A0

// Buffer and playback constants
#define BUFFER_LENGTH 512
#define BUFFER_MASK (BUFFER_LENGTH - 1)
#define PHASE_SHIFT 7
#define SPEED_CENTER 128
#define SPEED_MIN 1
#define SPEED_MAX 255

// Snap frequency to nearest semitone and update LEDs
static float snapToNearestNote(float frequency) {
    float semiPosition = 12.0f * M_LOG2E * logf(frequency / 65.41f);
    int16_t semiIndex = (int16_t)roundf(semiPosition);
    float semiDiff = semiIndex - semiPosition;
    int redLevel = constrain((int)roundf(semiDiff * 512.0f), 0, 255);
    int blueLevel = constrain((int)roundf((0.5f - semiDiff) * 512.0f), 0, 255);
    analogWrite(LED_RED_PIN, redLevel);
    analogWrite(LED_BLUE_PIN, blueLevel);
    return 65.41f * powf(2.0f, semiIndex / 12.0f);
}

// ADC ISR: fill ring buffer, output tone, capture samples
ISR(ADC_vect) {
    static uint8_t  ringBuffer[BUFFER_LENGTH];
    static uint16_t writePos = 0;
    static uint16_t phasePosition = 0;
    static uint16_t capturePos = BUFFER_LENGTH;

    uint8_t sampleVal = ADCH;
    ringBuffer[writePos] = sampleVal;
    writePos = (writePos + 1) & BUFFER_MASK;
    phasePosition += playbackSpeed;
    PORTD = ringBuffer[phasePosition >> PHASE_SHIFT];
    if (captureFlag) {
        if (capturePos >= BUFFER_LENGTH) capturePos = 0;
        capBuffer[capturePos++] = sampleVal;
        if (capturePos >= BUFFER_LENGTH) {
            captureFlag = false;
            capturePos = BUFFER_LENGTH;
        }
    }
}

// Encoder ISR: adjust speed
ISR(PCINT0_vect) {
    if (digitalRead(ENCODER_BUTTON) == LOW) adjustment = 0;
    static bool lastClkState = true;
    bool clkState = digitalRead(ENCODER_CLK);
    if (clkState != lastClkState) {
        lastClkState = clkState;
        if (digitalRead(ENCODER_DT) == clkState) {
            if (adjustment > -128) adjustment--;
        } else {
            if (adjustment < 127) adjustment++;
        }
    }
}
static uint8_t capBuffer[BUFFER_LENGTH];
volatile uint8_t playbackSpeed = SPEED_CENTER;
volatile int8_t adjustment = 0;
volatile bool captureFlag = false;
void setup() {
    noInterrupts();
    // Configure I/O
    pinMode(AUDIO_PIN, INPUT);
    pinMode(MODE_SWITCH_PIN, INPUT_PULLUP);
    for (uint8_t pin = 0; pin < 8; ++pin) pinMode(pin, OUTPUT);
    pinMode(LED_RED_PIN, OUTPUT);
    pinMode(LED_GREEN_PIN, OUTPUT);
    pinMode(LED_BLUE_PIN, OUTPUT);
    pinMode(ENCODER_CLK, INPUT);
    pinMode(ENCODER_DT, INPUT);
    pinMode(ENCODER_BUTTON, INPUT_PULLUP);
    // Initialize detector
    pitch_init(0.05f);
    // ADC: AVcc ref, left adjust, channel A5
    ADMUX  = (1<<REFS0)|(1<<ADLAR)|AUDIO_PIN;
    ADCSRA = (1<<ADPS2)|(1<<ADPS1)|(1<<ADATE)|(1<<ADEN)|(1<<ADIE)|(1<<ADSC);
    ADCSRB = 0;
    // Enable pin-change interrupts for encoder
    PCMSK0 = bit(PCINT0)|bit(PCINT1)|bit(PCINT4);
    PCIFR  = bit(PCIF0);
    PCICR  = bit(PCIE0);
    interrupts();
}

void loop() {
    if (digitalRead(MODE_SWITCH_PIN) != HIGH) {
        digitalWrite(LED_GREEN_PIN, HIGH);
        playbackSpeed = constrain(SPEED_CENTER + adjustment, SPEED_MIN, SPEED_MAX);
        analogWrite(LED_RED_PIN,  SPEED_CENTER - 1 - adjustment);
        analogWrite(LED_BLUE_PIN, SPEED_CENTER + adjustment);
    } else {
        digitalWrite(LED_GREEN_PIN, LOW);
        captureFlag = true;
        while (captureFlag) {};
        float detected = roundf(pitch_detect(capBuffer));
        if (detected >= 78.0f) {
            static float smoothFreq = 0;
            smoothFreq = 0.5f * (smoothFreq + detected);
            float targetFreq = snapToNearestNote(smoothFreq);
            playbackSpeed = constrain((int)roundf(targetFreq * SPEED_CENTER / smoothFreq) + adjustment, SPEED_MIN, SPEED_MAX);
        } else {
            playbackSpeed = constrain(SPEED_CENTER + adjustment, SPEED_MIN, SPEED_MAX);
            analogWrite(LED_RED_PIN,  0);
            analogWrite(LED_BLUE_PIN, 0);
        }
    }
}