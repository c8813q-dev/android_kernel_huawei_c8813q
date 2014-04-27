/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "hw_lcd_common.h"

#define LCD_DEVICE_NAME "mipi_cmd_otm9608a_qhd"

static lcd_panel_type lcd_panel_qhd = LCD_NONE;


static struct mipi_dsi_phy_ctrl dsi_cmd_mode_phy_db_otm9608a_qhd = {
    /* DSI Bit Clock at 419 MHz, 2 lane, RGB888 */
    /* regulator */
    {0x03, 0x01, 0x01, 0x00, 0x00}, 
    /* timing */ 
    {0x79, 0x2E, 0x10, 0x00, 0x3E, 0x45, 0x15, 0x32, 
        0x14, 0x3, 0x04, 0x00}, 
    /* phy ctrl */ 
    {0x7f, 0x00, 0x00, 0x00}, 
    /* strength */ 
    {0xbb, 0x02, 0x06, 0x00}, 
    /* pll control */ 
    {0x01, 0x9e, 0x31, 0xd2, 0x00, 0x40, 0x37, 0x62, 
        0x01, 0x0f, 0x07, 
        0x05, 0x14, 0x03, 0x0, 0x0, 0x0, 0x20, 0x0, 0x02, 0x0}, 
};

static struct dsi_buf otm9608a_tx_buf;
#if LCD_OTM9608A_TIANMA_ESD_SIGN
#define TIANMA_OTM9608A_PANEL_ALIVE    (1)
#define TIANMA_OTM9608A_PANEL_DEAD     (-1)

static struct dsi_buf otm9608a_rx_buf;
#endif
static struct sequence * otm9608a_lcd_init_table_debug = NULL;


const struct sequence otm9608a_qhd_standby_enter_table[]= 
{
	/*close Vsync singal,when lcd sleep in*/

	{0x00028,MIPI_DCS_COMMAND,0}, //28h
	{0x00010,MIPI_DCS_COMMAND,20},
	{0x00029,MIPI_TYPE_END,150}, // add new command for 
};
const struct sequence otm9608a_qhd_standby_exit_table[] =
{
	{0x00011,MIPI_DCS_COMMAND,0},	//sleep out 
	{0x00029,MIPI_DCS_COMMAND,150}, //Display ON
	{0x00000,MIPI_TYPE_END,20}, //end flag
};


struct sequence otm9608a_qhd_write_cabc_brightness_table[]= 
{
	/* solve losing control of the backlight */
	{0x00051,MIPI_DCS_COMMAND,0},
	{0x000ff,TYPE_PARAMETER,0},
	{0x00,MIPI_TYPE_END,0},
};

