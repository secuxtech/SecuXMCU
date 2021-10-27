/**
 * Copyright (c) 2017 - 2018, Nordic Semiconductor ASA
 * 
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 * 
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 * 
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 * 
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#include "sdk_common.h"

#if NRF_MODULE_ENABLED(ST7789)

#include "nrf_lcd.h"
#include "nrf_drv_spi.h"
#include "nrfx_spim.h"
#include "nrf_delay.h"
#include "nrf_gpio.h"
#include "boards.h"

extern uint8_t option_pin_mode1;
// Set of commands described in ST7789 data sheet.

//SYSTEM FUNCTION COMMAND TABLE 1
#define ST7789_NOP       0x00
#define ST7789_SWRESET   0x01
#define ST7789_RDDID     0x04
#define ST7789_RDDST   	 0x09
#define ST7789_RDDPM	 0x0A
#define ST7789_RDDMADCTL 0x0B
#define ST7789_RDDCOLMOD 0x0C
#define ST7789_RDDIM	 0x0D
#define ST7789_RDDSM	 0x0E
#define ST7789_RDDSDR	 0x0F

#define ST7789_SLPIN   0x10
#define ST7789_SLPOUT  0x11
#define ST7789_PTLON   0x12
#define ST7789_NORON   0x13

#define ST7789_INVOFF  0x20
#define ST7789_INVON   0x21
#define ST7789_GAMSET  0x26
#define ST7789_DISPOFF 0x28
#define ST7789_DISPON  0x29
#define ST7789_CASET   0x2A
#define ST7789_RASET   0x2B
#define ST7789_RAMWR   0x2C
#define ST7789_RAMRD   0x2E

#define ST7789_PTLAR    0x30
#define ST7789_VSCRDEF  0x33
#define ST7789_TEOFF    0x34
#define ST7789_TEON		0x35
#define ST7789_MADCTL   0x36
#define ST7789_VSCRSADD 0x37
#define ST7789_IDMOFF   0x38
#define ST7789_IDMON    0x39
#define ST7789_COLMOD   0x3A
#define ST7789_RAMWRC   0x3C
#define ST7789_RAMRDC   0x3E

#define ST7789_TESCAN   0x44
#define ST7789_RDTESCAN 0x45

#define ST7789_WRDISBV  0x51
#define ST7789_RDDISBV  0x52
#define ST7789_WRCTRLD  0x53
#define ST7789_RDCTRLD  0x54
#define ST7789_WRCACE	0x55
#define ST7789_RDCABC   0x56
#define ST7789_WRCABCMB 0x5E
#define ST7789_RDCABCMB 0x5F

#define ST7789_RDABCSDR 0x68

#define ST7789_RDID1   0xDA
#define ST7789_RDID2   0xDB
#define ST7789_RDID3   0xDC

//SYSTEM FUNCTION COMMAND TABLE 2

#define ST7789_RAMCTRL 0xB0
#define ST7789_RGBCTRL 0xB1
#define ST7789_PORCTRL 0xB2
#define ST7789_FRCTRL1 0xB3
#define ST7789_PARCTRL 0xB5
#define ST7789_GCTRL   0xB7
#define ST7789_GTADJ   0xB8
#define ST7789_DGMEN   0xBA
#define ST7789_VCOMS   0xBB

#define ST7789_LCMCTRL  0xC0
#define ST7789_IDSET    0xC1
#define ST7789_VDVVRHEN 0xC2
#define ST7789_VRHS     0xC3
#define ST7789_VDVSET   0xC4
#define ST7789_VCMOFSET 0xC5
#define ST7789_FRCTRL2  0xC6
#define ST7789_CABCCTRL 0xC7
#define ST7789_REGSEL1  0xC8
#define ST7789_REGSEL2  0xCA
#define ST7789_PWMFRSEL 0xCC

#define ST7789_PWCTRL1   0xD0
#define ST7789_VAPVANEN  0xD2
#define ST7789_CMD2EN    0XDF

#define ST7789_PVGAMCTRL 0xE0
#define ST7789_NVGAMCTRL 0xE1
#define ST7789_DGMLUTR   0xE2
#define ST7789_DGMLUTB   0xE3
#define ST7789_GATECTRL  0xE4
#define ST7789_SPI2EN    0xE7
#define ST7789_PWCTRL2   0xE8
#define ST7789_EQCTRL    0xE9
#define ST7789_PROMCTRL  0xEC

#define ST7789_PROMEN    0xFA
#define ST7789_NVMSET    0xFC
#define ST7789_PROMACT   0xFE



#define ST7789_MADCTL_MY  0x80
#define ST7789_MADCTL_MX  0x40
#define ST7789_MADCTL_MV  0x20
#define ST7789_MADCTL_ML  0x10
#define ST7789_MADCTL_RGB 0x00
#define ST7789_MADCTL_BGR 0x08
#define ST7789_MADCTL_MH  0x04

/* @} */

