#ifndef __DAQ_
#define __DAQ_

#include <string.h>
#include <time.h>

#include "board.h"
#include "adc_spi.h"
#include "delay.h"
#include "adc_spi.h"
#include "ff.h"
#include "ring_buff.h"
#include "sys_error.h"
#include "log.h"
#include "fixed.h"
#include "config.h"

#define SYS_CLOCK_RATE 72000000 // System clock rate in Hz

#define ADC_US 4 // Microseconds between ADC samples

#define CONVERSION_RATE 40000 // Rate of conversion from ADC, limits sub sampling

#define VOUT_PWM_RATE 10000 // Vout pwm rate in Hz, also rate of updates to output value

#define MAX_SAMPLE_RATE 10000 // Maximum rate samples can be recorded to the sd card

#define MAX_CHAN 3 // Total count of available channels

#define BLOCK_SIZE 512 // Size of blocks to write to the file system

#define SAMPLE_STR_SIZE 60 // Maximum size of a single sample string

#define clamp(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

// Voltage range type
typedef enum {
	V5,
	V24,
} VRANGE_T;

// Data type
typedef enum {
	READABLE,
	BINARY
} DATA_T;

// Configuration data for each channel
typedef struct Channel_Config {

// Configured for each use case in config file
	bool enable;			// enable / disable channel
	VRANGE_T range;			// select input voltage range
	dec_float_t units_per_volt; // sensitivity in units/volt
	fix64_t offset_uV;		// zero offset in uV
	char unit_name[9];		// name of channel unit

// Configured by setting the calibration flag in the config file, then running a calibration cycle. Backed up in device eeprom
	// volts = (raw_val - v5_zero_offset) / v5_LSB_per_volt
	fix64_t v5_zero_offset;	// value of raw 16-bit sample for 0 input voltage
	fix64_t v5_uV_per_LSB;	// sensitivity of reading in uV / LSB

	// volts = (raw_val - v24_zero_offset) / v24_LSB_per_volt
	fix64_t v24_zero_offset;	// value of raw 16-bit sample for 0 input voltage
	fix64_t v24_uV_per_LSB;	// sensitivity of reading in uV / LSB
} Channel_Config;

// Configuration data for the entire DAQ
typedef struct DAQ {
	Channel_Config channel[MAX_CHAN];
	uint8_t channel_count;	// Number of channels enabled, calculated from Channel_Config enables
	int32_t mv_out;			// Output voltage in mv, valid_range = <5000..24000>
	int32_t sample_rate;	// Sample rate in Hz, valid range = <1..10000>
	int8_t time_res;		// Sample time resolution in n digits where time is s.n
	uint32_t subsamples;	// Number of sub samples per data sample, normally 40k/sample_rate
	int32_t trigger_delay;	// Delay in seconds before starting the data collection
	DATA_T data_type;		// data mode, can be READABLE or COMPACT
	char user_comment[101];	// User comment to appear at the top of each data file
} DAQ;

extern uint8_t rsel_pins[3];

// Raw data buffer
extern struct RingBuffer *rawBuff;

// FatFS volume
extern FATFS fatfs[_VOLUMES];

// DAQ configuration data
extern DAQ daq;

// DAQ loop function
extern void (*daq_loop)(void);

// ADC sample timing interrupt, called from main MRT interrupt in system
void MRT1_IRQHandler(void);

// Update vout PWM value
void daq_updateVout(void);

// Set up daq
void daq_init(void);

// Start acquiring data
void daq_record(void);

// Make the data file
void daq_makeDataFile(void);

// Wait for the trigger time to start
void daq_triggerDelay(void);

// Enable the output voltage
void daq_voutEnable(void);

// Disable the output voltage
void daq_voutDisable(void);

// Write data file header
void daq_header(void);

// Stop acquiring data
void daq_stop(void);

// Write data from raw buffer to file, formatting to string  buffer as an intermediate step if needed
void daq_writeData(void);

// Flush data from raw buffer to file, formatting to string  buffer as an intermediate step if needed
void daq_flushData(void);

// Write a single block to the data file from the string buffer
void daq_writeBlock(void *data, int32_t data_size);

// Convert rawData into a readable formatted output string
void daq_readableFormat(uint16_t *rawData, char *sampleStr);

// Limit configuration values to valid ranges
void daq_configCheck(void);

// Set channel configuration defaults
void daq_configDefault(void);

#endif /* __DAQ_ */
