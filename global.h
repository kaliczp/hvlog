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
#define TIMESTAMP_CAPTURED    0x1
#define DAILY_ALARM           0x2
#define SPI_SAVEROM           0x4
#define STORE_TIMESTAMP_DAT   0x8
#define STORE_TIMESTAMP_TIM   0x10
#define SPI_READROM           0x20
#define UART_SEND             0x40
#define INIT_SPIREAD          0x80
#define INIT_UART             0x100
#define UART_PROGRESS         0x200

/* SPI Macros */
#define WREN             0b00000110 /* 0x06 Write enable */
#define WRDI             0b00000100 /* 0x04 Write disable */
#define RDSR             0b00000101 /* 0x05 Read Status Register */
#define WRITE            0b00000010 /* 0x02 */
#define READ             0b00000011 /* 0x03 */
#define TO_EPR_LENGTH    7
#define TSTO_EPR_LENGTH    11

/* Lowpower mode macros */
#define ModeSleep             0x0
#define ModeSTOP              0x1
#define ModeStandby           0x2
#endif /* MACROS_DEFINED */

/* Status register to follow state */
extern volatile uint32_t MyStateRegister;

/* Timestamp values */
extern volatile uint32_t TimestampTime;
extern volatile uint32_t TimestampDate;

extern uint8_t ToEEPROM[TO_EPR_LENGTH];
extern volatile uint8_t uartsend;
