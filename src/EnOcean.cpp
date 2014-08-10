/*
 * enocean.cpp
 *
 *  Created on: 28.07.2014
 *      Author: rensky
 */

#include "EnOcean.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>

#define STATUS_NICHT_VOLLSTAENDIG 0
#define STATUS_VOLLSTAENDIG 1


struct enOceanESP3 {
	unsigned short HeaderDataLength;
	unsigned char HeaderOptionalLength;
	unsigned char HeaderPacketType;
	unsigned char HeaderCRC8;
	unsigned char *Data;
	unsigned char *OptionalData;
	unsigned char DataCRC8;
};

#define BS4DATALENG 4
struct enOcean4BS {
	unsigned char rOrg;  	// R-ORG = 0xA5
	unsigned char data0; 	// 0b0000n000 -> DB_0.BIT 3 = Learn Bit, Normal mode = 1 / Teach In = 0
	unsigned char data1; 	// Temperature value 255 ... 0
	unsigned char data2; 	// unused
	unsigned char data3; 	// unused
	bool lerntaste;
	unsigned char senderID[4];
	unsigned char status; 	// Telegram control bits – used in case of
							// repeating, switch telegram encapsulation,
							// checksum type identification
};

struct enOceanESP3OptionalData {
	unsigned char subTelNum;		// Number of subtelegram; Send: 3 / receive: 1 ... y
	unsigned char destinationID[4]; // ADT radio: Destination ID (= address)
	unsigned char dBm;				// Send case: FF
									// Receive case: best RSSI value of all
									// received subtelegrams (value decimal
									// without minus)
	unsigned char securityLevel;	// 0x0n -> 0 = telegram unencrypted
									// n = type of encryption (not supported any more)

};

EnOcean::EnOcean() {
	// TODO Auto-generated constructor stub
	running = false;
	uart0_filestream = -1;
	sem_init(&sem_data, 0, 1);

}

EnOcean::~EnOcean() {
	// TODO Auto-generated destructor stub
}

const unsigned char EnOcean::u8CRC8Table[256] = {
	0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15,
	0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d,
	0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65,
	0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d,
	0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5,
	0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd,
	0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85,
	0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd,
	0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2,
	0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea,
	0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2,
	0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a,
	0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32,
	0x1f, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0d, 0x0a,
	0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42,
	0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a,
	0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c,
	0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4,
	0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec,
	0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4,
	0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c,
	0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44,
	0x19, 0x1e, 0x17, 0x10, 0x05, 0x02, 0x0b, 0x0c,
	0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34,
	0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b,
	0x76, 0x71, 0x78, 0x7f, 0x6A, 0x6d, 0x64, 0x63,
	0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b,
	0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13,
	0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb,
	0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8D, 0x84, 0x83,
	0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb,
	0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3
};


