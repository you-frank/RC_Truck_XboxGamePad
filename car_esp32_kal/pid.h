#ifndef PIDControllerLib
#define PIDControllerLib

#include "Arduino.h"

class PIDController {
  private:
    unsigned long lastTime;
    double output, timeChanged;
    double error,lastErr,errSum, dErr;

    bool doLimit,_init;

    double Kp, Ki, Kd;
    double divisor, minOut, maxOut, setPoint;
    
  public:
    // Constructor
    PIDController() {};

    void begin(){
      Kp= Ki= Kd = 1;
      divisor = 10;
      doLimit = false;
      _init = true;
    };

    void tune(double _Kp, double _Ki, double _Kd) {
      if (_Kp < 0 || _Ki < 0 || _Kd < 0) 
        return;
      Kp = _Kp;
      Ki = _Ki;
      Kd = _Kd;;
    };

    void limit(double min, double max){
      minOut = min;
      maxOut = max;
      doLimit = true;
    };
    
    void setpoint(double newSetpoint) { setPoint = newSetpoint; };
    double getpoint() {return setPoint; };
    double getOutput() { return output; };
    
    double compute(double input){
      // Return false if it could not execute;
      // This is the actual PID algorithm executed every loop();
    
      // Failsafe, return if the begin() method hasn't been called
      if (!_init) return 0;
    
      // Calculate time difference since last time executed
      unsigned long now = millis();
      double timeChange = (double)(now - lastTime);
    
      // Calculate error (P, I and D)
      double error = setPoint - input;
      errSum += error * timeChange;
      if (doLimit) {
        errSum = constrain(errSum, minOut * 1.1, maxOut * 1.1); 
      }
      double dErr = (error - lastErr) / timeChange;
    
      // Calculate the new output by adding all three elements together
      double newOutput = (Kp * error + Ki * errSum + Kd * dErr) / divisor;
    
      // If limit is specifyed, limit the output
      if (doLimit) {
        output = constrain(newOutput, minOut, maxOut);
      } else {
        output = newOutput;  
      }
    
      // Update lastErr and lastTime to current values for use in next execution
      lastErr = error;
      lastTime = now;
   
      // Return the current output
      return output;
    };
};
#endif