//#define RGB2BGR(x)      (x << 11) | (x & 0x07E0) | (x >> 11)  //5-6-5 RGB
#define RGB2BGR(x)      (x << 16) | (x & 0xFC00) | (x >> 16)  //8(6)-8(6)-8(6) RGB

static const nrfx_spim_t spi = NRFX_SPIM_INSTANCE(ST7789_SPI_INSTANCE);  /**< SPIM instance. */



/**
 * @brief Structure holding ST7789 controller basic parameters.
 */
typedef struct
{
    uint8_t tab_color;      /**< Color of tab attached to the used screen. */
}st7789_t;

/**
 * @brief Enumerator with TFT tab colors.
 */
typedef enum{
    INITR_GREENTAB = 0,     /**< Green tab. */
    INITR_REDTAB,           /**< Red tab. */
    INITR_BLACKTAB,         /**< Black tab. */
    INITR_280GREENTAB       /**< Green tab, 2.80" display. */
}st7789_tab_t;

static st7789_t m_st7789;

static inline void spi_write(const void * data, size_t size)
{
	//SPIM
	nrfx_spim_xfer_desc_t const spim_xfer_desc =
    {
        .p_tx_buffer = data,
        .tx_length   = size,
        .p_rx_buffer = NULL,
        .rx_length   = 0,
    };
    nrfx_spim_xfer(&spi, &spim_xfer_desc, 0);
	
}

static inline void write_command(uint8_t c)
{
    nrf_gpio_pin_clear(ST7789_DC_PIN);
    spi_write(&c, sizeof(c));
}

static inline void write_data(uint8_t c)
{
    nrf_gpio_pin_set(ST7789_DC_PIN);
    spi_write(&c, sizeof(c));
}

static void set_addr_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    ASSERT(x0 <= x1);
    ASSERT(y0 <= y1);

    write_command(ST7789_CASET);						        // Column Address Set
    write_data((x0>>8)&0xFF);                       // For a 240x320 display, it is always 0.
    write_data(x0&0xFF);
    write_data((x1>>8)&0xFF);                       // For a 240x320 display, it is always 0.
    write_data(x1&0xFF);
   
    write_command(ST7789_RASET);                    // Row Address Set
    write_data((y0>>8)&0xFF);                       // For a 240x320 display, it is always 0.
    write_data(y0&0xFF);
    write_data((y1>>8)&0xFF);                       // For a 240x320 display, it is always 0.
    write_data(y1&0xFF);
    write_command(ST7789_RAMWR);
}

static void command_list(void)
{
    //write_command(ST7789_SWRESET);
    //nrf_delay_ms(10);
    nrf_gpio_pin_set(ST7789_LCD_RESET);
    nrf_delay_ms(1);
    nrf_gpio_pin_clear(ST7789_LCD_RESET);
    nrf_delay_ms(10);
    nrf_gpio_pin_set(ST7789_LCD_RESET);
    nrf_delay_ms(120);

    write_command(ST7789_SLPOUT);
    nrf_delay_ms(120);

    /*write_command(0x35);
    write_data(0x01);*/

    //-------------------------------display and color format setting------------------------------//
    write_command(ST7789_MADCTL);     // if MV = 1, set this to 0x60
    write_data(0x60);
    write_command(0x3a);
    write_data(0x06);

    //-------------------------------- Frame rate setting------------------------------------------//
    write_command(0xb2);
    write_data(0x0c);
    write_data(0x0c);
    write_data(0x00);
    write_data(0x33);
    write_data(0x33);
    write_command(0xb7);
    write_data(0x35);

    //---------------------------------ST7789 Power setting--------------------------------------//
    write_command(0xbb);
    write_data(0x1f);
    write_command(0xc0);
    write_data(0x2c);
    write_command(0xc2);
    write_data(0x01);
    write_command(0xc3);
    write_data(0x11);
    write_command(0xc4);
    write_data(0x20);
    write_command(0xc6);
    write_data(0x05);
    write_command(0xd0);
    write_data(0xa4);
    write_data(0xa1);

    //--------------------------------ST7789V gamma setting---------------------------------------//
    write_command(0xe0);
    write_data(0xd0);
    write_data(0x00);
    write_data(0x14);
    write_data(0x15);
    write_data(0x13);
    write_data(0x2c);
    write_data(0x42);
    write_data(0x43);
    write_data(0x4e);
    write_data(0x09);
    write_data(0x16);
    write_data(0x14);
    write_data(0x18);
    write_data(0x21);
    write_command(0xe1);
    write_data(0xd0);
    write_data(0x00);
    write_data(0x14);
    write_data(0x15);
    write_data(0x13);
    write_data(0x0b);
    write_data(0x43);
    write_data(0x55);
    write_data(0x53);
    write_data(0x0c);
    write_data(0x17);
    write_data(0x14);
    write_data(0x23);
    write_data(0x20);

    write_command(ST7789_CASET);	
    write_data(0x00);
    write_data(0x00);
    write_data(0x01);
    write_data(0x3F);

    write_command(ST7789_RASET);		
    write_data(0x00);
    write_data(0x00);
    write_data(0x00);
    write_data(0xEF);

    //write_command(ST7789_INVON);		// for untouched panel


    write_command(0x29);
    write_command(0x2c);
}

