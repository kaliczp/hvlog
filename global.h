/* Define time and date if not defined in command line*/
#ifndef CURR_TIM
#define CURR_TIM         0x00212000
#endif

#ifndef CURR_DAT
#define CURR_DAT         0x00170315
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
#define STORE_TIMESTAMP_TIM   0x10
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
#define FIRST_DATA       4          /* The first data after SPI address */
#define TO_EPR_LENGTH    8
#define TSTO_EPR_LENGTH    12
#define SPI_EPR_PG_SUB1  127        /* SPI EEPROM page size in bytes - 1 */
#define SPIEEPROM_LENGTH 131072     /* SPI EEPROM size in bytes (128 Kbytes, 1Mbit) */

#define SPI_IS_STANDALONE     1
#define SPI_IS_NOT_STANDALONE     0

/* Lowpower mode macros */
#define ModeSleep             0x0
#define ModeSTOP              0x1
#define ModeStandby           0x2
#endif /* MACROS_DEFINED */

/* Status register to follow state */
extern volatile uint32_t MyStateRegister;

/* Time and date global variables */
extern volatile uint32_t SubSecondRegister;
extern volatile uint32_t TimeRegister;
extern volatile uint32_t DateRegister;

extern uint8_t ToEEPROM[TO_EPR_LENGTH];
extern volatile uint8_t uartsend;
extern volatile uint8_t CharToReceive;
