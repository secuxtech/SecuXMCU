#ifndef EPD_H
#define EPD_H

#include "nrf_drv_spi.h"
// #include "nrf_gpio.h"
//#define PIN_RESET 22
//#define PIN_BUSY  25
#define AMD_HEIGHT  64
#define AMD_WIDTH   142
#define AMD_DISPLAY_BOUNDARY_X_MIN  3
#define AMD_DISPLAY_BOUNDARY_X_MAX  145
#define AMD_DISPLAY_BOUNDARY_Y_MIN  3
#define AMD_DISPLAY_BOUNDARY_Y_MAX  67

enum SD_COLOR{
    SD_COLOR_NORMAL = 0,
    SD_COLOR_INVERT = 1
};

enum SD_GRAYSCALE{
    SD_GRAYSCALE_OFF = 0,
    SD_GRAYSCALE_ON = 1
};


void SUB_DISPLAY_INIT(nrf_drv_spi_t const * const p_instance);
void SUB_DISPLAY_FLASH(nrf_drv_spi_t const * const p_instance,uint8_t color);
void SUB_DISPLAY_DPA(nrf_drv_spi_t const * const p_instance,const unsigned char * p_rx_buffer,uint8_t color,uint8_t  grayscale );
uint8_t SUB_DISPLAY_AREA_Pattern(nrf_drv_spi_t const * const p_instance,const unsigned char * p_rx_buffer,uint8_t col_start,uint8_t col_size,uint8_t row_start,uint8_t row_size,uint8_t color,uint8_t epdmode,uint8_t grayscale);
uint8_t SUB_GET_TEMPERATURE(nrf_drv_spi_t const * const p_instance);
void SUB_DISPLAY_HVs_on(nrf_drv_spi_t const * const p_instance);
void SUB_DISPLAY_HVs_off(nrf_drv_spi_t const * const p_instance);

#endif