#if LCD_OTM9608A_TIANMA_ESD_SIGN
/* re-initial code of tianma otm8009a */
static struct sequence otm9608a_tianma_lcd_init_table[] =
{
	//ff00h Command 2 enable: Read and write,only access when Orise mode enable
	//{0x00000,MIPI_DCS_COMMAND,200},
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x000FF,MIPI_GEN_COMMAND,0},
	{0x00096,TYPE_PARAMETER,0},
	{0x00008,TYPE_PARAMETER,0},
	{0x00001,TYPE_PARAMETER,0},

	//ff80h Orise CMD enable
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x00080,TYPE_PARAMETER,0},
	{0x000FF,MIPI_GEN_COMMAND,0},
	{0x00096,TYPE_PARAMETER,0},
	{0x00008,TYPE_PARAMETER,0},
	// only into Orise Mode, the OTM9608 registers can be setted


	//a000h OTP select region()
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x000a0,MIPI_GEN_COMMAND,0},
	{0x00000,TYPE_PARAMETER,0},

	//b380h Command Set Option Parameter
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x00080,TYPE_PARAMETER,0},
	{0x000b3,MIPI_GEN_COMMAND,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00020,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},

	//b3c0h Ram power control,SRAM Setting
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x000c0,TYPE_PARAMETER,0},
	{0x000b3,MIPI_GEN_COMMAND,0},
	{0x00009,TYPE_PARAMETER,0},
	/*clear gram*/
	{0x00010,TYPE_PARAMETER,0},

	//c080h TCON Setting Parameter
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x00080,TYPE_PARAMETER,0},
	{0x000C0,MIPI_GEN_COMMAND,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00048,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x0000f,TYPE_PARAMETER,0},
	{0x00011,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00048,TYPE_PARAMETER,0},
	{0x0000f,TYPE_PARAMETER,0},
	{0x00011,TYPE_PARAMETER,0},

	//c092h PTSP1:Panel Timing Setting Parameter1
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x00092,TYPE_PARAMETER,0},
	{0x000C0,MIPI_GEN_COMMAND,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00010,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00013,TYPE_PARAMETER,0},

	//c0a2h PTSP3:Panel Timing Setting Parameter3
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x000a2,TYPE_PARAMETER,0},
	{0x000c0,MIPI_GEN_COMMAND,0},
	{0x0000c,TYPE_PARAMETER,0},
	{0x00005,TYPE_PARAMETER,0},
	{0x00002,TYPE_PARAMETER,0},


	//c0b3h ISC:Interval Scan Frame Setting
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x000b3,TYPE_PARAMETER,0},
	{0x000c0,MIPI_GEN_COMMAND,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00050,TYPE_PARAMETER,0},

	//c181h Oscillator Adjustment for Idle/Normal Mode
	//This command is used to set the Oscillator frequency in normal mode and idle mode
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x00081,TYPE_PARAMETER,0},
	{0x000C1,MIPI_GEN_COMMAND,0},
	{0x00066,TYPE_PARAMETER,0},


	//c480h Source driver precharge control
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x00080,TYPE_PARAMETER,0},
	{0x000c4,MIPI_GEN_COMMAND,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00084,TYPE_PARAMETER,0},
	//{0x000fa,TYPE_PARAMETER,0},

	//c4a0h DC2DC Setting
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x000a0,TYPE_PARAMETER,0},
	{0x000C4,MIPI_GEN_COMMAND,0},
	{0x00033,TYPE_PARAMETER,0},
	{0x00009,TYPE_PARAMETER,0},
	{0x00090,TYPE_PARAMETER,0},
	{0x0002b,TYPE_PARAMETER,0},
	{0x00033,TYPE_PARAMETER,0},
	{0x00009,TYPE_PARAMETER,0},
	{0x00090,TYPE_PARAMETER,0},
	{0x00054,TYPE_PARAMETER,0},


	//c580h Power control setting:is used to adjust analog power behavior
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x00080,TYPE_PARAMETER,0},
	{0x000C5,MIPI_GEN_COMMAND,0},
	{0x00008,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x000a0,TYPE_PARAMETER,0},
	{0x00011,TYPE_PARAMETER,0},


	//c590h PWR_CTRL2:Power Control Setting 2 for Normal Mode
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x00090,TYPE_PARAMETER,0},
	{0x000C5,MIPI_GEN_COMMAND,0},
	{0x00096,TYPE_PARAMETER,0},
	{0x00019,TYPE_PARAMETER,0},
	{0x00001,TYPE_PARAMETER,0},
	{0x00079,TYPE_PARAMETER,0},
	{0x00033,TYPE_PARAMETER,0},
	{0x00033,TYPE_PARAMETER,0},
	{0x00034,TYPE_PARAMETER,0},


	//c5a0h PWR_CTRL3:Power Control Setting 3 for Idlel Mode
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x000a0,TYPE_PARAMETER,0},
	{0x000C5,MIPI_GEN_COMMAND,0},
	{0x00096,TYPE_PARAMETER,0},
	{0x00016,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00079,TYPE_PARAMETER,0},
	{0x00033,TYPE_PARAMETER,0},
	{0x00033,TYPE_PARAMETER,0},
	{0x00034,TYPE_PARAMETER,0},

	{0x00000,MIPI_DCS_COMMAND,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x000d8,MIPI_GEN_COMMAND,0},
	{0x0005f,TYPE_PARAMETER,0},
	{0x0005f,TYPE_PARAMETER,0},

	{0x00000,MIPI_DCS_COMMAND,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x000e1,MIPI_GEN_COMMAND,0},
	{0x00001,TYPE_PARAMETER,0},
	{0x0000e,TYPE_PARAMETER,0},
	{0x00015,TYPE_PARAMETER,0},
	{0x0000e,TYPE_PARAMETER,0},
	{0x00007,TYPE_PARAMETER,0},
	{0x00013,TYPE_PARAMETER,0},
	{0x0000c,TYPE_PARAMETER,0},
	{0x0000b,TYPE_PARAMETER,0},
	{0x00003,TYPE_PARAMETER,0},
	{0x00006,TYPE_PARAMETER,0},
	{0x00009,TYPE_PARAMETER,0},
	{0x00007,TYPE_PARAMETER,0},
	{0x0000c,TYPE_PARAMETER,0},
	{0x0000d,TYPE_PARAMETER,0},
	{0x00008,TYPE_PARAMETER,0},
	{0x00001,TYPE_PARAMETER,0},

	{0x00000,MIPI_DCS_COMMAND,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x000e2,MIPI_GEN_COMMAND,0},
	{0x00001,TYPE_PARAMETER,0},
	{0x0000e,TYPE_PARAMETER,0},
	{0x00015,TYPE_PARAMETER,0},
	{0x0000e,TYPE_PARAMETER,0},
	{0x00007,TYPE_PARAMETER,0},
	{0x00013,TYPE_PARAMETER,0},
	{0x0000c,TYPE_PARAMETER,0},
	{0x0000b,TYPE_PARAMETER,0},
	{0x00003,TYPE_PARAMETER,0},
	{0x00006,TYPE_PARAMETER,0},
	{0x00009,TYPE_PARAMETER,0},
	{0x00007,TYPE_PARAMETER,0},
	{0x0000c,TYPE_PARAMETER,0},
	{0x0000d,TYPE_PARAMETER,0},
	{0x00008,TYPE_PARAMETER,0},
	{0x00001,TYPE_PARAMETER,0},

	//c5b0h PWR_CTRL4:Power Power Control Setting 4 for DC Volatge Settings
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x000b0,TYPE_PARAMETER,0},
	{0x000C5,MIPI_GEN_COMMAND,0},
	{0x00004,TYPE_PARAMETER,0},
	{0x000a8,TYPE_PARAMETER,0},

	//c680h ABC Parameter1:is used to set the internal founction block of LABC
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x00080,TYPE_PARAMETER,0},
	{0x000C6,MIPI_GEN_COMMAND,0},
	{0x00064,TYPE_PARAMETER,0},

	//c6b0h ABC Parameter2
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x000b0,TYPE_PARAMETER,0},
	{0x000C6,MIPI_GEN_COMMAND,0},
	{0x00003,TYPE_PARAMETER,0},
	/*for pwd  the register set LCD_backlight PWM frequency (22kHz)*/
	{0x00005,TYPE_PARAMETER,0}, //10-->05
	{0x00000,TYPE_PARAMETER,0},
	{0x0005f,TYPE_PARAMETER,0}, //1f -->5f
	{0x00012,TYPE_PARAMETER,0},

	{0x00000,MIPI_DCS_COMMAND,0},
	{0x000b7,TYPE_PARAMETER,0},
	{0x000b0,MIPI_GEN_COMMAND,0},
	{0x00010,TYPE_PARAMETER,0},

	{0x00000,MIPI_DCS_COMMAND,0},
	{0x000c0,TYPE_PARAMETER,0},
	{0x000b0,MIPI_GEN_COMMAND,0},
	{0x00055,TYPE_PARAMETER,0},

	{0x00000,MIPI_DCS_COMMAND,0},
	{0x000b1,TYPE_PARAMETER,0},
	{0x000b0,MIPI_GEN_COMMAND,0},
	{0x00003,TYPE_PARAMETER,0},

	//cb80h TCON_GOA_WAVE (panel timing state control)
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x00080,TYPE_PARAMETER,0},
	{0x000cb,MIPI_GEN_COMMAND,0},
	{0x00005,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00005,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},

	//cb90h TCON_GOA_WAVE (panel timing state control)
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x00090,TYPE_PARAMETER,0},
	{0x000cb,MIPI_GEN_COMMAND,0},
	{0x00055,TYPE_PARAMETER,0},
	{0x00055,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},

	//cba0h TCON_GOA_WAVE (panel timing state control)rol)
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x000a0,TYPE_PARAMETER,0},
	{0x000cb,MIPI_GEN_COMMAND,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00055,TYPE_PARAMETER,0},
	{0x00055,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},

	//cbb0h TCON_GOA_WAVE (panel timing state control)l)
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x000b0,TYPE_PARAMETER,0},
	{0x000Cb,MIPI_GEN_COMMAND,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},

	//cbc0h TCON_GOA_WAVE (panel timing state control)rol)rol)
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x000c0,TYPE_PARAMETER,0},
	{0x000Cb,MIPI_GEN_COMMAND,0},
	{0x00055,TYPE_PARAMETER,0},
	{0x00055,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00004,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00004,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00004,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00004,TYPE_PARAMETER,0},
	{0x00004,TYPE_PARAMETER,0},
	{0x00004,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},

	//cbd0h TCON_GOA_WAVE (panel timing state control)rol)rol)rol)
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x000d0,TYPE_PARAMETER,0},
	{0x000cb,MIPI_GEN_COMMAND,0},
	{0x00004,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00055,TYPE_PARAMETER,0},
	{0x00055,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00004,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00004,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00004,TYPE_PARAMETER,0},//pd040ia-05d gama2.2


	//cbe0h TCON_GOA_WAVE (panel timing state control)l)
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x000e0,TYPE_PARAMETER,0},
	{0x000cb,MIPI_GEN_COMMAND,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00004,TYPE_PARAMETER,0},
	{0x00004,TYPE_PARAMETER,0},
	{0x00004,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00004,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0}, //pd040ia-05d gama2.2


	//cbf0h TCON_GOA_WAVE (panel timing state control)ol)l)
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x000f0,TYPE_PARAMETER,0},
	{0x000cb,MIPI_GEN_COMMAND,0},
	{0x0000f,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x000cc,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x0000f,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x000cc,TYPE_PARAMETER,0},
	{0x000c3,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0}, //800-->320  //854 -->0x356 //864 -->0x360


	//cc80h TCON_GOA_WAVE (panel pad mapping control)ol)l)
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x00080,TYPE_PARAMETER,0},
	{0x000cc,MIPI_GEN_COMMAND,0},
	{0x00025,TYPE_PARAMETER,0},
	{0x00026,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x0000c,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x0000a,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00010,TYPE_PARAMETER,0},


	//cc90h TCON_GOA_WAVE (panel timing state control)rol)
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x00090,TYPE_PARAMETER,0},
	{0x000cc,MIPI_GEN_COMMAND,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x0000e,TYPE_PARAMETER,0},
	{0x00002,TYPE_PARAMETER,0},
	{0x00004,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00006,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00025,TYPE_PARAMETER,0},
	{0x00026,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},

	//cca0h TCON_GOA_WAVE (panel timing state control)rol)
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x000a0,TYPE_PARAMETER,0},
	{0x000cc,MIPI_GEN_COMMAND,0},
	{0x0000b,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00009,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x0000f,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x0000d,TYPE_PARAMETER,0},
	{0x00001,TYPE_PARAMETER,0},
	{0x00003,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00005,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},

	//ccb0h TCON_GOA_WAVE (panel pad mapping control)ol)l)
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x000b0,TYPE_PARAMETER,0},
	{0x000cc,MIPI_GEN_COMMAND,0},
	{0x00026,TYPE_PARAMETER,0},
	{0x00025,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x0000d,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x0000f,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00009,TYPE_PARAMETER,0},

	//ccc0h TCON_GOA_WAVE (panel timing state control)rol)
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x000c0,TYPE_PARAMETER,0},
	{0x000cc,MIPI_GEN_COMMAND,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x0000b,TYPE_PARAMETER,0},
	{0x00003,TYPE_PARAMETER,0},
	{0x00001,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00005,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00026,TYPE_PARAMETER,0},
	{0x00025,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},

	//ccd0h TCON_GOA_WAVE (panel timing state control)rol)
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x000d0,TYPE_PARAMETER,0},
	{0x000cc,MIPI_GEN_COMMAND,0},
	{0x0000e,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00010,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x0000a,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x0000c,TYPE_PARAMETER,0},
	{0x00004,TYPE_PARAMETER,0},
	{0x00002,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00006,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},

	//ce80h GOA VST Settimg
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x00080,TYPE_PARAMETER,0},
	{0x000ce,MIPI_GEN_COMMAND,0},
	{0x0008a,TYPE_PARAMETER,0},
	{0x00003,TYPE_PARAMETER,0},
	{0x00028,TYPE_PARAMETER,0},
	{0x00089,TYPE_PARAMETER,0},
	{0x00003,TYPE_PARAMETER,0},
	{0x00028,TYPE_PARAMETER,0},
	{0x00088,TYPE_PARAMETER,0},
	{0x00003,TYPE_PARAMETER,0},
	{0x00028,TYPE_PARAMETER,0},
	{0x00087,TYPE_PARAMETER,0},
	{0x00003,TYPE_PARAMETER,0},
	{0x00028,TYPE_PARAMETER,0},

	//ce90h GOA VEND Setting  GOA Group Setting
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x00090,TYPE_PARAMETER,0},
	{0x000ce,MIPI_GEN_COMMAND,0},
	{0x00038,TYPE_PARAMETER,0},
	{0x0000f,TYPE_PARAMETER,0},
	{0x00028,TYPE_PARAMETER,0},
	{0x00038,TYPE_PARAMETER,0},
	{0x0000e,TYPE_PARAMETER,0},
	{0x00028,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},

	//cea0h GOA CLKA1 Setting  GOA CLKA2 Setting
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x000a0,TYPE_PARAMETER,0},
	{0x000ce,MIPI_GEN_COMMAND,0},
	{0x00038,TYPE_PARAMETER,0},
	{0x00006,TYPE_PARAMETER,0},
	{0x00003,TYPE_PARAMETER,0},
	{0x000c1,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00028,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00038,TYPE_PARAMETER,0},
	{0x00005,TYPE_PARAMETER,0},
	{0x00003,TYPE_PARAMETER,0},
	{0x000c2,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00028,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},

	//ceb0h GOA CLKA3 Setting  GOA CLKA4 Setting
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x000b0,TYPE_PARAMETER,0},
	{0x000ce,MIPI_GEN_COMMAND,0},
	{0x00038,TYPE_PARAMETER,0},
	{0x00004,TYPE_PARAMETER,0},
	{0x00003,TYPE_PARAMETER,0},
	{0x000c3,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00028,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00038,TYPE_PARAMETER,0},
	{0x00003,TYPE_PARAMETER,0},
	{0x00003,TYPE_PARAMETER,0},
	{0x000c4,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00028,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},

	//cec0h GOA CLKB1 Setting  GOA CLKB2 Setting
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x000c0,TYPE_PARAMETER,0},
	{0x000ce,MIPI_GEN_COMMAND,0},
	{0x00038,TYPE_PARAMETER,0},
	{0x00002,TYPE_PARAMETER,0},
	{0x00003,TYPE_PARAMETER,0},
	{0x000c5,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00028,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00038,TYPE_PARAMETER,0},
	{0x00001,TYPE_PARAMETER,0},
	{0x00003,TYPE_PARAMETER,0},
	{0x000c6,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00028,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},

	//ced0h GOA CLKB3 Setting  GOA CLKB4 Setting
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x000d0,TYPE_PARAMETER,0},
	{0x000ce,MIPI_GEN_COMMAND,0},
	{0x00038,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00003,TYPE_PARAMETER,0},
	{0x000c7,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00028,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00030,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00003,TYPE_PARAMETER,0},
	{0x000c8,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00028,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},

	//cf80h GOA CLKC1 Setting  GOA CLKC2 Setting
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x00080,TYPE_PARAMETER,0},
	{0x000cf,MIPI_GEN_COMMAND,0},
	{0x000f0,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00010,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x000f0,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00010,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},

	//cf90h GOA CLKC3 Setting  GOA CLKC4 Setting
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x00090,TYPE_PARAMETER,0},
	{0x000cf,MIPI_GEN_COMMAND,0},
	{0x000f0,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00010,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x000f0,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00010,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},

	//cfa0h GOA CLKD1 Setting  GOA CLKD2 Setting
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x000a0,TYPE_PARAMETER,0},
	{0x000cf,MIPI_GEN_COMMAND,0},
	{0x000f0,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00010,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x000f0,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00010,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},

	//cfb0h GOA CLKD3 Setting  GOA CLKD4 Setting
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x000b0,TYPE_PARAMETER,0},
	{0x000cf,MIPI_GEN_COMMAND,0},
	{0x000f0,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00010,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x000f0,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00010,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},

	//cfc0h GOA ECLK Setting  GOA Signal Toggle Option Setting
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x000c0,TYPE_PARAMETER,0},
	{0x000cf,MIPI_GEN_COMMAND,0},
	{0x00001,TYPE_PARAMETER,0},
	{0x00001,TYPE_PARAMETER,0},
	{0x00020,TYPE_PARAMETER,0},
	{0x00020,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00002,TYPE_PARAMETER,0},
	{0x00001,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,TYPE_PARAMETER,0},

	/* optimize display performance, 960 * 15/16 == 384h */
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00035,MIPI_DCS_COMMAND,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x00044,MIPI_DCS_COMMAND,0},
	{0x00003,TYPE_PARAMETER,0},
	{0x00084,TYPE_PARAMETER,0},

	/* disable all pixel on & inversion on */
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x00083,TYPE_PARAMETER,0},
	{0x000b3,MIPI_GEN_COMMAND,0},
	{0x000c0,TYPE_PARAMETER,0},

	/* disable cmd2 */
	{0x00000,MIPI_DCS_COMMAND,0},
	{0x00000,TYPE_PARAMETER,0},
	{0x000ff,MIPI_GEN_COMMAND,0},
	{0x000ff,TYPE_PARAMETER,0},
	{0x000ff,TYPE_PARAMETER,0},
	{0x000ff,TYPE_PARAMETER,0},

	{0x00011,MIPI_DCS_COMMAND,0},	//sleep out
	{0x00029,MIPI_DCS_COMMAND,150}, //Display ON

	{0x00013,MIPI_DCS_COMMAND,20},
	{0x00000,TYPE_PARAMETER,0},

	{0x00051,MIPI_DCS_COMMAND,20}, // displaybright
	{0x0007F,TYPE_PARAMETER,0},
	{0x00053,MIPI_DCS_COMMAND,0}, // ctrldisplay1
	{0x00024,TYPE_PARAMETER,0},
	{0x00055,MIPI_DCS_COMMAND,0}, // ctrldisplay2
	{0x00001,TYPE_PARAMETER,0},

	{0x0002c,MIPI_DCS_COMMAND,20},

	{0x00000,MIPI_TYPE_END,20}, //end flag
};

