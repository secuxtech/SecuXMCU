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

#if NRF_MODULE_ENABLED(NRF_GFX)

#include "nrf_gfx.h"
#include <stdlib.h>
#include "app_util_platform.h"
#include "nrf_assert.h"

#define NRF_LOG_MODULE_NAME gfx
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();

void nrf_clear_screen(nrf_lcd_t const * p_instance)
{
    uint16_t lcd_width = nrf_gfx_width_get(p_instance);
    uint16_t lcd_height = nrf_gfx_height_get(p_instance);
    p_instance->lcd_clear_screen(lcd_width, lcd_height);
}

static inline void pixel_draw(nrf_lcd_t const * p_instance,
                              uint16_t x,
                              uint16_t y,
                              uint32_t color)
{
    uint16_t lcd_width = nrf_gfx_width_get(p_instance);
    uint16_t lcd_height = nrf_gfx_height_get(p_instance);

    if ((x >= lcd_width) || (y >= lcd_height))
    {
        return;
    }

    p_instance->lcd_pixel_draw(x, y, color);
}

static void rect_draw(nrf_lcd_t const * p_instance,
                      uint16_t x,
                      uint16_t y,
                      uint16_t width,
                      uint16_t height,
                      uint32_t color)
{
    uint16_t lcd_width = nrf_gfx_width_get(p_instance);
    uint16_t lcd_height = nrf_gfx_height_get(p_instance);

    if ((x >= lcd_width) || (y >= lcd_height))
    {
        return;
    }

    if (width > (lcd_width - x))
    {
        width = lcd_width - x;
    }

    if (height > (lcd_height - y))
    {
        height = lcd_height - y;
    }

    p_instance->lcd_rect_draw(x, y, width, height, color);
}

static void rect_draw_data(nrf_lcd_t const * p_instance,
                      uint16_t x,
                      uint16_t y,
                      uint16_t width,
                      uint16_t height,
                      uint8_t *data)
{
    uint16_t lcd_width = nrf_gfx_width_get(p_instance);
    uint16_t lcd_height = nrf_gfx_height_get(p_instance);

    if ((x >= lcd_width) || (y >= lcd_height))
    {
        return;
    }

    if (width > (lcd_width - x))
    {
        width = lcd_width - x;
    }

    if (height > (lcd_height - y))
    {
        height = lcd_height - y;
    }

    p_instance->lcd_rect_draw_data(x, y, width, height, data);
}

static void line_draw(nrf_lcd_t const * p_instance,
                      uint16_t x_0,
                      uint16_t y_0,
                      uint16_t x_1,
                      int16_t y_1,
                      uint32_t color)
{
    uint16_t x = x_0;
    uint16_t y = y_0;
    int16_t d;
    int16_t d_1;
    int16_t d_2;
    int16_t ai;
    int16_t bi;
    int16_t xi = (x_0 < x_1) ? 1 : (-1);
    int16_t yi = (y_0 < y_1) ? 1 : (-1);
    bool swapped = false;

    d_1 = abs(x_1 - x_0);
    d_2 = abs(y_1 - y_0);

    pixel_draw(p_instance, x, y, color);

    if (d_1 < d_2)
    {
        d_1 = d_1 ^ d_2;
        d_2 = d_1 ^ d_2;
        d_1 = d_2 ^ d_1;
        swapped = true;
    }

    ai = (d_2 - d_1) * 2;
    bi = d_2 * 2;
    d = bi - d_1;

    while ((y != y_1) || (x != x_1))
    {
        if (d >= 0)
        {
            x += xi;
            y += yi;
            d += ai;
        }
        else
        {
            d += bi;
            if (swapped)
            {
                y += yi;
            }
            else
            {
                x += xi;
            }
        }
        pixel_draw(p_instance, x, y, color);
    }
}

