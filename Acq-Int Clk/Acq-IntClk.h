#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <NIDAQmx.h>

#define FILE_TYPE ".txt"
#define SAMPLING_RATE 5000 //5KHz
#define NUM_OF_PDATA(sec)	(sec * SAMPLING_RATE)
#define NUM_OF_RAILS 3
#define RAIL0_OHM 0.05
#define RAIL1_OHM 0.3
#define RAIL2_OHM 0.0025
 
#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else

char		counter='0';

struct rail_power {
	float64 v0;
	float64 v1;
	float64 ampere;
	float64 watt;
};

struct p_data{
	int32 time_stamp;
	struct rail_power rail[NUM_OF_RAILS];
};

void write_power_data(char *file_name, int32 n_measure, struct p_data *power_data)
{
	int32 i;
	int32 j;
	FILE  *fp;
	fp=fopen(file_name, "a");
	if(fp==NULL){
		fprintf(stderr, "file open error...\n");
		exit(1);
	}

	for( i=0 ; i < n_measure ; i++ )
	{
		//fprintf(fp, "%d", power_data[i].time_stamp);
		for( j=0 ; j < 2 ; j++ )
		{
			fprintf(fp, "  %f,   %f,   %f",
					power_data[i].rail[j].v0, power_data[i].rail[j].ampere, power_data[i].rail[j].watt);
		}
		fprintf(fp, "\n");
	}
}

void compute_power(int32 n_measure, struct p_data *power_data)
{
	int32 i, j;
	float64 delta_v;
	float64 rail_ohm[NUM_OF_RAILS]= {RAIL0_OHM, RAIL1_OHM, RAIL2_OHM};

	for( i=0 ; i < n_measure ; i++ )
	{			power_data[i].time_stamp=i;
		for( j=0 ; j < NUM_OF_RAILS; j++ )
		{	
			// (v0 -v1) / ohm
			delta_v = power_data[i].rail[j].v0 - power_data[i].rail[j].v1;
			if(delta_v <= 0)
				power_data[i].rail[j].ampere = power_data[i].rail[j].watt = 0;
			else
			{
				power_data[i].rail[j].ampere = delta_v / rail_ohm[j];
				power_data[i].rail[j].watt = power_data[i].rail[j].v0 * power_data[i].rail[j].ampere;
			}
 		}
	}
}