static const struct read_sequence otm9608a_esd_read_table_0A[] =
{
	{0x0A,MIPI_DCS_COMMAND,1},
};

static const struct read_sequence otm9608a_esd_read_table_0B[] =
{
	{0x0B,MIPI_DCS_COMMAND,1},
};

static const struct read_sequence otm9608a_esd_read_table_0C[] =
{
	{0x0C,MIPI_DCS_COMMAND,1},
};

static const struct read_sequence otm9608a_esd_read_table_0D[] =
{
	{0x0D,MIPI_DCS_COMMAND,1},
};

static const struct read_sequence otm9608a_esd_read_table_0E[] =
{
	{0x0E,MIPI_DCS_COMMAND,1},
};


/* read registers on LCD IC to check if error occur */
static int otm9608a_monitor_reg_status(struct msm_fb_data_type *mfd)
{
	struct dsi_buf *rp, *tp;
	int err = 0;//ok
	uint8 read_value[3]={0, 0, 0};
	static uint32 count=0;
	tp = &otm9608a_tx_buf;
	rp = &otm9608a_rx_buf;

	mipi_dsi_buf_init(tp);
	mipi_dsi_buf_init(rp);

	process_mipi_read_table(mfd,tp,rp,(struct read_sequence*)&otm9608a_esd_read_table_0A);
	read_value[0] = *((unsigned char *)rp->data);
	if (0x9c != read_value[0])
	{
		LCD_DEBUG("%s: 0x0a value0 = %02x\n", __func__, read_value[0]);
		process_mipi_read_table(mfd,tp,rp,(struct read_sequence*)&otm9608a_esd_read_table_0A);
		read_value[1] = *((unsigned char *)rp->data);
		if (0x9c != read_value[1])
		{
			LCD_DEBUG("%s: 0x0a value1 = %02x\n", __func__, read_value[1]);
			process_mipi_read_table(mfd,tp,rp,(struct read_sequence*)&otm9608a_esd_read_table_0A);
			read_value[2] = *((unsigned char *)rp->data);
			if (0x9c != read_value[2])
			{
				LCD_DEBUG("%s: 0x0a value2 = %02x\n", __func__, read_value[2]);
				err = 1;
			}
		}
	}

	process_mipi_read_table(mfd,tp,rp,(struct read_sequence*)&otm9608a_esd_read_table_0B);
	read_value[0] = *((unsigned char *)rp->data);
	if (0x00 != read_value[0])
	{
		LCD_DEBUG("%s: 0x0b value0 = %02x\n", __func__, read_value[0]);
		process_mipi_read_table(mfd,tp,rp,(struct read_sequence*)&otm9608a_esd_read_table_0B);
		read_value[1] = *((unsigned char *)rp->data);
		if (0x00 != read_value[1])
		{
			LCD_DEBUG("%s: 0x0b value1 = %02x\n", __func__, read_value[1]);
			process_mipi_read_table(mfd,tp,rp,(struct read_sequence*)&otm9608a_esd_read_table_0B);
			read_value[2] = *((unsigned char *)rp->data);
			if (0x00 != read_value[2])
			{
				LCD_DEBUG("%s: 0x0b value2 = %02x\n", __func__, read_value[2]);
				err = 2;
			}
		}
	}
	process_mipi_read_table(mfd,tp,rp,(struct read_sequence*)&otm9608a_esd_read_table_0C);
	read_value[0] = *((unsigned char *)rp->data);
	if (0x07 != read_value[0])
	{
		LCD_DEBUG("%s: 0x0c value0 = %02x\n", __func__, read_value[0]);
		process_mipi_read_table(mfd,tp,rp,(struct read_sequence*)&otm9608a_esd_read_table_0C);
		read_value[1] = *((unsigned char *)rp->data);
		if (0x07 != read_value[1])
		{
			LCD_DEBUG("%s: 0x0c value1 = %02x\n", __func__, read_value[1]);
			process_mipi_read_table(mfd,tp,rp,(struct read_sequence*)&otm9608a_esd_read_table_0C);
			read_value[2] = *((unsigned char *)rp->data);
			if (0x07 != read_value[2])
			{
				LCD_DEBUG("%s: 0x0c value2 = %02x\n", __func__, read_value[2]);
				err = 3;
			}
		}
	}
	process_mipi_read_table(mfd,tp,rp,(struct read_sequence*)&otm9608a_esd_read_table_0D);
	read_value[0] = *((unsigned char *)rp->data);
	if (0x00 != read_value[0])
	{
		LCD_DEBUG("%s: 0x0d value0 = %02x\n", __func__, read_value[0]);
		process_mipi_read_table(mfd,tp,rp,(struct read_sequence*)&otm9608a_esd_read_table_0D);
		read_value[1] = *((unsigned char *)rp->data);
		if (0x00 != read_value[1])
		{
			LCD_DEBUG("%s: 0x0d value1 = %02x\n", __func__, read_value[1]);
			process_mipi_read_table(mfd,tp,rp,(struct read_sequence*)&otm9608a_esd_read_table_0D);
			read_value[2] = *((unsigned char *)rp->data);
			if (0x00 != read_value[2])
			{
				LCD_DEBUG("%s: 0x0d value2 = %02x\n", __func__, read_value[2]);
				err = 4;
			}
		}
	}

	process_mipi_read_table(mfd,tp,rp,(struct read_sequence*)&otm9608a_esd_read_table_0E);
	read_value[0] = *((unsigned char *)rp->data);
	if (0x80 != read_value[0])
	{
		LCD_DEBUG("%s: 0x0e value0 = %02x\n", __func__, read_value[0]);
		process_mipi_read_table(mfd,tp,rp,(struct read_sequence*)&otm9608a_esd_read_table_0E);
		read_value[1] = *((unsigned char *)rp->data);
		if (0x80 != read_value[1])
		{
			LCD_DEBUG("%s: 0x0e value1 = %02x\n", __func__, read_value[1]);
			process_mipi_read_table(mfd,tp,rp,(struct read_sequence*)&otm9608a_esd_read_table_0E);
			read_value[2] = *((unsigned char *)rp->data);
			if (0x80 != read_value[2])
			{
				LCD_DEBUG("%s: 0x0e value2 = %02x\n", __func__, read_value[2]);
				err = 5;
			}
		}
	}

	if (err != 0)
	{
		LCD_DEBUG("%s: end, err = %d, count = 0x%0X.\n", __func__, err, count);
	}
	count++;

	return err;
}

