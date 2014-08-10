/**************************************************************************

 S0/Impulse to Volkszaehler 'RaspberryPI deamon'.

 https://github.com/w3llschmidt/s0vz.git

 Henrik Wellschmidt  <w3llschmidt@gmail.com>
 René Galow <rensky.g@googlemail.com>

 **************************************************************************/

/***************************************************************************
 * Compileranweisungen
 */

#define ENTWICKLUNG
/**************************************************************************/

#define DAEMON_NAME "s0vz"
#define DAEMON_VERSION "1.4"
#define DAEMON_BUILD "4"

/**************************************************************************

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 **************************************************************************/

#include "EnOcean.h"
#include "DataStruct.h"
#include <stdio.h>              /* standard library functions for file input and output */
#include <stdlib.h>             /* standard library for the C programming language, */
#include <string.h>             /* functions implementing operations on strings  */
#include <unistd.h>             /* provides access to the POSIX operating system API */
#include <sys/stat.h>           /* declares the stat() functions; umask */
#include <fcntl.h>              /* file descriptors */
#include <syslog.h>             /* send messages to the system logger */
#include <errno.h>              /* macros to report error conditions through error codes */
#include <signal.h>             /* signal processing */
#include <stddef.h>             /* defines the macros NULL and offsetof as well as the types ptrdiff_t, wchar_t, and size_t */
#include <dirent.h>				/* constructs that facilitate directory traversing */

#include <libconfig.h++>          /* reading, manipulating, and writing structured configuration files */
//#include <curl/curl.h>          /* multiprotocol file transfer library */
#include <poll.h>			/* wait for events on file descriptors */
#include <pthread.h>
#include <semaphore.h>
#include <string>
#include <iostream>     // std::cout
#include <sstream>
#include <sys/time.h>

#include <sys/ioctl.h>		/* */


using namespace std;
using namespace libconfig;

#define ENOCEAN_DEVICE "/dev/ttyUSB0"

#define BUF_LEN 64

void daemonShutdown();
void signal_handler(int sig);
void daemonize(char *rundir, char *pidfile);

int pidFilehandle, vzport, len, running_handles, rc, count, tempSensors, enOceanNumberSensors;
unsigned int LogLevel;
const char *Datafolder, *Messstellenname, *uuid, *EnOceanDevice;
const char *W1Sensor[100];
const char *EnOceanSensor[100], *EnOceanTemperaturbereich[100];
int Mittelwertzeit = 0;
int Impulswerte[6];
int tempraturIntervall = 0;
EnOcean *TheOcean;

char crc_ok[] = "YES";
char not_found[] = "not found.";

char gpio_pin_id[] = { 17, 18, 27, 22, 23, 24 }, url[128];

int inputs = sizeof(gpio_pin_id) / sizeof(gpio_pin_id[0]);

double temp;
Config cfg;

struct timeval tv;

struct valuePack *values;

sem_t sem_averrage;

//CURL *easyhandle[sizeof(gpio_pin_id) / sizeof(gpio_pin_id[0])];
//CURLM *multihandle;
//CURLMcode multihandle_res;

//static char errorBuffer[CURL_ERROR_SIZE + 1];

void signal_handler(int sig) {

	switch (sig) {
	case SIGHUP:
		syslog(LOG_WARNING, "Received SIGHUP signal.");
		break;
	case SIGINT:
	case SIGTERM:
		syslog(LOG_INFO, "Daemon exiting");
		daemonShutdown();
		exit(EXIT_SUCCESS);
		break;
	default:
		syslog(LOG_WARNING, "Unhandled signal %s", strsignal(sig));
		break;
	}
}

void daemonShutdown() {
	close(pidFilehandle);
	remove("/tmp/s0vz.pid");
}

