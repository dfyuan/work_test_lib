#include <linux/kernel.h>
#include <linux/fs.h>  
#include <linux/init.h>  
#include <linux/delay.h>  
#include <asm/uaccess.h>  
#include <asm/irq.h>  
#include <asm/io.h>
#include <asm-generic/gpio.h>
#include <linux/module.h>  
#include <linux/device.h>
#include <linux/gpio.h>
#include "ath204.h"
#include "sha256.h"

#define MEM_MAJOR 0
#define DEV_NAME "first_sha204"
#define CONFIG_GPIO_SEC_DATA 145 //gpio4_c1 
#define KEY_ID1   (0xFFFF)
#define KEY_ID2   (0x89AB)
#define SA_SUCCESS 0
//#define SA_FAIL -1
#define u8 unsigned int
#define s8 int
#define u16 unsigned long
#define s16 long
#define PIN_DIR_INPUT 0
#define PIN_DIR_OUTPUT 1
#define PIN_LVL_LOW 0
#define PIN_LVL_HIGH 1
#define FLAG_CLIENT_COMMAND   0x77 	//!< Tells the client that a command follows.
#define FLAG_CLIENT_TRANSMIT  0x88 	//!< Requests the client to send the result of last operation.
#define FLAG_SLEEP            0xCC 	//!< Requests the client to go to sleep.
#define WAKE_TOKEN   		  0xF0 	//!< byte value when sending wake token through UART
#define ZERO_BIT     		  0x7D 	//!< byte value when sending a 'zero' bit through UART
#define ONE_BIT      		  0x7F 	//!< byte value when sending a 'one' bit through UART
#define WAKE_SYNC			  0x11	//!< wakeup sync
#define WAKE_FLAG			  0x01	//!< flag: send wakeup token before command
#define SLEEP_FLAG			  0x02	//!< flag: send sleep flag after command
#define CHIP_DELAY(dd) {volatile unsigned int delay; for(delay=0; delay<dd; delay++);}
#define MAC_MODE 0x54

volatile unsigned long  *GPIO4_IOMUX_BASE = NULL;
volatile unsigned long  *GPIO4_CLK_BASE = NULL;
volatile unsigned long  *GPIO4_BASE = NULL;

#define GRF_GPIO4_IOMUX_OFFSET 	 0x0000
#define CRU_GPIO4_CLKGATE_OFFSET 0x0004
#define CRU_GPIO4_DIR_OFFSET 	 0x0004
#define CRU_GPIO4_DATA_OFFSET 	 0x0000

#define __raw_readll(a)   	*(volatile unsigned long *)(a)
#define __raw_writell(v,a)  *(volatile unsigned long *)(a) = (v)


static struct class *sha204_class;  
static struct device *sha204_device;  
s8 major; 

u16 ticks;
static u8  cmd_MAC[37] = 
{
	0x27, 	// Length of the packet (includes length + CRC)
	0x08, 	// MAC command code
	0x00, 	// Mode flags
	0xff,		// Key ID (2 bytes) 
	0xff,
	// Challenge
	0x13,0x13,0xf3,0x47,0x22,0x85,0x83,0x90,0x71,0x01,0x81,0x72,0x77,0x7c,0xec,0x34,
	0x91,0xcc,0xd3,0x5d,0x6c,0xaa,0x76,0x23,0x85,0x12,0x1d,0x4e,0x23,0x8e,0xe1,0xd3,
};
static s8 ReceivePacket(u8 data[],u8 *retlen);
s8 sha204_GenMac(u16 KeyID,u8 *Key,u8 *Challenge, u8 Mode, u8 *MAC,u8 *sn);
void get_random_bytes(void *buf, int nbytes);
static void chip_delay4us(void);

static ssize_t sha204_open(struct inode * inode, struct file * file)  
{ 
    printk("sha204_open \n");
    
    return 0;  
}  

static int sha204_read(struct file * file, char __user * buffer, size_t count,  loff_t * ppos) 
{
    printk("sha204_read \n");
    
    return 0; 
}