/* check if panel alive */
static int mipi_cmd_otm9608a_check_live_status(struct msm_fb_data_type *mfd)
{
	int ret_bta = 0;
	int ret_monitor = 0;
	static uint32 check_cnt = 0;

	/* do not check while booting, ignore the first 5 times */
	if (check_cnt < 5)
	{
		check_cnt++;
		return TIANMA_OTM9608A_PANEL_ALIVE;
	}

	ret_bta = mipi_dsi_wait_for_bta_ack();

	/* only read registers of LCD IC when bta check return good */
	if (ret_bta > 0)
	{
		ret_monitor = otm9608a_monitor_reg_status(mfd);
		if (ret_monitor != 0)
		{
			mfd->is_panel_alive = FALSE;
			pr_info("check_live_status: ret_bta=%d, ret_monitor=%d.\n", ret_bta, ret_monitor);
		}
	}
	else
	{
		mfd->is_panel_alive = FALSE;
		pr_info("check_live_status: ret_bta=%d, ret_monitor=%d.\n", ret_bta, ret_monitor);
	}

	return ((ret_monitor != 0) ? TIANMA_OTM9608A_PANEL_DEAD : ret_bta);
}

#endif

static int mipi_otm9608a_qhd_lcd_on(struct platform_device *pdev)
{
	boolean para_debug_flag = FALSE;
	uint32 para_num = 0;
	struct msm_fb_data_type *mfd;
	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL; 

	#if LCD_OTM9608A_TIANMA_ESD_SIGN
	if (!mfd->is_panel_alive)
	{
		LCD_DEBUG("%s: panel not alive, reset and reinit begin.\n", __func__);

		/* toggle GPIO129 to reset LCD */
		lcd_reset();

		/* change to LP mode then send initial code */
		mipi_set_tx_power_mode(1);
		process_mipi_table(mfd,&otm9608a_tx_buf,(struct sequence*)&otm9608a_tianma_lcd_init_table,
			ARRAY_SIZE(otm9608a_tianma_lcd_init_table), lcd_panel_qhd);
		mipi_set_tx_power_mode(0);

		msleep(30);
		LCD_DEBUG("%s: panel not alive, reset and reinit end.\n", __func__);

		/* restore is_panel_alive */
		mfd->is_panel_alive = TRUE;
		return 0;
	}
	#endif

	para_debug_flag = lcd_debug_malloc_get_para( "otm9608a_lcd_init_table_debug", 
			(void**)&otm9608a_lcd_init_table_debug,&para_num);

	if( (TRUE == para_debug_flag) && (NULL != otm9608a_lcd_init_table_debug))
	{
		process_mipi_table(mfd,&otm9608a_tx_buf,otm9608a_lcd_init_table_debug,
			 para_num, lcd_panel_qhd);
	}
	else
	{
		mipi_set_tx_power_mode(1);
		process_mipi_table(mfd,&otm9608a_tx_buf,(struct sequence*)&otm9608a_qhd_standby_exit_table,
		 	ARRAY_SIZE(otm9608a_qhd_standby_exit_table), lcd_panel_qhd);
		mipi_set_tx_power_mode(0);
	}

	if((TRUE == para_debug_flag)&&(NULL != otm9608a_lcd_init_table_debug))
	{
		lcd_debug_free_para((void *)otm9608a_lcd_init_table_debug);
	}
	pr_info("leave mipi_otm9608a_lcd_on \n");

	return 0;
}