// Original table 
void ST7789VPanelTurnOnDisplay(void)
{
		write_command(0x29);
}
void ST7789VPanelTurnOffDisplay(void)
{
		write_command(0x28);
}
void ST7789VPanelTurnOnPartial(uint16_t PSL , uint16_t PEL )
{
		write_command(0x30);
		write_data((PSL>>8)&0xFF);
		write_data(PSL&0xFF); //PSL: Start Line
		write_data((PEL>>8)&0xFF);
		write_data(PEL&0xFF); //PEL: End Line
		write_command(0x12);
}
void ST7789VPanelTurnOffPartial (void)
{
		write_command(0x13);
}
void ST7789VPanelTurnOnIdle (void)
{
		write_command(0x39);
}
void ST7789VPanelTurnOffIdle (void)
{
		write_command(0x38);
}
void ST7789VPanelSleepInMode (void)
{
		write_command(0x10);
		nrf_delay_ms(120);
}
void ST7789VPanelSleepOutMode (void)
{
		write_command(0x11);
		nrf_delay_ms(120);
}

static ret_code_t hardware_init(void)
{
    ret_code_t err_code;

    nrf_gpio_cfg_output(ST7789_DC_PIN);
	nrf_gpio_cfg_output(ST7789_LCD_RESET);
	
	nrfx_spim_config_t spi_config = NRFX_SPIM_DEFAULT_CONFIG;

    if (option_pin_mode1 == 1) // st7789 v1
        spi_config.frequency = NRF_SPIM_FREQ_8M;
    else
        spi_config.frequency = NRF_SPIM_FREQ_32M;
    spi_config.sck_pin  = ST7789_SCK_PIN;
    spi_config.miso_pin = ST7789_MISO_PIN;
    spi_config.mosi_pin = ST7789_MOSI_PIN;
    spi_config.ss_pin   = ST7789_SS_PIN;

	err_code = nrfx_spim_init(&spi, &spi_config, NULL, NULL); //SPIM

    return err_code;
}

static ret_code_t st7789_init(void)
{
    ret_code_t err_code;

    m_st7789.tab_color = ST7789_TAB_COLOR;

    err_code = hardware_init();
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    command_list();

    return err_code;
}

static void st7789_uninit(void)
{
	nrfx_spim_uninit(&spi);  //SPIM
}

//static void st7789_pixel_draw(uint16_t x, uint16_t y, uint32_t color)
void st7789_pixel_draw(uint16_t x, uint16_t y, uint32_t color)
{
    set_addr_window(x, y, x, y);

    //color = RGB2BGR(color);

    const uint8_t data[3] = { (color>>16)&0xFF , (color>>8)&0xFF , color&0xFF };

    nrf_gpio_pin_set(ST7789_DC_PIN);

    spi_write(data, sizeof(data));

    nrf_gpio_pin_clear(ST7789_DC_PIN);
}

#define SPI_TX_BYTES_LIMIT      240
#define SPI_TX_PIXELS_LIMIT     80

static void st7789_rect_draw(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color)
{
    set_addr_window(x, y, x + width - 1, y + height - 1);

    //color = RGB2BGR(color);

    //const uint8_t data[3] = { (color>>16)&0xFF , (color>>8)&0xFF , color&0xFF };
		uint8_t data[SPI_TX_BYTES_LIMIT];
		uint8_t len;
		uint8_t i,j;
		uint8_t t_80,m_80;
		uint8_t r_color;
		uint8_t g_color;
 		uint8_t b_color;
        nrf_gpio_pin_set(ST7789_DC_PIN);
		r_color=(color>>16)&0xFF;
		g_color=(color>>8)&0xFF;
		b_color=color&0xFF;
		len=0;
		if (width>=SPI_TX_PIXELS_LIMIT){
				for (i=0;i<SPI_TX_PIXELS_LIMIT;i++) {
					data[len++]=r_color;
					data[len++]=g_color;
					data[len++]=b_color;
				}
		} else {
				for (i=0;i<width;i++) {
					data[len++]=r_color;
					data[len++]=g_color;
					data[len++]=b_color;
				}
		}
        
		t_80=width/SPI_TX_PIXELS_LIMIT;
		m_80=width%SPI_TX_PIXELS_LIMIT;
		for (i=0;i<height;i++) {
			if (t_80>0) {
				for (j=0;j<t_80;j++) {
                    spi_write(data, SPI_TX_BYTES_LIMIT);
				}
			}
			if (m_80>0) {
				j=m_80*3;
                spi_write(data, j);
			}
		}

/*
    // Duff's device algorithm for optimizing loop.
    uint32_t i = (height * width + 7) / 8;

//lint -save -e525 -e616 -e646 
    switch ((height * width) % 8) {
        case 0:
            do {
                spi_write(data, sizeof(data));
        case 7:
                spi_write(data, sizeof(data));
        case 6:
                spi_write(data, sizeof(data));
        case 5:
                spi_write(data, sizeof(data));
        case 4:
                spi_write(data, sizeof(data));
        case 3:
                spi_write(data, sizeof(data));
        case 2:
                spi_write(data, sizeof(data));
        case 1:
                spi_write(data, sizeof(data));
            } while (--i > 0);
        default:
            break;
    }
//lint -restore */

    nrf_gpio_pin_clear(ST7789_DC_PIN);
}

