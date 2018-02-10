#define ROTATION_X A0
#define ROTATION_Y A1
#define ROTATION_Z A2
#define THROTTLE A6
#define STAGE_LOCK 23

#define THROTTLE_MIN 30
#define THROTTLE_MAX 1000

#define BUTTON_DEBOUNCE_TIME 100

#define LOOP_DELAY 3

#include <krpc.h>
#include <krpc/services/krpc.h>
#include <krpc/services/space_center.h>

// krpc values
HardwareSerial * conn;
krpc_SpaceCenter_Vessel_t vessel;
krpc_SpaceCenter_Control_t vesselControl;

void retrieveActiveVessel() {
  krpc_SpaceCenter_ActiveVessel(conn, &vessel);
  krpc_SpaceCenter_Vessel_Control(conn, &vesselControl, vessel);  
}

// Throttle control
int lastThrottleSetting = 0;

void readThrottle() {
  int rawValue = analogRead(THROTTLE);
  int newSetting;
  if (rawValue < THROTTLE_MIN) {
    newSetting = 0;
  } else if (rawValue > THROTTLE_MAX) {
    newSetting = 1000;
  } else {
    newSetting = map(rawValue, THROTTLE_MIN, THROTTLE_MAX, 0, 1000);
  }
  // Invert the values
  newSetting = 1000 - newSetting;

  if (abs(newSetting - lastThrottleSetting) > 5) {
    lastThrottleSetting = newSetting;
    float newThrottle = (float)newSetting / 1000.0;
    krpc_SpaceCenter_Control_set_Throttle(conn, vesselControl, newThrottle);
  }
}

#include <InputDebounce.h>

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