static int mipi_otm9608a_qhd_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	#if LCD_OTM9608A_TIANMA_ESD_SIGN
	if (!mfd->is_panel_alive)
	{
		LCD_DEBUG("%s: panel not alive.\n", __func__);
		return 0;
	}
	#endif

	process_mipi_table(mfd,&otm9608a_tx_buf,(struct sequence*)&otm9608a_qhd_standby_enter_table,
		 ARRAY_SIZE(otm9608a_qhd_standby_enter_table), lcd_panel_qhd);
	pr_info("leave mipi_otm9608a_lcd_off \n");
	return 0;
}


#ifdef CONFIG_FB_AUTO_CABC
static struct sequence otm9608a_auto_cabc_set_table[] =
{
	{0x00055,MIPI_DCS_COMMAND,0}, 
	{0x00001,TYPE_PARAMETER,0},
	{0x00029,MIPI_TYPE_END,0}, 
};
/***************************************************************
Function: otm9608a_config_cabc
Description: Set CABC configuration
Parameters:
	struct msmfb_cabc_config cabc_cfg: CABC configuration struct
Return:
	0: success
***************************************************************/
static int otm9608a_config_auto_cabc(struct msmfb_cabc_config cabc_cfg,struct msm_fb_data_type *mfd)
{
	int ret = 0;

	#if LCD_OTM9608A_TIANMA_ESD_SIGN
	if (!mfd->is_panel_alive)
	{
		LCD_DEBUG("%s: panel not alive.\n", __func__);
		return 0;
	}
	#endif

	switch(cabc_cfg.mode)
	{
		case CABC_MODE_UI:
			otm9608a_auto_cabc_set_table[1].reg = 0x0001;
			break;
		case CABC_MODE_MOVING:
		case CABC_MODE_STILL:
			otm9608a_auto_cabc_set_table[1].reg = 0x0003;
			break;
		default:
			LCD_DEBUG("%s: invalid cabc mode: %d\n", __func__, cabc_cfg.mode);
			ret = -EINVAL;
			break;
	}
	if(likely(0 == ret))
	{
		process_mipi_table(mfd,&otm9608a_tx_buf,otm9608a_auto_cabc_set_table,
			 ARRAY_SIZE(otm9608a_auto_cabc_set_table), lcd_panel_qhd);
	}

	LCD_DEBUG("%s: change cabc mode to %d\n",__func__,cabc_cfg.mode);
	return ret;
}
#endif // CONFIG_FB_AUTO_CABC


