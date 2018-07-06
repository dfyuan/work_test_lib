
/******************** (C) COPYRIGHT 2013 sunnyoptical *************************
* File Name          : motor.c
* Author             : zoom camera r&d team
* Version            : V1.0.0
* Date               : Apr.16, 2013
* Description        : motor driver
*******************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <pthread.h>

#include "motor.h"
#include "curve.h"

#define PI_EN_PIN 33
#define PI_ZOOM_PIN 11
#define PI_FOCUS_PIN 12
#define VD_IS_PIN 13
#define VD_FZ_PIN 14
#define GPIO_17 17
#define GPIO_18 18

static pthread_mutex_t mutex_az = PTHREAD_MUTEX_INITIALIZER;
#define AF_MOVE_MAX	20
#define ZM_MOVE_MAX	10

#define ZM_PI gpio_get_value(PI_ZOOM_PIN)
#define AF_PI gpio_get_value(PI_FOCUS_PIN)

#define MOTOR_DEBUG



#ifdef MOTOR_DEBUG
#define motor_debug printf
#else
#define motor_debug(...)
#endif

extern int mFd;
//1.5m
int focus_pos_2m[ZM_MAXSTEPS] = {
-336,-335,-335,-335,-334,-334,-333,-334,-333,-333,-332,-332,-332,-331,-331,-330,-330,-330,-329,-329,-329,-328,-328,-327,-327,-327,-326,-326,-326,-325,-325,-325,-324,-324,-323,-323,-323,-322,-322,-322,-321,-321,-321,-320,-320,-320,-319,-319,-320,-318,-318,-318,-317,-317,-316,-316,-315,-315,-314,-314,-314,-313,-313,-313,-313,-312,-311,-311,-311,-311,-310,-310,-309,-309,-308,-308,-307,-307,-307,-307,-306,-306,-306,-305,-305,-305,-304,-304,-303,-303,-302,-302,-302,-301,-301,-301,-300,-300,-299,-299,-299,-298,-298,-298,-297,-297,-296,-296,-296,-295,-295,-294,-294,-294,-288,-293,-292,-292,-292,-292,-291,-291,-290,-290,-289,-289,-289,-288,-288,-287,-287,-287,-286,-286,-285,-285,-285,-284,-283,-283,-283,-283,-282,-282,-281,-281,-281,-280,-280,-279,-279,-279,-278,-278,-278,-277,-277,-276,-276,-275,-275,-275,-274,-274,-273,-273,-273,-272,-271,-271,-271,-270,-270,-270,-269,-269,-268,-268,-268,-267,-267,-266,-266,-266,-265,-265,-264,-264,-263,-263,-263,-262,-261,-262,-261,-261,-260,-260,-259,-259,-258,-258,-258,-257,-257,-256,-256,-256,-255,-255,-254,-254,-254,-253,-253,-252,-252,-252,-251,-251,-250,-250,-249,-249,-249,-248,-247,-247,-247,-246,-246,-246,-245,-245,-244,-244,-243,-243,-242,-242,-242,-241,-240,-241,-240,-240,-239,-239,-238,-238,-237,-237,-236,-236,-236,-235,-234,-234,-233,-233,-233,-231,-232,-232,-231,-231,-230,-230,-229,-229,-228,-228,-227,-227,-227,-226,-226,-225,-225,-225,-224,-224,-223,-223,-222,-222,-221,-221,-220,-220,-219,-219,-219,-218,-218,-217,-217,-217,-216,-216,-215,-218,-214,-214,-214,-214,-213,-212,-212,-212,-211,-210,-210,-209,-209,-204,-208,-208,-207,-207,-206,-206,-206,-205,-204,-204,-203,-203,-203,-200,-202,-201,-201,-200,-200,-199,-199,-198,-198,-197,-197,-197,-196,-196,-195,-195,-194,-194,-193,-193,-192,-192,-191,-191,-192,-190,-189,-189,-188,-188,-187,-187,-186,-186,-185,-185,-184,-184,-183,-183,-183,-182,-182,-181,-181,-180,-180,-179,-179,-179,-178,-177,-177,-177,-176,-176,-175,-175,-174,-174,-173,-173,-175,-172,-171,-171,-170,-170,-169,-169,-169,-168,-167,-167,-166,-166,-165,-165,-164,-164,-164,-163,-163,-162,-162,-161,-161,-160,-160,-159,-158,-158,-158,-157,-156,-156,-155,-155,-154,-154,-153,-153,-152,-152,-151,-151,-150,-150,-149,-149,-148,-148,-147,-147,-146,-146,-145,-145,-144,-144,-143,-143,-142,-142,-141,-141,-141,-140,-139,-139,-138,-138,-137,-137,-137,-136,-135,-135,-134,-134,-133,-133,-132,-132,-131,-131,-130,-130,-129,-129,-128,-128,-127,-127,-126,-126,-125,-125,-124,-124,-123,-123,-122,-121,-121,-120,-120,-119,-119,-118,-118,-117,-117,-116,-116,-115,-114,-114,-113,-113,-112,-112,-111,-111,-110,-110,-109,-109,-108,-108,-107,-107,-106,-106,-105,-105,-104,-104,-103,-102,-102,-101,-100,-100,-99,-99,-98,-98,-97,-97,-96,-96,-95,-95,-94,-94,-93,-93,-92,-92,-91,-90,-90,-90,-89,-89,-88,-87,-86,-86,-85,-85,-84,-84,-84,-83,-82,-82,-81,-81,-80,-80,-79,-78,-78,-78,-77,-76,-75,-75,-74,-74,-73,-73,-72,-72,-71,-71,-70,-69,-69,-68,-67,-67,-66,-66,-65,-65,-64,-64,-63,-62,-62,-61,-60,-60,-60,-59,-59,-58,-57,-57,-56,-56,-55,-54,-54,-54,-53,-52,-51,-51,-51,-50,-49,-49,-48,-48,-47,-47,-46,-46,-45,-44,-43,-43,-42,-42,-41,-41,-40,-39,-39,-38,-38,-37,-36,-36,-35,-35,-34,-34,-33,-33,-32,-31,-31,-30,-29,-29,-28,-28,-27,-27,-26,-25,-24,-24,-24,-23,-22,-22,-21,-21,-20,-19,-18,-18,-17,-17,-16,-15,-15,-15,-14,-14,-13,-12,-11,-11,-10,-10,-9,-9,-8,-7,-7,-6,-6,-5,-4,-4,-3,-3,-2,-1,-1,0,1,1,2,2,3,4,4,5,6,6,7,7,8,9,9,10,10,11,12,12,13,14,14,15,15,16,17,17,18,18,19,19,20,21,22,22,23,23,24,25,25,26,26,27,28,28,29,29,30,30,31,32,33,33,34,35,35,36,36,37,38,38,39,39,40,40,41,42,42,43,44,45,45,46,46,47,48,48,49,49,50,50,51,52,53,53,54,54,56,56,57,57,58,58,59,59,60,61,62,62,63,63,64,64,65,66,66,67,68,68,69,69,70,70,71,71,72,73,74,74,75,75,76,76,78,78,79,79,80,80,81,82,83,83,83,84,85,86,86,87,87,88,89,90,90,90,92,92,93,93,94,94,95,96,97,97,98,98,99,99,100,101,102,102,102,103,103,104,105,106,106,107,107,108,108,109,109,110,111,111,112,113,113,114,114,115,116,116,117,117,118,119,119,119,120,121,122,122,123,124,124,124,125,126,126,126,127,128,129,129,130,130,131,131,132,133,133,134,134,135,136,136,137,137,138,138,139,140,140,
140,141,142,142,142,143,143,144,145,146,146,146,147,148,148,149,149,149,150,151,151,152,152,153,153,154,154,155,155,155,156,157,157,158,158,159,159,159,160,161,161,162,162,163,163,164,164,165,165,166,165,166,167,167,168,168,169,169,170,170,170,171,171,171,172,172,173,173,173,174,174,175,175,176,176,176,177,177,178,178,178,178,179,179,179,180,181,181,181,182,182,182,182,183,183,183,184,184,185,185,185,186,185,186,186,186,186,187,187,188,188,188,188,188,189,190,189,190,189,190,190,190,190,191,191,191,191,192,192,192,192,192,182,187,177,180,190,180,170,160,167,177,167,177,167,177,167,177,187,194,194,194,194,195,194,194,194,194,194,194,194,194,194,194,194,194,194,194,194,192,191,201,193,183,183,183,190,191,191,191,189,192,192,193,191,192,191,192,195,197,189,199,192,183,187,196,190,190,188,191,188,187,177,183,173,163,153,163,173,183,173,163,153,163,173,163,173,163,173,163,172,162,152,162,152,142,132,123,123,123,123,125,135,145,155,165,171,170,170,167,168,166,165,166,165,164,163,163,162,161,161,159,159,158,157,156,155,153,153,152,152,150,149,148,148,146,146,144,143,142,141,140,139,138,137,135,134,133,132,130,129,128,127,125,124,122,122,120,118,117,116,114,113,112,111,109,107,106,105,102,102,99,98,96,97,91,92,90,88,86,85,82,81,79,77,82,73,72,70,68,66,64,62,60,59,56,55,52,50,48,47,44,42,40,37,35,33,30,29,26,24,21,20,16,15,11,10,7,6,2,-1,-4,-6,-9,-10,-14,-16,-19,-21,-24,-27,-30,-32,-35,-38,-42,-44,-47,-50,-53,-55,-60,-62,-65,-67,-71,-74,-78,-80,-85,-87,-91,-94,-98,-100,-105,-107,-112,-114,-119,-122,-127,-129,-134,-136,-141,-144,-149,-151,-157,-159,-164,-167,-171,-175,-180,-183,-189,-192,-197,-200,-206,-209,-215,-218,-223,-227,-233,-236,-242,-245,-251,-255,-261,-264,-270,-274,-281,-284,-291,-295,-302,-305};
//10m
int focus_pos[ZM_MAXSTEPS] = {
-336,-335,-335,-334,-334,-334,-333,-333,-333,-332,-332,-331,-331,-331,-330,-330,-330,-329,-329,-329,-328,-328,-327,-327,-327,-326,-326,-326,-325,-325,-324,-324,-324,-323,-323,-323,-322,-322,-322,-321,-321,-320,-320,-320,-319,-319,-319,-318,-318,-317,-317,-317,-316,-316,-316,-315,-315,-314,-314,-314,-313,-313,-313,-312,-312,-311,-311,-311,-310,-310,-310,-309,-309,-308,-308,-308,-307,-307,-306,-306,-306,-305,-305,-305,-304,-304,-303,-303,-303,-302,-302,-302,-301,-301,-300,-300,-300,-299,-299,-298,-298,-298,-297,-297,-297,-296,-296,-295,-295,-295,-294,-294,-293,-293,-293,-292,-292,-291,-291,-291,-290,-290,-289,-289,-289,-288,-288,-287,-287,-287,-286,-286,-285,-285,-285,-284,-284,-283,-283,-283,-282,-282,-281,-281,-281,-280,-280,-279,-279,-279,-278,-278,-277,-277,-277,-276,-276,-275,-275,-275,-274,-274,-273,-273,-273,-272,-272,-271,-271,-271,-270,-270,-269,-269,-268,-268,-268,-267,-267,-266,-266,-266,-265,-265,-264,-264,-264,-263,-263,-262,-262,-261,-261,-261,-260,-260,-259,-259,-259,-258,-258,-257,-257,-257,-256,-256,-255,-255,-254,-254,-254,-253,-253,-252,-252,-251,-251,-251,-250,-250,-249,-249,-248,-248,-248,-247,-247,-246,-246,-246,-245,-245,-244,-244,-243,-243,-243,-242,-242,-241,-241,-240,-240,-240,-239,-239,-238,-238,-237,-237,-237,-236,-236,-235,-235,-234,-234,-234,-233,-233,-232,-232,-231,-231,-230,-230,-230,-229,-229,-228,-228,-227,-227,-226,-226,-226,-225,-225,-224,-224,-223,-223,-223,-222,-222,-221,-221,-220,-220,-219,-219,-219,-218,-218,-217,-217,-216,-216,-215,-215,-215,-214,-214,-213,-213,-212,-212,-211,-211,-210,-210,-210,-209,-209,-208,-208,-207,-207,-206,-206,-205,-205,-205,-204,-204,-203,-203,-202,-202,-201,-201,-200,-200,-200,-199,-199,-198,-198,-197,-197,-196,-196,-195,-195,-195,-194,-194,-193,-193,-192,-192,-191,-191,-190,-190,-189,-189,-188,-188,-188,-187,-187,-186,-186,-185,-185,-184,-184,-183,-183,-182,-182,-181,-181,-180,-180,-179,-179,-179,-178,-178,-177,-177,-176,-176,-175,-175,-174,-174,-173,-173,-172,-172,-171,-171,-170,-170,-170,-169,-169,-168,-168,-167,-167,-166,-166,-165,-165,-164,-164,-163,-163,-162,-162,-161,-161,-160,-160,-159,-159,-158,-158,-157,-157,-156,-156,-155,-155,-154,-154,-153,-153,-152,-152,-151,-151,-150,-150,-149,-149,-148,-148,-147,-147,-146,-146,-145,-145,-144,-144,-143,-143,-142,-142,-141,-141,-140,-140,-139,-139,-138,-138,-137,-137,-136,-136,-135,-135,-134,-134,-133,-133,-132,-132,-131,-131,-130,-130,-129,-129,-128,-128,-127,-127,-126,-126,-125,-125,-124,-124,-123,-123,-122,-121,-121,-120,-120,-119,-119,-118,-118,-117,-117,-116,-116,-115,-115,-114,-114,-113,-113,-112,-112,-111,-111,-110,-109,-109,-108,-108,-107,-107,-106,-106,-105,-105,-104,-104,-103,-103,-102,-102,-101,-101,-100,-99,-99,-98,-98,-97,-97,-96,-96,-95,-95,-94,-94,-93,-93,-92,-92,-91,-91,-90,-89,-89,-88,-88,-87,-87,-86,-86,-85,-85,-84,-84,-83,-82,-82,-81,-81,-80,-80,-79,-79,-78,-78,-77,-76,-76,-75,-75,-74,-74,-73,-73,-72,-72,-71,-71,-70,-69,-69,-68,-68,-67,-67,-66,-66,-65,-64,-64,-63,-63,-62,-62,-61,-61,-60,-60,-59,-58,-58,-57,-57,-56,-56,-55,-55,-54,-53,-53,-52,-52,-51,-51,-50,-50,-49,-48,-48,-47,-47,-46,-46,-45,-44,-44,-43,-43,-42,-42,-41,-40,-40,-39,-39,-38,-38,-37,-36,-36,-35,-35,-34,-34,-33,-32,-32,-31,-31,-30,-30,-29,-29,-28,-27,-27,-26,-26,-25,-25,-24,-23,-23,-22,-22,-21,-21,-20,-19,-19,-18,-18,-17,-17,-16,-15,-15,-14,-14,-13,-13,-12,-11,-11,-10,-10,-9,-9,-8,-7,-7,-6,-6,-5,-5,-4,-3,-3,-2,-2,-1,-1,0,1,1,2,2,3,3,4,5,5,6,6,7,7,8,9,9,10,10,11,12,12,13,13,14,14,15,16,16,17,17,18,18,19,20,20,21,21,22,22,23,24,24,25,25,26,26,27,28,28,29,29,30,30,31,32,32,33,33,34,34,35,36,36,37,37,38,38,39,40,40,41,41,42,42,43,44,44,45,45,46,46,47,48,48,49,49,50,50,51,51,52,53,53,54,54,55,55,56,56,57,58,58,59,59,60,60,61,61,62,63,63,64,64,65,65,66,66,67,68,68,69,69,70,70,71,72,72,73,73,74,74,75,75,76,76,77,77,78,78,79,79,80,80,81,81,82,82,83,84,84,85,85,86,86,87,87,88,88,89,89,90,90,91,91,92,92,93,93,94,94,95,96,96,97,97,98,98,99,99,99,100,100,101,101,102,102,103,103,104,104,104,105,105,106,106,107,107,108,108,108,109,109,110,110,111,111,112,112,113,113,113,114,114,115,115,116,116,117,117,118,118,118,119,119,120,120,121,
121 ,122,122,122,123,123,123,124,124,125,125,125,126,126,127,127,127,128,128,128,129,129,130,130,130,131,131,131,132,132,133,133,133,134,134,134,134,135,135,135,136,136,136,136,137,137,137,137,138,138,138,138,139,139,139,140,140,140,140,141,141,141,141,142,142,142,142,142,142,143,143,143,143,143,143,144,144,144,144,144,144,144,145,145,145,145,145,145,145,146,146,146,146,146,146,146,146,146,146,146,146,146,146,146,146,146,146,146,146,146,146,146,146,146,146,146,146,146,146,146,145,145,145,145,145,145,145,145,145,145,145,145,145,144,144,144,144,144,143,143,143,143,143,142,142,142,142,142,141,141,141,140,140,140,139,139,139,138,138,138,137,137,136,136,136,135,135,134,134,133,132,132,131,131,130,129,129,128,128,127,127,126,125,125,124,124,123,123,122,121,121,120,120,119,119,118,117,117,116,115,114,113,112,111,110,109,108,107,106,105,104,103,102,102,101,100,99,98,97,96,95,94,93,92,91,90,89,88,87,86,85,84,83,82,81,80,79,78,77,76,74,72,71,69,67,66,64,63,61,59,58,56,54,53,51,50,48,46,45,43,41,40,38,36,35,33,32,30,28,27,25,23,22,20,19,17,15,14,12,10,9,7,5,4,1,-1,-4,-6,-9,-11,-14,-17,-19,-22,-24,-27,-29,-32,-35,-37,-40,-42,-45,-47,-50,-52,-55,-58,-60,-63,-65,-68,-70,-73,-75,-78,-81,-83,-86,-88,-91,-93,-96,-98,-101,-104,-106,-109,-111,-114,-116,-119,-122,-124,-128,-131,-135,-138,-142,-146,-148,-149,-153,-156,-160,-164,-167,-171,-174,-178,-181,-185,-189,-192,-196,-199,-203,-207,-210,-214,-218,-222,-227,-231,-236,-240,-244,-249,-253,-257,-262,-266,-271,-275,-279,-284,-289,-295,-300,-305,-310,-315,-320,-325,-330,-335,-340,-345,-351,-356,-361,-366,-371,-376,-381,-387,-393,-399,-405,-412,-418,-424,-430,-436,-443,-449,-456,-464,-471,-478,-485,-493,-500,-507,-514,-522,-530,-539,-547,-556,-564,-572,-581,-589,-598,-606,-615,-623,};
azpos focus_offset;

// static 
static motor_t cam_motors;
static motor_s motor_stat;

static void set_reg(u8 reg, u16 val)
{
	int ret = 0;
	AN41908_INFO_T stAN41908_info;
	stAN41908_info.regAddr = reg;
	stAN41908_info.regVal = val;
	ret = ioctl(mFd, CMD_SET, &stAN41908_info);
    if(ret < 0)
    {
        perror("ioctl file failed\n");
    }
}

static u8 get_gpio_val(int gpio)
{
	int ret = 0;
	AN41908_GPIO_T stAn41908_gpio;

	memset(&stAn41908_gpio, 0, sizeof(AN41908_GPIO_T));
	stAn41908_gpio.gpio = gpio;
	ret = ioctl(mFd, CMD_GPIO_GET, &stAn41908_gpio);
    if(ret < 0)
    {
        perror("ioctl file failed\n");
    }
	return stAn41908_gpio.val;
}

static void generate_vd_pulse(void)
{
	int ret = 0;

	ret = ioctl(mFd, CMD_GENERATE_VD_PULSE, NULL);
    if(ret < 0)
    {
        perror("ioctl file failed\n");
    }
}

static void mt_cmd_create(u16 af_stp, u16 af_direct, u16 af_speed, u16 zm_stp, u16 zm_direct,u16 zm_speed)
{	 
	int zm_steps;
	int af_steps;

	//af control  move
	if (af_stp == 0 && zm_stp == 0 )
		return;

	af_speed = (af_speed+1) * 20;
	zm_speed = (zm_speed+1) * 20;
		
	af_stp = af_stp & 0x00ff;
	if (af_stp != 0)
	{
		if (af_direct == 0)
		{
			af_steps= af_stp | 0x3500; 
		}
		else 
		{
			 af_steps= af_stp | 0x3400;
		}

		if(zm_stp == 0)
		{
			set_reg(0x24,af_steps);
			set_reg(0x25,af_speed);

			set_reg(0x29,0x3500);
		}
		else
		{
			set_reg(0x24,af_steps);
			set_reg(0x25,af_speed);
		}
	}
	
	//zoom comtrol move
	zm_stp= zm_stp & 0x00ff;
	if (zm_stp != 0)
	{
		if (zm_direct == 1)
		{
			zm_steps= zm_stp | 0x3500;
		}
		else 
		{
			 zm_steps= zm_stp |0x3400;
		}
		if(af_stp ==0)
		{
			set_reg(0x29,zm_steps);	
			set_reg(0x2a,zm_speed); 
			
			set_reg(0x24,0x3400);
		}
		else
		{
			set_reg(0x29,zm_steps);	
			set_reg(0x2a,zm_speed); 
		}
	}
	generate_vd_pulse();

	return;
}

motor_stat_t motor_stat_get(void)
{
	int ret = 0;

	ret = ioctl(mFd, CMD_GET_MOTORSTAT, &motor_stat);
    if(ret < 0)
    {
        perror("ioctl file failed\n");
    }
	
	return motor_stat.motor_stat;
}

void az_motor_wait_stop(void)
{
	u32 count = 0;
	
	while(motor_stat_get() == run_flag)
	{
		count++;
		if(count > 100) //3 //three secs
		{
			motor_debug("######Motor runing overtime######\n\r");			 
			break;
		}
		msleep(5);
	}
}

static void az_motor_drive(s16 af, u16 af_speed, s16 zm, s16 zm_speed, u16 flag)
{
	int af_steps, zm_steps;
	
	af_steps = af;
	zm_steps = zm;
	
	if((0 == af_steps) && (0 == zm_steps))
		return; 

	if(af_steps > AF_MOVE_MAX)
	{
		af_steps = AF_MOVE_MAX;
	}
	else if(af_steps < -AF_MOVE_MAX)
	{
		af_steps = -AF_MOVE_MAX;
	}

	if(zm_steps > ZM_MOVE_MAX)
	{
		zm_steps = ZM_MOVE_MAX;
	}
	else if(zm_steps < -ZM_MOVE_MAX)
	{
		zm_steps = -ZM_MOVE_MAX;
	}

	cam_motors.af_pos_des = cam_motors.af_pos_cur + af_steps;
	cam_motors.zm_pos_des = cam_motors.zm_pos_cur + zm_steps;

	if(cam_motors.af_pos_des > AF_POS_MAX)
	{
		cam_motors.af_pos_des = AF_POS_MAX;
	}
	else if(cam_motors.af_pos_des < AF_POS_MIN)
	{
		cam_motors.af_pos_des = AF_POS_MIN;
	}

	 if(cam_motors.zm_pos_des > ZM_POS_MAX)
	{
		cam_motors.zm_pos_des = ZM_POS_MAX;
	}
	else if(cam_motors.zm_pos_des < ZM_POS_MIN)
	{
		cam_motors.zm_pos_des = ZM_POS_MIN;
	}
	
	af_steps = cam_motors.af_pos_des - cam_motors.af_pos_cur;
	zm_steps = cam_motors.zm_pos_des - cam_motors.zm_pos_cur;
	
	if(af_steps >= 0)
		cam_motors.af_mv_direct = 1; 
	else
		cam_motors.af_mv_direct = 0; 
	
	if(zm_steps >= 0)
		cam_motors.zm_mv_direct = 0; 
	else
		cam_motors.zm_mv_direct = 1; 


	af_steps = abs(af_steps);
	zm_steps = abs(zm_steps);

	af_steps <<= 2;
	zm_steps <<= 3;

	mt_cmd_create(af_steps, cam_motors.af_mv_direct, af_speed,	zm_steps, cam_motors.zm_mv_direct, zm_speed);
	
	if (af_steps)
	{
		cam_motors.af_run_flag = run_flag;
		cam_motors.motor_stat = run_flag;
	}

	if (zm_steps)
	{
		cam_motors.zm_run_flag = run_flag;
		cam_motors.motor_stat = run_flag;
	}
	
	if(flag)
	{
		cam_motors.af_pos_cur = cam_motors.af_pos_des;
		cam_motors.zm_pos_cur = cam_motors.zm_pos_des;
		//motor_debug("%d; cam_motors.af_pos_cur=%d cam_motors.zm_pos_cur=%d \n ", __LINE__,cam_motors.af_pos_cur,cam_motors.zm_pos_cur);
	}
	else
	{
		az_motor_wait_stop();
	}
	
	return;
}

void az_motor_goto_pos(s16 af, u16 af_speed, s16 zm, u16 zm_speed)
{
	int af_pos_des, zm_pos_des;
	int af_steps, zm_steps, af_move_counts, zm_move_counts;
	
	af_pos_des = af;
	zm_pos_des = zm;
	
	if (af_pos_des > AF_POS_MAX)
		af_pos_des = AF_POS_MAX;
	
	if(af_pos_des < AF_POS_MIN)
		af_pos_des = AF_POS_MIN;
	
	if (zm_pos_des > ZM_POS_MAX)
		zm_pos_des = ZM_POS_MAX;
	
	if (zm_pos_des < ZM_POS_MIN)
		zm_pos_des = ZM_POS_MIN;
	
	if(motor_stat_get()==run_flag)
	{
		az_motor_wait_stop();
	}
	
	af_move_counts = af_pos_des - cam_motors.af_pos_cur;
	zm_move_counts = zm_pos_des - cam_motors.zm_pos_cur;
	

	while((af_move_counts != 0) || (zm_move_counts != 0))
	{
		if(af_move_counts > AF_MOVE_MAX)
		{
			af_steps = AF_MOVE_MAX;
		}
		else if(af_move_counts < -AF_MOVE_MAX)
		{
			af_steps = -AF_MOVE_MAX;
		}
		else 
		{
			af_steps = af_move_counts;
		}
		   
		if(zm_move_counts > ZM_MOVE_MAX)
		{
			zm_steps = ZM_MOVE_MAX;
		}
		else if(zm_move_counts < -ZM_MOVE_MAX)
		{
			zm_steps = -ZM_MOVE_MAX;
		}
		else
		{
			zm_steps = zm_move_counts;
		}

		az_motor_drive(af_steps, af_speed, zm_steps, zm_speed, 1);
		az_motor_wait_stop();
		af_move_counts = af_pos_des -cam_motors.af_pos_cur;
		zm_move_counts = zm_pos_des -cam_motors.zm_pos_cur;
	}
	//motor_debug("cam_motors.af_pos_cur=%d cam_motors.zm_pos_cur=%d \n ", cam_motors.af_pos_cur,cam_motors.zm_pos_cur); 
	return;
}

void az_pos_wait_get(s16 *af, s16 *zm)
{
	az_motor_wait_stop();
    
	*af = cam_motors.af_pos_cur;
	*zm = cam_motors.zm_pos_cur;
    
	return;
}

s16 af_pos_wait_get(void)
{
	az_motor_wait_stop();
	return(cam_motors.af_pos_cur);
}

s16 zm_pos_wait_get(void)
{
	az_motor_wait_stop();
	return(cam_motors.zm_pos_cur);
}

s16 af_pos_no_wait_get(void)
{
	return(cam_motors.af_pos_cur);
}

s16 zm_pos_no_wait_get(void)
{
	return(cam_motors.zm_pos_cur);
}

/*ÈÕÒ¹ÇÐ»»AF²¹³¥*/
void icr_af_compensation(s16 af, s16 zm)
{
	az_motor_wait_stop();
	az_motor_drive(af, 15, zm, 8, 0);
    
	return;
}

