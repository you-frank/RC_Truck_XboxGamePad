/*
 * https://github.com/kanestoboi/AS5600
 */

#ifndef AS5600_h
#define AS5600_h

#include "Arduino.h"
#include <Wire.h>
/**
 * @brief I2C Magnetic Encoder
 *
 * @param 
**/
class AS5600
{
public:
    bool en, fwd;
    int8_t prev_pos, diff_pos;
    static const uint8_t buf_num=10;//8;
    int16_t rpm_buf[buf_num]; // save last 4 rpm 
    int16_t rpm_avg;
    uint8_t rpm_buf_idx;
    unsigned long cur_prev;
    // bool dir; // 해상도가 낮아서 방향을 알수는 없음. 실제 rpm이 댑다 빠르면 읽을 수있는 rpm도 정확도가 꽤 떨이질지도...
    
    AS5600(){ Wire.begin(); prev_pos=0; cur_prev=0; for(int i=0; i<buf_num; i++) {rpm_buf[i]=0;} };
    bool begin(){ en=isMagnetDetected(); prev_pos=diff_pos=rpm_buf_idx=rpm_avg=0; return en; };

    uint16_t getAvgRpm() { return rpm_avg; }
    
    // getAngle()하면 앞으로 가면 15->0으로 줄어듬.
    // 뒤로 가면 0->15로 증가함.
    void tick(unsigned long cur, int16_t speed_cur){
      float tp_rpm=0;
      if(cur==cur_prev)
        return;
      
      if(!en) return;
      uint8_t cur_pos = getAngle();

      if(cur_pos==0xff) return; // do nothing when i2c is busy
      //Serial.printf("encoder :%d\n", cur_pos);
      
      if(cur==cur_prev)  return;

      if(cur_pos!=prev_pos){
        diff_pos = cur_pos-prev_pos;
        if(speed_cur>=0){// 전진중. cur_pos가 감소함.
          if(diff_pos>0) diff_pos-=16; // 0~15범위 넘어갔으므로 제대로 맞춰줌.
        } else { // 후진중. cur_pos가 증가함.
          if(diff_pos<0) diff_pos+=16; // 0~15범위 넘어갔으므로 제대로 맞춰줌.
        }
        if(abs(speed_cur)<5 && abs(diff_pos)>10)
          diff_pos=diff_pos/abs(diff_pos);
        //int16_t tp_rpm = (int16_t)( (22*abs(diff_pos)*60000) / ((cur-cur_prev)*360));
        tp_rpm =  (24*abs(diff_pos)) / ((cur-cur_prev));
      }

      rpm_buf[rpm_buf_idx++]=tp_rpm;
      //rpm_buf_idx&=0x07;  // buffer size바뀌면 같이 바꿔야함.
      if(rpm_buf_idx>=buf_num)
        rpm_buf_idx=0;
      int16_t sum=0;
      for(int i=0; i<buf_num; i++) {
        sum+=rpm_buf[i];
      }
      //rpm_avg=(int16_t)(sum>>3); // buffer size바뀌면 같이 바꿔야함.
      rpm_avg=(int16_t)(sum/buf_num); 
      
      cur_prev=cur;
      prev_pos=cur_pos;
/*
      pos=tp_val-8; // -8~7로 범위를 옮김.
      if(pos==prev_pos) // 정확히 한바퀴를 돈것인지, 멈춤건지 확인이 불가함. 멈춘걸로 간주함.
        return;
      if(pos>prev_pos)
        fwd=true;
      else
        fwd=false;
        
      if(prev_pos>=0){
        if(pos<0){
          Serial.printf("encoder [%d]\n", cur-cur_bk);
          rpm++;
          cur_bk=cur;
          
        }
      }else{
        if(pos>=0){
          rpm++;
          cur_bk=cur;
          Serial.printf("[%d,%d] r:%d\n", pos, prev_pos, rpm);
        }
      }
      prev_pos=pos;
      */
    } // end of tick(cur)
    
    // return 0~15
    uint8_t getAngle() {
      return _getRegister(_ANGLEAddressMSB);//, _ANGLEAddressLSB);
    }

    uint8_t _getRegister(byte register1) {  
        uint8_t _b=0;
        if (Wire.available()) { return 0xff; } // return 0xff when i2c is busy!!!!!!  should Check it.
        Wire.beginTransmission(_AS5600Address);
        Wire.write(register1);
        Wire.endTransmission();
      
        Wire.requestFrom(_AS5600Address, 1);
      
        while (Wire.available() == 0) { }
        _b = Wire.read();
      
        return _b;
    }

    bool isMagnetDetected() {
        uint8_t _b=0;
        _b = _getRegister(_STATUSAddress) & 0b00111000;
        Serial.printf("Status : 0x%x\n", _b);
        if (_b & (1<<5)) { return true; }
        return false;
    }
private:
    private:
      int _AS5600Address = 0x36; // I2C address for AS5600
      byte _RAWANGLEAddressMSB = 0x0C;
      byte _RAWANGLEAddressLSB = 0x0D;
      byte _ANGLEAddressMSB = 0x0E;
      byte _ANGLEAddressLSB = 0x0F;
      byte _STATUSAddress = 0x0B;
/*
      byte _ZMCOAddress = 0x00;
      byte _ZPOSAddressMSB = 0x01;
      byte _ZPOSAddressLSB = 0x02;
      byte _MPOSAddressMSB = 0x03;
      byte _MPOSAddressLSB = 0x04;
      byte _MANGAddressMSB = 0x05;
      byte _MANGAddressLSB = 0x06;
      byte _CONFAddressMSB = 0x07;
      byte _CONFAddressLSB = 0x08;
      byte _AGCAddress = 0x1A;
      byte _MAGNITUDEAddressMSB = 0x1B;
      byte _MAGNITUDEAddressLSB = 0x1C;
      byte _BURNAddress = 0xFF;
*/      
};

#endif