void daemonize(char *rundir, char *pidfile) {
	int pid, sid, i;
	char str[10];
	struct sigaction newSigAction;
	sigset_t newSigSet;

	if (getppid() == 1) {
		return;
	}

	sigemptyset(&newSigSet);
	sigaddset(&newSigSet, SIGCHLD);
	sigaddset(&newSigSet, SIGTSTP);
	sigaddset(&newSigSet, SIGTTOU);
	sigaddset(&newSigSet, SIGTTIN);
	sigprocmask(SIG_BLOCK, &newSigSet, NULL);

	newSigAction.sa_handler = signal_handler;
	sigemptyset(&newSigAction.sa_mask);
	newSigAction.sa_flags = 0;

	sigaction(SIGHUP, &newSigAction, NULL);
	sigaction(SIGTERM, &newSigAction, NULL);
	sigaction(SIGINT, &newSigAction, NULL);

	pid = fork();

	if (pid < 0) {
		exit(EXIT_FAILURE);
	}

	if (pid > 0) {
		printf("Child process created: %d\n", pid);
		exit(EXIT_SUCCESS);
	}

	umask(027);

	sid = setsid();
	if (sid < 0) {
		exit(EXIT_FAILURE);
	}

	for (i = getdtablesize(); i >= 0; --i) {
		close(i);
	}

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	chdir(rundir);

	pidFilehandle = open(pidfile, O_RDWR | O_CREAT, 0600);

	if (pidFilehandle == -1) {
		syslog(LOG_INFO, "Could not open PID lock file %s, exiting", pidfile);
		exit(EXIT_FAILURE);
	}

	if (lockf(pidFilehandle, F_TLOCK, 0) == -1) {
		syslog(LOG_INFO, "Could not lock PID lock file %s, exiting", pidfile);
		exit(EXIT_FAILURE);
	}

	sprintf(str, "%d\n", getpid());

	write(pidFilehandle, str, strlen(str));
}