static void write_character(nrf_lcd_t const * p_instance,
                            nrf_gfx_font_desc_t const * p_font,
                            uint8_t character,
                            uint16_t * p_x,
                            uint16_t y,
                            uint32_t font_color)
{
    uint8_t char_idx = character - p_font->startChar;
    uint16_t bytes_in_line = CEIL_DIV(p_font->charInfo[char_idx].widthBits, 8);

    if (character == ' ')
    {
        *p_x += p_font->height / 2;
        return;
    }

    for (uint16_t i = 0; i < p_font->height; i++)
    {
        for (uint16_t j = 0; j < bytes_in_line; j++)
        {
            for (uint8_t k = 0; k < 8; k++)
            {
                if ((1 << (7 - k)) &
                    p_font->data[p_font->charInfo[char_idx].offset + i * bytes_in_line + j])
                {
                    pixel_draw(p_instance, *p_x + j * 8 + k, y + i, font_color);
                }
            }
        }
    }

    *p_x += p_font->charInfo[char_idx].widthBits + p_font->spacePixels;
}

ret_code_t nrf_gfx_init(nrf_lcd_t const * p_instance)
{
    ASSERT(p_instance != NULL);
    ASSERT(p_instance->p_lcd_cb->state == NRFX_DRV_STATE_UNINITIALIZED);
    ASSERT(p_instance->lcd_init != NULL);
    ASSERT(p_instance->lcd_uninit != NULL);
    ASSERT(p_instance->lcd_pixel_draw != NULL);
    ASSERT(p_instance->lcd_rect_draw != NULL);
    ASSERT(p_instance->lcd_rect_draw_data != NULL);
    ASSERT(p_instance->lcd_display != NULL);
    ASSERT(p_instance->lcd_rotation_set != NULL);
    ASSERT(p_instance->lcd_display_invert != NULL);
    ASSERT(p_instance->p_lcd_cb != NULL);
    ASSERT(p_instance->lcd_clear_screen != NULL);

    ret_code_t err_code;

    err_code = p_instance->lcd_init();

    if (err_code == NRF_SUCCESS)
    {
        p_instance->p_lcd_cb->state = NRFX_DRV_STATE_INITIALIZED;
    }

    return err_code;
}

void nrf_gfx_uninit(nrf_lcd_t const * p_instance)
{
    ASSERT(p_instance != NULL);
    ASSERT(p_instance->p_lcd_cb->state != NRFX_DRV_STATE_UNINITIALIZED);

    p_instance->p_lcd_cb->state = NRFX_DRV_STATE_UNINITIALIZED;

    p_instance->lcd_uninit();
}

void nrf_gfx_point_draw(nrf_lcd_t const * p_instance,
                        nrf_gfx_point_t const * p_point,
                        uint32_t color)
{
    ASSERT(p_instance != NULL);
    ASSERT(p_instance->p_lcd_cb->state != NRFX_DRV_STATE_UNINITIALIZED);
    ASSERT(p_point != NULL);

    pixel_draw(p_instance, p_point->x, p_point->y, color);
}

