#include <Arduino.h>
#include <gdb_hooks.h>
#ifdef ENABLE_GDB
#	warning "GDBStub enabled!"
#	include <GDBStub.h>
#endif

void setup()
{
	Serial.begin(115200);
	Serial.println(F("\033[2J\nHello!")); // also clears serial output garbage

	pinMode(LED_BUILTIN, OUTPUT);

	// Animate the built-in from bright to off over about 3 seconds
	constexpr int stepDuration = 1732;
	int duration = stepDuration;
	do {
		delayMicroseconds(duration);
		digitalWrite(LED_BUILTIN, HIGH);
		delayMicroseconds(stepDuration - duration);
		digitalWrite(LED_BUILTIN, LOW);
		duration -= 1;
		ESP.wdtFeed();
	}
	while (duration > 0);
	digitalWrite(LED_BUILTIN, HIGH);

	if (!gdb_present()) {
		// Even if debugger is not attached it can be true!
		Serial.println(F("GDBStub not present"));
	}
	gdb_do_break();

	Serial.println(F("Hello again!"));
}

int ledOnDuration = 1000;
int ledOffDuration = 1000;

void loop()
{
	digitalWrite(LED_BUILTIN, HIGH);
	delay(ledOffDuration);
	digitalWrite(LED_BUILTIN, LOW);
	delay(ledOnDuration);

	static int n = 0;
	Serial.printf("Looping %d times\n", n++);

	if (n % 17 == 0) {
		Serial.println("Yay!");
		gdb_do_break();
	}
}
