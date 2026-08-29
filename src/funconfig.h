#ifndef FUNCONFIG_H
#define FUNCONFIG_H

// ตั้งค่าสัญญาณนาฬิกาของระบบ (Core Clock) เป็น 48MHz โดยใช้ PLL และ HSI (Internal RC Oscillator 24MHz)
#define FUNCONF_SYSTEM_CORE_CLOCK 48000000
#define FUNCONF_USE_PLL 1

// เปิดการใช้ printf ดีบั๊กผ่านสาย SWIO/SDI (ไม่ต้องใช้สายซีเรียล TX/RX เพิ่มเติม)
#define FUNCONF_USE_DEBUGPRINTF 1

// ปิดการใช้ printf ผ่าน UART
#define FUNCONF_USE_UARTPRINTF 0

#endif