static inline void gpio_config_iomux(void)
{
	unsigned long data;
	__raw_writell(0xffff4550, GPIO4_IOMUX_BASE+GRF_GPIO4_IOMUX_OFFSET);
	data=__raw_readll(GPIO4_IOMUX_BASE+GRF_GPIO4_IOMUX_OFFSET);
	printk("gpio_config_iomux data=0x%x \n",data);
}
static inline void gpio_config_clk(void)
{
	unsigned long data;
	__raw_writell(0xffff019800000c0c, GPIO4_CLK_BASE);;
	data=__raw_readll(GPIO4_CLK_BASE);
	printk("gpio_config_clk  11 low bytes data=0x%x \n",data);
}
static inline void gpio_dir_output(void)
{
	 *GPIO4_BASE |= 0x0002000000000000;   
}
static inline void gpio_dir_input(void)
{
	 *GPIO4_BASE &= 0xfffdffffffffffff;  //input  
}
static inline void gpio_set_high(void)
{
	*GPIO4_BASE |= 0x0002000000020000;   //high
}
static inline void gpio_set_low(void)
{
	*GPIO4_BASE &= 0xfffffffffffdffff;    //low
}
static inline int gpio_get_bit(void)
{
	u32 val = __raw_readll(GPIO4_BASE);
	val=val&0x00020000;
	if(val)
		return 1;
	else
		return 0;
}

static void gpio_init(void)
{
	GPIO4_IOMUX_BASE = (volatile unsigned long *)ioremap(0xff77e028, 0x4);
	GPIO4_CLK_BASE 	 = (volatile unsigned long *)ioremap(0xff760378, 0x8);
	GPIO4_BASE 	     = (volatile unsigned long *)ioremap(0xff790000, 0x8);
	//config the GPIO4_C1 to GPIO fuction
	gpio_config_iomux();
	//enable GPIO4 clk
	gpio_config_clk();
	//config GPIO4 C1 diretion output mode
	gpio_dir_output();
}

static inline void chip_delay600ns(u8 n)
{
    volatile u8 i;
    for(i=0;i<n;i++)
	{
	     CHIP_DELAY(13);
	}			
}

static inline void chip_delayus(u8 n)
{
    volatile u8 i;
    for(i=0;i<n;i++)
	{
	     CHIP_DELAY(13);
	}			
}
static void chip_delay4us(void)
{
	CHIP_DELAY(860);			
}
static inline void chip_delayms(u8 n)
{
    volatile u8 i;
    for(i=0;i<n;i++)
	{
	     chip_delayus(1000);
	}			
}

static inline void chip_wakeup(void)
{
	gpio_dir_output();
	while(1)
	{
		gpio_set_high();
		chip_delay600ns(100);
		gpio_set_low();
	    chip_delay600ns(100);//60us
		gpio_set_high();
	    chip_delayms(5000);//3ms
	}
}
static inline void chip_setone(void)
{
	gpio_set_low();
	chip_delay600ns(7);
	gpio_set_high();
    chip_delay600ns(60);//36us
}
static inline void chip_setzero(void)
{
	gpio_set_low();
	chip_delay600ns(7);//4us
	gpio_set_high();
    chip_delay600ns(7);//4us
	gpio_set_low();
   	chip_delay600ns(7);//4us
	gpio_set_high();
    chip_delay600ns(45);//27us
}
static void chip_sendbyte(unsigned char data)
{
	unsigned char i;
	
	for (i = 0; i < 8; i++)
	{
		if ((data & (1 << i)) != 0)
		{
			chip_setone();	     
		}
		else
		{
			chip_setzero();
		}
	}
}

static char chip_readbit(void)
{
	unsigned int high_count=0;
	unsigned int low_count=0;
	unsigned long count; 
 
	do
	{     
		count++;
		if( count>50000)
		{
			printk("Read IO TIME OUT!!!\n");
			return SA_FAIL;
		}
	}while(gpio_get_bit() != 0 );
	
	while(1)
	{	
		if (gpio_get_bit()==0)   
		{     
			low_count++;	
			if(low_count>((3*high_count)>>1) && high_count !=0)
			{
				while(gpio_get_bit() == 0 );
				return 0;
			}
		}
		else
		{
			high_count++;
			if(high_count>((3*low_count)>>1) && low_count !=0)
			{
				return 1;
			}
		}
	}
}
static s8 chip_receive_bytes(u8 len,u8 *buffer)
{
	int i;
	u8 bit_mask;
	s8 retval;
	u8 cal;
	
	gpio_dir_input();
      	  
	if (!buffer || len==0)
		return ;
	
	for(i=0; i < len; i++)
	{
		for(bit_mask=1; bit_mask >0; bit_mask<<=1)
		{
			retval= chip_readbit();
			if(retval==1)
			{
				buffer[i] |= bit_mask;	
			}
			if(retval==-1)
			{
				printk("read bit Error!\n");
				return SA_FAIL;
			}
		}
	}
	return SA_SUCCESS;
}