static void st7789_rect_draw_data(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t *data)
{
    uint8_t _data[SPI_TX_BYTES_LIMIT];
    uint32_t bytes = 0;
    uint8_t size = 0;

    set_addr_window(x, y, x + width - 1, y + height - 1);
    nrf_gpio_pin_set(ST7789_DC_PIN);

    for (int i = 0; i < width*height;)
    {
        size = i + SPI_TX_PIXELS_LIMIT < width*height ? SPI_TX_PIXELS_LIMIT : width*height - i;
        memcpy(_data, data + bytes, size * 3);
        spi_write(_data, size * 3);
        bytes += size * 3;
        i += size;
    }

    nrf_gpio_pin_clear(ST7789_DC_PIN);
}

static void st7789_dummy_display(void)
{
    /* No implementation needed. */
}

static void st7789_rotation_set(nrf_lcd_rotation_t rotation)
{
    write_command(ST7789_MADCTL);
    switch (rotation) {
        case NRF_LCD_ROTATE_0:
            if (m_st7789.tab_color == INITR_BLACKTAB)
            {
                write_data(ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_RGB);
            }
            else
            {
                write_data(ST7789_MADCTL_MX | ST7789_MADCTL_MY | ST7789_MADCTL_BGR);
            }
            break;
        case NRF_LCD_ROTATE_90:
            if (m_st7789.tab_color == INITR_BLACKTAB)
            {
                write_data(ST7789_MADCTL_MY | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
            }
            else
            {
                write_data(ST7789_MADCTL_MY | ST7789_MADCTL_MV | ST7789_MADCTL_BGR);
            }
            break;
        case NRF_LCD_ROTATE_180:
            if (m_st7789.tab_color == INITR_BLACKTAB)
            {
                write_data(ST7789_MADCTL_RGB);
            }
            else
            {
                write_data(ST7789_MADCTL_BGR);
            }
            break;
        case NRF_LCD_ROTATE_270:
            if (m_st7789.tab_color == INITR_BLACKTAB)
            {
                write_data(ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_RGB);
            }
            else
            {
                write_data(ST7789_MADCTL_MX | ST7789_MADCTL_MV | ST7789_MADCTL_BGR);
            }
            break;
        default:
            break;
    }
}


static void st7789_display_invert(bool invert)
{
    write_command(invert ? ST7789_INVON : ST7789_INVOFF);
}

void st7789_clear_screen(uint16_t width,uint16_t height)
{
    set_addr_window(0, 0, width - 1,  height - 1);
    //color = RGB2BGR(color);
    uint8_t data[240] ;
		memset(data,0,sizeof(data));
    nrf_gpio_pin_set(ST7789_DC_PIN);
		for (uint16_t i=0;i<height;i++)
		{
			for (uint16_t j=0;j<4;j++) {
                spi_write(data,240);
			}
		}
/*lint -restore */
    nrf_gpio_pin_clear(ST7789_DC_PIN);
}

static lcd_cb_t st7789_cb = {
    .height = ST7789_HEIGHT,
    .width = ST7789_WIDTH
};

const nrf_lcd_t nrf_lcd_st7789 = {
    .lcd_init = st7789_init,
    .lcd_uninit = st7789_uninit,
    .lcd_pixel_draw = st7789_pixel_draw,
    .lcd_rect_draw = st7789_rect_draw,
    .lcd_rect_draw_data = st7789_rect_draw_data,
    .lcd_display = st7789_dummy_display,
    .lcd_rotation_set = st7789_rotation_set,
    .lcd_display_invert = st7789_display_invert,
    .p_lcd_cb = &st7789_cb,
	.lcd_clear_screen = st7789_clear_screen
};

#endif // NRF_MODULE_ENABLED(ST7789)
