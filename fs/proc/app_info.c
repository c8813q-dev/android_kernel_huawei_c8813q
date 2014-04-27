/*
 *  linux/fs/proc/app_info.c
 *
 *
 * Changes:
 * mazhenhua      :  for read appboot version and flash id.
 */

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/kernel.h>
#include <linux/kernel_stat.h>
#include <linux/fs.h>
#include <linux/tty.h>
#include <linux/string.h>
#include <linux/mman.h>
#include <linux/quicklist.h>
#include <linux/proc_fs.h>
#include <linux/ioport.h>
#include <linux/mm.h>
#include <linux/mmzone.h>
#include <linux/pagemap.h>
#include <linux/interrupt.h>
#include <linux/swap.h>
#include <linux/slab.h>
#include <linux/genhd.h>
#include <linux/smp.h>
#include <linux/signal.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/seq_file.h>
#include <linux/times.h>
#include <linux/profile.h>
#include <linux/utsname.h>
#include <linux/blkdev.h>
#include <linux/hugetlb.h>
#include <linux/jiffies.h>
#include <linux/sysrq.h>
#include <linux/vmalloc.h>
#include <linux/crash_dump.h>
#include <linux/pid_namespace.h>
#include <linux/bootmem.h>
#include <asm/uaccess.h>
#include <asm/pgtable.h>
#include <asm/io.h>
#include <asm/tlb.h>
#include <asm/div64.h>
#include "internal.h"
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <linux/hardware_self_adapt.h>
#include "../../arch/arm/mach-msm/include/mach/socinfo.h"
#include <linux/touch_platform_config.h>
#define PROC_MANUFACTURER_STR_LEN 30
#define MAX_VERSION_CHAR 40
#define BOARD_ID_LEN 32
/* Redefine board sub version id len here */
#define BOARD_ID_SUB_VER_LEN 10
#define LCD_NAME_LEN 20
#define HW_VERSION   20
#define HW_VERSION_SUB_VER  6
#define AUDIO_PROPERTY_LEN 32

static char appsboot_version[MAX_VERSION_CHAR + 1];
static char str_flash_nand_id[PROC_MANUFACTURER_STR_LEN] = {0};
static u32 camera_id;
static u32 ts_id;
#ifdef CONFIG_HUAWEI_POWER_DOWN_CHARGE
static u32 charge_flag;
#endif
const char *dcdc_type = NULL;

typedef struct
{
   int  mach_type; 
   char s_board_id[BOARD_ID_LEN];
   char hw_version_id[HW_VERSION];
}s_board_hw_version_type;
/* this is s_board_id and hw_version_id list,
 * when you want to add s_board_id and hw_version_if for new product,
 * add you s_board_id and hw_version_id this list.
 */
const s_board_hw_version_type s_board_hw_version_table[] =
{  /* machine_arch_type        s_board_id           hw_version_id */

   {MACH_TYPE_MSM8X25_C8813,    "MSM8X25_C8813",	"HC1C8813M "},
 
   {MACH_TYPE_MSM8X25_C8950D,   "MSM8X25_C8950D",	"HC1C8950M "},

   {MACH_TYPE_MSM8X25_U8951,    "MSM8X25_U8951",    "HD2U8951M "},
   {MACH_TYPE_MSM8X25_G520U,    "MSM8X25_G520U",    "HD1G520M "},
   {MACH_TYPE_MSM8X25_C8813Q,    "MSM8X25_C8813Q",   "HC1G510DQM "},
   {MACH_TYPE_MSM8X25_G610C,    "MSM8X25_G610-C00",    "HC1G610M "},
};
void set_s_board_hw_version(char *s_board_id,char *hw_version_id)
{  
    unsigned int temp_num = 0;
    unsigned int table_num = 0;

    if ((NULL == s_board_id) || (NULL == hw_version_id))
    {
         printk("app_info : s_board_id or hw_version_type is null!\n");    
         return ;
    }

    table_num = sizeof(s_board_hw_version_table)/sizeof(s_board_hw_version_type);
    for(temp_num = 0;temp_num < table_num;temp_num++)
    {
         if(s_board_hw_version_table[temp_num].mach_type == machine_arch_type )
         {
             memcpy(s_board_id,s_board_hw_version_table[temp_num].s_board_id, BOARD_ID_LEN-1);
             memcpy(hw_version_id,s_board_hw_version_table[temp_num].hw_version_id, HW_VERSION-1);
             break;
         }
    }

    if(table_num == temp_num)
    {
        memcpy(s_board_id,"ERROR", (BOARD_ID_LEN-1));
        memcpy(hw_version_id,"ERROR", HW_VERSION-1);
    }
}

