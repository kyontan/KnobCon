#define USBCON

#include <Joystick.h>

#define PIN_LED 17
#define PIN_A 2
#define PIN_B 3
#define PIN_SW 7

#define CHATTERING_THRESHOLD_TIME_MS 100

//  CW: Clock-wise
// CCW: Counter-clock-wise

const bool enable_serial = false; // debug

void setup() {
  pinMode(PIN_LED, OUTPUT);

	pinMode(PIN_A, INPUT);
	pinMode(PIN_B, INPUT);

	digitalWrite(PIN_A, HIGH);
	digitalWrite(PIN_B, HIGH);

// pin | interrupt no
//  3  | 0
//  2  | 1
//  0  | 2
//  1  | 3
//  7  | 4
	attachInterrupt(1, a_change, CHANGE); // pin2: A
	attachInterrupt(0, b_change, CHANGE); // pin3: B
	attachInterrupt(4, switch_change, CHANGE);  // pin7: Switch

  if (enable_serial) {
  	Serial.begin(115200);
  	while (!Serial);
  }
}

volatile int state = LOW;
volatile int last_a = 0;
volatile int last_b = 0;

// joystick
volatile int rotation = 0;

void loop() {
  digitalWrite(LED, state);
}

void rotation_update(int dir) {
	rotation += 3.75 * dir; // 360 / (24 * 4)
	rotation = fmod(360 + rotation, 360); // (360 + rotation) % 360 => 0 <= rotation < 360
	Joystick.setXAxisRotation((int)rotation);
	if (enable_serial) Serial.println(rotation, DEC);

	if (dir == 1) { // CW
		state = 1;
	} else { // CCW
		state = 0;
	}
}

// volatile int last_update_mills = 0;
bool is_chattering() {
	// if ((last_update_mills - millis()) < CHATTERING_THRESHOLD_TIME_MS) {
	// 	return 1;
	// }
	// last_update_mills = millis();
	return 0;
}

// volatile int update_count = 0;
// bool is_chattering() {
// 	if (10 < update_count++) {
// 		update_count = 0;
// 		return 0;
// 	}
// 	return 1;
// }

void a_change() {
	if (is_chattering()) { return; }

	int current_a = digitalRead(PIN_A);
	if (last_a == current_a) { return; }

	last_b = digitalRead(PIN_B);
	int dir = 0;

	if (last_a < current_a) { // rising
		if (last_b) {
			rotation_update(1); // CW
			if (enable_serial) Serial.println("A:R:CW");
		} else {
			if (enable_serial) Serial.println("A:R:CCW");
			rotation_update(-1); // CCW
		}
	} else { // falling
		if (last_b) {
			rotation_update(-1); // CCW
			if (enable_serial) Serial.println("A:F:CCW");
		} else {
			rotation_update(1); // CW
			if (enable_serial) Serial.println("A:F:CW");
		}
	}
	last_a = current_a;
}

void b_change() {
	if (is_chattering()) { return; }

	int current_b = digitalRead(PIN_B);
	if (last_b == current_b) { return; }

	last_a = digitalRead(PIN_A);
	int dir = 0;

	if (last_b < current_b) { // rising
		if (last_a) {
			rotation_update(-1); // CCW
			if (enable_serial) Serial.println("B:R:CCW");
		} else {
			rotation_update(1); // CW
			if (enable_serial) Serial.println("B:R:CW");
		}
	} else { // falling
		if (last_a) {
			rotation_update(1); // CW
			if (enable_serial) Serial.println("B:F:CW");
		} else {
			rotation_update(-1); // CCW
			if (enable_serial) Serial.println("B:F:CCW");
		}
	}
	last_b = current_b;
}

void switch_change() {
	int current_sw = digitalRead(PIN_SW);
	Joystick.setButton(0, current_sw);
	if (enable_serial) Serial.println(current_sw);
  // state = !state;
}