int EnOcean::start(const char *device ){

	//OPEN THE UART
	//The flags (defined in fcntl.h):
	//	Access modes (use 1 of these):
	//		O_RDONLY - Open for reading only.
	//		O_RDWR - Open for reading and writing.
	//		O_WRONLY - Open for writing only.
	//
	//	O_NDELAY / O_NONBLOCK (same function) - Enables nonblocking mode. When set read requests on the file can return immediately with a failure status
	//											if there is no input immediately available (instead of blocking). Likewise, write requests can also return
	//											immediately with a failure status if the output can't be written immediately.
	//
	//	O_NOCTTY - When set and path identifies a terminal device, open() shall not cause the terminal device to become the controlling terminal for the process.
	uart0_filestream = open(device, O_RDWR | O_NOCTTY | O_NDELAY);		//Open in non blocking read/write mode
	if (uart0_filestream == -1)
	{
		//ERROR - CAN'T OPEN SERIAL PORT
		printf("Error - Unable to open UART.  Ensure it is not in use by another application\n");
	}

	//CONFIGURE THE UART
	//The flags (defined in /usr/include/termios.h - see http://pubs.opengroup.org/onlinepubs/007908799/xsh/termios.h.html):
	//	Baud rate:- B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200, B230400, B460800, B500000, B576000, B921600, B1000000, B1152000, B1500000, B2000000, B2500000, B3000000, B3500000, B4000000
	//	CSIZE:- CS5, CS6, CS7, CS8
	//	CLOCAL - Ignore modem status lines
	//	CREAD - Enable receiver
	//	IGNPAR = Ignore characters with parity errors
	//	ICRNL - Map CR to NL on input (Use for ASCII comms where you want to auto correct end of line characters - don't use for bianry comms!)
	//	PARENB - Parity enable
	//	PARODD - Odd parity (else even)
	struct termios options;
	tcgetattr(uart0_filestream, &options);
	options.c_cflag = B57600 | CS8 | CLOCAL | CREAD;		//<Set baud rate
	options.c_iflag = IGNPAR;
	options.c_oflag = 0;
	options.c_lflag = 0;
	tcflush(uart0_filestream, TCIFLUSH);
	tcsetattr(uart0_filestream, TCSANOW, &options);

	this->device = device;
	running = true;

	if (pthread_create(&runningThread, NULL, EnOcean::callRunFunction, this) != 0) {
		printf("Running Thread can not be create.");
	}
	else
	{
		printf("Enocean Thread gestartet.");
	}
	return true;
}

void EnOcean::stop(void){
	running = false;
}

int EnOcean::findSync(unsigned char *buffer, unsigned int len){
	unsigned int i;
	for(i=0;i<len; i++ )
	{
		if (buffer[i]== 0x55)
		{
			break;
		}
	}
	if (i == len)
		return -1;
	else
		return i;
}

int EnOcean::numberSync(unsigned char *buffer, unsigned int len){
	unsigned int i;
	int anzahl = 0;
	for(i=0;i<len; i++ )
	{
		if (buffer[i]== 0x55)
		{
			anzahl++;
		}
	}
	return anzahl;
}

/*
 * Empfängt EnOcean Stings und spiechert die Werte in einem Array ab
 * Sync	Header			CRC8	DATA	  						Optional Data			CRC8 DATA
 * 0	1     3	 4		5		6								6+0a					6+0a+07
 * 55   00 0a 07 01 	eb 		a5 00 00 5a 08 00 82 81 c9 00  	01 ff ff ff ff 2f 00 	02
 */

int EnOcean::charToHex (char c)
{
     int n = 0;

	 n *= 16;
	 if (c >='0' && c <= '9')
		 n = n + c-'0';
	 else if (c >= 'A' && c <='F')
		 n = n + c-'A' + 10;
	 else if (c >= 'a' && c <='f')
		 n = n + c-'a' + 10;
	 else
		 return -1;

     return n;
}

int EnOcean::addSensor(char *id, int min, int max)
{
	unsigned char sensorID[4];
	for (int i = 0; i<4; i++)
	{
		char one = charToHex(id[i*2]);
		char two = charToHex(id[(i*2)+1]);
		if (one == -1 || two == -1)
			return -1;
		sensorID[i] = (one << 4) | two;
	}

	bs4Data element; //*element = malloc(sizeof(bs4Data));
	memcpy(element.sensorID, sensorID, sizeof(unsigned char)* 4 );
	element.sumValue = 0;
	element.values = 0;
	element.minValue = min;
	element.maxValue = max;
	element.lastValue = KELVINNULL;
	dataList.push_back(element);


	return 0;
}

void EnOcean::addValueToList(double value, unsigned char sensorID[4])
{

	for (std::list<bs4Data>::iterator it=dataList.begin(); it != dataList.end(); ++it)
	{
		if (it->sensorID[0] == sensorID[0] &&
			it->sensorID[1] == sensorID[1] &&
			it->sensorID[2] == sensorID[2] &&
			it->sensorID[3] == sensorID[3] )
		{
			it->sumValue += value;
			it->values++;
		}
	}
}