void az_motor_stop(void)
{
	msleep(20);
  	set_reg(0x24,0x3500);
  	set_reg(0x29,0x3500);
  	generate_vd_pulse();
	
	pthread_mutex_lock(&mutex_az);
	cam_motors.af_run_flag = stop_flag;
	cam_motors.zm_run_flag = stop_flag;
	cam_motors.motor_stat = stop_flag;
	pthread_mutex_unlock(&mutex_az);
	return;
}

void af_move_finish(void)
{
	pthread_mutex_lock(&mutex_az);
	cam_motors.af_run_flag = stop_flag;
	if(cam_motors.zm_run_flag == stop_flag)
		cam_motors.motor_stat = stop_flag;
	pthread_mutex_unlock(&mutex_az);
	return;
}

void zm_move_finish(void)
{
	pthread_mutex_lock(&mutex_az);
	cam_motors.zm_run_flag = stop_flag;
	if(cam_motors.af_run_flag == stop_flag)
		cam_motors.motor_stat = stop_flag;
	pthread_mutex_unlock(&mutex_az);
	return;
}

u16 motor_get_irisval(void)
{
	int ret = 0;
	u16 data;
	ret = ioctl(mFd, CMD_GET_IRISVAL, &data);
    if(ret < 0)
    {
        perror("ioctl file failed\n");
    }
	
	return data;
}
void  motor_set_irisval(u16 data)
{
	set_reg(0x00,data);
}

