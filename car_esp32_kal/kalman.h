/*
 * SimpleKalmanFilter - a Kalman Filter implementation for single variable models.
 * Created by Denys Sene, January, 1, 2017.
 * Modified by Frank You, Jan, 1, 2023
 * Released under MIT License - see LICENSE file for details.
 *
 * https://github.com/denyssene/SimpleKalmanFilter
 */
#include <stdbool.h>
#include <math.h>

#ifndef INC_KALMAN_H_
#define INC_KALMAN_H_

Class Kalman{
  private:
    float _err_measure;
    float _err_estimate;
    float _q;
    float _current_estimate = 0;
    float _last_estimate = 0;
    float _kalman_gain = 0;
    bool _init=false;
  
    uint32_t prev_tick;
    uint16_t _dt;
    
  public:  
    // need to set dt for encoder. Because measuring value should be double if dt is double.
    // should NOT use dt for normal sensor or not time related.
    void SimpleKalmanFilter(float mea_e, float est_e, float q, uint16_t dt=0)
    {
      _err_measure=mea_e;
      _err_estimate=est_e;
      _q = q;
    
      _dt=dt; // ysys adding dt feature for updateEstimate()
    }
    
    float updateEstimate(float mea)
    {
      if(_dt!=0) { // encoder need dt value. Because measuring value should be double if dt is double.
        uint16_t tm = millis()-prev_tick;
        prev_tick=millis();
        if(tm!=_dt) {// Increasing the 'mea' value in proportion to the increased time.  update가 불리우는 interval일 일치하니 않으면 증가된 시간만큼 mea값을 증가시켜줌.
          mea*=_dt/tm;
        }
      }
      if(_init==0){
        _current_estimate=_last_estimate=mea;
        _init=true;
      }
      _kalman_gain = _err_estimate/(_err_estimate + _err_measure);
      _current_estimate = _last_estimate + _kalman_gain * (mea - _last_estimate);
      _err_estimate =  (1.0 - _kalman_gain)*_err_estimate + fabs(_last_estimate-_current_estimate)*_q;
      _last_estimate=_current_estimate;
    
      return _current_estimate;
    }
    
    void setMeasurementError(float mea_e) { _err_measure=mea_e; }
    void setEstimateError(float est_e) { _err_estimate=est_e; }
    void setProcessNoise(float q) { _q=q; }
    float getKalmanGain() { return _kalman_gain; }
    float getEstimateError() { return _err_estimate; }
}

#endif /* INC_KALMAN_H_ */