void set_s_product_version(char *s_product_name)
{  
    int board_id           = machine_arch_type - MACH_ID_START_NUM;
    unsigned int temp_num  = 0;
    unsigned int table_num = 0;
    char product_ver[16]   = {0};
    hw_product_sub_type product_sub_type = get_hw_sub_board_id();

    if ((NULL == s_product_name) )
    {
         printk("app_info : s_product_name is null!\n");    
         return ;
    }

    table_num = sizeof(s_board_hw_version_table)/sizeof(s_board_hw_version_type);
    for(temp_num = 0;temp_num < table_num;temp_num++)
    {
         if(s_board_hw_version_table[temp_num].mach_type == machine_arch_type )
         {
             memcpy(s_product_name,s_board_hw_version_table[temp_num].s_board_id, BOARD_ID_LEN-1);
             break;
         }
    }

    if(table_num == temp_num)
    {
        memcpy(s_product_name, "ERROR", (BOARD_ID_LEN-1));
    }
    
    printk("app_info : board_id:0x%X , sub_id:%d\n", board_id, product_sub_type);    
    if ( IS_8X25Q_UMTS(board_id) )
    {   
        printk("app_info 2 : sub: %d\n", (product_sub_type & HW_VER_PRODUCT_MASK));
        if ( HW_VER_SUB_V0 == (product_sub_type & HW_VER_PRODUCT_MASK) )
        {
            sprintf(product_ver, "%s", "D");
        }
        else if ( HW_VER_SUB_V4 == (product_sub_type & HW_VER_PRODUCT_MASK) ) 
        {
            sprintf(product_ver, "%s", "-1");
        }
        else if ( HW_VER_SUB_V8 == (product_sub_type & HW_VER_PRODUCT_MASK) ) 
        {
            sprintf(product_ver, "%s", "-51");
        }    
        strcat(s_product_name, product_ver);
    }

}
/*===========================================================================


FUNCTION     set_s_board_hw_version_special

DESCRIPTION
  This function deal with special hw_version_id s_board_id and so on
DEPENDENCIES
  
RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
static void set_s_board_hw_version_special(char *hw_version_id,char *hw_version_sub_ver,
                                char *s_board_id,char *sub_ver)
{
                                             
    if ((NULL == s_board_id) || (NULL == sub_ver) || (NULL == hw_version_id) || (NULL == hw_version_sub_ver))
    {
         printk("app_info : parameter pointer is null!\n");    
         return ;
    }

}

/* same as in proc_misc.c */
static int
proc_calc_metrics(char *page, char **start, off_t off, int count, int *eof, int len)
{
	if (len <= off + count)
		*eof = 1;
	*start = page + off;
	len -= off;
	if (len > count)
		len = count;
	if (len < 0)
		len = 0;
	return len;
}

#define ATAG_BOOT_READ_FLASH_ID 0x4d534D72
static int __init parse_tag_boot_flash_id(const struct tag *tag)
{
    char *tag_flash_id=(char*)&tag->u;
    memset(str_flash_nand_id, 0, PROC_MANUFACTURER_STR_LEN);
    memcpy(str_flash_nand_id, tag_flash_id, PROC_MANUFACTURER_STR_LEN);
    
    printk("########proc_misc.c: tag_boot_flash_id= %s\n", tag_flash_id);

    return 0;
}
__tagtable(ATAG_BOOT_READ_FLASH_ID, parse_tag_boot_flash_id);

/*parse atag passed by appsboot, ligang 00133091, 2009-4-13, start*/
#define ATAG_BOOT_VERSION 0x4d534D71 /* ATAG BOOT VERSION */
static int __init parse_tag_boot_version(const struct tag *tag)
{
    char *tag_boot_ver=(char*)&tag->u;
    memset(appsboot_version, 0, MAX_VERSION_CHAR + 1);
    memcpy(appsboot_version, tag_boot_ver, MAX_VERSION_CHAR);
     
    //printk("nand_partitions.c: appsboot_version= %s\n\n", appsboot_version);

    return 0;
}
__tagtable(ATAG_BOOT_VERSION, parse_tag_boot_version);


#define ATAG_CAMERA_ID 0x4d534D74
static int __init parse_tag_camera_id(const struct tag *tag)
{
    char *tag_boot_ver=(char*)&tag->u;
	
    memcpy((void*)&camera_id, tag_boot_ver, sizeof(u32));
     
    return 0;
}
__tagtable(ATAG_CAMERA_ID, parse_tag_camera_id);


#define ATAG_TS_ID 0x4d534D75
static int __init parse_tag_ts_id(const struct tag *tag)
{
    char *tag_boot_ver=(char*)&tag->u;
	
    memcpy((void*)&ts_id, tag_boot_ver, sizeof(u32));
     
    return 0;
}


