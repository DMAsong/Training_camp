/*
 * colours_control.h
 *
 *  Created on: 2021年8月3日
 *      Author: Admin
 */

#ifndef COLOURS_CONTROL_H_
#define COLOURS_CONTROL_H_
enum
{
  OPEN_LIGHT = 2, //开灯灯条
  CLOSE_LIGHT,    //关灯灯条
  LIGHT_MODE,     //照明模式
  COLORS_LIGHT,   //幻彩灯光
  RED_EFFECT, //红色效果
  GREEN_EFFECT, //绿色效果
  BLUE_EFFECT, //蓝色效果
  YELLOW_EFFECT, //黄色效果
  CYAN_EFFECT, //青色效果
  PWRPLE_EFFECT, //紫色效果
  BREATH_EFFECT, //呼吸效果
  DEC_BRIGHT, //调低亮度
  ADD_BRIGHT, //调高亮度
  MEDIUM_BRIGHT, //中等亮度
  MAX_BRIGHT, //最大亮度
  MIN_BRIGHT, //最小亮度
  EYES_MODE, //护眼模式
};
/*
*@brief 七彩灯控制初始化
*
*
*/
void my_colours_init(void);
/*
*@brief 选择灯条模式
*@param mode 对应模式的枚举值
*
*/
void colors_control_mode(void);
void set_mode(uint8_t id);
#endif /* COLOURS_CONTROL_H_ */
