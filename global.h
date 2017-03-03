#ifndef MACROS_DEFINED
#define MACROS_DEFINED

/* Define states for MyStateRegister */
#define NOTHING_TODO          0x0
#define TIMESTAMP_CAPTURED    0x1

#endif /* MACROS_DEFINED */

/* Status register to follow state */
extern volatile uint32_t MyStateRegister;

/* Timestamp values */
extern volatile uint32_t TimestampTime;
extern volatile uint32_t TimestampDate;
