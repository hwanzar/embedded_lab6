/*
 * system.c
 *
 *  Created on: Nov 24, 2023
 *      Author: huaho
 */

#include "system.h"

int volval = 99;//duty cycle of volume
uint8_t isnotify = 0;//to notify to computer

float power_samples[100] = {0}; // Mảng lưu giá trị công suất
uint8_t sampling_period = 1;    // Chu kỳ lấy mẫu (1 giây mặc định)
uint8_t grid_split =10;
uint16_t time_range = 100;      // Chiều dài trục OX (100 đơn vị thời gian)
float max_power = 300.0;         // Giá trị tối đa trục OY (10 mW mặc định)

void LCD_show_sensor();

/*
 * @brief:	update sensor value
 * @para:	none
 * @retval:	none
 * */
void sensor_diplay(){
	  if(!is_timer_on(0)){
		  set_timer(0, READ_BUTTON_TIME);
		  ds3231_ReadTime();
		  button_Scan();
		  sensor_Read();
		  //Check Potentiometer value and send notify
		  if(sensor_GetPotentiometer() >= 4095 * 0.7){
			  if(isnotify == 1){
				  buzzer_SetVolume(volval);
				  if(!is_timer_on(4)){
					  set_timer(4, ONE_SECOND);
					  isnotify = 0;
	  				  uart_Rs232SendString("Potentiometer > 70%\n");
	  				  uart_Rs232SendString("Please reduce Potentiometer\n\n");
				  }
			  }
			  else if(isnotify == 0){
				  buzzer_SetVolume(0);
				  if(!is_timer_on(4)){
					  set_timer(4, ONE_SECOND);
					  isnotify = 1;
				  }
			  }
	  	  }
		  else{
			  buzzer_SetVolume(0);
		  }

		  //change volume value
		  if(button_count[11] == 1){
			  volval += 10;
			  if(volval > 99){
				  volval = 0;
			  }
			  lcd_ShowIntNum(10, 300, volval, 2, WHITE, BLACK, 16);
		  }
		  if(button_count[15] == 1){
			  volval -= 10;
			  if(volval < 0){
				  volval = 99;
			  }
			  lcd_ShowIntNum(10, 300, volval, 2, WHITE, BLACK, 16);
		  }
	  }
	  //show sensor value to LCD
	  if(!is_timer_on(3)){
		  set_timer(3, LCD_SENSOR_TIME);
		  LCD_show_sensor();
//		  store_power_data(sensor_GetPotentiometer()*100/4095);
//		  plot_power_chart();
	  }
}

/*
 * @brief:	show sensor value to screen
 * @para:	none
 * @retval:	none
 * */
void LCD_show_sensor(){
	lcd_ShowStr(10, 100, "Voltage(V):", WHITE, BLACK, 16, 0);
	lcd_ShowFloatNum(130, 100, sensor_GetVoltage(), 4, WHITE, BLACK, 16);

	lcd_ShowStr(10, 120, "Current(mA):", WHITE, BLACK, 16, 0);
	lcd_ShowFloatNum(130, 120, sensor_GetCurrent(), 4, WHITE, BLACK, 16);

	lcd_ShowStr(10, 140, "Power(mW):", WHITE, BLACK, 16, 0);
	lcd_ShowFloatNum(130, 140, sensor_GetCurrent() * sensor_GetVoltage(), 4, WHITE, BLACK, 16);

	lcd_ShowStr(10, 160, "Light:", WHITE, BLACK, 16, 0);
	if(sensor_GetLight() <= 4095*0.75){
			lcd_ShowStr(60, 160, "(Strong)", WHITE, BLACK, 16, 0);
		}
		else{
			lcd_ShowStr(60, 160, "(Weak)  ", WHITE, BLACK, 16, 0);
		}
	lcd_ShowIntNum(130, 160, sensor_GetLight(), 4, WHITE, BLACK, 16);


	lcd_ShowStr(10, 180, "Poten(Ohm):", WHITE, BLACK, 16, 0);
	lcd_ShowIntNum(130, 180, sensor_GetPotentiometer()*100/4095, 2, WHITE, BLACK, 16);
	lcd_ShowStr(180, 180, "%", WHITE, BLACK, 16, 0);

	lcd_ShowStr (10, 200, "Temp(C):", WHITE, BLACK, 16, 0);
	lcd_ShowFloatNum (130, 200, sensor_GetTemperature(), 4, WHITE, BLACK, 16);
}

void system_loop(void) {
	sensor_diplay();
	fsm_clock();
}




void store_power_data(int power) {
    // Shift all elements one step to the left
    for (int i = 0; i < time_range - 1; i++) {
        power_samples[i] = power_samples[i + 1];
    }
    // Store the new power value at the last position
    power_samples[time_range - 1] = power;
}


int get_digit_count(int number) {
    int count = 0;
    if (number == 0) return 1; // Trường hợp đặc biệt, số 0 có 1 chữ số
    while (number != 0) {
        number /= 10;
        count++;
    }
    return count;
}


void plot_power_chart() {
    // Xóa vùng biểu đồ
    lcd_Fill(10, 10, 210, 210, BLACK);

    // Vẽ grid dọc (OX) theo grid_split
    for (int i = 0; i <= grid_split; i++) {
        int x = 10 + (200 * i / grid_split); // Tính tọa độ x dựa trên grid_split
        lcd_DrawLine(x, 10, x, 210, LIGHTGRAY);
    }

    // Vẽ grid ngang (OY) theo grid_split
    for (int i = 0; i <= grid_split; i++) {
        int y = 210 - (200 * i / grid_split); // Tính tọa độ y dựa trên grid_split
        lcd_DrawLine(10, y, 210, y, LIGHTGRAY);
    }

    // Vẽ các nhãn trục OY
    for (int i = 0; i <= grid_split; i++) {
        int label = (int)(i * max_power / grid_split); // Giá trị nhãn trục OY
        int y = 210 - (200 * i / grid_split);         // Tọa độ y
        int len = get_digit_count(label);             // Số chữ số của `label`
        lcd_ShowIntNum(0, y, label, len, WHITE, BLACK, 16);
    }

    // Vẽ các nhãn trục OX
    for (int i = 0; i <= grid_split; i++) {
        int label = (int)(time_range - (i * time_range / grid_split)); // Giá trị nhãn trục OX
        int x = 10 + (200 * i / grid_split);                          // Tính tọa độ x
        int len = get_digit_count(label);                             // Số chữ số của `label`
        lcd_ShowIntNum(x, 215, label, len, WHITE, BLACK, 16);
    }

    // Vẽ đường biểu diễn công suất
    for (int i = 0; i < time_range - 1; i++) {
        // Chuyển đổi giá trị từ `power_samples` thành tọa độ pixel
        int x1 = 10 + (200 * i / time_range);
        int y1 = 210 - (int)((200 * power_samples[i]) / max_power);
        int x2 = 10 + (200 * (i + 1) / time_range);
        int y2 = 210 - (int)((200 * power_samples[i + 1]) / max_power);

        // Đảm bảo tọa độ y1 và y2 nằm trong phạm vi hợp lệ
        if (y1 < 10) y1 = 10;
        if (y1 > 210) y1 = 210;
        if (y2 < 10) y2 = 10;
        if (y2 > 210) y2 = 210;

        // Vẽ đường nối giữa các điểm
        lcd_DrawLine(x1, y1, x2, y2, RED);
    }
}









