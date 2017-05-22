#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>

#include "common.h"
#include "engine.h"
#include "logic.h"
#include "helper.h"
#include "piezo.h"

struct timespec sleeptime;
int logic_mode = -1;

char ir_state = -1;
long us_distance = -1;
int rfid_state = -1;



void logic_test_engine(){
	//left test
	engineDrive(forward, stop);
	sleepAbsolute(1 * NANOSECONDS_PER_SECOND, &sleeptime); 
	engineDrive(reverse, stop);
	sleepAbsolute(1 * NANOSECONDS_PER_SECOND, &sleeptime);

	//right test
	engineDrive(stop, forward);
	sleepAbsolute(1 * NANOSECONDS_PER_SECOND, &sleeptime);
	engineDrive(stop, reverse);
	sleepAbsolute(1 * NANOSECONDS_PER_SECOND, &sleeptime);

	//both
	engineDrive(forward, forward);
	sleepAbsolute(1 * NANOSECONDS_PER_SECOND, &sleeptime);
	engineDrive(reverse, reverse);
	sleepAbsolute(1 * NANOSECONDS_PER_SECOND, &sleeptime);

	engineStop();
	sleepAbsolute(1 * NANOSECONDS_PER_SECOND, &sleeptime);
    
    // PWM test
    pwmTest();
    sleepAbsolute(3 * NANOSECONDS_PER_SECOND, &sleeptime);
}


void logic_test_rfid(){
	//simple rfid detectiont est: drive forward and stop if we detect a chip
	if (rfid_state == 1) {
		engineStop();
		logic_mode = none;

		if (VERBOSE_DEF) {
			printf("found chip, stopping logic");
		}
	} else {
		engineDrive(forward, forward);
	}

}

void logic_test_us(){
	//simple test: drive until we found a object
	if (us_distance < 15 * 1000) {
		engineStop();
		logic_mode = none;

		if (VERBOSE_DEF) {
			printf("found obejct, stopping logic");
		}
	} else {
		engineDrive(forward, forward);
	}

}

void logic_test_ir(){
	//TODO: find a useful testcase
	
}

void logic_test_piezo(){
    playTone();
    sleepAbsolute(1 * NANOSECONDS_PER_SECOND, &sleeptime);

    piezo_playReverse();
    sleepAbsolute(3 * NANOSECONDS_PER_SECOND, &sleeptime);
    piezo_stopReverse();
    logic_mode = none;
}

void logic_setup(int mode){
	logic_mode = mode;
}
void logic_shutdown(){
    
}
void logic_compute(){
	switch(logic_mode){
        case none:
            break;

		case track_path:
			break;
		case track_rfid_search:
			break;


		case test_ir:
			logic_test_ir();
			break;


		case test_us:
			logic_test_us();
			break;

		case test_rfid:
			logic_test_rfid();
			break;

		case test_engine:
			logic_test_engine();

        case test_piezo:
            logic_test_piezo();	

			break;
	}

}


void *exploitMeasurements(void *arg) {
    exploiterParams explparam = *(exploiterParams*) arg;
    
    while (true) {
         sleepAbsolute(INTERVAL_LOGIC * NANOSECONDS_PER_MILLISECOND, &sleeptime);


        //TODO: check timestamps, maybe include trylocks
        //infrared
        if(pthread_rwlock_rdlock(explparam.ir->lock)){
            perror("ir_rdlock failed");
        }
        
        if (explparam.ir->data != NULL) {
            ir_state = *((char*) explparam.ir->data);
        }
        
        if(pthread_rwlock_unlock(explparam.ir->lock)){
            perror("ir_wrlock failed");
        }
        
        //ultrasonic
        if(pthread_rwlock_rdlock(explparam.us->lock)){
            perror("us_rdlock failed");
        }
        
        if (explparam.us->data != NULL) {
            us_distance = *((long*) explparam.us->data);
        }
        
        if(pthread_rwlock_unlock(explparam.us->lock)){
            perror("us_wrlock failed");
        }
        
        //rifd
        if(pthread_rwlock_rdlock(explparam.rfid->lock)){
            perror("rfid_rdlock failed");
        }
        
        if (explparam.rfid->data != NULL) {        
            rfid_state = *((int*) explparam.rfid->data);
        }
        
        if(pthread_rwlock_unlock(explparam.rfid->lock)){
 			perror("rifd_unlock failed");
        }
          
        
        if (VERBOSE_DEF){
            printf("collectData:: ir_state %d, us_distance %ld, rfid_state %d \n", ir_state, us_distance, rfid_state);
        }
	        
	    if (ir_state == -1 || us_distance == -1 || rfid_state == -1) {
		    continue;
	    }

		logic_compute();
    }
}



