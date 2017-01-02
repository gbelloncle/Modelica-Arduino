#include "SoftArduino.h"
#include "ModelicaUtilities.h"

extern "C" {

SoftArduino SoftArduino::instance;

#define INSTANCE SoftArduino::instance


void SoftInterrupt::update(int potential) {

	bool executeISR = false;

	switch(mode) {
	case RISING:
		executeISR = prePotential == LOW && potential == HIGH;
		break;
	case FALLING:
		executeISR = prePotential == HIGH && potential == LOW;
		break;
	case CHANGE:
		executeISR = prePotential != potential;
		break;
	}

	prePotential = potential;

	if (executeISR) {
		// execute the service routine
		isr();
	}

}

void pinMode(uint8_t pin, uint8_t mode) {
	//ModelicaFormatMessage("pinMode(%d, %d)\n", pin, mode);
	SoftArduino::instance.portMode[pin] = mode;
}

void digitalWrite(uint8_t pin, uint8_t val) {
	SoftArduino::instance.pulseWidth[pin] = (val == HIGH) ? 255 : 0;
	//ModelicaFormatMessage("digitalWrite(%d, %d) -> %d\n", pin, val, SoftArduino::instance.pulseWidth[pin]);
}

int digitalRead(uint8_t pin) {
	// ModelicaFormatMessage("digitalRead(%d) -> %f\n", pin, instance.digital[pin]);
	return SoftArduino::instance.digital[pin] > 2.5 ? HIGH : LOW;
}

int analogRead(uint8_t pin) {

	if (pin < A0 || pin > A7) {
		ModelicaFormatError("Illegal analog pin: %d\n", pin);
	}

	// TODO: clip [0, 1023]?

	//ModelicaFormatMessage("analogRead(%d) -> %d\n", pin, val);
	
	return SoftArduino::instance.analog[pin - A0];
}

void analogReference(uint8_t mode) {
	
	switch(mode) {
	case DEFAULT:
	//case INTERNAL:
	//case INTERNAL1V1:
	case EXTERNAL:
		SoftArduino::instance.analogReferenceMode = mode;
		break;
	default:
		ModelicaFormatError("Illegal analog reference mode: %d\n", mode);
		break;
	}
}

void analogWrite(uint8_t pin, int val) {
	SoftArduino::instance.portMode[pin] = OUTPUT;
	SoftArduino::instance.pulseWidth[pin] = val; 
}

void delay(unsigned long ms) {

	//ModelicaFormatMessage("delay(%d) at %g s\n", ms, INSTANCE.time * 1e-6);
	
	const unsigned long end_time = SoftArduino::instance.time + ms * 1000;

	while (SoftArduino::instance.time < end_time) {
		// idle
	}
}

void delayMicroseconds(unsigned int us) {
	
	const unsigned long end_time = SoftArduino::instance.time + us;
	
	while (SoftArduino::instance.time < end_time) {
		// idle
	}
}

unsigned long millis() {
	return SoftArduino::instance.time / 1000;
}

void attachInterrupt(uint8_t interrupt, void (*isr)(void), int mode) {

	ModelicaFormatMessage("attachInterrupt(%d, %d, %d)\n", interrupt, isr, mode);

	if (interrupt != 0 && interrupt != 1) {
		ModelicaFormatError("Illegal interrupt: %d", interrupt);
	}

	int pin = interruptToDigitalPin(interrupt);
	int potential = INSTANCE.digital[pin];
	INSTANCE.interrupts[interrupt] = new SoftInterrupt(isr, mode, potential);
}

void detachInterrupt(uint8_t interrupt) {
	auto ir = INSTANCE.interrupts[interrupt];
	INSTANCE.interrupts[interrupt] = nullptr;
	delete ir;
}

void interrupts() { 
	INSTANCE.interruptsEnabled = true;
}

void noInterrupts() {
	INSTANCE.interruptsEnabled = false;
}

unsigned long pulseIn(uint8_t pin, uint8_t state, unsigned long timeout) {

	unsigned long currentTime = INSTANCE.time;
	const unsigned long startTime = currentTime;
	const unsigned long endTime = startTime + timeout;
	uint8_t preState = INSTANCE.digital[pin];

	while(currentTime < endTime) {

		const uint8_t currentState = INSTANCE.digital[pin];
		currentTime = INSTANCE.time;

		if (preState != currentState && currentState == state) {
			return currentTime - startTime;
		}

		preState = currentState;
	}

	// timed out
	return 0;
}

unsigned long pulseInLong(uint8_t pin, uint8_t state, unsigned long timeout) {
	return pulseIn(pin, state, timeout);
}

}
