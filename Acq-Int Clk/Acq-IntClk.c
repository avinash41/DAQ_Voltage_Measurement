/*********************************************************************
*
* ANSI C Example program:
*    Acq-IntClk.c
*
* Example Category:
*    AI
*
* Description:
*    This example demonstrates how to acquire a finite amount of data
*    using the DAQ device's internal clock.
*
* Instructions for Running:
*    1. Select the physical channel to correspond to where your
*       signal is input on the DAQ device.
*    2. Enter the minimum and maximum voltages.
*    Note: For better accuracy try to match the input range to the
*          expected voltage level of the measured signal.
*    3. Select the number of samples to acquire.
*    4. Set the rate of the acquisition.
*    Note: The rate should be AT LEAST twice as fast as the maximum
*          frequency component of the signal being acquired.
*
* Steps:
*    1. Create a task.
*    2. Create an analog input voltage channel.
*    3. Set the rate for the sample clock. Additionally, define the
*       sample mode to be finite and set the number of samples to be
*       acquired per channel.
*    4. Call the Start function to start the acquisition.
*    5. Read all of the waveform data.
*    6. Call the Clear Task function to clear the task.
*    7. Display an error if any.
*
* I/O Connections Overview:
*    Make sure your signal input terminal matches the Physical
*    Channel I/O Control. For further connection information, refer
*    to your hardware reference manual.
*
*********************************************************************/
#include <time.h>
//#include <crtdefs.h>
#include "Acq-IntClk.h"


/////////////// Global Variable //////////////
char*		file_name = NULL;
int32		measure_time=0, n_measure=0;

struct p_data* power_data;

struct tm* local_now;
struct tm* local_prev;

time_t now, prev;
/////////////// Global Variable //////////////



// Declare function
int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void* callbackData);
int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void* callbackData);


int main(int argc, char **argv)
{
	int32       i, error=0, temp=0;
	char		errBuff[2048] = {'\0'};
	TaskHandle  taskHandle=0;
	float64 data[20000] = {0};
	int32 read = 0;


	// check & assign argument value
	if(argc<3){
		fprintf(stderr, "Argument error...number of args\n");
		exit(1);	
	} else if(!isdigit(*argv[2])) {
		fprintf(stderr, "Argument error...wrong sec arg %c\n", *argv[2]);
		exit(1);	
	}

    file_name = argv[1];
	measure_time = atoi(argv[2]);

	// modified parameter of NUM_OF_PDATA macro
	n_measure = NUM_OF_PDATA(1);
	
	// string contcatenation 
	strcat(file_name, FILE_TYPE);
	fprintf(stderr, "file name=%s, n_measure=%d\n", file_name, n_measure);

	// Initialize p_data structure 
	power_data = (struct p_data*)malloc(sizeof(struct p_data) * n_measure);
	memset(power_data, 0, sizeof(struct p_data) * n_measure);	

	/*********************************************/
	// DAQmx Configure Code
	/*********************************************/
	DAQmxErrChk (DAQmxCreateTask("test1",&taskHandle));
	//DAQmxErrChk (DAQmxCreateAIVoltageChan(taskHandle,"Dev1/ai15,Dev1/ai31","",DAQmx_Val_RSE,-2.0,2.0,DAQmx_Val_Volts,NULL));
	//DAQmxErrChk (DAQmxCreateAIVoltageChan(taskHandle,"Dev1/ai15,Dev1/ai31,Dev1/ai4,Dev1/ai21,Dev1/ai6,Dev1/ai23","",DAQmx_Val_RSE,-1.0,1.0,DAQmx_Val_Volts,NULL));
	DAQmxErrChk (DAQmxCreateAIVoltageChan(taskHandle,"Dev1/ai15,Dev1/ai31,Dev1/ai21, Dev1/ai4","",DAQmx_Val_RSE,-2.0,2.0,DAQmx_Val_Volts,NULL));

	DAQmxErrChk (DAQmxCfgSampClkTiming(taskHandle,"",5000.0,DAQmx_Val_Rising,DAQmx_Val_ContSamps,5000));
	
	DAQmxErrChk (DAQmxRegisterEveryNSamplesEvent(taskHandle, DAQmx_Val_Acquired_Into_Buffer, 5000, 0, EveryNCallback, NULL));
	DAQmxErrChk (DAQmxRegisterDoneEvent(taskHandle, 0, DoneCallback, NULL));
	
	/*********************************************/
	// DAQmx Start Code
	/*********************************************/
	DAQmxErrChk (DAQmxStartTask(taskHandle));

	printf("Acquiring samples continuously. Press Enter to interrupt\n");

	/* Limitation of Time *********************/
	time(&prev);
	time(&now);
	
	while(1)
	{
		if((now - prev) == 6)
		{
			printf("reached time limit\n");
			return 1;
		}
				
		time(&now);
	}
	/****************************************/
		
Error:
	if( DAQmxFailed(error) )
		DAQmxGetExtendedErrorInfo(errBuff,2048);
	if( taskHandle!=0 )  {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(taskHandle);
		DAQmxClearTask(taskHandle);
	}

	if( DAQmxFailed(error) )
		printf("DAQmx Error: %s\n",errBuff);
	printf("End of program, press Enter key to quit\n");

	return 0;
}

// Read the Voltage data from DAQ machine then store into buffer that data
int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void* callbackData)
{
	int32 error = 0, i;
	char errBuff[2048] = {'\0'};
	static int totalRead = 0;
	int32 read = 0;	
	float64 data[20000] = {0};

	// DAQmx Read Code
	DAQmxErrChk (DAQmxReadAnalogF64(taskHandle, 5000, 10.0, DAQmx_Val_GroupByScanNumber, data, 20000, &read, NULL));
	
	for(i=0; i<5000; i++)
	{
		// Cortex A15
		power_data[i].rail[0].v0 = data[(4*i) + 0];
		power_data[i].rail[0].v1 = data[(4*i) + 1];

		// Cortex A7
		power_data[i].rail[1].v0 = data[(4*i) + 2];
		power_data[i].rail[1].v1 = data[(4*i) + 3];
	}
		
	if(read > 0)
	{
		printf("Acquired %d samples. Total %d\n", read, totalRead+=(4*read));
		fflush(stdout);
	}

	// Compute Power and Ampere with Voltage Difference
	compute_power(n_measure, power_data);
		
	// Write Power Data to File //////////////////////////////////////////////////////////////////////////////////////////////
	write_power_data(file_name, n_measure, power_data);
	
Error:
	if(DAQmxFailed(error))
	{
		DAQmxGetExtendedErrorInfo(errBuff, 2048);

		DAQmxStopTask(taskHandle);
		DAQmxClearTask(taskHandle);
		printf("DAQmx Error: %s\n", errBuff);
	}

	return 0;
}

// DoneCallBack function
int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void* callbackData)
{
	int32 error=0;
	char errBuff[2048]={'\0'};

	DAQmxErrChk(status);

Error:
	if(DAQmxFailed(error))
	{
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
		DAQmxClearTask(taskHandle);
		printf("DAQmx Error: %s\n", errBuff);
	}

	return 0;
}
