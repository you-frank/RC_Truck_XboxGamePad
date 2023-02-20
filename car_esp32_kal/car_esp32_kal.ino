/* 
 *  @brief Used SoftPWM intead HW PWM and assigned second core for it.
 *         Integrage All Motor controls in Soft PWM Class library.
 *         SC16IS752 I2C-UARTx2
 *           - Channel A : RF-Transceiver 9600bps
 *           - Channel B : GPS 115200bps
 *         AS5600 : I2C Magnetic Encoder.
 *         Servo : GPIO16
 *         ESC   : GPIO17
 */
//#define MY_DEBUG
#include <Wire.h>
#include "SC16IS752.h"
#include "as5600.h"
#include "util.h"
#include "SoftServo.h"

AS5600 encoder;
SC16IS752 i2cuart = SC16IS752(SC16IS750_PROTOCOL_I2C,SC16IS750_ADDRESS_AA);
#define baudrate_A 9600   // RF Transceiver cc1101
#define baudrate_B 115200 // GSP Neo-M8N

GamepadData gamepadData;  
static uint8_t buf_chA[64]; // UART Channel A receive Buffer
static char buf_chB[64]; // UART Channel B receive Buffer
static bool b_start_chA, b_start_chB; // data receive processing indicator
static bool b_rcvFrm_chA, b_rcvFrm_chB; // data arrived indicator
static uint8_t bufA_len, bufB_len; // data length
static uint16_t prev_btns;
static unsigned long b_cruise; // cruise status
#define CRUISE_STEP 5 // cruise step up/down speed steps

#define SERVO_PIN 16
#define ESC_PIN 17
static SoftServo esc_servo;

unsigned long cur;
static uint8_t cnt_1s;  // 1 second count
bool emergency_stop;  // for out of range
static unsigned long prev_frame; // last frame arrived time
static uint8_t frm_num; // frame number counting

hw_timer_t * timer = NULL;

// 50msec Timer handler for SoftPWM work with Task2, using Core0 (main task using Core1)
void ARDUINO_ISR_ATTR onTimer(){
  if(esc_servo.isAttached){
    esc_servo.refresh();
  }
  return;
}

TaskHandle_t Task2;
void task2( void * parameter) {
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 50000, true); // every 50msec
  // Start an alarm
  timerAlarmEnable(timer);
  
  esc_servo.attach(SERVO_PIN, ESC_PIN);
  delay(5000); // need time for ESC initializing.
  while(1){
    loop2();
  }
}

void setup() {
  Serial.begin(115200);

  if(encoder.begin()==false) {
    Serial.print("NOT ");
  }
  Serial.println("Found Encoder");

  int newTask = xPortGetCoreID()==1 ? 0 : 1; // Create new task working with unused core.
  xTaskCreatePinnedToCore(
      task2, //* Function to implement the task 
      "Loop2", //* Name of the task 
      10000,  //* Stack size in words 
      NULL,  //* Task input parameter 
      0,  //* Priority of the task 
      &Task2,  //* Task handle. 
      newTask); //* Core where the task should run 
      // usually main loop use core 1. That means task2 will use core 0.

  // I2C-UART Initialization
  i2cuart.begin(baudrate_A, baudrate_B); //baudrate setting
  i2cuart.EnableTransmit(SC16IS752_CHANNEL_A, true);
  if (i2cuart.ping()!=1) {
      Serial.println("I2C-UART device not found");
      while(1);
  } else {
      Serial.println("device found");
  }
}

// Woring with core1. PWM is gernerated by timer interrupt.
// Nothing to do here.
void loop2() {
  yield;
}