static s8 sha204_receive_response(u8 size, u8 *response)
{
	u8 count_byte;
	u8 i;
	u8 ret_code;

	for(i = 0; i < size; i++)
		response[i] = 0;

	chip_sendbyte(FLAG_CLIENT_TRANSMIT);	

	ret_code = chip_receive_bytes(size, response);
	if(ret_code == SA_SUCCESS)
	{
		count_byte=response[0];
		for(i = 0; i < size; i++)
		{
		printk("chip_receive_bytes data[%d]=0x%x\n", i, response[i]);
		}
		
		chip_delayms(3);//3ms need it very much!!!!!!
		
		if( (count_byte < size) || (count_byte > size))
			return SA_FAIL;

		return SA_SUCCESS; 
	}
	
	return SA_FAIL;
}
void sha204_calculate_crc(u8 length, u8 *data, u8 *crc)
{
	u8 counter;
	u8 shift_register;
	u8 data_bit, crc_bit;
	u16 crc_register = 0;
	u16 polynom = 0x8005; 

	for(counter = 0; counter < length; counter++)
	{
		for(shift_register = 0x01; shift_register > 0x00; shift_register <<= 1)
		{
			data_bit = (data[counter] & shift_register) ? 1 : 0;
			crc_bit = crc_register >> 15;
			crc_register <<= 1;
			if (data_bit != crc_bit)
				crc_register ^= polynom;
		}
	}
	
	crc[0] = (u8) (crc_register & 0x00FF);
	crc[1] = (u8) (crc_register >> 8);

}
static s8 sha204c_check_crc(u8 *response)
{
	u8 crc[2];
	u8 count = response[0];

	count -= 2;
	sha204_calculate_crc(count, response, crc);
	if((crc[0] == response[count] && crc[1] == response[count + 1]))
	{
		printk("CRC Check OK!\n");
		return SA_SUCCESS;
	}
	printk("CRC Check Error!\n");
	return  SA_FAIL;
}
static s8 sha204_wakeup(u8 *response)
{     
	s8 retval;
	s8 retry_count=2;  
	s8 i;
	
	chip_wakeup();        
	retval = sha204_receive_response(0x4, response); 
	if( retval != SA_SUCCESS)
	{
		printk("wake sha204 Failed!\n");
		return retval;
	}

	if( (response[0] != 0x4 )|| (response[1] != 0x11) || (response[2]=! 0x33) || (response[3] != 0x43))
	{
		printk("sha204_wakeup Failed ,wrong data!\n");
		return SA_FAIL;
	}
	return SA_SUCCESS;
}

static void sha204_sleep(void)
{
	gpio_dir_output();;
	chip_sendbyte(FLAG_SLEEP);
}
s8 sha204_wakeup_device(void)
{
	s8 retval;
	u8 wakeup_response[4];

	memset(wakeup_response, 0, sizeof(wakeup_response));
	retval = sha204_wakeup(wakeup_response);
	
	if (retval != SA_SUCCESS)
	{
		sha204_sleep();
		return retval;
	}
	return retval;	
}

static void sha204_send_command(u8 data_len,u8* data_buffer)
{
	s8 i;
	gpio_dir_output();;
	chip_sendbyte(FLAG_CLIENT_COMMAND);
	
	for(i=0; i < data_len; i++)
	{
		chip_sendbyte(data_buffer[i]);
	}	
}

