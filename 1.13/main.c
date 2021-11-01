#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lcgrand.h"

#define MAX_VALUE 1.0e+30
#define MAX_QUEUE_LIMIT 50
#define NUM_EVENTS 5
#define MEAN_ARRIVAL_A 9
#define MEAN_ARRIVAL_B 5
#define MEAN_TRAVEL 31
#define SD_TRAVEL 5
#define TRAVELING 0
#define STOPPED 1
#define A 0
#define B 1

int bus_status, bus_station, next_event_type, init_N, end_N, current_N, count_queue_A, count_queue_B, count_passengers_A, count_passengers_B;

float simulation_time, clock, time_next_event[NUM_EVENTS + 1], time_queue_A[MAX_QUEUE_LIMIT + 1], time_queue_B[MAX_QUEUE_LIMIT + 1],
      sum_waiting_time_A, sum_waiting_time_B, exp_mean_arrival_A, exp_mean_arrival_B;

FILE *infile, *outfile;

void initialize(void);
void timing(void);
void arrival_A(void);
void arrival_B(void);
void travel(void);
void arrive(void);
void report(void);
float expon(float);
float standardNormal(void);
float normal(float, float);
main()
{
    printf("Simulation Started\n\n");

    /* Set seed for lcgrand stream */
    lcgrandst(time(0), 1);

    /* Open input and output files. */
    infile  = fopen("mm1.in",  "r");
    outfile = fopen("mm1.out", "w");

    /* Read input file and save variables */
    fscanf(infile, "%d %d %f", &init_N, &end_N, &simulation_time);


    /* Iterate between init_N and end_N */
    for(int i = init_N; i <= end_N; i++)
    {
        current_N = i;
        initialize();
        do
        {
            timing();
            switch(next_event_type)
            {
            case 1:
                arrival_A();
                break;
            case 2:
                arrival_B();
                break;
            case 3:
                travel();
                break;
            case 4:
                arrive();
                break;
            case 5:
                report();
                break;
            }
        }
        while(next_event_type != 5);
    }

    printf("Simulation finished\n");


}

void initialize()
{
    /* Initialize the simulation clock */
    clock = 0;

    /* Calculate means for use Exponential from Poisson */
    exp_mean_arrival_A = 60 / MEAN_ARRIVAL_A;
    exp_mean_arrival_B = 60 / MEAN_ARRIVAL_B;

    /* Initialize the state variables */
    count_queue_A = 0;
    count_queue_B = 0;
    count_passengers_A = 0;
    count_passengers_B = 0;
    bus_status = STOPPED;
    bus_station = A;

    /* Initialize the statistical counters */
    sum_waiting_time_A = 0;
    sum_waiting_time_B = 0;
    count_passengers_A = 0;
    count_passengers_B = 0;

    /* Initialize event list */
    time_next_event[1] = clock + expon(exp_mean_arrival_A);/* Passenger arrives A */
    time_next_event[2] = clock + expon(exp_mean_arrival_B);  /* Passenger arrives B */
    time_next_event[3] = MAX_VALUE; /* Start bus travel */
    time_next_event[4] = MAX_VALUE; /* Bus arrival */
    time_next_event[5] = simulation_time; /* End of simulation */


}

void timing(void)  /* Timing function. */
{
    int   i;
    float min_time_next_event = MAX_VALUE;

    next_event_type = 0;

    /* Determine the event type of the next event to occur. */

    for (i = 1; i <= NUM_EVENTS; ++i)
        if (time_next_event[i] < min_time_next_event)
        {
            min_time_next_event = time_next_event[i];
            next_event_type     = i;
        }

    clock = min_time_next_event;
}

void arrival_A(void)
{
    /* Count new passenger */
    count_queue_A += 1;
    /* Save arrival time of passenger */
    time_queue_A[count_queue_A] = clock;

    /* Analize if bus is ready to travel */
    if(count_queue_A >= current_N && bus_station == A && bus_status == STOPPED) time_next_event[3] = clock;

    /* Set next time of event */
    time_next_event[1] = clock + expon(exp_mean_arrival_A);
}

void arrival_B(void)
{
    /* Count new passenger */
    count_queue_B++;
    /* Save arrival time of passenger */
    time_queue_B[count_queue_B] = clock;

    /* Analize if bus is ready to travel */
    if(count_queue_B >= current_N && bus_station == B && bus_status == STOPPED) time_next_event[3] = clock;

    /* Set next time of event */
    time_next_event[2] = clock + expon(exp_mean_arrival_B);
}

void travel(void)
{
    bus_status = TRAVELING;
    time_next_event[3] = MAX_VALUE;
    switch(bus_station)
    {
    case A:
        count_passengers_A += count_queue_A;
        for(int i=1; i <= count_queue_A; i++)
        {
            sum_waiting_time_A += clock - time_queue_A[i];
            time_queue_A[i] = -1.0;
        }
        count_queue_A = 0;
        break;

    case B:
        count_passengers_B += count_queue_B;
        for(int i=1; i <= count_queue_B; i++)
        {
            sum_waiting_time_B += clock - time_queue_B[i];
            time_queue_B[i] = -1.0;
        }
        count_queue_B = 0;
        break;
    }

    time_next_event[4] = clock + normal(MEAN_TRAVEL, SD_TRAVEL);
}

void arrive(void)
{
    bus_status = STOPPED;
    time_next_event[4] = MAX_VALUE;
    switch(bus_station)
    {
    case A:
        bus_station = B;
        if(count_queue_B > current_N) time_next_event[3] = clock;
        break;
    case B:
        bus_station = A;
        if(count_queue_A > current_N) time_next_event[3] = clock;
        break;
    }

}

void report(void)
{
    /* Compute and write estimates of desired measures of performance. */
    fprintf(outfile, "Report for N = %d \n\n", current_N);
    fprintf(outfile, "Waiting mean A queue: %f  Waiting mean B queue: %f    Total waiting mean: %f\n\n",
            sum_waiting_time_A/count_passengers_A, sum_waiting_time_B/count_passengers_B, (sum_waiting_time_A + sum_waiting_time_B)/(count_passengers_A + count_passengers_B));
}

/*The exponential distribution is used model the amount of time between events in a Poisson process. */
float expon(float mean)  /* Exponential variate generation function. */
{
    /* Return an exponential random variate with mean "mean". */
    return -mean * log(lcgrand(1));
}

float standardNormal()
{
    float u = lcgrand(1) * 2 - 1;
    float v = lcgrand(1) * 2 - 1;
    float r = u * u + v * v;
    if (r == 0 || r > 1) return standardNormal();
    float c = sqrt(-2 * log(r) / r);
    return u * c;
}

float normal(float mean, float sd)
{
    return mean + sd * standardNormal();
}