ret_code_t nrf_gfx_line_draw(nrf_lcd_t const * p_instance,
                             nrf_gfx_line_t const * p_line,
                             uint32_t color)
{
    ASSERT(p_instance != NULL);
    ASSERT(p_instance->p_lcd_cb->state != NRFX_DRV_STATE_UNINITIALIZED);
    ASSERT(p_line != NULL);

    uint16_t x_thick = 0;
    uint16_t y_thick = 0;

    if (((p_line->x_start > nrf_gfx_width_get(p_instance)) &&
        (p_line->x_end > nrf_gfx_height_get(p_instance))) ||
        ((p_line->y_start > nrf_gfx_width_get(p_instance)) &&
        (p_line->y_end > nrf_gfx_height_get(p_instance))))
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    if (abs(p_line->x_start - p_line->x_end) > abs(p_line->y_start - p_line->y_end))
    {
        y_thick = p_line->thickness;
    }
    else
    {
        x_thick = p_line->thickness;
    }

    if ((p_line->x_start == p_line->x_end) || (p_line->y_start == p_line->y_end))
    {
        rect_draw(p_instance,
                  p_line->x_start,
                  p_line->y_start,
                  abs(p_line->x_end - p_line->x_start) + x_thick,
                  abs(p_line->y_end - p_line->y_start) + y_thick,
                  color);
    }
    else
    {
        if (x_thick > 0)
        {
            for (uint16_t i = 0; i < p_line->thickness; i++)
            {
                line_draw(p_instance,
                          p_line->x_start + i,
                          p_line->y_start,
                          p_line->x_end + i,
                          p_line->y_end,
                          color);
            }
        }
        else if (y_thick > 0)
        {
            for (uint16_t i = 0; i < p_line->thickness; i++)
            {
                line_draw(p_instance,
                          p_line->x_start,
                          p_line->y_start + i,
                          p_line->x_end,
                          p_line->y_end + i,
                          color);
            }
        }
        else
        {
            line_draw(p_instance,
                      p_line->x_start + x_thick,
                      p_line->y_start + y_thick,
                      p_line->x_end + x_thick,
                      p_line->y_end + y_thick,
                      color);
        }
    }

    return NRF_SUCCESS;
}

ret_code_t nrf_gfx_circle_draw(nrf_lcd_t const * p_instance,
                               nrf_gfx_circle_t const * p_circle,
                               uint32_t color,
                               bool fill)
{
    ASSERT(p_instance != NULL);
    ASSERT(p_instance->p_lcd_cb->state != NRFX_DRV_STATE_UNINITIALIZED);
    ASSERT(p_circle != NULL);

    int16_t y = 0;
    int16_t err = 0;
    int16_t x = p_circle->r;

    if ((p_circle->x - p_circle->r > nrf_gfx_width_get(p_instance))     ||
        (p_circle->y - p_circle->r > nrf_gfx_height_get(p_instance)))
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    while (x >= y)
    {
        if (fill)
        {
            if ((-y + p_circle->x < 0) || (-x + p_circle->x < 0))
            {
                rect_draw(p_instance, 0, (-x + p_circle->y), (y + p_circle->x + 1), 1, color);
                rect_draw(p_instance, 0, (-y + p_circle->y), (x + p_circle->x + 1), 1, color);
                rect_draw(p_instance, 0, (y + p_circle->y), (x + p_circle->x + 1), 1, color);
                rect_draw(p_instance, 0, (x + p_circle->y), (y + p_circle->x + 1), 1, color);
            }
            else
            {
                rect_draw(p_instance, (-y + p_circle->x), (-x + p_circle->y), (2 * y + 1), 1, color);
                rect_draw(p_instance, (-x + p_circle->x), (-y + p_circle->y), (2 * x + 1), 1, color);
                rect_draw(p_instance, (-x + p_circle->x), (y + p_circle->y), (2 * x + 1), 1, color);
                rect_draw(p_instance, (-y + p_circle->x), (x + p_circle->y), (2 * y + 1), 1, color);
            }
        }
        else
        {
            pixel_draw(p_instance, (y + p_circle->x), (x + p_circle->y), color);
            pixel_draw(p_instance, (-y + p_circle->x), (x + p_circle->y), color);
            pixel_draw(p_instance, (x + p_circle->x), (y + p_circle->y), color);
            pixel_draw(p_instance, (-x + p_circle->x), (y + p_circle->y), color);
            pixel_draw(p_instance, (-y + p_circle->x), (-x + p_circle->y), color);
            pixel_draw(p_instance, (y + p_circle->x), (-x + p_circle->y), color);
            pixel_draw(p_instance, (-x + p_circle->x), (-y + p_circle->y), color);
            pixel_draw(p_instance, (x + p_circle->x), (-y + p_circle->y), color);
        }

        if (err <= 0)
        {
            y += 1;
            err += 2 * y + 1;
        }
        if (err > 0)
        {
            x -= 1;
            err -= 2 * x + 1;
        }
    }

    return NRF_SUCCESS;
}