void pi_check(void)
{
	u8 tmp_pi_val = 0;
	int mv_steps = 0, zoom_move_count = 0, af_move_count = 0;

	//check zoom zero point
	tmp_pi_val = get_gpio_val(PI_ZOOM_PIN);
	mv_steps = tmp_pi_val ? PI_ZM_MAXSTEPS : -PI_ZM_MAXSTEPS;
	
	while(tmp_pi_val == get_gpio_val(PI_ZOOM_PIN))
	{
		az_motor_drive(0, AF_DEFAULT_SPEEDS, mv_steps, ZM_DEFAULT_SPEEDS, 0);
		zoom_move_count += mv_steps;
		if(abs(zoom_move_count) >=  ZM_MAXSTEPS)
		{
			motor_debug("ZM move steps exceed maximum zoom steps: %d\n\r", zoom_move_count);
			set_reg(0x29,0x3500);
			return;
		}
	}
	
	tmp_pi_val = get_gpio_val(PI_ZOOM_PIN);
	mv_steps = tmp_pi_val ? PI_ZM_MINSTEPS : -PI_ZM_MINSTEPS;
	while(tmp_pi_val == get_gpio_val(PI_ZOOM_PIN))
	{
		az_motor_drive(0, AF_DEFAULT_SPEEDS, mv_steps, ZM_DEFAULT_SPEEDS, 0);
		zoom_move_count += mv_steps;
	}

	tmp_pi_val = get_gpio_val(PI_ZOOM_PIN);
	if(!tmp_pi_val)
	{
		az_motor_drive(0, AF_DEFAULT_SPEEDS, -mv_steps, ZM_DEFAULT_SPEEDS, 0);
		zoom_move_count += -mv_steps;
	}
	motor_debug("============Zoom Check Finished!===========\n");
	
	//check focus zero point
	tmp_pi_val = get_gpio_val(PI_FOCUS_PIN);
	mv_steps = tmp_pi_val ? -PI_AF_MAXSTEPS : PI_AF_MAXSTEPS;
	
	while(tmp_pi_val == get_gpio_val(PI_FOCUS_PIN))
	{
		az_motor_drive(mv_steps, AF_DEFAULT_SPEEDS, 0, ZM_DEFAULT_SPEEDS, 0);
		af_move_count += mv_steps;
		if(abs(af_move_count) >=  AF_MAXSTEPS)
		{
			motor_debug("af move steps exceed maximum af steps: %d\n\r", af_move_count);
			set_reg(0x24,0x3500);
			return;
		}
	}
	
	tmp_pi_val = get_gpio_val(PI_FOCUS_PIN);
	mv_steps = tmp_pi_val ? -PI_AF_MINSTEPS : PI_AF_MINSTEPS;
	while(tmp_pi_val == get_gpio_val(PI_FOCUS_PIN))
	{
		az_motor_drive(mv_steps, AF_DEFAULT_SPEEDS, 0, ZM_DEFAULT_SPEEDS, 0);
		af_move_count += mv_steps;
	}

	tmp_pi_val = get_gpio_val(PI_FOCUS_PIN);
	if(!tmp_pi_val)
	{
		az_motor_drive(0, AF_DEFAULT_SPEEDS, -mv_steps, ZM_DEFAULT_SPEEDS, 0);
		af_move_count += -mv_steps;
	}
	
	memset(&cam_motors, 0, sizeof(motor_t));
	printf("============Focus Check Finished!==========\n");

// for af curve offset setting
	int i,af_offset_value,direct;
	af_offset_value =16;

	//af_offset_value = -3;
	if(af_offset_value>=0)
		direct = 1;
	else
		direct = -1;
	
	af_offset_value = abs(af_offset_value);
	for(i=0;i<((af_offset_value-1)/PI_AF_MAXSTEPS);i++)
    {
		az_motor_drive(direct*PI_AF_MAXSTEPS, 15, 0, 8, 0);
	}
	az_motor_drive(direct*(af_offset_value-i*PI_AF_MAXSTEPS),15, 0, 8, 0);
	
	printf("af_offset_value:%d\n\r",af_offset_value*direct);
}

int get_zoom_cur_pos(void)
{
	return cam_motors.zm_pos_cur;
}

int get_focus_cur_pos(void)
{
	return cam_motors.af_pos_cur;
}

void set_zoom_cur_pos(int zoom_pos)
{
	cam_motors.zm_pos_cur = zoom_pos;
	return ;
}

void set_focus_cur_pos(int focus_pos)
{
	cam_motors.af_pos_cur = focus_pos;
	return ;
}