int cfile() {
	int i = 0;
	Config cfg;

	//config_setting_t *setting;

	//config_init(&cfg);

	//int chdir(const char *path);

	//chdir ("/etc");
	//char configfile[200];
	std::stringstream configfile;
	#ifdef ENTWICKLUNG
	//sprintf(configfile, "%s%s", DAEMON_NAME, ".cfg");

	configfile << "./s0vz" << ".cfg";
	#else
	//sprintf(configfile, "%s%s%s", "/etc/", DAEMON_NAME, ".cfg");
	configfile << "/etc/" << DAEMON_NAME << ".cfg";
	#endif

	try
	{
		cfg.readFile(configfile.str().c_str());
	}
	catch(const FileIOException &fioex)
	{
		std::cerr << "I/O error while reading file." << std::endl;
		daemonShutdown();
		return(EXIT_FAILURE);
	}
	catch(const ParseException &pex)
	{
	    std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine() << " - " << pex.getError() << std::endl;
		return(EXIT_FAILURE);
	}

	try
	{
		if (cfg.lookupValue("Datafolder",Datafolder))
		{
			char *tmp = (char*) malloc(strlen(Datafolder)+1);
			memcpy(tmp,Datafolder,strlen(Datafolder)+1);
			Datafolder = tmp;
			syslog(LOG_INFO, "Datafolder:%s", Datafolder);
		}
	}
	catch(const SettingNotFoundException &nfex)
	{
		cerr << "No Datafolder setting in configuration file." << endl;
	}

	try
	{
		if (cfg.lookupValue("Messstelle",Messstellenname))
		{
			char *tmp = (char*) malloc(strlen(Messstellenname)+1);
			memcpy(tmp,Messstellenname,strlen(Messstellenname)+1);
			Messstellenname = tmp;
			syslog(LOG_INFO, "Messstelle:%s", Datafolder);
		}
	}
	catch(const SettingNotFoundException &nfex)
	{
		cerr << "No Messstelle setting in configuration file." << endl;
	}

	try
	{
		cfg.lookupValue("Mittelwertzeit", Mittelwertzeit);
		syslog(LOG_INFO, "Mittelwertzeit:%d", Mittelwertzeit);
	}
	catch(const SettingNotFoundException &nfex)
	{
		cerr << "No Mittelwertzeit setting in configuration file." << endl;
	}

	try
	{
		cfg.lookupValue("TempraturIntervall",tempraturIntervall);
		syslog(LOG_INFO, "TemperaturInterval:%d", tempraturIntervall);
	}
	catch(const SettingNotFoundException &nfex)
	{
		cerr << "No TempraturIntervall setting in configuration file." << endl;
	}

	try
	{
		cfg.lookupValue("LogLevel", LogLevel);
		syslog(LOG_INFO, "LogLevel:%d", LogLevel);
	}
	catch(const SettingNotFoundException &nfex)
	{
		LogLevel = 4;
		cerr << "No LogLevel setting in configuration file." << endl;
	}

	stringstream name;
	for (i = 0; i < inputs; i++) {
		name.str("");
		name << "GPIO" << i;
		try
		{
			cfg.lookupValue(name.str(),Impulswerte[i]);
			syslog( LOG_INFO, "%s = %d", name.str().c_str(), Impulswerte[i]);
		}
		catch(const SettingNotFoundException &nfex)
		{
			cerr << "No " << name.str() << " setting in configuration file." << endl << std::flush;
		}
	}

	tempSensors = 0;
	for (i = 0; i < 100; i++) {
		name.str("");
		name << "W1Dev" << i;
		try
		{
			if (cfg.lookupValue(name.str(),W1Sensor[i]))
			{
				//cout << "Sensor ID: " << W1Sensor[i] << endl << std::flush;
				char *tmp = (char*) malloc(strlen(W1Sensor[i])+1);
				memcpy(tmp,W1Sensor[i],strlen(W1Sensor[i])+1);
				W1Sensor[i] = tmp;

				syslog( LOG_INFO, "%s = %s", name.str().c_str(), W1Sensor[i]);
				tempSensors++;

			}
		}
		catch(const SettingNotFoundException &nfex)
		{
			cerr << "No " << name.str() << " setting in configuration file." << endl << std::flush;
		}
	}

	try
	{
		if (cfg.lookupValue("EnOceanDevice",EnOceanDevice))
		{
			char *tmp = (char*) malloc(strlen(EnOceanDevice)+1);
			memcpy(tmp,Messstellenname,strlen(EnOceanDevice)+1);
			EnOceanDevice = tmp;
			syslog(LOG_INFO, "EnOceanDevice:%s", EnOceanDevice);
		}
	}
	catch(const SettingNotFoundException &nfex)
	{
		cerr << "No EnOceanDevice setting in configuration file." << endl;
	}


	enOceanNumberSensors = 0;
	stringstream name2;
		for (i = 0; i < 100; i++) {
			name.str("");
			name2.str("");
			name << "EnOceanSensor" << i;
			name2 << "EnOceanAria" << i;
			try
			{
				if (cfg.lookupValue(name.str(),EnOceanSensor[i]) &&
					cfg.lookupValue(name2.str(),EnOceanTemperaturbereich[i]))
				{
					char *tmp = (char*) malloc(strlen(EnOceanSensor[i])+1);
					memcpy(tmp,EnOceanSensor[i],strlen(EnOceanSensor[i])+1);
					EnOceanSensor[i] = tmp;
					tmp = (char*) malloc(strlen(EnOceanTemperaturbereich[i])+1);
					memcpy(tmp,EnOceanTemperaturbereich[i],strlen(EnOceanTemperaturbereich[i])+1);
					EnOceanTemperaturbereich[i] = tmp;

					//cout << "Sensor ID: " << W1Sensor[i] << endl << std::flush;
					syslog( LOG_INFO, "%s = %s aria %s", name.str().c_str(), EnOceanSensor[i],
														EnOceanTemperaturbereich[i]);
					enOceanNumberSensors++;
				}
			}
			catch(const SettingNotFoundException &nfex)
			{
				cerr << "No " << name.str() << " setting in configuration file." << endl << std::flush;
			}
		}

	return 0;
}

