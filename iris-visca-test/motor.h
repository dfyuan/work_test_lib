#ifndef __MOTOR_H__
#define __MOTOR_H__

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "basetypes.h"

#define GPIO_ISP_RST    GPIO_Pin_8
#define DEV_AN41908 "/dev/misc_test"
// define
#define PI_AF_MAXSTEPS 5
#define PI_ZM_MAXSTEPS 5

#define PI_AF_MINSTEPS 1
#define PI_ZM_MINSTEPS 1

// for union 20X lens parameters
#define AF_MAXSTEPS     (1031)       /*step of focus:850+180+1*/
#define ZM_MAXSTEPS     (1374)      /*step of zoom (From Wide to Tele):1285+88+1*/
#define AF_POS_MAX      ( 180)	    /*focus PI to mechanical terminal of near side:794*/
#define AF_POS_MIN      (-850)	    /* focus PI to the Tele INF:106 ,focus PI to mechanical terminal of far side:204*/
#define ZM_POS_MAX      (1285)      /*Wide 1X to zoom PI:1285 ,Wide 1X to mechanical teminal:19*/
#define ZM_POS_MIN      (-88)      /*Tele 18X to zoom PI:181,Tele 18X to mechanical teminal:29*/

#define ZM_DEFAULT_SPEEDS 8
#define AF_DEFAULT_SPEEDS 8

#define AF_SPEED 	8
#define ZM_SPEED 	8

#define	MOTOR_IOC_MAXNR		7
#define	MOTOR_IOC_BASE		'T'

#define CMD_SET         		_IOWR(MOTOR_IOC_BASE, 1, struct AN41908_INFO_T*)
#define CMD_GPIO_GET 			_IOWR(MOTOR_IOC_BASE, 2, struct AN41908_INFO_T*)
#define CMD_GET_MOTORSTAT 		_IOWR(MOTOR_IOC_BASE, 3, struct AN41908_INFO_T*)
#define CMD_GENERATE_VD_PULSE 	_IOWR(MOTOR_IOC_BASE, 4, struct AN41908_INFO_T*)
#define CMD_GET_IRISVAL			_IOWR(MOTOR_IOC_BASE, 5, unsigned int*)

#define msleep(x) usleep(x * 1000)

typedef struct 
{
    u8 regAddr;
    u16 regVal;
}AN41908_INFO_T;

typedef struct 
{
    u8 gpio;
    u8 val;
}AN41908_GPIO_T;

typedef struct az_pos_str
{
	s16	zm;		//当前zoom位置
	s16	af; 	//当前AF位置
} azpos;

typedef enum
{
	stop_flag,
	run_flag,
    err_flag,
} motor_stat_t;

typedef struct
{
    motor_stat_t af_run_flag;
    motor_stat_t zm_run_flag;
    motor_stat_t motor_stat;
}motor_s;

typedef struct
{
    s16 af_pos_cur;
    s16 af_pos_des;
    u8  af_mv_direct;   /*0:forward direction; 1:reverse direction*/
    volatile motor_stat_t  af_run_flag;
    s16 zm_pos_cur;
    s16 zm_pos_des;
    u8  zm_mv_direct;  /*0:forward direction; 1:reverse direction*/
    volatile motor_stat_t  zm_run_flag;
    volatile motor_stat_t motor_stat;
} motor_t;

typedef struct af_zm_pos_str
{
	s16 zm_pos;
	s16	af_pos; 	//当前AF位置
} AfZmPos;

typedef enum 
{
	DAY,
	NIGHT,
	DN_NONE,
} dn_state_t;

extern azpos focus_offset;
extern int focus_pos[ZM_MAXSTEPS];

// motor.c
void icr_af_compensation(s16 af, s16 zm);
void az_pos_wait_get(s16 *af, s16 *zm);
s16 af_pos_wait_get(void);
s16 zm_pos_wait_get(void);
s16 af_pos_no_wait_get(void);
s16 zm_pos_no_wait_get(void);
void az_motor_goto_pos(s16 af, u16 af_speed, s16 zm, u16 zm_speed);
void az_motor_wait_stop(void);
motor_stat_t motor_stat_get(void);
void af_move_finish(void);
void zm_move_finish(void);
void pi_check(void);

u16 motor_get_irisval(void);
void  motor_set_irisval(u16 data);

int get_zoom_cur_pos(void);
int get_focus_cur_pos(void);
void set_zoom_cur_pos(int zoom_pos);
void set_focus_cur_pos(int focus_pos);




//test_image.c
typedef enum {
	STATIS_AWB = 0,
	STATIS_AE,
	STATIS_AF,
	STATIS_HIST,
	STATIS_AF_HIST,
} STATISTICS_TYPE;

struct af_data
{
 u32 sum_fv1;
 u32 sum_fv2;
};

int start_aaa_main(void);
//int get_statistics_data(STATISTICS_TYPE type);
void  get_statistics_data(STATISTICS_TYPE type,int window,unsigned char *data);
#ifdef __GNUC__
/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
   set to 'Yes') calls __io_putchar() */
   
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif

#endif