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

	// Two `print`s instead single `println` to illustrate problem 
	// where VS Code debugger puts them into separate lines (harder to read).
	Serial.print(F("Hello "));
	Serial.print(F("again!\n"));
}

int ledOnDuration = 1000;
int ledOffDuration = 1000;
unsigned int yay = 17;

void loop()
{
	digitalWrite(LED_BUILTIN, HIGH);
	delay(ledOffDuration);
	digitalWrite(LED_BUILTIN, LOW);
	delay(ledOnDuration);

	static int n = 0;
	Serial.printf("Looping %d times\n", n++);

	if (n % yay == 0) {
		Serial.println("Yay!");
		gdb_do_break();
	}

	while (Serial.available()) {
		char buffer[16];
		char* p = buffer;
		size_t len = Serial.readBytesUntil('\n', buffer, sizeof(buffer) - 1);
		buffer[len] = 0;
		switch (buffer[0]) {
			case 'y':
				while (true) {
					p++;
					if (*p == 0) 
						break; // out of current loop; only display yay value
					if (*p == '=') {
						yay = strtoul(p + 1, nullptr, 10);
						break;
					}
				}
				Serial.printf("Yay every %d times\n", yay);
				break;
			case 'b':
				Serial.println("Let's break!");
				gdb_do_break();
				break;
			default:
				Serial.printf("Unknown command, length: %u, bytes: ", len);
				for (size_t i = 0; i < len; i++) {
					Serial.printf("%02X ", buffer[i]);
				}
				Serial.println();
				break;
		}
	}
}