void logPrint(char *msg, unsigned int level)
{
	char date_time_string[20];
	struct tm* ptm;
	gettimeofday(&tv, NULL);
	ptm = localtime(&tv.tv_sec);
	strftime(date_time_string, sizeof(date_time_string), "%Y-%m-%d %H:%M:%S",ptm);
	if (level >= LogLevel )
		printf("[%s] [%d] %s",date_time_string, level, msg);

}

unsigned long long unixtime() {

	gettimeofday(&tv, NULL);
	unsigned long long ms_timestamp = (unsigned long long) (tv.tv_sec) * 1000
			+ (unsigned long long) (tv.tv_usec) / 1000;

	return ms_timestamp;
}

int appendToFile(const char *filename, char *str) {
	FILE *fd;
	struct stat st = { 0 };
	struct tm* ptm;
	char date_time_string[20];
	char date_string[11];
	char filepath[200];
	char str2[200];

	/* Create directory if not exist*/
	if (stat(filename, &st) == -1) {
		mkdir(filename, 0700);
	}

	/* Filename ermitteln anhand des Datums */
	gettimeofday(&tv, NULL);
	ptm = localtime(&tv.tv_sec);
	strftime(date_string, sizeof(date_string), "%Y-%m-%d", ptm);
	strftime(date_time_string, sizeof(date_time_string), "%Y-%m-%d %H:%M:%S",
			ptm);
	sprintf(filepath, "%s/%s.csv", filename, date_string);
	sprintf(str2, "%s;%s\n", date_time_string, str);
	printf("Now will add to file: %s this string: %s", filepath, str2);

	fd = fopen(filepath, "a");
	if (fd != NULL) {
		fputs(str2, fd);
		fclose(fd);
		return 0;
	}
	return 1;
}

void update_average_values(struct valuePack *vP) {
	unsigned long ts = unixtime();
	int time = 0;
	double wattProImpuls = 0;
	double tmp_value = 0;
	if (vP->lastTs != 0) {
		sem_wait(&sem_averrage);
		time = (int) (ts - vP->lastTs);
		wattProImpuls = 1000.0 / (double) vP->impulsConst;
		tmp_value = wattProImpuls * (3.6 / (double) time) * 1000000.0; // Zeit in MS
		vP->valuesAsSumm += tmp_value / 1000.0;
		vP->numberOfValues++;
		sem_post(&sem_averrage);
		printf("Summe: %.3f Anzahl %d TMPValue: %.3f Zeit: %d ms \n",
				vP->valuesAsSumm, vP->numberOfValues, tmp_value, time);
	}

	vP->lastTs = ts;

}

void *intervallFunction(void *time) { // Der Type ist wichtig: void* als Parameter und Rückgabe
	int t = *((int*) time);
	int i = 0;
	double averrage[inputs + tempSensors];
	char str[200];
	str[0] = '\0';

	printf("Thread created\n");

	while (1) {
		sleep(t);
		// Temperatursorwerte holen und Mittelwert berechnen
		sem_wait(&sem_averrage);
		for (i = 0; i < (inputs + tempSensors); i++) {
			if (values[i].numberOfValues > 0) {
				averrage[i] = values[i].valuesAsSumm / values[i].numberOfValues;
			} else {
				averrage[i] = 0;
			}
			values[i].numberOfValues = 0;
			values[i].valuesAsSumm = 0;
			sprintf(str, "%s%.3f;", str, averrage[i]);
		}
		sem_post(&sem_averrage);

		TheOcean->getDataAndClean(&values[inputs + tempSensors ], enOceanNumberSensors);

		for (i = inputs + tempSensors; i < (inputs + tempSensors + enOceanNumberSensors); i++) {
			if (values[i].numberOfValues > 0) {
				if (values[i].valuesAsSumm <= KELVINNULL)
				{
					sprintf(str, "%sNoValue;", str);
				}
				else
				{
					averrage[i] = values[i].valuesAsSumm / values[i].numberOfValues;
					sprintf(str, "%s%.3f;", str, averrage[i]);
				}

			} else {
				averrage[i] = 0;
			}
			values[i].numberOfValues = 0;
			values[i].valuesAsSumm = 0;

		}

		sprintf(str, "%s", str);

		size_t len = strlen(str);
		if(len>0)
		  str[len-1] = '\0';

		if (appendToFile(Datafolder, str) != 0) {
			printf("Can not append to File %s.",
					"filename_noch_nicht_vergeben");
		}
		str[0] = '\0';
	}
	printf("Thread wird beendet\n");
	return NULL;  // oder in C++: return 0;// Damit kann man Werte zurückgeben
}

