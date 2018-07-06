#ifndef SA102S_H
#define SA102S_H

#define KEYSIZE				32	//!< size of personalization and SA100 key in bytes
#define CHALLENGESIZE		32	//!< size of challenge in bytes
#define DIGESTSIZE			32	//!< size of digest in bytes
#define SEEDSIZE			16	//!< size of seed in bytes
#define SECRETSIZE			8	//!< size of secret in bytes
#define MAPSIZE				11	//!< size of fuse map in bytes (SA102)
#define MAC_OPCODE			8	//!< MAP command code for SA102

#define SECRET_STATUS		(1 << 4) //!< Mode flag to include secret and status fuses in the SHA256 calculation
#define SECRET				(1 << 5) //!< Mode flag to include secret fuses in the SHA256 calculation
#define SERIALNUM			(1 << 6) //!< Mode flag to include serial number in the SHA256 calculation

#define RANDOM_SEED_UPDATE            0x00       //!< Random mode for automatic seed update
#define RANDOM_NO_SEED_UPDATE         0x01       //!< Random mode for no seed update

#define GENDIG_ZONE_IDX                 2      //!< GenDig command index for zone
#define GENDIG_KEYID_IDX                3      //!< GenDig command index for key id
#define GENDIG_DATA_IDX                 5        //!< GenDig command index for optional data
#define GENDIG_COUNT                    7    //!< GenDig command packet size without "other data"
#define GENDIG_COUNT_DATA               11                   //!< GenDig command packet size with "other data"
#define GENDIG_OTHER_DATA_SIZE          4                    //!< GenDig size of "other data"
#define GENDIG_ZONE_CONFIG              0          //!< GenDig zone id config
#define GENDIG_ZONE_OTP                 1          //!< GenDig zone id OTP
#define GENDIG_ZONE_DATA                2          //!< GenDig zone id data

#define READ_ZONE_MASK                   0x83       //!< Read zone bits 2 to 6 are 0.
#define READ_ZONE_MODE_32_BYTES          0x80       //!< Read mode: 32 bytes

#define SA_SUCCESS			0   //!< Function succeeded. Device status, if available, was okay.
#define SA_FAIL				-1  //!< Incorrect CRC received.

#define SHA204_ZONE_CONFIG              0x00      //!< Configuration zone
#define SHA204_ZONE_OTP                 0x01      //!< OTP (One Time Programming) zone
#define SHA204_ZONE_DATA                0x02      //!< Data zone
#define SHA204_ZONE_MASK                0x03      //!< Zone mask
#define SHA204_ZONE_COUNT_FLAG        	0x80      //!< Zone bit 7 set: Access 32 bytes, otherwise 4 bytes.
#define SHA204_ZONE_ACCESS_4            4      	  //!< Read or write 4 bytes.
#define SHA204_ZONE_ACCESS_32           32        //!< Read or write 32 bytes.
#define SHA204_ADDRESS_MASK_CONFIG      0x001F    //!< Address bits 5 to 7 are 0 for Configuration zone.
#define SHA204_ADDRESS_MASK_OTP         0x000F    //!< Address bits 4 to 7 are 0 for OTP zone.
#define SHA204_ADDRESS_MASK             0x007F    //!< Address bit 7 to 15 are always 0.

#define IO_BUF_SIZE			88			//!< CryptoAuth IO buffer

#define READ_DELAY			1			//4ms
#define MAC_DELAY			35         	//30ms
#define RANDOM_DELAY		30         //need to adj
#define GENDIG_DELAY		25		   	//need to adj
#define WRITE_DELAY  		10			//need to adj
#define NONCE_DELAY			30			//need to adj
#define CHECKMAC_DELAY 		30

#define WRITE_ZONE_MASK             0xC3       //!< Write zone bits 2 to 5 are 0.
#define WRITE_ZONE_WITH_MAC         0x40       //!< Write zone bit 6: write encrypted with MAC

#define NONCE_COUNT_SHORT               27         //!< Nonce command packet size for 20 bytes of data
#define NONCE_COUNT_LONG                39         //!< Nonce command packet size for 32 bytes of data
#define NONCE_MODE_MASK                 3          //!< Nonce mode bits 2 to 7 are 0.
#define NONCE_MODE_SEED_UPDATE          0x00       //!< Nonce mode: update seed
#define NONCE_MODE_NO_SEED_UPDATE       0x01       //!< Nonce mode: do not update seed
#define NONCE_MODE_INVALID              0x02       //!< Nonce mode 2 is invalid.
#define NONCE_MODE_PASSTHROUGH          0x03       //!< Nonce mode: pass-through
#define NONCE_NUMIN_SIZE                20         //!< Nonce data length
#define NONCE_NUMIN_SIZE_PASSTHROUGH    32         //!< Nonce data length in pass-through mode (mode = 3)

#define MAC_MODE_MASK     	            0x77      //!< MAC mode bits 3 and 7 are 0.
#define MAC_MODE_CHALLENGE              0x00       //!< MAC mode       0: first SHA block from data slot
#define MAC_MODE_BLOCK2_TEMPKEY         0x01       //!< MAC mode bit   0: second SHA block from TempKey
#define MAC_MODE_BLOCK1_TEMPKEY         0x02       //!< MAC mode bit   1: first SHA block from TempKey
#define MAC_MODE_SOURCE_FLAG_MATCH      0x04       //!< MAC mode bit   2: match TempKey.SourceFlag
#define MAC_MODE_PASSTHROUGH            0x07       //!< MAC mode bit 0-2: pass-through mode
#define MAC_MODE_INCLUDE_OTP_88         0x10      //!< MAC mode bit   4: include first 88 OTP bits
#define MAC_MODE_INCLUDE_OTP_64         0x20       //!< MAC mode bit   5: include first 64 OTP bits
#define MAC_MODE_INCLUDE_SN             0x40       //!< MAC mode bit   6: include serial number

#define CHECKMAC_MODE_MASK              0x27       //!< CheckMAC mode bits 3, 4, 6, and 7 are 0.

typedef struct s_gpio_1wire_cfg
{
	int pin_data;
	int delay;
}t_gpio_1wire_cfg;

//! response structure for reading manufacturer id
typedef  struct _mfrID_str
{
	u8		ROMdata[2];							//!< ROM part of manufacturing id
	u8		fusedata[1];						//!< fuse part of manufacturing id
}mfrID_str;

//! response union for reading manufacturer id
typedef   union _SA_mfrID_buf
{
	u8		bytes[sizeof(mfrID_str)];		//!< response as byte array
	mfrID_str	mfrID_data;							//!< response as structure
}SA_mfrID_buf;
//! response structure for reading serial number
typedef  struct _snBuf_str
{
	u8		ROMdata[2];							//!< ROM part of serial number
	u8		fusedata[4];						//!< fuse part of serial number
} snBuf_str;

//! response union for reading serial number
typedef union _snBuf
{
	u8		bytes[sizeof(snBuf_str)];		//!< response as byte array
	snBuf_str	sn_data;						//!< response as structure
}SA_snBuf;
//! response structure for reading status fuses
typedef  struct _SA_statusBuf
{
	u8		bytes[3];							//! fuse bit values lumped into bytes
}SA_statusBuf;

//! Read response structure
typedef  struct _SA_Read_out
{
	u8		count;					//!< packet size
	u8		SAdata[4];				//!< four bytes read from the device
	u16		CRC;					//!< CRC over entire response
} SA_Read_out;

void SHA204_init(void);

#endif
