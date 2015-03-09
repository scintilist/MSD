#include "config.h"


// Set channel configuration from the config file on the SD card
void config_fromFile(void){
	FIL config;		/* FatFS File Object */
	char line[101]; /* Line Buffer */
	int i = 0;
	int iVal = 0;
	char cVal = '';
	float fVal = 0;

	if (f_open(&config, "config.txt", FA_READ)) {
		error(ERROR_READ_CONFIG);
	}
	/* Read lines to get to update Date/Time */
	getNonBlankLine(line,&config);
	getNonBlankLine(line,&config);
	getNonBlankLine(line,&config);

	if (line[0] == 'Y' || line[0] == 'y') {
		/* Update Date and Time - 4 Lines */
		getNonBlankLine(line,&config);
		getNonBlankLine(line,&config);
		/* Line is now the date/time string in YYYY-MM-DD HH:MM:SS */
		setTime(line);

		/* Read lines to get to update config */
		getNonBlankLine(line,&config);
		getNonBlankLine(line,&config);

	} else {
		for (i = 0; i < 4; i++) {
			getNonBlankLine(line,&config);
		}
	}

	if (line[0] == 'Y' || line[0] == 'y') {
		/* Update Config - */
		getNonBlankLine(line,&config);
		getNonBlankLine(line,&config);
		/* Line is now the user header */
		memcpy(daq.user_comment, line, 101);
		getNonBlankLine(line,&config);
		getNonBlankLine(line,&config);
		/* Line is now Output Voltage */
		sscanf(line, "%f", fVal);
		daq.mv_out = (int32_t)(fVal * 1000);
		getNonBlankLine(line,&config);
		getNonBlankLine(line,&config);
		/* Line is now Sample Rate */
		sscanf(line, "%d", iVal);
		daq.sample_rate = (int32_t)iVal;
		getNonBlankLine(line,&config);
		getNonBlankLine(line,&config);
		/* Line is now trigger delay */
		sscanf(line, "%d", iVal);
		daq.trigger_delay = iVal;
		getNonBlankLine(line,&config);
		getNonBlankLine(line,&config);
		/* Line is now data mode */
		if (line[0] == 'R') {
			daq.data_type = READABLE;
		} else if (line[0] == 'H') {
			daq.data_type = HEX;
		} else if (line[0] == 'B') {
			daq.data_type = BINARY;
		} else {
			error(ERROR_READ_CONFIG);
		}
		for (i = 0; i<MAX_CHAN; i++) {
			getNonBlankLine(line,&config);

			/* Channel Config */
			/* CH Enabled */
			strtok(line,":");
			sscanf(line, "%c", cVal);
			if (cVal == 'Y') {
				daq.channel[i].enable = true;
			} else if (cVal == 'N') {
				daq.channel[i].enable = false;
			} else {
				error(ERROR_READ_CONFIG);
			}
			getNonBlankLine(line,&config);
			/* CH Range */
			strtok(line,":");
			sscanf(line, "%c", cVal);
			if (cVal == '5'){
				daq.channel[i].range = V5;
			} else if (cVal == '2') {
				daq.channel[i].range = V24;
			} else {
				error(ERROR_READ_CONFIG);
			}
			getNonBlankLine(line,&config);

			/* CH Units */
			memcpy(daq.channel[i].unit_name, line+23, 9);
			getNonBlankLine(line,&config);

			/* CH Units/Volt */
			strtok(line,":");
			sscanf(line, "%f", fVal);
			daq.channel[i].units_per_volt = floatToDecFloat(fVal);
			getNonBlankLine(line,&config);

			/* CH Zero Offset */
			strtok(line,":");
			sscanf(line, "%f", fVal);
			daq.channel[i].offset_uV = floatToFix(fVal);

			/* Read lines to get to update calibration */
			getNonBlankLine(line,&config);
			getNonBlankLine(line,&config);
		}
	} else {
		for (i = 0; i < 30; i++) {
			/* Move to next section if no update config */
			getNonBlankLine(line,&config);
		}
	}
	if (line[0] == 'Y' || line[0] == 'y') {
		/* Update Calibration - 18 Lines (Maybe) */
		for (i = 0; i<MAX_CHAN;i++) {
			getNonBlankLine(line,&config);

			/* 5V Zero Offset */
			strtok(line,":");
			sscanf(line, "%f", fVal);
			daq.channel[i].v5_zero_offset = floatToFix(fVal);
			getNonBlankLine(line,&config);

			/* 5V LSB/Volt */
			strtok(line,":");
			sscanf(line, "%f", fVal);
			daq.channel[i].v5_uV_per_LSB = floatToFix(fVal);
			getNonBlankLine(line,&config);

			/* 24V Zero Offset */
			strtok(line,":");
			sscanf(line, "%f", fVal);
			daq.channel[i].v24_zero_offset = floatToFix(fVal);
			getNonBlankLine(line,&config);

			/* 24V LSB/Volt */
			strtok(line,":");
			sscanf(line, "%f", fVal);
			daq.channel[i].v24_uV_per_LSB = floatToFix(fVal);
		}
	}
	/* Close File */
	f_close(&config);
	/* Run ConfigCheck After Reading */
	daq_configCheck();
}

// Set the current time given a time string in the format
// 2015-02-21 05:57:00
void setTime(char *timeStr){
	// Enable and configure Real-Time Clock
	Chip_Clock_EnableRTCOsc();
	Chip_RTC_Init(LPC_RTC);

	// Set time in human readable form
	struct tm tm;
	time_t t = 0;
	tm = *localtime(&t);
	sscanf(timeStr, "%u-%u-%u %u:%u:%u",
		&(tm.tm_year), &(tm.tm_mon), &(tm.tm_mday), &(tm.tm_hour), &(tm.tm_min), &(tm.tm_sec));
	tm.tm_year -= 1900;
	tm.tm_mon -= 1;
	tm.tm_isdst = 0; // No DST accounting
	t = mktime(&tm);

	Chip_RTC_Reset(LPC_RTC); // Set then clear SWRESET bit
	Chip_RTC_SetCount(LPC_RTC, t); // Set the time
	Chip_RTC_Enable(LPC_RTC); // Start counting
}

char *getTimeStr(){
	time_t rawtime = Chip_RTC_GetCount(LPC_RTC);
	struct tm * timeinfo;
	timeinfo = localtime(&rawtime);
	return asctime(timeinfo);
}

void getNonBlankLine(char* line, FIL* fil){
	f_gets(line, sizeof(line), &fil);
	while (line[0] == 0) {
		f_gets(line, sizeof(line), &fil);
	}
}
