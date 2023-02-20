#include "Arduino.h"
#include "pid.h"

/**
 * @brief Class for basic software servo control
 *
 */
class SoftServo {
public:
  SoftServo(void);
  void attach(uint8_t spin, uint8_t epin);
  void detach();
  boolean attached();
  void SetServoMicros(int16_t a); // turn 1~100 speed level to servo pwm length(usec).
  void SetEscMicros(int16_t s); // turn -100~100 servo angle to servo pwm length(usec).
  
  int16_t GetCurSpeed() { return speed_cur; }
  void SetForwardSpeed(int16_t v) { fwd_speed_val=v; if(v) { pid.setpoint((double)v);} }
  int16_t GetForwardSpeed(void) { return fwd_speed_val; }
  void SetBackwardSpeed(int16_t v) { bwd_speed_val=v; if(v) {pid.setpoint((double)v);} }
  void refresh(void); // generate PWM
  void process(int16_t rpm, bool b_crouse); // Polling function
  boolean isAttached;
  
  bool b_pid; // pid starting flag
  PIDController pid;
  double kp, ki, kd;
  static const uint8_t STEP_UP=4;  // cruise speed stepup
  static const uint8_t STEP_DN=8;  // cruise speed stepdown

private:
  uint8_t servoPin, escPin;
  int16_t speed_cur;
  int16_t fwd_speed_val, bwd_speed_val, angle_val;
  int16_t stepUp, stepDown;
  uint16_t micros_esc, micros_servo;
  
};
