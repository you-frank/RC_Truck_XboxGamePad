#include "Arduino.h"

#include "SoftServo.h"

#define KP 0.6
#define KI 0.06
#define KD 0.05

#define U_LMT 250

/**
 * @brief Construct a new SoftServo::SoftServo object
 *
 */
SoftServo::SoftServo(void) {
  isAttached = false;
  escPin=servoPin = 255;
  angle_val = 90;
  fwd_speed_val=0; // 92~94 : Stop
  bwd_speed_val=0;
  stepUp = STEP_UP;
  stepDown = STEP_DN;
  kp=KP; ki=KI; kd=KD;
}

/**
 * @brief Attacht to a supplied pin
 *
 * @param pin The pin to attach to for controlling the servo
**/
void SoftServo::attach(uint8_t sPin, uint8_t ePin) {
  servoPin = sPin;
  escPin = ePin;
  angle_val = 0;
  SetServoMicros(angle_val);
  fwd_speed_val=0;
  bwd_speed_val=0;
  SetEscMicros(fwd_speed_val);
  
  pinMode(servoPin, OUTPUT);
  pinMode(escPin, OUTPUT);
  isAttached = true;
  
  b_pid=true;
  pid.begin();
  pid.tune(kp, ki, kd); // kP, kI, kD
  
  pid.limit(0, U_LMT);
}


/**
 * @brief Detach from the supplied pin
 *
 */
void SoftServo::detach(void) {
  isAttached = false;
  pinMode(servoPin, INPUT);
  pinMode(escPin, INPUT);
}


/**
 * @brief Get the attachment status of the pin
 *
 * @return boolean true: a pin is attached false: no pin is attached
 */
boolean SoftServo::attached(void) { return isAttached; }


/**
 * @brief Update the servo's angle setting and the corresponding pulse width
 *
 * @param a The target servo angle
 */
void SoftServo::SetServoMicros(int16_t a) 
{
//  if (!isAttached)
//    return;
  //micros_servo = map(a, 0, 180, 1000, 2000);
  //LJoyX값 -100~100을 전체범위 1000~2000(0~180deg)중 가운데 절반인 1250~1750(=45~135deg)로 mapping.
  //micros_servo = map(a, -100, 100, 1750, 1250);
  micros_servo = map(a, -100, 100, 1875, 1125);
}


void SoftServo::SetEscMicros(int16_t spd) 
{
//  if (!isAttached)
//    return;
  if(spd>0){
    //micros_esc = map(spd, 0, 100, 1530, 1700);//2000); // R-Trigger 0~100
    micros_esc = map(spd, 0, 100, 1530, 1600);//2000); // R-Trigger 0~100
  }else if(spd<0){
    // R-Trigger가 0일때만 후진임.(R-Trigger가 눌려있으면 RTrig(acel)-LTrig(brk_rev)('-'면 0)값이 speed임.
    //micros_esc = map(spd, -100,0, 1400, 1527); 
    micros_esc = map(spd, -100,0, 1350, 1500); 
  }else{
    micros_esc = 1516; // 1511~1522 : stop
  }
  //Serial.printf("spd:%d\n", micros_esc);
}

/**
 * @brief Pulse the control pin for the amount of time determined when the angle
 * was set with `write`
 *
 */
void SoftServo::refresh(void) {
  extern bool emergency_stop;
  if(emergency_stop)
    micros_esc=1516;

  digitalWrite(servoPin, HIGH);
  delayMicroseconds(micros_servo);
  digitalWrite(servoPin, LOW);

  digitalWrite(escPin, HIGH);
  delayMicroseconds(micros_esc);
  digitalWrite(escPin, LOW);
}