__tagtable(ATAG_TS_ID, parse_tag_ts_id);

extern char back_camera_name[];
extern char front_camera_name[];
static int app_version_read_proc(char *page, char **start, off_t off,
				 int count, int *eof, void *data)
{
	int len;
	char *lcd_name = NULL;
	char * touch_info = NULL;
	char * battery_name = NULL;
	char *wifi_device_name = NULL;
	char *bt_device_name = NULL;
	char audio_property[AUDIO_PROPERTY_LEN] = {0};
	char s_board_id[BOARD_ID_LEN + BOARD_ID_SUB_VER_LEN] = {0};
    char s_product_name[BOARD_ID_LEN + BOARD_ID_SUB_VER_LEN] = {0};
    char sub_ver[BOARD_ID_SUB_VER_LEN] = {0};
	char hw_version_id[HW_VERSION + HW_VERSION_SUB_VER] = {0};
	char hw_version_sub_ver[HW_VERSION_SUB_VER] = {0};	
	char *compass_gs_name = NULL;
	char *sensors_list_name = NULL;

    set_s_board_hw_version(s_board_id,hw_version_id);
    set_s_product_version(s_product_name);
    sprintf(sub_ver, ".Ver%X", (char)get_hw_sub_board_id());
    sprintf(hw_version_sub_ver, "VER.%X", (char)get_hw_sub_board_id());
    set_s_board_hw_version_special(hw_version_id,hw_version_sub_ver,s_board_id,sub_ver);
	compass_gs_name=get_compass_gs_position_name();
	sensors_list_name = get_sensors_list_name();
	lcd_name = get_lcd_panel_name();
	wifi_device_name = get_wifi_device_name();
	bt_device_name = get_bt_device_name();
	get_audio_property(audio_property);

	touch_info = get_touch_info();
	if (touch_info == NULL)
	{
		touch_info = "Unknow touch";
	}

	battery_name = get_battery_manufacturer_info();
	if (NULL == battery_name)
	{
		battery_name = "Unknown battery";
	}
	
#ifdef CONFIG_HUAWEI_POWER_DOWN_CHARGE
    charge_flag = get_charge_flag();
	/*export dcdc_type to userspace */
	len = snprintf(page, PAGE_SIZE, 
	"board_id:\n%s\n"
    "sub_board_id:\n%s\n"
    "product_name:\n%s\n"
	"lcd_id:\n%s\n"
	"External_camera:\n%s\n"
	"Internal_camera:\n%s\n"
	"ts_id:\n%d\n"
	"charge_flag:\n%d\n"
	"compass_gs_position:\n%s\n"
	"sensors_list:\n%s\n"
	"hw_version:\n%s\n"
    "wifi_chip:\n%s\n"
    "bt_chip:\n%s\n"
	"audio_property:\n%s\n"
	"touch_info:\n%s\n"
	"battery_id:\n%s\n"
	"dcdc_type:\n%s\n",
	 s_board_id, sub_ver, s_product_name, lcd_name, back_camera_name, front_camera_name, ts_id,charge_flag, compass_gs_name,sensors_list_name, hw_version_id,wifi_device_name, bt_device_name, audio_property, touch_info, battery_name, ((NULL == dcdc_type) ? "unknown" : dcdc_type));

#else
	len = snprintf(page, PAGE_SIZE, "APPSBOOT:\n"
	"%s\n"
	"KERNEL_VER:\n"
	"%s\n"
	 "FLASH_ID:\n"
	"%s\n"
	"board_id:\n%s\n"
	"lcd_id:\n%s\n"
	"cam_id:\n%d\n"
	"ts_id:\n%d\n"
	"compass_gs_position:\n%s\n"
	"sensors_list:\n%s\n"
	"hw_version:\n%s\n"
	"audio_property:\n%s\n"
	"touch_info:\n%s\n"
	"battery_id:\n%s\n",
	appsboot_version, ker_ver, str_flash_nand_id, s_board_id, lcd_name, camera_id, ts_id, compass_gs_name,sensors_list_name, hw_version_id,audio_property, touch_info, battery_name);
#endif
	
	return proc_calc_metrics(page, start, off, count, eof, len);
}
void __init proc_app_info_init(void)
{
	static struct {
		char *name;
		int (*read_proc)(char*,char**,off_t,int,int*,void*);
	} *p, simple_ones[] = {
		
        {"app_info", app_version_read_proc},
		{NULL,}
	};
	for (p = simple_ones; p->name; p++)
		create_proc_read_entry(p->name, 0, NULL, p->read_proc, NULL);

}


