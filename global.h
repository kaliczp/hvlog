#ifndef MACROS_DEFINED
#define MACROS_DEFINED

/* Time and date */
#define CURR_TIM         0x00212000
#define CURR_DAT         0x00170315

/* Define states for MyStateRegister */
#define NOTHING_TODO          0x0
#define TIMESTAMP_CAPTURED    0x1
#define DAILY_ALARM           0x2

/* Lowpower mode macros */
#define ModeSTOP              0x1
#define ModeStandby           0x2
#endif /* MACROS_DEFINED */

/* Status register to follow state */
extern volatile uint32_t MyStateRegister;

/* Timestamp values */
extern volatile uint32_t TimestampTime;
extern volatile uint32_t TimestampDate;