// main task loop
// processing data from i2c-uart
void loop() {
  cur=millis();

  if(cur-prev_frame>600){ // emergency Stop if it exceeds 600msec after receiving the last frame
    if(emergency_stop==false)
      Serial.println("no sig! STOP");
    emergency_stop=true; // Stop by PWM Generator.
  }else{
    if(emergency_stop)
      Serial.println("rcv sig!");
    emergency_stop=false;
  }
  
  // Channel_A : RF-receicver
  if (i2cuart.available(SC16IS752_CHANNEL_A) > 0){
    bufA_len += i2cuart.readBytes(SC16IS752_CHANNEL_A, &buf_chA[bufA_len]);
    if(bufA_len > sizeof(GamepadData)){ // reset buffer when receiving wrong size
      b_start_chA=false;
      bufA_len=0;
    }
    
    if(bufA_len>2){ // Start_Frame Detected 0xffff
      if(buf_chA[0]==0xff && buf_chA[1]==0xff){
        b_start_chA=true; // Set b_start_chA (receiving started),
      }else{              // otherwise, keep resetting(ignoring) until detect 0xffff(Start_Frame)
        bufA_len=0;
        b_start_chA=false;
      }
    }

    // calculate check sum.
    if(b_start_chA && bufA_len == FRAME_DATA_SIZE+3){ 
      uint8_t chkSum_chA=0;
      for(int i=0; i<FRAME_DATA_SIZE; i++){
        chkSum_chA+=buf_chA[2+i];
      }

      // compare rcv checksum and calc checksum.
      if(chkSum_chA == buf_chA[FRAME_DATA_SIZE+2]){ // Set  b_rcvFrm_chA set if collect
        memncpy(buf_chA, gamepadData.buf, bufA_len);
        b_rcvFrm_chA=true;
      }else{  // reset and ignore if wrong
        Serial.printf("ChkSum Err %x, %x\n", chkSum_chA, buf_chA[FRAME_DATA_SIZE+2]);
        for(int i=0; i<sizeof(GamepadData)-1; i++)
          Serial.printf("%d, ", buf_chA[i]);
        Serial.println();
      }
      b_start_chA=false;
      bufA_len=0;
    } // end of chksum process
  } // end of channel_A(RF Transiver)receiving

  // received frame processing
  if(b_rcvFrm_chA){
#ifdef MY_DEBUG
    Serial.printf("%d, %d, %d, %d, spd:%d, rpm:%d\n",
        gamepadData.data.left_right, gamepadData.data.l_trigger,
        gamepadData.data.r_trigger,  gamepadData.data.btns.var, esc_servo.GetCurSpeed(), encoder.getAvgRpm());//encoder.rpm );
#endif
    // Serial.printf("itv: %d\n", cur-prev_frame); // should be 110msec +-2.
    prev_frame=cur;
    
    if(prev_btns != gamepadData.data.btns.var){ // process button when button state is changed
      if(prev_btns==0){
        if(gamepadData.data.btns.btn_set.y){ // y: Toggle Cruise 
          // Cruise On when move forward and rpm is higher then 15.
          if(esc_servo.GetCurSpeed()>0 && encoder.getAvgRpm()>=15) { 
            esc_servo.SetForwardSpeed(encoder.getAvgRpm());
            //esc_servo.SetForwardSpeed(15);  ////////////////////////////// set cruise speed 15.
            Serial.printf("Cruise ON %d\n", encoder.getAvgRpm());
            // b_cruise will ignore R-Trigger(Acceleration) input for 1s to give a time 
            // to allow time for the button released.
            b_cruise= (b_cruise==0)? cur : 0; 
          }
        }else if(gamepadData.data.btns.btn_set.x){ // switch PID or manual Speed control
          esc_servo.b_pid^=true;
          Serial.printf("PID Status %d\n", esc_servo.b_pid);
        }
#ifdef TUNE_PID  ///////// Tune PID by gamepad buttons
        else if(gamepadData.data.btns.btn_set.b){  // B : increase Kp0.1
          esc_servo.kp+=0.1;
          esc_servo.pid.tune(esc_servo.kp, esc_servo.ki, esc_servo.kd);
          Serial.printf("Kp:%.2f, Ki:%.2f, Kd:%.2f\n", esc_servo.kp, esc_servo.ki, esc_servo.kd);
        }else if(gamepadData.data.btns.btn_set.a){ // A : decrease Kp 0.1
          esc_servo.kp-=0.1;
          if(esc_servo.kp<0.1)
            esc_servo.kd=0.1;
          esc_servo.pid.tune(esc_servo.kp, esc_servo.ki, esc_servo.kd);
          Serial.printf("Kp:%.2f, Ki:%.2f, Kd:%.2f\n", esc_servo.kp, esc_servo.ki, esc_servo.kd);
        }
        else if(gamepadData.data.btns.btn_set.up){  //up-pad : increase Ki 0.01
          esc_servo.ki+=0.01;
          esc_servo.pid.tune(esc_servo.kp, esc_servo.ki, esc_servo.kd);
          Serial.printf("Kp:%.2f, Ki:%.2f, Kd:%.2f\n", esc_servo.kp, esc_servo.ki, esc_servo.kd);
        }else if(gamepadData.data.btns.btn_set.dn){  //down-pad : decrease Ki 0.01
          esc_servo.ki-=0.01;
          if(esc_servo.ki<0)
            esc_servo.ki=0;
          esc_servo.pid.tune(esc_servo.kp, esc_servo.ki, esc_servo.kd);
          Serial.printf("Kp:%.2f, Ki:%.2f, Kd:%.2f\n", esc_servo.kp, esc_servo.ki, esc_servo.kd);
        }
        else if(gamepadData.data.btns.btn_set.right){  //right-pad : increase Kd 0.01
          esc_servo.kd+=0.01;
          esc_servo.pid.tune(esc_servo.kp, esc_servo.ki, esc_servo.kd);
          Serial.printf("Kp:%.2f, Ki:%.2f, Kd:%.2f\n", esc_servo.kp, esc_servo.ki, esc_servo.kd);
        }else if(gamepadData.data.btns.btn_set.left){  //left-pad : decrease Kd 0.01
          esc_servo.kd-=0.01;
          if(esc_servo.kd<0)
            esc_servo.kd=0;
          esc_servo.pid.tune(esc_servo.kp, esc_servo.ki, esc_servo.kd);
          Serial.printf("Kp:%.2f, Ki:%.2f, Kd:%.2f\n", esc_servo.kp, esc_servo.ki, esc_servo.kd);
        }
#endif // #ifdef TUNE_PID
        else if(gamepadData.data.btns.btn_set.lb){ // left bumper: cruise speed step dn
          if(b_cruise){
            if(esc_servo.GetCurSpeed()>0 && esc_servo.GetForwardSpeed()>=20)
              esc_servo.SetForwardSpeed(esc_servo.GetForwardSpeed()-5);
            else
              esc_servo.SetForwardSpeed(15);
          }
        }else if(gamepadData.data.btns.btn_set.rb){ // right bumper: cruise speed step up
          if(b_cruise ){
            if(esc_servo.GetCurSpeed()>0 && esc_servo.GetForwardSpeed()<95)
              esc_servo.SetForwardSpeed(esc_servo.GetForwardSpeed()+5);
          }
        }
      } // if(prev_btns==0){
      prev_btns=gamepadData.data.btns.var;
    }
    /*
    if(encoder.rpm<10){ // turn crouse off when it is too slow.
      Serial.printf("Off CR1 %d\n", encoder.rpm);
      b_cruise=false;
    }
    */
    // L-R: 0~199. Left:0, Right:199, Center:100
    esc_servo.SetServoMicros(uint16_t(gamepadData.data.left_right-100));

    if(b_cruise!=0){
      if(cur-b_cruise>1000 && (gamepadData.data.r_trigger>3 || gamepadData.data.l_trigger>3)){
        //Serial.printf("Cruise Off %d, %d\n", gamepadData.data.r_trigger,gamepadData.data.l_trigger);
        b_cruise=0;
      }
    }else{
      esc_servo.SetForwardSpeed(gamepadData.data.r_trigger);
    }
    
    esc_servo.SetBackwardSpeed(gamepadData.data.l_trigger);
    b_rcvFrm_chA=false;
    
    esc_servo.process(encoder.rpm_avg, (bool)b_cruise); // Should be inside of frame processing.
  } //if(b_rcvFrm_chA) Frame Precessing. 10hz

  // Channel_B : GPS 115200, 1hz
  if (i2cuart.available(SC16IS752_CHANNEL_B) > 0){
    // Read GLL String from GPS.
    bufB_len = i2cuart.readLine(SC16IS752_CHANNEL_B, buf_chB);
    if(bufB_len)
    {
      //Serial.println(buf_chB);
      int pos_from=0, pos_to, subStr_num=0;;
      String subStr[10]; // GLL Sub String storage
      String gll_str=String(buf_chB);
      float la, lo;
      
      do{ // split gll string to sub_str[]. should be getting 10 substrings.
        pos_to=gll_str.indexOf(',', pos_from);
        if(pos_to){
          //Serial.printf("pos: %s\n", gll.substring(pos_from, pos_to));
          subStr[subStr_num++] = gll_str.substring(pos_from, pos_to);
          pos_from=pos_to+1;
          if(subStr_num>10) // fail if getting more than 10 sub string from GLL String..
            return;
        }
      }while(pos_to>=0);
      
      // get la, lo from subStr
      if(subStr_num==8){ // $GNGLL,5102.3425,N,11411.2239,W,155603.000,A,A*59     
        if(subStr[6]=="A"){
          la = subStr[1].toFloat();
          if(subStr[2]=="S")
            la*=-1;
          lo = subStr[3].toFloat();
          if(subStr[4]=="W")
            lo*=-1;
        }else{
          la=lo=0;
        }
      }

      // Send la, lo, RPM to python application.
      uint8_t* p = (uint8_t*)&la;
      for(int i=0; i<4; i++){
        try{
          i2cuart.write(SC16IS752_CHANNEL_A, *(p+i));
        }catch(int err){
          Serial.printf("Write Error#1 %d\n", err);
        }
      }
      p = (uint8_t*)&lo;
      for(int i=0; i<4; i++){
        try{
          i2cuart.write(SC16IS752_CHANNEL_A, *(p+i));
        }catch(int err){
          Serial.printf("Write Error#2 %d\n", err);
        }
      }

      // Send RPM to python applicaion.
      try{
        i2cuart.write(SC16IS752_CHANNEL_A, (uint8_t)encoder.rpm_avg);
      }catch(int err){
        Serial.printf("Write Error#3 %d\n", err);
      }
      bufB_len=0;
    }
  }
  encoder.tick(cur, esc_servo.GetCurSpeed()); // use speed to find direction of rotation.
  yield;
}