ret_code_t nrf_gfx_rect_draw(nrf_lcd_t const * p_instance,
                             nrf_gfx_rect_t const * p_rect,
                             uint16_t thickness,
                             uint32_t color,
                             bool fill)
{
    ASSERT(p_instance != NULL);
    ASSERT(p_instance->p_lcd_cb->state != NRFX_DRV_STATE_UNINITIALIZED);
    ASSERT(p_rect != NULL);

    uint16_t rect_width = p_rect->width - thickness;
    uint16_t rect_height = p_rect->height - thickness;

    if ((p_rect->width == 1)                ||
        (p_rect->height == 1)               ||
        (thickness * 2 > p_rect->width)     ||
        (thickness * 2 > p_rect->height)    ||
        ((p_rect->x > nrf_gfx_width_get(p_instance)) &&
        (p_rect->y > nrf_gfx_height_get(p_instance))))
    {
        return NRF_ERROR_INVALID_PARAM;
    }


    if (fill)
    {
        rect_draw(p_instance,
                  p_rect->x,
                  p_rect->y,
                  p_rect->width,
                  p_rect->height,
                  color);
    }
    else
    {
        nrf_gfx_line_t line;

        // Top horizontal line.
        line.x_start = p_rect->x;
        line.y_start = p_rect->y;
        line.x_end = p_rect->x + p_rect->width;
        line.y_end = p_rect->y;
        line.thickness = thickness;
        (void)nrf_gfx_line_draw(p_instance, &line, color);
        // Bottom horizontal line.
        line.x_start = p_rect->x;
        line.y_start = p_rect->y + rect_height;
        line.x_end = p_rect->x + p_rect->width;
        line.y_end = p_rect->y + rect_height;
        (void)nrf_gfx_line_draw(p_instance, &line, color);
        // Left vertical line.
        line.x_start = p_rect->x;
        line.y_start = p_rect->y + thickness;
        line.x_end = p_rect->x;
        line.y_end = p_rect->y + rect_height;
        (void)nrf_gfx_line_draw(p_instance, &line, color);
        // Right vertical line.
        line.x_start = p_rect->x + rect_width;
        line.y_start = p_rect->y + thickness;
        line.x_end = p_rect->x + rect_width;
        line.y_end = p_rect->y + rect_height;
        (void)nrf_gfx_line_draw(p_instance, &line, color);
    }

    return NRF_SUCCESS;
}

void nrf_gfx_screen_fill(nrf_lcd_t const * p_instance, uint32_t color)
{
    rect_draw(p_instance, 0, 0, nrf_gfx_width_get(p_instance), nrf_gfx_height_get(p_instance), color);
}

ret_code_t nrf_gfx_bmp565_draw(nrf_lcd_t const * p_instance,
                               nrf_gfx_rect_t const * p_rect,
                               uint16_t const * img_buf)
{
    ASSERT(p_instance != NULL);
    ASSERT(p_instance->p_lcd_cb->state != NRFX_DRV_STATE_UNINITIALIZED);
    ASSERT(p_rect != NULL);
    ASSERT(img_buf != NULL);

    if ((p_rect->x > nrf_gfx_width_get(p_instance)) || (p_rect->y > nrf_gfx_height_get(p_instance)))
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    size_t idx;
    uint16_t pixel;
    uint8_t padding = p_rect->width % 2;

    for (int32_t i = 0; i < p_rect->height; i++)
    {
        for (uint32_t j = 0; j < p_rect->width; j++)
        {
            idx = (uint32_t)((p_rect->height - i - 1) * (p_rect->width + padding) + j);

            pixel = (img_buf[idx] >> 8) | (img_buf[idx] << 8);

            pixel_draw(p_instance, p_rect->x + j, p_rect->y + i, pixel);
        }
    }

    return NRF_SUCCESS;
}