void otm9608a_qhd_set_cabc_backlight(struct msm_fb_data_type *mfd,uint32 bl_level)
{
	#if LCD_OTM9608A_TIANMA_ESD_SIGN
	if (!mfd->is_panel_alive)
	{
		LCD_DEBUG("%s: panel not alive.\n", __func__);
		return;
	}
	#endif

	otm9608a_qhd_write_cabc_brightness_table[1].reg = bl_level; // 1 will be changed if modify init code

	process_mipi_table(mfd,&otm9608a_tx_buf,(struct sequence*)&otm9608a_qhd_write_cabc_brightness_table,
		 ARRAY_SIZE(otm9608a_qhd_write_cabc_brightness_table), lcd_panel_qhd);
}


static int __devinit mipi_otm9608a_qhd_lcd_probe(struct platform_device *pdev)
{
	struct platform_device *current_pdev = NULL;

	current_pdev = msm_fb_add_device(pdev);

	if (current_pdev == NULL) 
	{
            LCD_DEBUG("error in %s\n",__func__);
	}
	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_otm9608a_qhd_lcd_probe,
	.driver = {
		.name   = LCD_DEVICE_NAME,
	},
};


static struct msm_fb_panel_data otm9608a_qhd_panel_data = {
	.on					= mipi_otm9608a_qhd_lcd_on,
	.off					= mipi_otm9608a_qhd_lcd_off,
	.set_backlight 		= pwm_set_backlight,
	/*add cabc control backlight*/
	.set_cabc_brightness 	= otm9608a_qhd_set_cabc_backlight,
#ifdef CONFIG_FB_AUTO_CABC
	.config_cabc = otm9608a_config_auto_cabc,
#endif
	#if LCD_OTM9608A_TIANMA_ESD_SIGN
	.check_live_status = mipi_cmd_otm9608a_check_live_status,
	#endif
};

