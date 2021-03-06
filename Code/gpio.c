#include "common.h"
#include "gpio.h"
#include "helper.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <limits.h>



const int BUFFER_SIZE = 16;
#define GPIO_COUNT 30

const char* PATH = "/sys/class/gpio/gpio";
const char* PATH_SUFFIX_VALUE = "/value";
const char* PATH_SUFFIX_DIR = "/direction";

const char* PATH_EXPORT = "/sys/class/gpio/export";
const char* PATH_UNEXPORT = "/sys/class/gpio/unexport";

char* PATH_CACHE[GPIO_COUNT];

void initPathCache(){
    for (int i = 0; i < GPIO_COUNT; i++){
        char pin[3];
        sprintf(pin, "%d", i);
        PATH_CACHE[i] = GPIO_getPath(pin, PATH_SUFFIX_VALUE);
    }   
}

void destroyPathCache(){
    for (int i = 0; i < GPIO_COUNT; i++){
        free(PATH_CACHE[i]);
    }
}

char* GPIO_getPath(char* pin, const char* suffix) {
	 //preparing for open
	int sizeConcat = strlen(PATH) + strlen(pin) + strlen(suffix);
	
	char* concatPath = (char*) malloc(sizeConcat + 1); //nullbyte
	enforceMalloc(concatPath);
	sprintf(concatPath, "%s%s%s", PATH, pin, suffix);
	
	return concatPath;
}

void writeSafe(char* path, char* value){
	if (VERBOSE_LOG_GPIO_DEF) printf("writeSafe path %s value %s\n", path, value);

    if (!GPIO_ENABLED) {
        if (VERBOSE_LOG_GPIO_DEF) {     
            printf("writeSafe skip"); 
        }
        return;
    }
	
	int gpio;
    int writeCount; 
	gpio = open(path, O_WRONLY);
	if (gpio != -1){
		if ((writeCount = write(gpio, value, strlen(value))) != strlen(value)){
			//ebusy is occuring on already exported files
			if (errno != EBUSY) {
				perror("write failed");
	            printf("cnt %d expected %d\n", writeCount, (int) strlen(value));
				exit(EXIT_FAILURE);
			}
		}

		if (close(gpio) != 0) {
			perror("writeSafe: close failed");
			exit(EXIT_FAILURE);
		}
	} else {
		perror("writeSafe: open failed");
		exit(EXIT_FAILURE);
	}
	
}


int GPIO_read(char* pin) {
	int value = -1; //default ret
	int readCount = 0;
	int gpio;
	char buffer[BUFFER_SIZE];
	

    if (!GPIO_ENABLED) {
        if (VERBOSE_LOG_GPIO_DEF) {     
            printf("GPIO_read skip"); 
        }
        return 0;
    }

    int pin_int = atoi(pin);	
    char* path = PATH_CACHE[pin_int];
	//char* path = GPIO_getPath(pin, PATH_SUFFIX_VALUE);
	
	gpio = open(path, O_RDONLY);
	if (gpio != -1){
		if ((readCount = read(gpio, buffer, BUFFER_SIZE)) != 2) {
			perror("read failed");
			printf("cnt %d\n", readCount);
			exit(EXIT_FAILURE);
		}
		
		value = buffer[0] - '0'; 
		
		//value validty check
		if (value != 0 && value != 1){
			printf("Value is unexpected. Buffer: %s\n", buffer);
			exit(EXIT_FAILURE);
		}

		if (close(gpio) != 0) {
			perror("close failed");
			exit(EXIT_FAILURE);
		}
	} else {
		perror("fopen failed");
		exit(EXIT_FAILURE);
	}
	return value;
}

void GPIO_set(char* pin, char value) {
    int pin_int = atoi(pin);	
    char* path = PATH_CACHE[pin_int];
	//char* path = GPIO_getPath(pin, PATH_SUFFIX_VALUE);

	
	char strValue = value + '0';
	char buffer[2] = { strValue, 0 };   
	writeSafe(path, (char*) buffer);
}


void GPIO_export(char* pin) {
	writeSafe((char*) PATH_EXPORT, pin);
}

void GPIO_unexport(char* pin) {
	writeSafe((char*)PATH_UNEXPORT, pin);
}

void GPIO_setDirection(char* pin, char* direction){
	char* path = GPIO_getPath(pin, PATH_SUFFIX_DIR);
	writeSafe(path, direction);
	
	free(path);
}