static s8 sha204c_send_and_receive(u8 *tx_buffer, u8 rx_size, u8 *rx_buffer,
									u8 execution_delay)
{
	u8 count = tx_buffer[0];//data len;
	u8 count_minus_crc = count - 2;//real data len for crc
	s8 retval;
	memset(rx_buffer,0,rx_size);
	
	sha204_calculate_crc(count_minus_crc, tx_buffer, tx_buffer + count_minus_crc);
	
	sha204_send_command(count, tx_buffer);
	
	chip_delayms(execution_delay);
	
	retval = sha204_receive_response(rx_size, rx_buffer);
	if(retval !=SA_SUCCESS)
	{
		printk("SHA204 recevie data Error!\n");
		return SA_FAIL;
	}
	retval=sha204c_check_crc(rx_buffer);
	return retval;
}
static s8 sha204m_mac(u8 *tx_buffer, u8 *rx_buffer,u8 mode, u16 key_id, u8 *challenge)
{
	if (!tx_buffer || !rx_buffer || (mode & ~MAC_MODE_MASK)
			|| (!(mode & MAC_MODE_BLOCK2_TEMPKEY) && !challenge))

		return SA_FAIL;

	tx_buffer[0] = 7;
	tx_buffer[1] = 0x08;
	tx_buffer[2] = 0x0;
	tx_buffer[3] = key_id & 0xFF;
	tx_buffer[4] = key_id >> 8;

	if ((mode & MAC_MODE_BLOCK2_TEMPKEY) == 0)
	{
		memcpy(&tx_buffer[5], challenge, 32);
		tx_buffer[0] = 39;
	}

//	printk("mode=%d,tx_buffer[3]=%d,tx_buffer[4]=%d,tx_buffer[0]=%d\n",tx_buffer[2],tx_buffer[3],tx_buffer[4],tx_buffer[0]);
	  
	return sha204c_send_and_receive(&tx_buffer[0], 35, &rx_buffer[0],MAC_DELAY);
}

static s8 sha204m_random(u8 *tx_buffer, u8 *rx_buffer, u8 mode)
{
	if (!tx_buffer || !rx_buffer || (mode > RANDOM_NO_SEED_UPDATE))
		return SA_FAIL;

	tx_buffer[0] = 7;
	tx_buffer[1] = 0x1b;
	tx_buffer[2] = mode & RANDOM_NO_SEED_UPDATE;
	tx_buffer[3] =0;
	tx_buffer[4] =0;
	return sha204c_send_and_receive(&tx_buffer[0], 32, &rx_buffer[0],RANDOM_DELAY);
}

static s8 sha204m_gen_dig(u8 *tx_buffer, u8 *rx_buffer,u8 zone, u8 key_id, u8 *other_data)
{
	if (!tx_buffer || !rx_buffer || (zone > GENDIG_ZONE_DATA))
		return SA_FAIL;

	if (((zone == GENDIG_ZONE_OTP) && (key_id > 1))
				|| ((zone == GENDIG_ZONE_DATA) && (key_id > 15)))
		return SA_FAIL;

	tx_buffer[1] = 0x15;
	tx_buffer[2] = zone;
	tx_buffer[3] = key_id;
	tx_buffer[4] = 0;
	if (other_data != NULL)
	{
		memcpy(&tx_buffer[5], other_data, 4);
		tx_buffer[0] = 11;
	}
	else
		tx_buffer[0] = 7;

	return sha204c_send_and_receive(&tx_buffer[0], 4, &rx_buffer[0],GENDIG_DELAY);
}