static struct platform_device this_device = {
	.name   = LCD_DEVICE_NAME,
	.id	= 0,
	.dev	= {
		.platform_data = &otm9608a_qhd_panel_data,
	},
};


static int __init mipi_cmd_otm9608a_qhd_init(void)
{
	int ret = 0;
	struct msm_panel_info *pinfo = NULL;
	
	lcd_panel_qhd = get_lcd_panel_type();
    
	if (MIPI_CMD_OTM9608A_TIANMA_QHD != lcd_panel_qhd)
	{
		return 0;
	}

	pr_info("enter mipi_cmd_otm9608a_qhd_init \n");
	mipi_dsi_buf_alloc(&otm9608a_tx_buf, DSI_BUF_SIZE);
	#if LCD_OTM9608A_TIANMA_ESD_SIGN
	mipi_dsi_buf_alloc(&otm9608a_rx_buf, DSI_BUF_SIZE);
	#endif

	ret = platform_driver_register(&this_driver);
	if (!ret)
	{
	 	pinfo = &otm9608a_qhd_panel_data.panel_info;
		pinfo->xres = 540;
		pinfo->yres = 960;
		pinfo->type = MIPI_CMD_PANEL;
		pinfo->pdest = DISPLAY_1;
		pinfo->wait_cycle = 0;
		pinfo->bpp = 24;		
		pinfo->bl_max = 255;
		pinfo->bl_min = 30;		
		pinfo->fb_num = 2;
		pinfo->clk_rate = 419000000;  //419M
		pinfo->lcd.refx100 = 6000; /* adjust refx100 to prevent tearing */

		pinfo->mipi.mode = DSI_CMD_MODE;
		pinfo->mipi.dst_format = DSI_CMD_DST_FORMAT_RGB888;
		pinfo->mipi.vc = 0;////8888
		pinfo->mipi.rgb_swap = DSI_RGB_SWAP_RGB;
		pinfo->mipi.data_lane0 = TRUE;
		pinfo->mipi.data_lane1 = TRUE;
		pinfo->mipi.t_clk_post = 0x19;
		pinfo->mipi.t_clk_pre = 0x2e;
		pinfo->mipi.stream = 0; /* dma_p */
		pinfo->mipi.mdp_trigger = DSI_CMD_TRIGGER_SW;
		pinfo->mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
		/*set hw te sync*/
		pinfo->lcd.hw_vsync_mode = TRUE;
		pinfo->lcd.vsync_enable = TRUE;
		pinfo->mipi.te_sel = 1; /* TE from vsync gpio */
		pinfo->mipi.interleave_max = 1;
		pinfo->mipi.insert_dcs_cmd = TRUE;
		pinfo->mipi.wr_mem_continue = 0x3c;
		pinfo->mipi.wr_mem_start = 0x2c;
		pinfo->mipi.dsi_phy_db = &dsi_cmd_mode_phy_db_otm9608a_qhd;
		pinfo->mipi.tx_eot_append = 0x01;
		pinfo->mipi.rx_eot_ignore = 0;

		ret = platform_device_register(&this_device);
		if (ret)
			pr_err("%s: failed to register device!\n", __func__);
	}
	return ret;
}

module_init(mipi_cmd_otm9608a_qhd_init);