/** *********************************
 *Beginn der Temperatur Funktionen
 */

int ds1820read(const char *sensorid, double *temp) {

	FILE *fp;
	char crc_buffer[64], temp_buffer[64], fn[128];

	printf("Lese Temperatur von %s.\n", sensorid);
	sprintf(fn, "/sys/bus/w1/devices/%s/w1_slave", sensorid);

	if ((fp = fopen(fn, "r")) == NULL) {
		return (-1);
	} else {
		fgets(crc_buffer, sizeof(crc_buffer), fp);
		if (!strstr(crc_buffer, crc_ok)) {
			syslog(LOG_INFO, "CRC check failed, SensorID: %s", sensorid);
			fclose(fp);
			return (-1);
		} else {
			fgets(temp_buffer, sizeof(temp_buffer), fp);
			fgets(temp_buffer, sizeof(temp_buffer), fp);
			char *pos = strstr(temp_buffer, "t=");
			if (pos == NULL)
				return -1;

			pos += 2;
			*temp = atof(pos) / 1000;
			fclose(fp);
			return 0;
		}
	}
}

void *intervallTemperatur(void *time) { // Der Type ist wichtig: void* als Parameter und Rückgabe
	int t = *((int*) time);
	int i = 0;
	int SensorNumber = 0;
	double temp;
	int returnValue;
	printf("Temperatur Thread created\n");

	while (1) {
		SensorNumber = 0;
		for (i = 0; i < 100; i++) {
			if (W1Sensor[i] != NULL) {
				returnValue = ds1820read(W1Sensor[i], &temp);
				if (returnValue == 0) {
					sem_wait(&sem_averrage);
					values[inputs + SensorNumber].valuesAsSumm += temp;
					values[inputs + SensorNumber].numberOfValues++;
					sem_post(&sem_averrage);
				}
			}
			SensorNumber++;
		}

		sleep(t);
	}
	printf("Thread wird beendet\n");
	return NULL;  // oder in C++: return 0;// Damit kann man Werte zurückgeben
}

