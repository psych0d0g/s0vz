/*
 * enocean.h
 *
 *  Created on: 28.07.2014
 *      Author: rensky
 */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <iostream>
#include <semaphore.h>
#include <pthread.h>
#include <list>
#include "DataStruct.h"

#ifndef ENOCEAN_H_
#define ENOCEAN_H_

#define TERM_SPEED B57600
#define KELVINNULL -273

struct bs4Data {
	double sumValue;
	int values;
	unsigned char sensorID[4];
	int minValue;
	int maxValue;
	double lastValue;
};

class EnOcean {
public:
	EnOcean();
	int start(const char *device);
	void stop(void);
	int addSensor(char *id, int min, int max);
	void getDataAndClean(valuePack *values, int number);
	virtual ~EnOcean();

protected:
	static void* callRunFunction(void *arg) { return ((EnOcean*)arg)->run(arg); }
	void* run(void *This);
	int findSync(unsigned char *buffer, unsigned int len);
	int numberSync(unsigned char *buffer, unsigned int len);
	void addValueToList(double value, unsigned char sensorID[4]);
	int charToHex (char c);
	int uart0_filestream;
	bool running;
	const static unsigned char u8CRC8Table[256];
	std::list<bs4Data> dataList;
	sem_t sem_data;
	pthread_t runningThread;
	const char *device;

};

#endif /* ENOCEAN_H_ */
