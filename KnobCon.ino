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

// キーボードモードで入力される文字
// 特殊キーの一覧: https://www.arduino.cc/en/Reference/KeyboardModifiers
// 例: const char KEY_PUSH  = KEY_RETURN;
const char KEY_ROTATE_LEFT  = '<';
const char KEY_ROTATE_RIGHT = '>';
const char KEY_PUSH  = '%';

//// ジョイスティックモードの設定

// ロータリーエンコーダの回転に対して、ジョイスティックとしては回転軸をどれだけ回転したことにするか
// 回転軸の値の範囲: 0 ~ 1
//
// KnobCon で使用しているロータリーエンコーダは1回転すると96回、回転したことを検知します。
// そのため、 ROTARY_ENCODER_ANGLE_PER_STEP を 1.0/96 とすると、
// 中立位置(=0) から1回転だけ時計回りに回転すると値が 1.0/96 * 96 = 1 変化します。
const double ROTARY_ENCODER_ANGLE_PER_STEP = 1.0/96;

// ROTARY_ENCODER_LOOP_MODE: true にすると、
// 回転軸の値が 0 ~ 1 の範囲を超えた場合に、1 から 0, 0 から 1 へ値が飛ぶようになります。
const bool ROTARY_ENCODER_LOOP_MODE = false;

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

  digitalWrite(PIN_LED, mode_keyboard);
}

// joystick
volatile int rotation = 0.5; // neutral position
volatile unsigned long last_switch_changed_at = 0;
volatile bool last_sent_sw_state = 0;

void loop() {
	if (mode_joystick) {
		Joystick.setXAxisRotation(rotation);
		delay(50);
	}
}

void rotation_update(int dir) {
	if (millis() < last_switch_changed_at) {
		last_switch_changed_at = millis(); // overflow support
	}

	if ((millis() - last_switch_changed_at) < ROTARY_ENCODER_CATTERING_THRESHOULD) {
		if (ENABLE_DEBUG) {
			Serial.println("chattering: rotary encoder");
		}
		return;
	}

	rotation += 360 * ROTARY_ENCODER_ANGLE_PER_STEP * dir;
	if (ROTARY_ENCODER_LOOP_MODE) {
		if (rotation < 0) {
			rotation = fmod(rotation + 360, 360);
		}

		if (360 < rotation) {
			rotation = fmod(rotation - 360, 360);
		}
	} else {
		rotation = constrain(rotation, 0, 359.9);
	}

	if (ENABLE_DEBUG) Serial.println(rotation, DEC);
	if (mode_joystick) Joystick.setXAxisRotation(rotation);

	if (mode_keyboard) {
		if (dir == 1) { // CW
			Keyboard.write(KEY_ROTATE_RIGHT);
		} else { // CCW
			Keyboard.write(KEY_ROTATE_LEFT);
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

bool is_switch_chattering() {
	int current_sw = digitalRead(PIN_SW);
	if (current_sw == last_sw) {
		return true;
	}
	last_sw = current_sw;

	int diff = millis() - last_switch_changed_at;
	last_switch_changed_at = millis();
	if (diff < SWITCH_CATTERING_THRESHOULD) {
		return true;
	}

	if (last_sent_sw_state == current_sw) {
		return true;
	}

	last_sent_sw_state = current_sw;

	return false;
}

void switch_change() {
	if (is_switch_chattering()) {
		return;
	}

	int current_sw = last_sent_sw_state;

	if (ENABLE_DEBUG) {
		Serial.print("switch_change: ");
		Serial.println(current_sw);
	}

	if (mode_joystick) Joystick.setButton(0, current_sw);
	if (mode_keyboard) {
		if (current_sw) {
			Keyboard.press(KEY_PUSH);
		} else {
			Keyboard.release(KEY_PUSH);
		}
	}

	last_sent_sw_state = current_sw;
}