void nrf_gfx_background_set(nrf_lcd_t const * p_instance, uint16_t const * img_buf)
{
    ASSERT(p_instance != NULL);
    ASSERT(p_instance->p_lcd_cb->state != NRFX_DRV_STATE_UNINITIALIZED);
    ASSERT(img_buf != NULL);

    const nrf_gfx_rect_t rectangle =
    {
        .x = 0,
        .y = 0,
        .width = nrf_gfx_width_get(p_instance),
        .height = nrf_gfx_height_get(p_instance)
    };

    (void)nrf_gfx_bmp565_draw(p_instance, &rectangle, img_buf);
}

void nrf_gfx_display(nrf_lcd_t const * p_instance)
{
    ASSERT(p_instance != NULL);

    p_instance->lcd_display();
}

void nrf_gfx_rotation_set(nrf_lcd_t const * p_instance, nrf_lcd_rotation_t rotation)
{
    ASSERT(p_instance != NULL);
    ASSERT(p_instance->p_lcd_cb->state != NRFX_DRV_STATE_UNINITIALIZED);

    bool rotated = (bool)(p_instance->p_lcd_cb->rotation % 2);

    uint16_t height = !rotated ? nrf_gfx_height_get(p_instance) :
                                 nrf_gfx_width_get(p_instance);
    uint16_t width = !rotated ? nrf_gfx_width_get(p_instance) :
                                nrf_gfx_height_get(p_instance);

    p_instance->p_lcd_cb->rotation = rotation;

    switch (rotation) {
        case NRF_LCD_ROTATE_0:
            p_instance->p_lcd_cb->height = height;
            p_instance->p_lcd_cb->width = width;
            break;
        case NRF_LCD_ROTATE_90:
            p_instance->p_lcd_cb->height = width;
            p_instance->p_lcd_cb->width = height;
            break;
        case NRF_LCD_ROTATE_180:
            p_instance->p_lcd_cb->height = height;
            p_instance->p_lcd_cb->width = width;
            break;
        case NRF_LCD_ROTATE_270:
            p_instance->p_lcd_cb->height = width;
            p_instance->p_lcd_cb->width = height;
            break;
        default:
            break;
    }

    p_instance->lcd_rotation_set(rotation);
}

void nrf_gfx_invert(nrf_lcd_t const * p_instance, bool invert)
{
    ASSERT(p_instance != NULL);

    p_instance->lcd_display_invert(invert);
}

ret_code_t nrf_gfx_print(nrf_lcd_t const * p_instance,
                         nrf_gfx_point_t const * p_point,
                         uint32_t font_color,
                         const char * string,
                         const nrf_gfx_font_desc_t * p_font,
                         bool wrap)
{
    ASSERT(p_instance != NULL);
    ASSERT(p_instance->p_lcd_cb->state != NRFX_DRV_STATE_UNINITIALIZED);
    ASSERT(p_point != NULL);
    ASSERT(string != NULL);
    ASSERT(p_font != NULL);

    uint16_t x = p_point->x;
    uint16_t y = p_point->y;

    if (y > (nrf_gfx_height_get(p_instance) - p_font->height))
    {
        // Not enough space to write even single char.
        return NRF_ERROR_INVALID_PARAM;
    }

    for (size_t i = 0; string[i] != '\0' ; i++)
    {
        if (string[i] == '\t')
        {
            x = p_point->x;
            y += p_font->height;
        }
        else if (string[i] == '\n')
        {
            x = p_point->x;
            y += p_font->height + p_font->height / 8;
        }
        else
        {
            uint8_t char_idx = string[i] - p_font->startChar;
            uint16_t char_width = (string[i] == ' ') ? 
                                  (p_font->height / 2) :
                                  (p_font->charInfo[char_idx].widthBits+p_font->spacePixels);
        
            if ((x + char_width) > (nrf_gfx_width_get(p_instance)))
            {
                if (wrap)
                {
                    x = p_point->x;
                    y += p_font->height + p_font->height / 10;
                }
                else
                {
                    break;
                }
                if ((y + p_font->height) > nrf_gfx_height_get(p_instance))
                {
                    break;
                }
            }
            write_character(p_instance, p_font, (uint8_t)string[i], &x, y, font_color);
        }
    }
    return NRF_SUCCESS;
}

