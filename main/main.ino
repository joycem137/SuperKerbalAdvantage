#include <krpc.h>
#include <krpc/services/krpc.h>
#include <krpc/services/space_center.h>
#include <InputDebounce.h>
#include <ResponsiveAnalogRead.h>

// System config
#define ANALOG_MAX 1023
#define BUTTON_DEBOUNCE_TIME 100
#define LOOP_DELAY 3

// Wiring
#define ROTATION_X A0
#define ROTATION_Y A1
#define ROTATION_Z A2
#define THROTTLE A6
#define STAGE_LOCK 23

// krpc values
HardwareSerial * conn;
krpc_SpaceCenter_Vessel_t vessel;
krpc_SpaceCenter_Control_t vesselControl;

void retrieveActiveVessel() {
  krpc_SpaceCenter_ActiveVessel(conn, &vessel);
  krpc_SpaceCenter_Vessel_Control(conn, &vesselControl, vessel);  
}

int readAnalogWithDeadZone(ResponsiveAnalogRead analog, int deadzone, bool invert) {  
  int value = analog.getValue();
  int trueMax = ANALOG_MAX - deadzone;
  int scaledValue;

  if (value < deadzone) {
    scaledValue = 0;
  } else if (value > trueMax) {
    scaledValue = ANALOG_MAX;
  } else {
    // Scale to between our two values
    scaledValue = map(value, deadzone, trueMax, 0, ANALOG_MAX);
  }
  
  if (invert) {
    scaledValue = ANALOG_MAX - scaledValue;
  }
  
  return scaledValue;
}

ResponsiveAnalogRead throttle(A6, true);

int lastThrottleValue = 0;

void readThrottle() {
  throttle.update();
  int value = readAnalogWithDeadZone(throttle, 23, true);
  if (value != lastThrottleValue) {    
    lastThrottleValue = value;    
    float realValue = value / (float)ANALOG_MAX;
    krpc_SpaceCenter_Control_set_Throttle(conn, vesselControl, realValue);
  }
}

InputDebounce stageButton;

void onStageButtonPressed(uint8_t pin) {
  int lockState = digitalRead(STAGE_LOCK);
  if (lockState == HIGH) {
    krpc_list_object_t result; // We don't care about this.
    krpc_SpaceCenter_Control_ActivateNextStage(conn, &result, vesselControl);
    retrieveActiveVessel();
  }
}

void readButtons() {
  unsigned long now = millis();
  stageButton.process(now);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  // put your setup code here, to run once:  
  conn = &Serial;
  // Open the serial port connection
  krpc_open(&conn, NULL);
  // Set up communication with the server
  krpc_connect(conn, "Super Kerbal Advantage");  
  // Indicate succesful connection by lighting the on-board LED
  digitalWrite(LED_BUILTIN, HIGH);

  // Setup our debounced inputs
  stageButton.registerCallbacks(onStageButtonPressed, NULL, NULL);
  stageButton.setup(22, BUTTON_DEBOUNCE_TIME, InputDebounce::PIM_EXT_PULL_DOWN_RES);
  pinMode(STAGE_LOCK, INPUT);
}

void loop() {  
  retrieveActiveVessel();
  readThrottle();
  readButtons();
  delay(LOOP_DELAY);
}
