#define USBCON

#include <Joystick.h>
#include <Keyboard.h>

#define PIN_LED 17
#define PIN_A 3
#define PIN_B 7
#define PIN_SW 2

//  CW: Clock-wise
// CCW: Counter-clock-wise

// true にセットするとデバッグモードになります (シリアルモニタにセンサの変更を出力します)
// デバッグモードでは、ジョイスティック, キーボードとしては機能しません
const bool ENABLE_DEBUG = false;

// SWITCH_CATTERING_THRESHOULD [ms] 以内の連続するボタン状態変化(押す/離す)を無視します
const int SWITCH_CATTERING_THRESHOULD = 45; // [ms]

// ボタン状態変化(押す/離す) から ROTARY_ENCODER_CATTERING_THRESHOULD [ms] 以内の回転を無視します
const int ROTARY_ENCODER_CATTERING_THRESHOULD = 150; // [ms]


bool mode_joystick = false;
bool mode_keyboard = false;

volatile int last_a = 0;
volatile int last_b = 0;
volatile int last_sw;

void setup() {
  pinMode(PIN_LED, OUTPUT);

	pinMode(PIN_A, INPUT);
	pinMode(PIN_B, INPUT);
	pinMode(PIN_SW, INPUT);

	digitalWrite(PIN_A, HIGH);
	digitalWrite(PIN_B, HIGH);

  // 起動時にスイッチを押していなければキーボードになる
	last_sw = digitalRead(PIN_SW);
	mode_joystick =  last_sw;
	mode_keyboard = !last_sw;

// pin | interrupt no
//  3  | 0
//  2  | 1
//  0  | 2
//  1  | 3
//  7  | 4
	attachInterrupt(0, a_change, CHANGE); // pin2: A
	attachInterrupt(4, b_change, CHANGE); // pin3: B
	attachInterrupt(1, switch_change, CHANGE);  // pin7: Switch

  if (ENABLE_DEBUG) {
  	Serial.begin(115200);
  	while (!Serial);

  	Serial.print("mode_joystick: ");
  	Serial.println(mode_joystick);
  	Serial.print("mode_keyboard: ");
  	Serial.println(mode_keyboard);

  	mode_joystick = false;
  	mode_keyboard = false;
  } else {
		if (mode_joystick) Joystick.begin();
		if (mode_keyboard) Keyboard.begin();
	}
}

// joystick
volatile int rotation = 0;

void loop() {
  digitalWrite(PIN_LED, mode_keyboard);
}

void rotation_update(int dir) {
	rotation += 3.75 * dir; // 360 / (24 * 4)
	rotation = fmod(360 + rotation, 360); // (360 + rotation) % 360 => 0 <= rotation < 360
	if (ENABLE_DEBUG) Serial.println(rotation, DEC);
	if (mode_joystick) Joystick.setXAxisRotation((int)rotation);

	if (mode_keyboard) {
		if (dir == 1) { // CW
			Keyboard.write('>');
		} else { // CCW
			Keyboard.write('<');
		}
	}
}

void a_change() {
	int current_a = digitalRead(PIN_A);
	if (last_a == current_a) { return; }

	last_b = digitalRead(PIN_B);
	if (last_a < current_a) { // rising
		if (last_b) {
			rotation_update(1); // CW
			if (ENABLE_DEBUG) Serial.println("A:R:CW");
		} else {
			if (ENABLE_DEBUG) Serial.println("A:R:CCW");
			rotation_update(-1); // CCW
		}
	} else { // falling
		if (last_b) {
			rotation_update(-1); // CCW
			if (ENABLE_DEBUG) Serial.println("A:F:CCW");
		} else {
			rotation_update(1); // CW
			if (ENABLE_DEBUG) Serial.println("A:F:CW");
		}
	}
	last_a = current_a;
}

void b_change() {
	int current_b = digitalRead(PIN_B);
	if (last_b == current_b) { return; }

	last_a = digitalRead(PIN_A);
	if (last_b < current_b) { // rising
		if (last_a) {
			rotation_update(-1); // CCW
			if (ENABLE_DEBUG) Serial.println("B:R:CCW");
		} else {
			rotation_update(1); // CW
			if (ENABLE_DEBUG) Serial.println("B:R:CW");
		}
	} else { // falling
		if (last_a) {
			rotation_update(1); // CW
			if (ENABLE_DEBUG) Serial.println("B:F:CW");
		} else {
			rotation_update(-1); // CCW
			if (ENABLE_DEBUG) Serial.println("B:F:CCW");
		}
	}
	last_b = current_b;
}

void switch_change() {
	int current_sw = digitalRead(PIN_SW);

	if (ENABLE_DEBUG) {
		Serial.print("switch_change: ");
		Serial.println(current_sw);
	}

	if (mode_joystick) Joystick.setButton(0, current_sw);
	if (mode_keyboard) {
		if (current_sw) {
			Keyboard.press(KEY_RETURN);
		} else {
			Keyboard.release(KEY_RETURN);
		}
	}
}