void SoftServo::process(int16_t rpm , bool b_cruise) {
  static uint8_t counter;
  
  stepDown = STEP_DN;
  stepUp = STEP_UP;
  counter++;
  
  if(speed_cur>=0 && fwd_speed_val>0 && bwd_speed_val==0){ // 전진 모드
    if(b_cruise){ // Cruise가 켜있어도 서서히 계속 감속시켜서 PID가 동작하게끔 함
      stepDown=1;
      stepUp=1;
    }
    //if(speed_cur<fwd_speed_val){ // 추가 가속 : pid를 써서 가속.
    if(rpm<fwd_speed_val){ // Should use rpm not moter speed value.
      if(b_pid){
        int out=pid.compute(rpm);
        speed_cur+=out;
        Serial.printf("set:%d, cur:%d, out:%d [%.2f,%.2f,%.2f]\n", (int)pid.getpoint(), rpm, (int)out, kp, ki, kd);
      } else if(b_cruise==false || (b_cruise==true && counter&7==7)) {  // no pid
        speed_cur+=stepUp;
      }
      if(speed_cur>100) speed_cur=100;    
    }else { // if(speed_cur>fwd_speed_val){ // acel 위치로 breaking 또는 cruise가 켜있고, 속도가 설정속도보다 빠를때
      if(b_cruise==false || (b_cruise==true && counter&0x1f==0x1f)) // 크루즈가 켜있을때는 속도 떨어뜨리는걸 천천히 함.
        speed_cur-=stepDown;
      if(speed_cur<0) speed_cur=0;    
    }
  } else if(speed_cur<=0 && bwd_speed_val>0 && fwd_speed_val==0){ // 후진 모드.  // L-Trigger(break) 값이 있으면 R-Trigger(accel)값은 무시함. 무조건 감속 or 후진.
    //if(abs(speed_cur)<bwd_speed_val){ // 후진 추가 가속(-가 커지면 후진속도 빨라짐.) PID로 추가 가속
    if(rpm<bwd_speed_val){ // Should use rpm not moter speed value.
      if(b_pid){
        int out=pid.compute(rpm);
        speed_cur-=out;
        Serial.printf("set:%d, cur:%d, out:%d [%.2f,%.2f,%.2f]\n", (int)pid.getpoint(), rpm, (int)out, kp, ki, kd);
      } else {  // no pid
        speed_cur-=stepUp;
      }
      
/*    
      speed_cur-=stepUp; // 후진이어서 음수값인 속도를 더 가속.
      if(speed_cur<-100) speed_cur=-100;

      int out=pid.compute(rpm);
      speed_cur-=out;
      //speed_cur++;
      //Serial.printf("bwd: o:%d, s:%d, p:%d\n", out, speed_cur, (int)pid.getpoint());    
      //Serial.printf("RPM: %d\n", rpm);
*/      
      if(speed_cur<-100) speed_cur=-100;
      
    }else if(abs(speed_cur)>bwd_speed_val){ // break(L-Trig) 위치로 breaking
      speed_cur+=stepDown; // 후진 속도를 stepDown만큼 감속.
      if(speed_cur>0) speed_cur=0;    
      // if(abs(speed_cur)<bwd_speed_val) speed_cur=-1*bwd_speed_val;
    }
  } else if(speed_cur!=0 && fwd_speed_val==0 && bwd_speed_val==0 ) { // 관성 또는 크루즈로 움직이는중. 
    if(speed_cur>0){ // 전진중 속도 0으로 낮춰서 서서히 slow down
      speed_cur-=stepDown;
      if(speed_cur<0)speed_cur=0;
    }else{ // 후진중 속도 0으로 낮춰서 서서히 slowdown
      speed_cur+=stepDown;
      if(speed_cur>0)speed_cur=0;
    }
  } else if(speed_cur>0 && bwd_speed_val>0) { // 전진중 break(L-Trig) 눌러서 감속.
    if(bwd_speed_val>4){
      speed_cur-=bwd_speed_val/2; //한번에 확 줄이지 않고, 눌린값의 1/2씩 줄임.
    } else if(bwd_speed_val>95){ // 반대방향 거의 끝까지 눌렸으면 급정지.
      speed_cur=0;
    } else {
      speed_cur-=stepDown;
    }

    if(speed_cur<0)speed_cur=0;
  } else if(speed_cur<0 && fwd_speed_val>0) { // 후진중 accel(R-Trig) 눌러서 감속.
    if(fwd_speed_val>4) {
      speed_cur+=fwd_speed_val/2; //한번에 확 줄이지 않고, 눌린값의 1/2씩 줄임.
    } else if(fwd_speed_val>95) { // 반대방향 거의 끝까지 눌렸으면 급정지.
      speed_cur=0;
    } else {
      speed_cur+=stepDown;
    }
    if(speed_cur>0)speed_cur=0;
  } else if(speed_cur!=0 && fwd_speed_val>0 && bwd_speed_val>0){
    speed_cur = speed_cur/2;
  } else {
    //Serial.printf("Unhandled sc:%d, sv:%d, bv:%d\n", speed_cur, fwd_speed_val, bwd_speed_val);
  }
  
  SetEscMicros(speed_cur);
  /*
  // PID가 원활하게 돌아기기 위혀선 결과값에 상관없이 무조건 속도 줄이는게 필요함.
  extern unsigned long cur;
  if(cur&1)
  {
    if(speed_cur>0)
      speed_cur--;  
    else if(speed_cur<0)
      speed_cur++;
  }
  */
}
/*
void SoftServo::process(void) {
  //if(fwd_speed_val<3) fwd_speed_val=0;
  //if(bwd_speed_val<3) bwd_speed_val=0;
  
  if(bwd_speed_val>0){ // breaking or 후진중.  bwd_speed_val는 양수이고, speed_xxx는 음수로 유지되어야함(후진중이므로)!!!
    if(fwd_speed_val>0 || speed_cur>0){ // 전진중 breaking mode
      if(bwd_speed_val>4)        speed_cur-=bwd_speed_val/4;
      else        speed_cur-=stepDown;
      
      if(speed_cur<0) speed_cur=0;
    }else if(speed_cur<0 && fwd_speed_val>0){ // 후진중 acel로 breaking mode
      if(fwd_speed_val>4)        speed_cur+=fwd_speed_val/4;
      else        speed_cur+=stepDown;
      
      if(speed_cur>0) speed_cur=0;
    }else if(fwd_speed_val==0){ // 후진 가속 모드.(rTrigger=0) 후진모드중 속도는 모두 -임. 크기비교 조심할것!
      if(bwd_speed_val<speed_cur){ // 후진 acel
        speed_cur-=stepUp;
        if(speed_cur<-100) speed_cur=-100;
      }else if(bwd_speed_val>speed_cur){ //후진중 acel 위치로 breaking. (후진중이므로 현재 속도는 음수임!)
        speed_cur+=stepDown;
        if(speed_cur>bwd_speed_val) speed_cur=bwd_speed_val;
      }
    }else{
      Serial.println("Breakning Error111!!! Unhandled Situation!");
      speed_cur=fwd_speed_val=0;
    }
  }else if(fwd_speed_val>0){ // acel mode
    if(speed_cur<fwd_speed_val){ // 추가 가속
      speed_cur+=stepUp;
      if(speed_cur>100) speed_cur=100;    
    }else if(speed_cur>fwd_speed_val){ // acel 위치로 breaking
      speed_cur-=stepDown;
      if(speed_cur<fwd_speed_val) speed_cur=fwd_speed_val;
    }else{
      Serial.println("Breakning Error222!!! Unhandled Situation!");
      speed_cur=fwd_speed_val=0;
    }
  }else{ // acel, break 모두 0일때.
    if(speed_cur>0){ // 전진중 속도 0으로 낮춰서 서서히 slow down
      speed_cur-=stepDown;
      if(speed_cur<0)speed_cur=0;
    }else{ // 후진중 속도 0으로 낮춰서 서서히 slowdown
      speed_cur+=stepDown;
      if(speed_cur>0)speed_cur=0;
    }
  }

  SetEscMicros(speed_cur);
}*/
