#define	  RETRY_COUNT		      3
#define	  FD_DELAY	     	20*6000
//#define FLOPPY_DEBUG
#define   READ_TRACK                  2	     /* generates IRQ6 */
#define   SPECIFY                     3      /* set drive parameters */
#define   SENSE_DRIVE_STATUS          4
#define   WRITE_DATA                  5      /* write to disk */
#define   READ_DATA                   6      /* read from disk */
#define   RECALIBRATE                 7      /* seek to cylinder 0 */
#define   SENSE_INTERRUPT             8      /* acknowledge IRQ6 get status of last command */
#define   WRITE_DELETED_DATA          9
#define   READ_ID                     10     /* generates IRQ6 */
#define   READ_DELETED_DATA           12
#define   FORMAT_TRACK                13     /* fomat track */
#define   SEEK                        15     /* seek both heads to cylinder X */
#define   VERSION                     16     /* used during initialization once */
#define   SCAN_EQUAL                  17
#define   PERPENDICULAR_MODE          18     
#define   CONFIGURE                   19     /* set controller parameters */
#define   LOCK                        20     /* protect controller params from a reset */
#define   VERIFY                      22     /* verify track */
#define   SCAN_LOW_OR_EQUAL           25     /* scan low */
#define   SCAN_HIGH_OR_EQUAL          29

#define   DATA_REGISTER	              0x3F0  
#define   STATUS_REGISTER_A           0x3F0  
#define   STATUS_REGISTER_B           0x3F1 
#define   DIGITAL_OUTPUT_REGISTER     0x3F2
#define   TAPE_DRIVE_REGISTER         0x3F3
#define   MAIN_STATUS_REG	      0x3F4
#define   DATARATE_SELECT_REGISTER    0x3F4 
#define   DATA_REGISTER               0x3F5
#define   DIGITAL_INPUT_REGISTER      0x3F7
#define   CONFIGURATION_CONTROL_REGISTER 0x3F7

#define	RQM				0x80
#define DIO				0x40
#define NDMA				0x20
#define CB				0x10
#define ACTB				0x2
#define ACTA				0x1

#define	IMPLIED_SEEK		  	0
#define FIFO_DISABLED			0
#define DRIVE_POLLING_DISABLE		1
#define THRESHOLD			15

#define BIT_MT				0x80
#define	BIT_MF				0x40
#define BIT_SK				0x20

#define	MOTB				0x20
#define	MOTA				0x10
#define	IRQ				0x8
#define	NOT_RESET			0x4
#define	DSEL1				0x1
#define	DSEL0				0x0


#define NO_ERROR 			0
#define GENERIC_ERROR			1
#define BAD_COMMAND 			2
#define	NO_ADDRESS_MARK 		4
#define FD_WRITE_PROTECT_ERROR 		5
#define NO_DATA_FOUND 			6
#define FIFO_OVERFLOW 			7
#define CRC_ERROR 			8
#define BAD_SECTOR_TRACK 		9
#define NO_ADDRESS_MARK 		10
#define INVALID_TRACK 			11
#define INVALID_SECTOR 			12
#define INVALID_TRACK 			13
#define INVALID_TRACK 			14
#define NO_ADDRESS_MARK 		15

#define _READ  0
#define _WRITE 1