void EnOcean::getDataAndClean(valuePack *values, int number)
{
	int i = 0;
	sem_wait(&sem_data);
	for (std::list<bs4Data>::iterator it=dataList.begin(); it != dataList.end(); ++it)
	{
		if (i >= number) {
					break;
		}
		if (it->lastValue <= KELVINNULL && it->values == 0)
		{
			values[i].valuesAsSumm = KELVINNULL;
			values[i].numberOfValues = 1;
		}
		else if (it->values == 0)
		{
			values[i].valuesAsSumm = it->lastValue;
			values[i].numberOfValues = 1;
		}
		else
		{
			values[i].valuesAsSumm = it->sumValue;
			values[i].numberOfValues = it->values;
			it->lastValue = it->sumValue/it->values;
		}
it->sumValue = 0;
		it->values = 0;
		i++;
	}
	sem_post(&sem_data);
}

void* EnOcean::run(void *This){
	unsigned char rx_buffer[255];
	unsigned char proto_buffer[255];
	int position = 0;
	if (uart0_filestream != -1)
	{
		printf("%s is connected. \n",device);
	}
	if (((EnOcean *)This)->uart0_filestream != -1)
	{
		int rx_length = 0;
		while (((EnOcean *)This)->running)
		{
			rx_length = read(((EnOcean *)This)->uart0_filestream, (void*)rx_buffer, 255); //Filestream, buffer to store in, number of bytes to read (max)
			if (rx_length < 0)
			{
				//An error occured (will occur if there are no bytes)
			}
			else if (rx_length == 0)
			{
				//No data waiting
			}
			else
			{
				int i;
				for (i=0; i<rx_length; i++)
				{
					printf("%.2x ", rx_buffer[i]);
					proto_buffer[position] = rx_buffer[i];
					position++;
				}
				printf("\n");

				int syncBytePosition = 0;
				int status = STATUS_NICHT_VOLLSTAENDIG;
				do {

					syncBytePosition = ((EnOcean *)This)->findSync(proto_buffer, rx_length);
					if (syncBytePosition > 0 )
					{
						memcpy((void*)proto_buffer, &proto_buffer[syncBytePosition], sizeof(unsigned char) * position);
						position -= syncBytePosition;
						syncBytePosition = ((EnOcean *)This)->findSync(proto_buffer, rx_length);
					}
					if (position > 6)
					{
						//printf("Sync Byte on position %i\n", syncBytePosition);

						struct enOceanESP3 enOceanPacket;
						enOceanPacket.HeaderDataLength = proto_buffer[1] << 8 | proto_buffer[2];
						enOceanPacket.HeaderOptionalLength = proto_buffer[3];
						enOceanPacket.HeaderPacketType = proto_buffer[4];
						enOceanPacket.HeaderCRC8 = proto_buffer[5];
						unsigned char crch = 0x00;
						crch = u8CRC8Table[crch ^ proto_buffer[1]];
						crch = u8CRC8Table[crch ^ proto_buffer[2]];
						crch = u8CRC8Table[crch ^ proto_buffer[3]];
						crch = u8CRC8Table[crch ^ proto_buffer[4]];
						int DataLenght = 6 + enOceanPacket.HeaderOptionalLength + enOceanPacket.HeaderDataLength + 1;
						if(enOceanPacket.HeaderCRC8 == crch)
						{
							if (position >= DataLenght)
							{
								enOceanPacket.Data = (unsigned char *) malloc(enOceanPacket.HeaderDataLength * sizeof(unsigned char));
								unsigned char crc = 0x00;
								int to = 0;
								int from = 6;
								for (; to<enOceanPacket.HeaderDataLength ; to++,from++)
								{
									enOceanPacket.Data[to] =  proto_buffer[from];
									crc = u8CRC8Table[crc ^ proto_buffer[from]];
								}
								enOceanPacket.OptionalData = (unsigned char *) malloc(enOceanPacket.HeaderOptionalLength * sizeof(unsigned char));
								to = 0;
								from = 6 + enOceanPacket.HeaderDataLength;
								for (; to<enOceanPacket.HeaderOptionalLength ; to++,from++)
								{
									enOceanPacket.Data[to] =  proto_buffer[from];
									crc = u8CRC8Table[crc ^ proto_buffer[from]];
								}
								enOceanPacket.DataCRC8 = proto_buffer[ 6 + enOceanPacket.HeaderDataLength +
								                                       	   enOceanPacket.HeaderOptionalLength];
								if (enOceanPacket.DataCRC8 == crc)
								{
									// Alles Richtig, Datenpaket Auseiander nehmen

									// Datenstrucktur übernehmen
									struct enOcean4BS data4BS;
									data4BS.rOrg = proto_buffer[6];
									data4BS.data0 = proto_buffer[10];
									data4BS.data1 = proto_buffer[9];
									data4BS.data2 = proto_buffer[8];
									data4BS.data3 = proto_buffer[7];
									data4BS.lerntaste = (bool)((data4BS.data0 >> 3) & 0x01);
									memcpy((void*)data4BS.senderID, &proto_buffer[7 + BS4DATALENG], sizeof(unsigned char) * 4);
									data4BS.data0 = proto_buffer[11 + BS4DATALENG];

									// Optional Data übernehmen
									struct enOceanESP3OptionalData enOceanData;
									enOceanData.subTelNum = proto_buffer[6 + enOceanPacket.HeaderDataLength];
									memcpy((void*)enOceanData.destinationID,
										   &proto_buffer[7 + enOceanPacket.HeaderDataLength], sizeof(unsigned char) * 4);
									enOceanData.dBm = proto_buffer[11 + enOceanPacket.HeaderDataLength];
									enOceanData.securityLevel = proto_buffer[12 + enOceanPacket.HeaderDataLength];

									// Daten Erfolgreich Portiert in Structs
									if (data4BS.lerntaste || !data4BS.lerntaste)
									{
										int minTemperatur = 0;
										int maxTemperatur = 40;
										int bereich = maxTemperatur - minTemperatur;
										double temperatur = (double)bereich - (double)((double)data4BS.data1 * (double)bereich)/255.0;
										printf("Temperatur: %f SensorID:%.2X%.2X%.2X%.2X \n", temperatur,
												data4BS.senderID[0], data4BS.senderID[1],data4BS.senderID[2],data4BS.senderID[3]);
										bs4Data dataelement;
										((EnOcean *)This)->addValueToList(temperatur, data4BS.senderID);

									}
									else
									{
										printf("lerntaste gedruckt. \n");
									}
								}
								else
								{
									//Daten verwerfen bis zum nächsten Sync oder bis keine Daten mehr da
									printf("EnOcean Data CRC wrong \n");

								}
								//Aufräumen bzw Bytes löschen die wir benutzt haben.
								memcpy((void*)proto_buffer, &proto_buffer[DataLenght], sizeof(unsigned char) * position);
								position= position - DataLenght;
								status = STATUS_VOLLSTAENDIG;
							}
							else
							{
								// Erstmal nichts machen und auf weitere Daten warten.
								status = STATUS_NICHT_VOLLSTAENDIG;
							}
						}
						else
						{
							//Daten verwerfen bis zum nächsten Sync oder bis keine Daten mehr da
							printf("EnOcean Header CRC wrong \n");
							memcpy((void*)proto_buffer, &proto_buffer[DataLenght], sizeof(unsigned char) * position);
							position= position - DataLenght;
							status = STATUS_VOLLSTAENDIG;
						}
					}
					else
					{
						status = STATUS_NICHT_VOLLSTAENDIG;
					}

				} while( ((EnOcean *)This)->numberSync(proto_buffer, position) > 0 && status == STATUS_VOLLSTAENDIG );



			}
			sleep(1);
		}
	}
	close(((EnOcean *)This)->uart0_filestream);
}