static s8 sha204m_read(u8 *tx_buffer, u8 *rx_buffer, u8 zone, u16 address)
{
	u8 rx_size;

	if (!tx_buffer || !rx_buffer || (zone & ~READ_ZONE_MASK)
				|| ((zone & READ_ZONE_MODE_32_BYTES) && (zone == SHA204_ZONE_OTP)))
		return SA_FAIL;

	address >>= 2;
	if((zone & SHA204_ZONE_MASK) == SHA204_ZONE_CONFIG) 
	{
		if(address > SHA204_ADDRESS_MASK_CONFIG)
			return SA_FAIL;
	}
	else if ((zone & SHA204_ZONE_MASK) == SHA204_ZONE_OTP) 
	{
		if(address > SHA204_ADDRESS_MASK_OTP)
			return SA_FAIL;
	}
	else if ((zone & SHA204_ZONE_MASK) == SHA204_ZONE_DATA)
	{
		if (address > SHA204_ADDRESS_MASK)
			return SA_FAIL;
	}

	tx_buffer[0] = 0x07;
	tx_buffer[1] = 0x02;
	tx_buffer[2] = zone;
	tx_buffer[3] = (u8)(address & SHA204_ADDRESS_MASK);
	tx_buffer[4] = 0;

	rx_size = (zone & SHA204_ZONE_COUNT_FLAG) ? 35 : 7;

	return sha204c_send_and_receive(&tx_buffer[0], rx_size, &rx_buffer[0],READ_DELAY);
}
static s8 sha204m_write(u8 *tx_buffer, u8 *rx_buffer,u8 zone, u16 address, u8 *new_value, u8 *mac)
{
	u8 *p_command;
	u8 count;

	if (!tx_buffer || !rx_buffer || !new_value || (zone & ~WRITE_ZONE_MASK))
		return SA_FAIL;

	address >>= 2;
	if((zone & SHA204_ZONE_MASK) == SHA204_ZONE_CONFIG)
	{
		if(address > SHA204_ADDRESS_MASK_CONFIG)
			return SA_FAIL;
	}
	else if((zone & SHA204_ZONE_MASK) == SHA204_ZONE_OTP) 
	{
		if(address > SHA204_ADDRESS_MASK_OTP)
			return SA_FAIL;
	}
	else if((zone & SHA204_ZONE_MASK) == SHA204_ZONE_DATA)
	{
		if (address > SHA204_ADDRESS_MASK)
			return SA_FAIL;
	}

	p_command = &tx_buffer[1];
	*p_command++ = 0x12;
	*p_command++ = zone;
	*p_command++ = (u8)(address & SHA204_ADDRESS_MASK);
	*p_command++ = 0;

	count = (zone & SHA204_ZONE_COUNT_FLAG) ? SHA204_ZONE_ACCESS_32 : SHA204_ZONE_ACCESS_4;
	memcpy(p_command, new_value, count);
	p_command += count;

	if(mac != NULL)
	{
		memcpy(p_command, mac, 32);
		p_command += 32;
	}

	tx_buffer[0] = (u8)(p_command - &tx_buffer[0] + 2);

	return sha204c_send_and_receive(&tx_buffer[0], 4, &rx_buffer[0],WRITE_DELAY);

}
static s8 sha204e_read_serial_number(u8 *tx_buffer, u8 *sn)
{
	u8 rx_buffer[35];
	s8 status = sha204m_read(tx_buffer, rx_buffer, SHA204_ZONE_COUNT_FLAG | SHA204_ZONE_CONFIG, 0);
	memcpy(sn, &rx_buffer[1], 4);
	memcpy(sn + 4, &rx_buffer[1 + 8], 5);
	
	return status;
}
s8 sha204_GenMac(u16 KeyID,u8 *Key,u8 *Challenge, u8 Mode, u8 *MAC,u8 *sn)
{
	sha256_ctx ctx;
	u8 opCode = MAC_OPCODE;
	u8 opt_zone[11]={0};
	u8 sn_tmp[9]={0};
	u8 ID_tmp[2]={0};
	s8 retval;

	if (!Challenge || !MAC )
		return SA_FAIL;
	
	ID_tmp[0]=KeyID&0xFF;
	ID_tmp[1]=(KeyID&0xFF00)>>8;
	// Initialize the hash context.
	sha256_init(&ctx);
	// key data, challenge, input parameters
	sha256_update(&ctx, Key, KEYSIZE);
	sha256_update(&ctx, Challenge, CHALLENGESIZE);
	sha256_update(&ctx, &opCode, sizeof(opCode));
	sha256_update(&ctx, &Mode, sizeof(Mode));
	sha256_update(&ctx, ID_tmp, 2);
	// opt_zone 11bytes
	sha256_update(&ctx, opt_zone, 8);
	sha256_update(&ctx, opt_zone+8, 3);
	//SN[8]
	sha256_update(&ctx, &sn[8], 1);
	//SN[4:7] 4bytes
	sha256_update(&ctx, &sn_tmp, 4); 
	//SN[0:1] 2bytes
	sha256_update(&ctx, sn, 2);
	//SN[2:3] 2bytes
	sha256_update(&ctx, &sn_tmp[2], 2); 
	
	// final round
	sha256_final(&ctx, MAC);
	return SA_SUCCESS;
}