ret_code_t process_string_in_region(nrf_lcd_t const * p_instance,
                                    uint16_t point_x, uint16_t point_y,
                                    uint16_t width,
                                    uint16_t height,
                                    const char * string,
                                    const nrf_gfx_font_desc_t * p_font,
                                    uint32_t font_color,
                                    bool wrap,
                                    int *page,
                                    bool is_drawing)
{
    ASSERT(p_instance != NULL);
    ASSERT(p_instance->p_lcd_cb->state != NRFX_DRV_STATE_UNINITIALIZED);
    ASSERT(string != NULL);
    ASSERT(p_font != NULL);

    uint16_t x = point_x;
    uint16_t y = point_y;
    int _page = 1;

    uint16_t right_boundary = 0, bottom_boundary = 0;
    bool is_word_checking_needed = false, is_new_line = true;
    
    // estimate right boundary
    if ((x + width) < nrf_gfx_width_get(p_instance))
        right_boundary = (x + width);
    else
        right_boundary = nrf_gfx_width_get(p_instance);
    
    // estimate bottom boundary
    if ((y + height) < nrf_gfx_height_get(p_instance))
        bottom_boundary = (y + height);
    else
        bottom_boundary = nrf_gfx_height_get(p_instance);
    
    if (y > (nrf_gfx_height_get(p_instance) - p_font->height))
    {
        // Not enough space to write even single char.
        return NRF_ERROR_INVALID_PARAM;
    }

    for (size_t i = 0; string[i] != '\0' ; i++)
    {
        if (string[i] == '\n')
        {
            x = point_x;
            y += p_font->height + p_font->height / 10;
            is_new_line = true;
            is_word_checking_needed = false;
        }
        else if (string[i] == ' ' && true == is_new_line)
        {
            // skip the ' ' while the new line
        }
        else
        {
            // do not endline while already at the start
            if (is_new_line == false)
            {
                // start a new line if the word length exceed the region
                if (string[i] != ' ' && string[i] != '\t')
                {
                    if (is_word_checking_needed == true)
                    {
                        uint16_t word_width = 0;
                        for (int j = i; 
                            string[j] != '\0' && string[j] != '\t' && string[j] != '\n' && 
                            string[j] != ' '; 
                            j++)
                        {
                            uint8_t _char_idx = string[j] - p_font->startChar;
                            uint16_t _char_width = (p_font->charInfo[_char_idx].widthBits + p_font->spacePixels);
                            
                            word_width += _char_width;
                        }
                        
                        if ((x + word_width) > right_boundary)
                        {
                            if (wrap)
                            {
                                x = point_x;
                                y += p_font->height + p_font->height / 10;
                                is_new_line = true;
                            }
                            else
                            {
                                break;
                            }
                        }
                        is_word_checking_needed = false;
                    }
                }
                else
                    is_word_checking_needed = true;
            }

            // character processing
            uint8_t char_idx = string[i] - p_font->startChar;
            uint16_t char_width = 0;

            if (string[i] == ' ')
                char_width = p_font->height / 2;
            else if (string[i] == '\t')
                char_width = p_font->height;
            else
                char_width = p_font->charInfo[char_idx].widthBits + p_font->spacePixels;

            if (false == is_new_line)
            {
                // start a new line if the char length exceed the regior width
                // follow the word length checking, will only process ' ' here
                if ((x + char_width) > right_boundary)
                {
                    if (wrap)
                    {
                        x = point_x;
                        y += p_font->height + p_font->height / 10;
                        is_new_line = true;
                        is_word_checking_needed = false;
                    }
                    else
                    {
                        break;
                    }
                }
            }
            
            if ((y + p_font->height) > bottom_boundary && true == is_new_line)
            {
                if ((_page >= *page) && (true == is_drawing))
                    break;
                else
                {
                    _page++;
                    x = point_x;
                    y = point_y;
                }
            }

            if ((_page == *page) && (true == is_drawing) && (string[i] != ' ') && (string[i] != '\t'))
                write_character(p_instance, p_font, (uint8_t)string[i], &x, y, font_color);
            else
                x += char_width;

            is_new_line = false;
        }
    }
    *page = _page;
    return NRF_SUCCESS;
}