int main(void) {
	int i = 0;
	//freopen( "/dev/null", "r", stdin);
	//freopen( "/dev/null", "w", stdout);
	//freopen( "/dev/null", "w", stderr);

	FILE* devnull = NULL;
	devnull = fopen("/dev/null", "w+");

	setlogmask(LOG_UPTO(LOG_INFO));
	openlog(DAEMON_NAME, LOG_CONS | LOG_PERROR, LOG_USER);
	printf("Programm beginnt....");
	syslog( LOG_INFO, "S0/Impulse to Volkszaehler RaspberryPI deamon %s.%s",
			DAEMON_VERSION, DAEMON_BUILD);

	cfile();

	char pid_file[16];
	sprintf(pid_file, "/tmp/%s.pid", DAEMON_NAME);

	#ifdef ENTWICKLUNG
	#else
	daemonize( "/tmp/", pid_file );
	#endif

	values = (struct valuePack*) malloc(
			sizeof(struct valuePack) * (inputs + tempSensors + enOceanNumberSensors));

	char buffer[BUF_LEN];
	struct pollfd fds[inputs];

//	curl_global_init(CURL_GLOBAL_ALL);
//	multihandle = curl_multi_init();

	for (i = 0; i < inputs; i++) {
		snprintf(buffer, BUF_LEN, "/sys/class/gpio/gpio%d/value",
				gpio_pin_id[i]);

		if ((fds[i].fd = open(buffer, O_RDONLY | O_NONBLOCK)) == 0) {

			syslog(LOG_INFO, "Error:%s (%m)", buffer);
			exit(1);

		}

		fds[i].events = POLLPRI;
		fds[i].revents = 0;

//		easyhandle[i] = curl_easy_init();
//
//		curl_easy_setopt(easyhandle[i], CURLOPT_URL, url);
//		curl_easy_setopt(easyhandle[i], CURLOPT_POSTFIELDS, "");
//		curl_easy_setopt(easyhandle[i], CURLOPT_USERAGENT,
//				DAEMON_NAME " " DAEMON_VERSION);
//		curl_easy_setopt(easyhandle[i], CURLOPT_WRITEDATA, devnull);
//		curl_easy_setopt(easyhandle[i], CURLOPT_ERRORBUFFER, errorBuffer);
//
//		curl_multi_add_handle(multihandle, easyhandle[i]);

		values[i].numberOfValues = 0;
		values[i].valuesAsSumm = 0;
		values[i].impulsConst = Impulswerte[i];
		values[i].lastTs = 0;

	}

	for (i = inputs; i < (inputs + tempSensors + enOceanNumberSensors); i++) {
		values[i].numberOfValues = 0;
		values[i].valuesAsSumm = 0;
		values[i].impulsConst = 0;
		values[i].lastTs = 0;
	}

	sem_init(&sem_averrage, 0, 1);
	/* Thread erstellen für interval Berechnung*/
	pthread_t intervalThread, intervalTemperaturThread;
	if (Mittelwertzeit <= 0 )
		Mittelwertzeit = 60;
	if (pthread_create(&intervalThread, NULL, intervallFunction,
			(void *) &Mittelwertzeit) != 0) {
		printf("Thread can not be create.");
		exit(1);
	}

	if (tempraturIntervall <= 0 )
		tempraturIntervall = 30;
	if (pthread_create(&intervalTemperaturThread, NULL, intervallTemperatur,
			(void *) &tempraturIntervall) != 0) {
		printf("Thread can not be create.");
		exit(1);
	}

	TheOcean = new EnOcean();

	for (i = 0; i < 100; i++) {
		if (EnOceanSensor[i] != NULL) {
			string bereich = EnOceanTemperaturbereich[i];
			std::size_t pos = bereich.find(" ");
			std::string maxstr = bereich.substr (pos);
			int min = atoi(bereich.c_str());
			int max = atoi(maxstr.c_str());
			istringstream buffer(bereich);
			buffer >> min >> max;
			//std::size_t found = str.find(EnOceanTemperaturbereich[i]);
			//char *s = strchr (bereich, ' ');

			if (TheOcean->addSensor((char *)EnOceanSensor[i],min , max) != 0)
			{
				printf("Error can not add Sensor ID, ID %s is not a valid ID", "008281C9");
			}
		}
	}

	char device[] = ENOCEAN_DEVICE;
	TheOcean->start(EnOceanDevice);

	for (;;) {

//		if ((multihandle_res = curl_multi_perform(multihandle, &running_handles))
//				!= CURLM_OK) {
//			syslog(LOG_INFO, "HTTP_POST(): %s",
//					curl_multi_strerror(multihandle_res));
//		}

		int ret = poll(fds, inputs, 1000);

		if (ret > 0) {

			for (i = 0; i < inputs; i++) {
				if (fds[i].revents & POLLPRI) {
					len = read(fds[i].fd, buffer, BUF_LEN);
					//update_curl_handle(vzuuid[i]);
					update_average_values(&values[i]);
				}
			}
		}
	}

	//curl_global_cleanup();
	delete TheOcean;
	return 0;
}
