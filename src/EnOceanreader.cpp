///*
// *  EnOceanreader.cpp
// *
// *
// *  Created by Marc & Paul Troost on 10/04/12.
// *  Copyright 2012 Technische Universiteit Eindhoven. All rights reserved.
// *
// *  Change log
// *  ----------------------------------------------------------------------
// *  19/06/12 Added SR-MDS decoder
// */
//
//#include "EnOceanreader.h"
//#include "IntValue.h"
//#include "FloatValue.h"
//#include "DataPacket.h"
//#include "Vector.h"
//
//#include <cstdio>
//
//using namespace std;
//
//
//// Debug switch, runs in debug mode when flag = 1
//#define DEBUG 0
//
//// Init constants
//const EnOceanreader::FUNCTION_TABLE EnOceanreader::decoderTableFuncPtr[] = {
//	{"FTK", &EnOceanreader::decoder_FTK},
//	{"FIH63", &EnOceanreader::decoder_FIH63},
//	{"FBH63", &EnOceanreader::decoder_FBH63},
//	{"SR-MDS", &EnOceanreader::decoder_SR_MDS},
//	{"FSR61VA", &EnOceanreader::decoder_FSR61VA},
//	{"FT4F", &EnOceanreader::decoder_FT4F},
//	{"RAW", &EnOceanreader::decoder_RAW},
//	{"Teach-in", &EnOceanreader::decoder_Teach_in},
//	{"", NULL}
//};
//
//const unsigned char EnOceanreader::u8CRC8Table[256] = {
//	0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15,
//	0x38, 0x3f, 0x36, 0x31, 0x24, 0x23, 0x2a, 0x2d,
//	0x70, 0x77, 0x7e, 0x79, 0x6c, 0x6b, 0x62, 0x65,
//	0x48, 0x4f, 0x46, 0x41, 0x54, 0x53, 0x5a, 0x5d,
//	0xe0, 0xe7, 0xee, 0xe9, 0xfc, 0xfb, 0xf2, 0xf5,
//	0xd8, 0xdf, 0xd6, 0xd1, 0xc4, 0xc3, 0xca, 0xcd,
//	0x90, 0x97, 0x9e, 0x99, 0x8c, 0x8b, 0x82, 0x85,
//	0xa8, 0xaf, 0xa6, 0xa1, 0xb4, 0xb3, 0xba, 0xbd,
//	0xc7, 0xc0, 0xc9, 0xce, 0xdb, 0xdc, 0xd5, 0xd2,
//	0xff, 0xf8, 0xf1, 0xf6, 0xe3, 0xe4, 0xed, 0xea,
//	0xb7, 0xb0, 0xb9, 0xbe, 0xab, 0xac, 0xa5, 0xa2,
//	0x8f, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9d, 0x9a,
//	0x27, 0x20, 0x29, 0x2e, 0x3b, 0x3c, 0x35, 0x32,
//	0x1f, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0d, 0x0a,
//	0x57, 0x50, 0x59, 0x5e, 0x4b, 0x4c, 0x45, 0x42,
//	0x6f, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7d, 0x7a,
//	0x89, 0x8e, 0x87, 0x80, 0x95, 0x92, 0x9b, 0x9c,
//	0xb1, 0xb6, 0xbf, 0xb8, 0xad, 0xaa, 0xa3, 0xa4,
//	0xf9, 0xfe, 0xf7, 0xf0, 0xe5, 0xe2, 0xeb, 0xec,
//	0xc1, 0xc6, 0xcf, 0xc8, 0xdd, 0xda, 0xd3, 0xd4,
//	0x69, 0x6e, 0x67, 0x60, 0x75, 0x72, 0x7b, 0x7c,
//	0x51, 0x56, 0x5f, 0x58, 0x4d, 0x4a, 0x43, 0x44,
//	0x19, 0x1e, 0x17, 0x10, 0x05, 0x02, 0x0b, 0x0c,
//	0x21, 0x26, 0x2f, 0x28, 0x3d, 0x3a, 0x33, 0x34,
//	0x4e, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5c, 0x5b,
//	0x76, 0x71, 0x78, 0x7f, 0x6A, 0x6d, 0x64, 0x63,
//	0x3e, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2c, 0x2b,
//	0x06, 0x01, 0x08, 0x0f, 0x1a, 0x1d, 0x14, 0x13,
//	0xae, 0xa9, 0xa0, 0xa7, 0xb2, 0xb5, 0xbc, 0xbb,
//	0x96, 0x91, 0x98, 0x9f, 0x8a, 0x8D, 0x84, 0x83,
//	0xde, 0xd9, 0xd0, 0xd7, 0xc2, 0xc5, 0xcc, 0xcb,
//	0xe6, 0xe1, 0xe8, 0xef, 0xfa, 0xfd, 0xf4, 0xf3
//};
//
//
//// Decoder for FTK sensor (magnetic switch)
//DataPacket * EnOceanreader::decoder_FTK(const PACKET_SERIAL_TYPE *Packet, const unsigned char buf_size)
//{
//	DataPacket *values = NULL;
//
//	switch(Packet->DataBuffer[1]){
//		case 0x08: // contact open
//			values = new DataPacket;
//			values->dataVector.push_back(new IntValue(1));
//			break;
//
//		case 0x09: // contact closed
//			values = new DataPacket;
//			values->dataVector.push_back(new IntValue(0));
//			break;
//
//		case 0x00:
//			log( ".decoder_FTK: received teach-in telegram" );
//			break;
//
//		default:
//			log( ".decoder_FTK: payload interpretation failed" );
//			break;
//	}
//
//#if DEBUG
//	log( ".decoder_FTK" );
//	if (values != NULL)
//		values->toString(cout);
//#endif
//
//	return values;
//}
//
//
//// Decoder for FAH60+FAH63+FIH63 sensor (lux sensor)
//DataPacket * EnOceanreader::decoder_FIH63(const PACKET_SERIAL_TYPE *Packet, const unsigned char buf_size)
//{
//	DataPacket *values = NULL;
//
//	if (Packet->DataBuffer[4] == 0x87) { // check if teach-in telegram
//		log( ".decoder_FIH63: received teach-in telegram" );
//	} else {
//		values = new DataPacket;
//		if (Packet->DataBuffer[2] == 0x00) // select correct range
//			values->dataVector.push_back(new IntValue((100*Packet->DataBuffer[1])/255)); // 0 ... 100 lux
//		else
//			values->dataVector.push_back(new IntValue(((30000-300)*Packet->DataBuffer[2])/255+300)); // 300 ... 30.000 lux
//	}
//
//#if DEBUG
//	log( ".decoder_FIH63" );
//	if (values != NULL)
//		values->toString(cout);
//#endif
//
//	return values;
//}
//
///** \brief Decoder for FT4F (4 switches).
//  * It decodes the value to right rocker left rocker. Keep in mind that the
//  * sides are set when the 0 side of the rocker is pointing up.
//  * converts to two channel with the following values:
//  * 0: off
//  * 1: up
//  * 2: down
//  */
//DataPacket * EnOceanreader::decoder_FT4F(const PACKET_SERIAL_TYPE *Packet, const unsigned char buf_size)
//{
//	DataPacket *values = NULL;
//
//  short int r1 = Packet->DataBuffer[1] >> 4;
//  short int r2 = Packet->DataBuffer[1] & 0x0F;
//  int lr_state = 0;
//  int rr_state = 0;
//  getstate(r1,lr_state,rr_state);
//  getstate(r2,lr_state,rr_state);
//	values = new DataPacket;
//	values->dataVector.push_back(new IntValue(lr_state));
//	values->dataVector.push_back(new IntValue(rr_state));
//	return values;
//}
///** \brief Decoder for FSR61VA power meassurement.
//  * It decodes the the power telegram from the sensor.
//  */
//DataPacket * EnOceanreader::decoder_FSR61VA(const PACKET_SERIAL_TYPE *Packet, const unsigned char buf_size){
//  DataPacket *values = NULL;
//
//  if ( Packet->DataBuffer[4] & 0x08 == 0x08 ) { // check if teach-in telegram
//		log( ".decoder_FSR61VA: received teach-in telegram" );
//		return NULL;
//	}
//
//  values = new DataPacket;
//  long int r1 = (( Packet->DataBuffer[1] << 16 ) & 0x00FF0000) +
//                (( Packet->DataBuffer[2] << 8 )  & 0x0000FF00) +
//                Packet->DataBuffer[3];
//  int type = Packet->DataBuffer[4];
//	values = new DataPacket;
//	values->dataVector.push_back(new IntValue(r1));
//	values->dataVector.push_back(new IntValue(type));
//	return values;
//}
//
//// Decoder for FABH63+FBH55+FBH63+FIBH63 sensor (PIR sensor)
//DataPacket * EnOceanreader::decoder_FBH63(const PACKET_SERIAL_TYPE *Packet, const unsigned char buf_size)
//{
//	DataPacket *values = NULL;
//
//	if (Packet->DataBuffer[4] == 0x85) { // check if teach-in telegram
//		log( ".decoder_FBH63: received teach-in telegram" );
//	} else {
//		values = new DataPacket;
//		values->dataVector.push_back(new FloatValue((2048*Packet->DataBuffer[2])/255)); // 0 ... 2.048 lux
//
//		switch(Packet->DataBuffer[4]) {
//			case 0x0D:
//				values->dataVector.push_back(new IntValue(1));
//				break;
//
//			case 0x0F:
//				values->dataVector.push_back(new IntValue(0));
//				break;
//
//			default:
//				log( ".decoder_FBH63: Occupancy payload interpretation failed" );
//				break;
//		}
//	}
//
//#if DEBUG
//	log( ".decoder_FBH63" );
//	if (values != NULL)
//		values->toString(cout);
//#endif
//
//	return values;
//}
//
//// Decoder for Thermokon 361651 SR-MDS Solar 868MHz
//DataPacket * EnOceanreader::decoder_SR_MDS(const PACKET_SERIAL_TYPE *Packet, const unsigned char buf_size)
//{
//	DataPacket *values = NULL;
//
//	if (!(Packet->DataBuffer[4] | 0x80)) { // check if teach-in telegram
//		log( ".decoder_SR-MDS: received teach-in telegram" );
//	} else {
//		values = new DataPacket;
//		values->dataVector.push_back(new FloatValue((5.12*Packet->DataBuffer[2])/255.0)); // 0 ... 5.12 V
//		values->dataVector.push_back(new FloatValue((512*Packet->DataBuffer[2])/255)); // 0 ... 512 lux
//
//		switch(Packet->DataBuffer[4]) {
//			case 13:
//				values->dataVector.push_back(new IntValue(1));
//				break;
//
//			case 15:
//				values->dataVector.push_back(new IntValue(0));
//				break;
//
//			default:
//				log( ".decoder_FBH63: Occupancy payload interpretation failed" );
//				break;
//		}
//	}
//
//#if DEBUG
//	log( ".decoder_SR-MDS" );
//	if (values != NULL)
//		values->toString(cout);
//#endif
//
//	return values;
//}
//
//// Decoder for raw output of data bytes
//DataPacket * EnOceanreader::decoder_RAW(const PACKET_SERIAL_TYPE *Packet, const unsigned char buf_size)
//{
//	DataPacket *values = NULL;
//
//	switch(Packet->DataBuffer[0])
//	{
//		case TEL_ENC_1BS:
//			values = new DataPacket;
//			values->dataVector.push_back(new IntValue(Packet->DataBuffer[1]));
//			break;
//
//		case TEL_ENC_4BS:
//			values = new DataPacket;
//			for(int i=0; i<4; i++)
//				values->dataVector.push_back(new IntValue(Packet->DataBuffer[1+i]));
//			break;
//
//		default:
//			log( ".decoder_RAW: Unkown TELEGRAM format" );
//			break;
//	}
//
//#if DEBUG
//	log( ".decoder_RAW" );
//	if (values != NULL)
//		values->toString(cout);
//#endif
//
//	return values;
//}
//
//
//// Decoder for analyzing Teach-in telegram
//DataPacket * EnOceanreader::decoder_Teach_in(const PACKET_SERIAL_TYPE *Packet, const unsigned char buf_size)
//{
//	if (Packet->DataLength > 7) {
//		int manufacturer = Packet->DataBuffer[3];
//		int type = Packet->DataBuffer[2] >> 3;
//		int function = Packet->DataBuffer[1] >> 2;
//		int encoding = Packet->DataBuffer[0];
//
//		printf(".decoder_Teach_in: Manufacturer ID: 0x%.2X\n", manufacturer);
//		printf(".decoder_Teach_in: Sensor function: 0x%.2X\n", function);
//		printf(".decoder_Teach_in: Sensor type: 0x%.2X\n", type);
//		printf(".decoder_Teach_in: Telegram encoding: 0x%.2X\n", encoding);
//		printf(".decoder_Teach_in: Possible EEP: %.2d-%.2d-%.2d\n", 7, function, type); // Enocean Equipment Profile
//	} else {
//		log(".decoder_Teach_in: Not a valid Teach-in telegram");
//	}
//
//	return NULL;
//}
//
//
//// Create the reader element
//EnOceanreader::EnOceanreader(void) : StreamTask(0,1){
//#if DEBUG
//	log("constructor.");
//#endif
//}
//
//EnOceanreader::~EnOceanreader(void){
//	delete serial;
//}
//
//
////Create serial port
//void EnOceanreader::init_port(void)
//{
//	try {
//		serial = new SerialDevice( devname.c_str(), B57600);
//	}
//	catch( IOError& e ){
//		log( "Cannot create serial device" ) << " (ret=" << e.retVal << ")" << endl;
//		throw e;
//	}
//}
//
//
////Sample sensor
//bool EnOceanreader::sample_sensor(void)
//{
//	unsigned int *ID, *destination, out;
//	PACKET_SERIAL_TYPE Packet;
//
//	// Get one complete EnOcean telegram
//	while (uart_getPacket(serial->getChar(), &Packet, SER_MAX_PAYLOAD) != OK);
//
//#if DEBUG
//	printf(".sample_sensor ENOCEAN_TELEGRAM @ %s: ", devname.c_str());
//	for (unsigned int i=4; i<4+Packet.DataLength+Packet.OptionLength; i++) printf("%.2X ", Packet.Raw[i] & 0xFF);
//	printf("\n");
//#endif
//
//	if (Packet.DataBuffer[Packet.DataLength + Packet.OptionLength - 1]) {
//		log( ".sample_sensor: skipping encrypted telegram" );
//		return false;
//	}
//
//	switch(Packet.DataBuffer[0])
//	{
//		case TEL_ENC_1BS:
//		case TEL_ENC_4BS:
//		case TEL_ENC_RBS:
//			// Fix endianess of device ID and destination
//			swap(&Packet.DataBuffer[Packet.DataLength - 5], &Packet.DataBuffer[Packet.DataLength - 2]);
//			swap(&Packet.DataBuffer[Packet.DataLength - 4], &Packet.DataBuffer[Packet.DataLength - 3]);
//			swap(&Packet.DataBuffer[Packet.DataLength + Packet.OptionLength - 6], &Packet.DataBuffer[Packet.DataLength + Packet.OptionLength - 3]);
//			swap(&Packet.DataBuffer[Packet.DataLength + Packet.OptionLength - 5], &Packet.DataBuffer[Packet.DataLength + Packet.OptionLength - 4]);
//
//
//			ID = (unsigned int *)&(Packet.DataBuffer[Packet.DataLength - 5]);
//			destination = (unsigned int *)&(Packet.DataBuffer[Packet.DataLength + Packet.OptionLength - 6]);
//			break;
//
//		default:
//			log( ".sample_sensor: Unkown TELEGRAM format" );
//			return false;
//			break;
//	}
//
//	if (((out = getOutputForDev(*ID)) != UNKOWN_DEVICE) && idOfInterest(*destination))
//		stream_data(&Packet, 3+Packet.DataLength+Packet.OptionLength, out);
//#if DEBUG
//	else
//		printf(".sample_sensor: device %.8X transmitting to %.8X is not of interest\n", *ID, *destination);
//#endif
//
//	return true;
//}
//
//
//// check if DeviceID is present in devlist
//int EnOceanreader::getOutputForDev(const unsigned int DeviceID)
//{
//
//#if DEBUG
//	log(".getOutputForDev: Check if telegram is from known device");
//#endif
//
//	unsigned int offset = 0, j, i;
//
//	for (i=0; i<devlist.size(); i++){
//		for (j=0; j<devlist.at(i).size(); j++){
//			if (devlist.at(i).at(j) == DeviceID){
//#if DEBUG
//				printf(".getOutputForDev: Telegram is from known device. Using output port %d.\n", j+offset);
//#endif
//				return j+offset;
//			}
//		}
//		offset += j;
//	}
//
//#if DEBUG
//	log(".getOutputForDev: Telegram is from unknown device");
//#endif
//
//	return UNKOWN_DEVICE;
//}
//
//
//// Initializes map of devicetype to decoder function
//bool EnOceanreader::init_decoder_map(void)
//{
//	if(devtype.size() != devlist.size()){
//#if DEBUG
//		log() << ".init_decoder_map: invalid number of decoders specified. "
//		      << devlist.size() << "sensor types specified, "
//		      << devtype.size() << "decoders specified.";
//#endif
//		log( ".init_decoder_map: invalid number of decoders specified" );
//		return false;
//	}
//
//	for(unsigned int i=0; i<devtype.size(); i++){
//		unsigned int j=0;
//		while(devtype.at(i).compare(decoderTableFuncPtr[j].Name) && decoderTableFuncPtr[j].Name.compare(""))
//			j++;
//
//		if(!decoderTableFuncPtr[j].Name.compare("")){
//#if DEBUG
//			printf(".init_decoder_map: no decoder for sensor type \"%s\"\n", devtype.at(i).c_str());
//#endif
//			log( ".init_decoder_map: unknown sensor type specified in json file" );
//			return false;
//		}else{
//#if DEBUG
//			printf(".init_decoder_map: Found decoder \"%s\" => \"%s\", function id: %d\n", devtype.at(i).c_str(),  decoderTableFuncPtr[j].Name.c_str(), j);
//#endif
//			decoder_map.push_back(j);
//		}
//	}
//
//	log( ".init_decoder_map: finished mapping sensor type(s) to decoder(s)" );
//
//	return true;
//}
//
//
//// Decodes packet using device to decoder map
//DataPacket * EnOceanreader::decoder(const PACKET_SERIAL_TYPE *Packet, const unsigned char buf_size)
//{
//	unsigned int *ID;
//	unsigned char sensor_type;
//
//	ID = (unsigned int *)&(Packet->DataBuffer[Packet->DataLength - 5]);
//
//#if DEBUG
//	printf(".decode: decoding for id %.8X\n", *ID);
//#endif
//
//	for (unsigned int i=0; i<devlist.size(); i++){
//		for (unsigned int j=0; j<devlist.at(i).size(); j++){
//			if (devlist.at(i).at(j) == *ID)
//				sensor_type = i;
//		}
//	}
//
//#if DEBUG
//	printf(".decode: Using decoder %s\n", decoderTableFuncPtr[decoder_map[sensor_type]].Name.c_str());
//#endif
//
//	return (this->*decoderTableFuncPtr[decoder_map[sensor_type]].p)(Packet, buf_size);
//}
//
//
//// Stream data to logger task in crnt module
//int EnOceanreader::stream_data(const PACKET_SERIAL_TYPE *Packet, const unsigned char buf_size, const int port)
//{
//	//create data packet
//	DataPacket *values = NULL;
//
//	//decoder data
//	values = decoder(Packet, buf_size);
//
//	if (values != NULL) {
//		//send data packet
//		outPorts[port]->send(values);
//#if DEBUG
//		log( ".stream_data: stream data sucessful.");
//#endif
//	}
//}
//
//void EnOceanreader::cancelAllBlockingCalls(){
//  //FixMe: we need to be able to cancel writes to SerialDevice
//}
//
////Main routine
//void EnOceanreader::run(void)
//{
//	if(!init_decoder_map())
//		return;
//
//	while (running) {
//
//		//communicate with serial device
//		while (running) {
//			try{
//				if (serial) {
//					sample_sensor();
//#if DEBUG
//					log( ".run: sampling sensor passed.");
//#endif
//				}
//			}
//			catch ( IOError e ) {
//				log( ".run: Error while communication with serial device." );
//				serial -> close();
//				return;
//			}
//		}
//
//	}
//}
//
//
//// (Re)initialize number of output ports
//void EnOceanreader::paramsChanged(void)
//{
//	unsigned int num_devs = 0;
//#if DEBUG
//	log(".paramsChanged.");
//#endif
//
//	init_port();
//	serial->open(true);
//
//	for (unsigned int i=0; i<devlist.size(); i++){
//		num_devs += devlist.at(i).size();
//	}
//
//	if(num_devs && (num_devs != outPorts.size() )) {
//		setOutPortNum( num_devs );
//#if DEBUG
//		printf(".paramsChanged: Set number of output ports to %d\n", num_devs);
//#endif
//	}
//}
//
//
//// Check if destination address is of interest
//bool EnOceanreader::idOfInterest(const unsigned int DeviceID)
//{
//	for (unsigned int i=0; i<destinationid.size(); i++){
//		if (destinationid.at(i) == DeviceID)
//			return true;
//	}
//
//	return (false || (destinationid.size() == 0));
//}
//
//
//// State machine to capture a complete EnOcean telegram
//unsigned char EnOceanreader::uart_getPacket(const unsigned char RxByte, PACKET_SERIAL_TYPE *pPacket, const unsigned int BufferLength)
//{
//	// Checksum calculation
//	static unsigned char CRC = 0;
//	// Nr. of bytes received
//	static unsigned int Count = 0;
//	// State machine counter
//	static unsigned char State = GET_SYNC_STATE;
//	// Timeout measurement
//	static long TickCount = 0;
//	unsigned int i;
//
//	// Check for timeout between two bytes
//	if ((State != GET_SYNC_STATE) && ((time() - TickCount) > SER_INTERBYTE_TIME_OUT_MS))
//	{
//		// Reset state machine to init state
//		State = GET_SYNC_STATE;
//
//#if DEBUG
//		log( ".uart_getPacket: inter byte time out triggered.");
//#endif
//	}
//
//	// Tick count of last received byte
//	TickCount = time();
//
//	// State machine to load incoming packet bytes
//	switch(State)
//	{
//		// Waiting for packet sync byte 0x55
//		case GET_SYNC_STATE:
//
//			if (RxByte == SER_SYNCH_CODE)
//			{
//				State = GET_HEADER_STATE;
//				Count = 0;
//				CRC   = 0;
//
//#if DEBUG
//				log( ".uart_getPacket: SER_SYNC_CODE found.");
//#endif
//			}
//
//			break;
//
//
//		// Read the header bytes
//		case GET_HEADER_STATE:
//
//			// Copy received data to buffer
//			pPacket->Raw[Count++] = RxByte;
//			CRC = crc8(CRC, RxByte);
//
//			// All header bytes received?
//			if(Count == SER_HEADER_NR_BYTES)
//			{
//				State = CHECK_CRC8H_STATE;
//			}
//
//			break;
//
//
//		// Check header checksum & try to resynchonise if error happened
//		case CHECK_CRC8H_STATE:
//			// Header CRC correct?
//			if (CRC != RxByte)
//			{
//				log( ".uart_getPacket: invalid CRC on packet header");
//#if DEBUG
//				printf(".uart_getPacket: received CRC <%.2x>, Calculated CRC <%.2x>\n",RxByte, CRC);
//#endif
//
//				// No. Check if there is a sync byte (0x55) in the header
//				int a = -1;
//				for (i = 0 ; i < SER_HEADER_NR_BYTES ; i++)
//					if (pPacket->Raw[i] == SER_SYNCH_CODE)
//					{
//						// indicates the next position to the sync byte found
//						a=i+1;
//						break;
//					};
//
//				if ((a == -1) && (RxByte != SER_SYNCH_CODE))
//				{
//					// Header and CRC8H does not contain the sync code
//					State = GET_SYNC_STATE;
//					break;
//				}
//				else if((a == -1) && (RxByte == SER_SYNCH_CODE))
//				{
//					// Header does not have sync code but CRC8H does.
//					// The sync code could be the beginning of a packet
//					State = GET_HEADER_STATE;
//					Count = 0;
//					CRC   = 0;
//					break;
//				}
//
//				// Header has a sync byte. It could be a new telegram.
//				// Shift all bytes from the 0x55 code in the buffer.
//				// Recalculate CRC8 for those bytes
//				CRC = 0;
//				for (i = 0 ; i < (SER_HEADER_NR_BYTES - a) ; i++)
//				{
//					pPacket->Raw[i] = pPacket->Raw[a+i];
//					CRC = crc8(CRC, pPacket->Raw[i]);
//				}
//				Count = SER_HEADER_NR_BYTES - a;
//
//				// Copy the just received byte to buffer
//				pPacket->Raw[Count++] = RxByte;
//				CRC = crc8(CRC, RxByte);
//
//				if(Count < SER_HEADER_NR_BYTES)
//				{
//					State = GET_HEADER_STATE;
//					break;
//                      		}
//
//				break;
//			}
//
// 			// CRC8H correct. Length fields values valid?
//			if((pPacket->DataLength + pPacket->OptionLength) == 0)
//			{
//				//No. Sync byte received?
//				if((RxByte == SER_SYNCH_CODE))
//				{
//					//yes
//					State = GET_HEADER_STATE;
//					Count = 0;
//					CRC   = 0;
//					break;
//				}
//
//				// Packet with correct CRC8H but wrong length fields.
//				State = GET_SYNC_STATE;
//				return OUT_OF_RANGE;
//			}
//
//			// Correct header CRC8. Go to the reception of data.
//			State = GET_DATA_STATE;
//			Count = 0;
//			CRC   = 0;
//
//			// Fix endianess of Datalength field
//			swap(&pPacket->Raw[1], &pPacket->Raw[0]);
//			break;
//
//
//		// Copy the information bytes
//		case GET_DATA_STATE:
//
//			// Copy byte in the packet buffer only if the received bytes have enough room
//			if(Count < BufferLength)
//			{
//				pPacket->DataBuffer[Count] = RxByte;
//				CRC = crc8(CRC, RxByte);
//			}
//			Count++;
//
//			// When all expected bytes received, go to calculate data checksum
//			if(Count == (pPacket->DataLength + pPacket->OptionLength))
//			{
//				State = CHECK_CRC8D_STATE;
//			}
//
//			break;
//
//
//		// Check the data CRC8
//		case CHECK_CRC8D_STATE:
//#if DEBUG
//				printf(".uart_getPacket: received CRC <%.2x>, Calculated CRC <%.2x>\n",RxByte, CRC);
//#endif
//			// In all cases the state returns to the first state: waiting for next sync byte
//			State = GET_SYNC_STATE;
//
//			// Received packet bigger than space to allocate bytes?
//			if (Count > BufferLength)
//			{
//#if DEBUG
//				log( ".uart_getPacket: received payload larger then SER_MAX_PAYLOAD.");
//#endif
//
//				return OUT_OF_RANGE;
//			}
//
//			// Enough space to allocate packet. Equals last byte the calculated CRC8?
//			if (CRC == RxByte)
//			{
//#if DEBUG
//				log( ".uart_getPacket: received valid telegram.");
//#endif
//				return OK;               // Correct packet received
//			}
//
//			// False CRC8.
//			log( ".uart_getPacket: invalid CRC on packet data.");
//#if DEBUG
//			printf(".uart_getPacket: received CRC <%.2x>, Calculated CRC <%.2x>\n",RxByte, CRC);
//#endif
//
//			// If the received byte equals sync code, then it could be sync byte for next paquet.
//			if((RxByte == SER_SYNCH_CODE))
//			{
//				State = GET_HEADER_STATE;
//				Count = 0;
//				CRC   = 0;
//			}
//
//			return NOT_VALID_CHKSUM;
//
//		default:
//
//			// Yes. Go to the reception of info.
//			State = GET_SYNC_STATE;
//			break;
//	}
//
//	return (State == GET_SYNC_STATE) ? NO_RX_TEL : NEW_RX_BYTE;
//}
//
//// Get the value of a system timer with a millisecond precision.
//inline long EnOceanreader::time(void) {
//	struct timeval st_time;
//	gettimeofday(&st_time,NULL);
//	return (long)(st_time.tv_usec/1000 + st_time.tv_sec*1000);
//}
//
////Covert data byte from RTS datagram to state values. (EEP: F6-02-01)
//inline void EnOceanreader::getstate(int code,int &lr, int &rr){
//  switch(code){
//    case 0x07:
//      lr += 0;
//      rr += 1;
//      break;
//    case 0x05:
//      lr += 0;
//      rr += 2;
//      break;
//    case 0x03:
//      lr += 1;
//      rr += 0;
//      break;
//    case 0x01:
//      lr += 2;
//      rr += 0;
//      break;
//    default:
//      lr += 0;
//      rr += 0;
//      break;
//  }
//}
//
//// Calculate the CRC8 using the polynomial x^8 + x^2 + x^1 + x^0
//inline unsigned char EnOceanreader::crc8(const unsigned char CRC, const unsigned char Data)
//{
//	return u8CRC8Table[CRC ^ Data];
//}
//
//
//// Swaps the bytes a and b
//inline void EnOceanreader::swap(unsigned char *a, unsigned char *b)
//{
//	unsigned char temp;
//
//	temp = *a;
//	*a = *b;
//	*b = temp;
//}