ret_code_t nrf_gfx_string_in_region(nrf_lcd_t const * p_instance,
                                     nrf_gfx_point_t const * p_point,
                                     uint16_t width,
                                     uint16_t height,
                                     const char * string,
                                     const nrf_gfx_font_desc_t * p_font,
                                     uint32_t font_color,
                                     bool wrap,
                                     int page)
{
    ASSERT(p_point != NULL);

    return process_string_in_region(p_instance, p_point->x, p_point->y, width, height,
                                    string, p_font, font_color, wrap, &page, true);
}

uint16_t nrf_gfx_height_get(nrf_lcd_t const * p_instance)
{
    ASSERT(p_instance != NULL);

    return p_instance->p_lcd_cb->height;
}

uint16_t nrf_gfx_width_get(nrf_lcd_t const * p_instance)
{
    ASSERT(p_instance != NULL);

    return p_instance->p_lcd_cb->width;
}

ret_code_t gfx_arc_rect_draw(nrf_lcd_t const * p_instance,
                             nrf_gfx_rect_t const * p_rect,
                             uint16_t radius,
                             uint32_t color, bool fill)
{
	ASSERT(p_instance != NULL);
    ASSERT(p_instance->p_lcd_cb->state != NRFX_DRV_STATE_UNINITIALIZED);
    ASSERT(p_rect != NULL);

    uint16_t rect_width = p_rect->width - 2 * radius;
    uint16_t rect_height = p_rect->height - 2 * radius;

    if ((rect_width <= 0)                ||
        (rect_height <= 0)               ||					
        (p_rect->x > nrf_gfx_width_get(p_instance)) ||
        (p_rect->y > nrf_gfx_height_get(p_instance)))
    {
        return NRF_ERROR_INVALID_PARAM;
    }
	
	nrf_gfx_point_t c1 = NRF_GFX_POINT(p_rect->x + radius, p_rect->y + radius);
	nrf_gfx_point_t c2 = NRF_GFX_POINT(p_rect->x + p_rect->width - radius - 1, p_rect->y + radius);
	nrf_gfx_point_t c3 = NRF_GFX_POINT(p_rect->x + radius, p_rect->y + p_rect->height - radius - 1);				
	nrf_gfx_point_t c4 = NRF_GFX_POINT(p_rect->x + p_rect->width - radius - 1, p_rect->y + p_rect->height - radius - 1);
	
	if (fill)
    {
		// middle part
		if(rect_height - 2 > 0)
			rect_draw(p_instance,
					  p_rect->x,
					  p_rect->y + radius + 1,
					  p_rect->width,
					  rect_height - 2,
					  color);
		
		// the rest part
		int16_t y = 0; 
		int16_t err = 0;
		int16_t x = radius;

		while (x >= y)
		{
			// top part
			rect_draw(p_instance, (-y + c1.x), (-x + c1.y), (2 * y + rect_width), 1, color);
			rect_draw(p_instance, (-x + c1.x), (-y + c1.y), (2 * x + rect_width), 1, color);
			// bottom part
			rect_draw(p_instance, (-x + c3.x), (y + c3.y), (2 * x + rect_width), 1, color);
			rect_draw(p_instance, (-y + c3.x), (x + c3.y), (2 * y + rect_width), 1, color);
			
			if (err <= 0)
			{
				y += 1;
				err += 2 * y + 1;
			}
			if (err > 0)
			{
				x -= 1;
				err -= 2 * x + 1;
			}
		}

    } 
	else
	{
		line_draw(p_instance, c1.x + 1, p_rect->y, c2.x - 1, p_rect->y, color);
		line_draw(p_instance, p_rect->x, c1.y + 1, p_rect->x, c3.y - 1, color);
		line_draw(p_instance, p_rect->x + p_rect->width - 1, c2.y + 1, p_rect->x + p_rect->width - 1, c4.y - 1, color);
		line_draw(p_instance, c3.x + 1, p_rect->y + p_rect->height - 1, c4.x - 1, p_rect->y + p_rect->height - 1, color);
		int16_t y = 0; 
		int16_t err = 0;
		int16_t x = radius;

		while (x >= y)
		{
			//top part(left)	
			pixel_draw(p_instance, (-x + c1.x), (-y + c1.y), color);
			pixel_draw(p_instance, (-y + c1.x), (-x + c1.y), color);
            //top part(right)
			pixel_draw(p_instance, (x + c2.x), (-y + c2.y), color);
			pixel_draw(p_instance, (y + c2.x), (-x + c2.y), color);
            
            
			//bottom part(left)
			pixel_draw(p_instance, (-x + c3.x), (y + c3.y), color);
			pixel_draw(p_instance, (-y + c3.x), (x + c3.y), color);
			//bottom part(right)
			pixel_draw(p_instance, (y + c4.x), (x + c4.y), color);
            pixel_draw(p_instance, (x + c4.x), (y + c4.y), color);
            
           
			
			if (err <= 0)
			{
				y += 1;
				err += 2 * y + 1;
			}
			if (err > 0)
			{
				x -= 1;
				err -= 2 * x + 1;
			}
		}
		
		
	}
	return NRF_SUCCESS;
}								
ret_code_t nrf_gfx_bmp666_draw(nrf_lcd_t const * p_instance,
                               nrf_gfx_rect_t const * p_rect,
                               uint8_t const * img_buf)
{
    ASSERT(p_instance != NULL);
    ASSERT(p_instance->p_lcd_cb->state != NRFX_DRV_STATE_UNINITIALIZED);
    ASSERT(p_rect != NULL);
    ASSERT(img_buf != NULL);

    if ((p_rect->x > nrf_gfx_width_get(p_instance)) || (p_rect->y > nrf_gfx_height_get(p_instance)))
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    uint32_t idx;
    uint32_t pixel;
    uint8_t padding = p_rect->width % 2;

    for (int32_t i = 0; i < p_rect->height; i++)
    {
        for (uint32_t j = 0; j < p_rect->width; j++)
        {
            idx = (uint32_t)((p_rect->height - i - 1) * (p_rect->width + padding) + j) * 3;
			pixel = 0;
            pixel += img_buf[idx];
			pixel <<= 8;
			pixel += img_buf[idx + 1];
			pixel <<= 8;
			pixel += img_buf[idx + 2];
            pixel_draw(p_instance, p_rect->x + j, p_rect->y + i, pixel);
        }
    }

    return NRF_SUCCESS;
}

ret_code_t nrf_gfx_bmp888_draw(nrf_lcd_t const * p_instance,
                               nrf_gfx_rect_t const * p_rect,
                               uint8_t *img_buf)
{
    ASSERT(p_instance != NULL);
    ASSERT(p_instance->p_lcd_cb->state != NRFX_DRV_STATE_UNINITIALIZED);
    ASSERT(p_rect != NULL);
    ASSERT(img_buf != NULL);

    if ((p_rect->x > nrf_gfx_width_get(p_instance)) || (p_rect->y > nrf_gfx_height_get(p_instance)))
    {
        return NRF_ERROR_INVALID_PARAM;
    }

    rect_draw_data(p_instance, p_rect->x, p_rect->y, p_rect->width, p_rect->height, img_buf);

    return NRF_SUCCESS;
}
#endif //NRF_MODULE_ENABLED(NRF_GFX)

