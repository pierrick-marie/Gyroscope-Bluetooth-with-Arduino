/*
    Accelerometer Control

    This program scans for Bluetooth® Low Energy peripherals until one with the advertised service
    "19b10000-e8f2-537e-4f6c-d104768a1214" UUID is found. Once discovered and connected,
    it will remotely control the Bluetooth® Low Energy peripheral's Accelerometer.

    The circuit:
      - Arduino MKR WiFi 1010, Arduino Uno WiFi Rev2 board, Arduino Nano 33 IoT, Arduino Nano 33 BLE, or Arduino Nano 33 BLE Sense board.

    Licence
      DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
                   Version 2, December 2004

      Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>

      Everyone is permitted to copy and distribute verbatim or modified
      copies of this license document, and changing it is allowed as long
      as the name is changed.

      DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
      TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

      0. You just DO WHAT THE FUCK YOU WANT TO.
*/

#include <ArduinoBLE.h>
#include <Arduino_LSM6DS3.h>

#define MAX_VALUE 10
#define WAIT_VALUE 100

#define NORTH 0
#define EAST 1
#define UP 0
#define DOWN 1

float x, y, z;
int degreesX = 0;
int degreesY = 0;
int degreesZ = 0;

int i;
int upAngle, leftAngle;
int upValues[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int leftValues[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

byte pole, direction, value;

void setup() {

	Serial.begin(9600);

	if (!IMU.begin()) {
		Serial.println("Failed to initialize IMU!");
		while (1);
	}
	Serial.print(IMU.accelerationSampleRate());

	// initialize the Bluetooth® Low Energy hardware
	BLE.begin();

	Serial.println("Bluetooth® Low Energy Central - LED control");

	// start scanning for peripherals
	BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");
}

void loop() {

	// check if a peripheral has been discovered
	BLEDevice peripheral = BLE.available();

	if (peripheral) {
		// discovered a peripheral, print out address, local name, and advertised service
		Serial.print("Found ");
		Serial.print(peripheral.address());
		Serial.print(" '");
		Serial.print(peripheral.localName());
		Serial.print("' ");
		Serial.print(peripheral.advertisedServiceUuid());
		Serial.println();

		if (peripheral.localName() != "ACCELEROMETER") {
			return;
		}

		// stop scanning
		BLE.stopScan();

		fn_start_service(peripheral);

		// peripheral disconnected, start scanning again
		BLE.scanForUuid("19b10000-e8f2-537e-4f6c-d104768a1214");
	}
}

void fn_start_service(BLEDevice peripheral) {

	// connect to the peripheral
	Serial.println("Connecting ...");

	if (peripheral.connect()) {
		Serial.println("Connected");
	} else {
		Serial.println("Failed to connect!");
		return;
	}

	// discover peripheral attributes
	Serial.println("Discovering attributes ...");
	if (peripheral.discoverAttributes()) {
		Serial.println("Attributes discovered");
	} else {
		Serial.println("Attribute discovery failed!");
		peripheral.disconnect();
		return;
	}

	// retrieve the Accelerometer characteristic
	BLECharacteristic poleCharacteristic = peripheral.characteristic("19b10001-e8f2-537e-4f6c-d104768a1215");
	BLECharacteristic directionCharacteristic = peripheral.characteristic("19b10001-e8f2-537e-4f6c-d104768a1216");
	BLECharacteristic valueCharacteristic = peripheral.characteristic("19b10001-e8f2-537e-4f6c-d104768a1217");

	if (!poleCharacteristic) {
		Serial.println("Peripheral does not have Accelerator characteristic!");
		peripheral.disconnect();
		return;
	} else if (!poleCharacteristic.canWrite()) {
		Serial.println("Peripheral does not have a writable Accelerator characteristic!");
		peripheral.disconnect();
		return;
	}

	// send values
	int angle;
	while (peripheral.connected()) {

		angle = fn_get_angle();
		Serial.print("Final up angle = ");
		Serial.print(angle);
		Serial.println("°");

		if (angle > 315 && angle <= 360) {
			pole = (byte)NORTH;
			direction = (byte)DOWN;
			value = (byte)abs(360 - angle);
		}
		if (angle >= 0 && angle < 45) {
			pole = (byte)NORTH;
			direction = (byte)UP;
			value = (byte)angle;
		}
		if (angle > 225 && angle <= 270) {
			pole = (byte)EAST;
			direction = (byte)DOWN;
			value = (byte)abs(270 - angle);
		}
		if (angle >= 270 && angle < 315) {
			pole = (byte)EAST;
			direction = (byte)UP;
			value = (byte)abs(270 - angle);
		}

		Serial.print("Pole: ");
		Serial.print(pole);
		Serial.print(" Direction: ");
		Serial.print(direction);
		Serial.print(" Value: ");
		Serial.println(value);

		poleCharacteristic.writeValue(pole);
		directionCharacteristic.writeValue(direction);
		valueCharacteristic.writeValue(value);
	}

	Serial.println("Peripheral disconnected");
}

int fn_get_angle() {

	for (i = 0; i < MAX_VALUE; i++) {
		if (IMU.accelerationAvailable()) {
			IMU.readAcceleration(x, y, z);
		}

		x = -100 * x;
		degreesX = map(x, -100, 100, -90, 90);
		upValues[i] = degreesX;

		z = 100 * z;
		degreesZ = map(z, -100, 100, -90, 90);
		leftValues[i] = degreesZ;

		delay(WAIT_VALUE);
	}

	upAngle = fn_mean(upValues);
	leftAngle = fn_mean(leftValues);

	if (leftAngle > 0) { // left side
		upAngle = 270 + upAngle;
	} else { // leftAngle > 0 => rigt side
		upAngle = 90 - upAngle;
	}

	return upAngle;
}

int fn_decade(int value) {

	int res = value / MAX_VALUE;
	return res * MAX_VALUE;
}

int fn_unit(int value) {

	int decade = fn_decade(value);
	return value - decade;
}

int fn_simplify(int value) {

	int unit = fn_unit(value);

	if (unit >= 5) {
		return fn_decade(value) + 10;
	} else {
		return fn_decade(value);
	}
}

int fn_mean(int values[]) {

	int res = 0;
	for (int i = 0; i < MAX_VALUE; i++) {
		res = res + values[i];
	}
	return res / MAX_VALUE;
}
