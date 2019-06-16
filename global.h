/* Define time and date if not defined in command line*/
#ifndef CURR_TIM
#define CURR_TIM         0x00212000
#endif

#ifndef CURR_DAT
#define CURR_DAT         0x00170315
#endif

#ifndef RTC_CALM
#define RTC_CALM         0x55 /* It is 9 bit in the register */
#endif


#ifndef MACROS_DEFINED
#define MACROS_DEFINED

/* Define states for MyStateRegister */
#define NOTHING_TODO          0x0
#define COUNTER1              0x1
#define COUNTER2              0x2
#define COUNTER3              0x4
#define COUNTER4              0x8
#define COUNTER_MSK           0xF
/* #define STORE_TIMESTAMP_TIM   0x10 */
#define STORE_TIMESTAMP_DAT   0x20
#define UART_SEND_HEADER      0x40
#define INIT_SPIREAD          0x80
#define READY_UART            0x100
#define UART_PROGRESS         0x200
#define DAILY_ALARM           0x400
#define SPI_READROM           0x800
#define TIMESTAMP_CAPTURED    0x1000
#define SHORTER_ALARM         0x2000
#define CHAR_RECEIVED         0x4000
#define CHAR_SEND             0x8000
#define SET_DATE              0x10000
#define SET_TIME              0x20000
#define SEND_TIME             0x40000
#define INIT_UART             0x80000

/* SPI Macros */
#define WREN             0b00000110 /* 0x06 Write enable */
#define WRDI             0b00000100 /* 0x04 Write disable */
#define RDSR             0b00000101 /* 0x05 Read Status Register */
#define WRITE            0b00000010 /* 0x02 */
#define READ             0b00000011 /* 0x03 */
#define WEL              0b00000010 /* Write enable latch */
#define WIP              0b00000001 /* Write in progress bit */
#define TIME_LENGTH      4          /* Time lenght in bytes */
#define TIME_DATE_LENGTH 8          /* Time and date lenght in bytes */
#define TO_EPR_ADDRESSLENGTH      3 /* SPI address in bytes */
#define TO_EPR_ADDRESSLENGTHwCOMMAND      1 + TO_EPR_ADDRESSLENGTH /* SPI address in bytes */
#define TO_EPR_LENGTH    TIME_LENGTH + TO_EPR_ADDRESSLENGTHwCOMMAND
#define SPI_EPR_PG_SUB1  127        /* SPI EEPROM page size in bytes - 1 */
#define SPIEEPROM_LENGTH 65536     /* SPI EEPROM size in bytes (64 Kbytes, 512Kbit) */

#define SPI_IS_STANDALONE     1
#define SPI_IS_NOT_STANDALONE     0

/* Data macros */

/* Lowpower mode macros */
#define ModeSleep             0x0
#define ModeSTOP              0x1
#define ModeStandby           0x2
#endif /* MACROS_DEFINED */

/* Type definition */
#ifndef DATETIMEREG
#define DATETIMEREG
typedef struct
{
  uint8_t length;
  uint8_t padalign[3];
  uint8_t SPICommand;
  uint8_t SPIAddress[TO_EPR_ADDRESSLENGTH];
  uint32_t TimeRegister;
  uint32_t DateRegister;
} time_date_reg_t;
#endif  /* DATETIMEREG */

/* Status register to follow state */
extern volatile uint32_t MyStateRegister;

/* Time and date global variables */
extern volatile time_date_reg_t TimeDateRegS;
extern volatile uint8_t *PtrTimDatS;
extern volatile uint8_t *PtrTDTimeR;
extern volatile uint8_t uartsend;
extern volatile uint8_t CharToReceive;