const struct file_operations sha204_fops = {  
    .owner      =  THIS_MODULE,  
    .open       =   sha204_open,   
    .read        =   sha204_read, 
};  

static s8 sha204_init(void)  
{  
    /* 0 for allocating major device nu mber automatically */  
    major = register_chrdev(MEM_MAJOR, DEV_NAME, &sha204_fops);  

    /* create firstdrv class */   
    sha204_class = class_create(THIS_MODULE, "sha204");  

    /* create device under the firstdrv class */  
   sha204_device = device_create(sha204_class, NULL, MKDEV(major, 0), NULL, "sha204_test");  

   s8 index;
   s8 retval;
	u8 buffer[80];
	u8 len;
	s8 i;
	static s8 retry_count=0;

	u8 wakeup_response[4];
	u8 command[39];
	// padded serial number (9 bytes + 23 zeros)
	u8 serial_number[9]={0};
	u8 serial_cmmand[7];
	//MAC 
	u8 MAC_command[84]={0};
	u8 MAC_command_tmp[84];
	u8 response_mac[35];
	u8 response_mac_real[32];
	//Random
	u8 Random_command[7]={0};
	u8 random_data[32];
	//Nonce 
	u8 Nonce_command[39]={0};
	u8 response_nonce[4];
	//Challenge
	u8 challenge[32];
	//CheckMAC
	u8 CheckMAC_command[84];
	u8 response_CheckMAC[4];
	u8 other_data[13];
	//genMAC
	u8 genMAC[32];
	u16 KEY_ID=15;
	//memset(&g_gpio_1wire_cfg, 0, sizeof(t_gpio_1wire_cfg));	
	//g_gpio_1wire_cfg.pin_data = CONFIG_GPIO_SEC_DATA;
	//g_gpio_1wire_cfg.delay = 10;
	u8 key_data[32]={
			0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55,
			0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,
			0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,
			0x88,0x88,0x88,0x88,0x88,0x88,0x88,0x88
		};
              
	printk("#########DFYUAN TEST wakeup new!#############\n");
	gpio_init();
	#if 0
	while(1)
	{
		gpio_set_high();
		chip_delay600ns(1);
		gpio_set_low();
		chip_delay600ns(1);
	}
	#endif
	#if 1
	while(1)
	{
		retry_count++;
		if(retry_count>10) {     
			printk("Authentication Failed!\n");
			return SA_FAIL;
		}
		
		retval=sha204_wakeup(wakeup_response);
		if (retval==SA_SUCCESS) {	  
			printk("dfyuan wakeup success !\n");	 
		}else {   
			sha204_sleep();
			chip_delayms(100);
			continue;
		}
		retval=sha204e_read_serial_number(serial_cmmand,serial_number);
		if(retval==SA_FAIL){
			sha204_sleep();
			chip_delayms(100);
			printk("read serial number failed !\n"); 
			continue;
		}else if(retval==SA_SUCCESS){	  
		    printk("read serial number success !\n"); 
		}


	}

		#if 0
		retval=sha204_GenMac(KEY_ID,key_data,challenge,0,genMAC,serial_number);

		if(retval==SA_FAIL)
		{
			sha204_sleep();
			chip_delayms(6000);
			continue;
		}
		if(memcmp(response_mac+1,genMAC,32)==0)
		{
			printk("Authentication OK,Boot The System Now!\n");
			break;
		}
		#endif
		chip_delayms(10);
	#endif
	return 0;
}  

static void sha204_exit(void)  
{  
    unregister_chrdev(major, DEV_NAME);  
    device_unregister(sha204_device);  //uninstall device which is under the firstdrv class  
    class_destroy(sha204_class);      //uninstall class  	
    iounmap((void *)GPIO4_IOMUX_BASE);
    iounmap((void *)GPIO4_CLK_BASE);
	iounmap((void *)GPIO4_BASE);
}  
module_init(sha204_init);  
module_exit(sha204_exit);   
  
MODULE_AUTHOR("DQ");   
MODULE_DESCRIPTION("Just for test");  
MODULE_LICENSE("GPL"); 

