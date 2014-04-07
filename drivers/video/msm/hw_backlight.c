/* drivers\video\msm\hw_backlight.c
 * backlight driver for 7x30 platform
 *
 * Copyright (C) 2010 HUAWEI Technology Co., ltd.
 * 
 * Date: 2010/12/07
 * By lijianzhao
 * 
 */

#include "msm_fb.h"
#include <linux/mfd/pmic8058.h>
#include <mach/gpio.h>
#include <mach/vreg.h>
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/pwm.h>
#include <mach/pmic.h>
#include <linux/earlysuspend.h>
#include <linux/spinlock.h>
#include <linux/semaphore.h>
#include <linux/hardware_self_adapt.h>
#include "hw_lcd_common.h"
#include <mach/rpc_pmapp.h>

/*CABC CTL MACRO , RANGE 0 to 255*/
#define PWM_PERIOD ( NSEC_PER_SEC / ( 22 * 1000 ) )	/* ns, period of 22Khz */
#define PWM_LEVEL 255
#define PWM_DUTY_LEVEL (PWM_PERIOD / PWM_LEVEL)
#define PM_GPIO25_PWM_ID  1
#define PM_GPIO26_PWM_ID  2
#define ADD_VALUE			4
#define PWM_LEVEL_ADJUST	226
/*modify the min value*/
#define BL_MIN_LEVEL 1
#define G610C_BL_MIN_LEVEL 1
#define G610C_BL_MAX_LEVEL 250
static struct msm_fb_data_type *mfd_local;
static boolean backlight_set = FALSE;
static atomic_t suspend_flag = ATOMIC_INIT(0);

void msm_backlight_set(int level)
{
    static uint8 last_level = 0;
	if(level)
	{
		if (level < BL_MIN_LEVEL)        
		{    
			level = BL_MIN_LEVEL;      
		}
	}
    if (last_level == level)
    {
        return ;
    }
    last_level = level;
	pmapp_disp_backlight_set_brightness(last_level);

}

void cabc_backlight_set(struct msm_fb_data_type * mfd)
{	     
	struct msm_fb_panel_data *pdata = NULL;   
	uint32 bl_level = mfd->bl_level;
		/* keep duty 10% < level < 100% */
	if (bl_level)    
   	{   
	/****delete one line codes for backlight*****/
		if (bl_level < BL_MIN_LEVEL)        
		{    
			bl_level = BL_MIN_LEVEL;      
		}  
	}
	/* backlight ctrl by LCD-self, like as CABC */  
	pdata = (struct msm_fb_panel_data *)mfd->pdev->dev.platform_data;  
	if ((pdata) && (pdata->set_cabc_brightness))   
   	{       
		pdata->set_cabc_brightness(mfd,bl_level);
	}

}

void pwm_set_backlight(struct msm_fb_data_type *mfd)
{
	uint32 bl_level = mfd->bl_level;
	/*< Delete unused variable */
	lcd_panel_type lcd_panel = get_lcd_panel_type();
	/*When all the device are resume that can turn the light*/
	if(atomic_read(&suspend_flag)) 
	{
		mfd_local = mfd;
		backlight_set = TRUE;
		return;
	}
	/*< Delete some lines,control backlight in hardware lights.c by property */
	if (MIPI_CMD_OTM8009A_CHIMEI_FWVGA == lcd_panel)
	{
		/* Improve brightness for CMI OTM8009A, 67 is default, 79 is experimental value */
		mfd->bl_level *= (79 / 67);
	}
	else if (MIPI_CMD_OTM8009A_TIANMA_FWVGA == lcd_panel)
	{
		/* if bl_level bigger than 160(experimental value) */
		if (bl_level > 160)
		{
			/* increase bl_level by multiplying 79/67(experimental value) */
			bl_level = (bl_level * 79 / 67);
			if (bl_level > PWM_LEVEL)
			{
				bl_level = PWM_LEVEL;
			}

			//pr_info("%s: cur_bl_level = %d, new_bl_level = %d\n", __func__, mfd->bl_level, bl_level);
			mfd->bl_level = bl_level;
		}
	}
	if(machine_is_msm8x25_G610C())
	{
		//workaround for G610C
		if(bl_level < G610C_BL_MIN_LEVEL)
			bl_level = G610C_BL_MIN_LEVEL;
		else if(bl_level > G610C_BL_MAX_LEVEL)
			bl_level = G610C_BL_MAX_LEVEL;

		//printk("%s: cur_bl_level = %d, new_bl_level = %d \n", __func__,mfd->bl_level,bl_level);
		mfd->bl_level = bl_level;
	}
	if (get_hw_lcd_ctrl_bl_type() == CTRL_BL_BY_MSM)
	{
		msm_backlight_set(mfd->bl_level);
 	}   
	else    
 	{
		cabc_backlight_set(mfd);  
 	}
	return;
}
#ifdef CONFIG_HAS_EARLYSUSPEND
static void pwm_backlight_suspend( struct early_suspend *h)
{
	atomic_set(&suspend_flag,1);
}

static void pwm_backlight_resume( struct early_suspend *h)
{
	atomic_set(&suspend_flag,0);
	
	if (backlight_set == TRUE)
	{
		if (get_hw_lcd_ctrl_bl_type() == CTRL_BL_BY_LCD)
		{
			/* MIPI use two semaphores */
			if(get_hw_lcd_interface_type() == LCD_IS_MIPI_CMD)
			{
				down(&mfd_local->dma->mutex);
				down(&mfd_local->sem);
				pwm_set_backlight(mfd_local);
				up(&mfd_local->sem);
				up(&mfd_local->dma->mutex);
			}
			/*add mipi video interface*/
			else if(get_hw_lcd_interface_type() == LCD_IS_MIPI_VIDEO)
			{
				down(&mfd_local->dma->mutex);
				pwm_set_backlight(mfd_local);
				up(&mfd_local->dma->mutex);
			}
			/* MDDI don't use semaphore */
			else if((get_hw_lcd_interface_type() == LCD_IS_MDDI_TYPE1)
				||(get_hw_lcd_interface_type() == LCD_IS_MDDI_TYPE2))
			{
				pwm_set_backlight(mfd_local);
			}
			else
			{
				down(&mfd_local->sem);
				pwm_set_backlight(mfd_local);
				up(&mfd_local->sem);
			}
		}
		else
		{
			down(&mfd_local->sem);
			pwm_set_backlight(mfd_local);
			up(&mfd_local->sem);
		}
	}
}
/*add early suspend*/
static struct early_suspend pwm_backlight_early_suspend = {
	.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN - 1,
	.suspend = pwm_backlight_suspend,
	.resume = pwm_backlight_resume,
};
#endif


static int __init pwm_backlight_init(void)
{
	
#ifdef CONFIG_HAS_EARLYSUSPEND
	register_early_suspend(&pwm_backlight_early_suspend);
#endif

	return 0;
}
module_init(pwm_backlight_init);
