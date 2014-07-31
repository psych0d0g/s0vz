///*
// *  EnOceanreader.h
// *
// *
// *  Created by Paola Andrea Jaramillo Garcia on 8/11/11.
// *  Copyright 2011 Technische Universiteit Eindhoven. All rights reserved.
// *
// */
//
//
//
//#ifndef EnOceanreader_H
//#define EnOceanreader_H
//
//#include "StreamTask.h"
//#include "SerialDevice.h"
//#include "Vector.h"
//#include <sys/time.h>
//
////EnOcean constants
//#ifndef EnOceanConstants
//#define EnOceanConstants
//#define SER_INTERBYTE_TIME_OUT_MS	100
//#define SER_SYNCH_CODE				0x55
//#define SER_HEADER_NR_BYTES			4
//#define SER_MAX_PAYLOAD				32
//
//#define TEL_ENC_1BS					0xD5
//#define TEL_ENC_4BS					0xA5
//#define TEL_ENC_RBS					0xF6
//
//#define UNKOWN_DEVICE				-1
//
////EnOcean state machine
//#define GET_SYNC_STATE				0
//#define GET_HEADER_STATE			1
//#define CHECK_CRC8H_STATE			2
//#define GET_DATA_STATE				3
//#define CHECK_CRC8D_STATE			6
//#define OUT_OF_RANGE				4
//#define NOT_VALID_CHKSUM			3
//
//#define OK							0
//#define NO_RX_TEL 					1
//#define NEW_RX_BYTE					2
//#endif
//class EnOceanreader : public StreamTask{
//	GIBN_MAKE_AVAILABLE(EnOceanreader, StreamTask)
//
//	//EnOcean serial packet container
//	typedef union
//	{
//		struct
//		{
//			// Amount of raw data bytes to be received. The most significant byte is sent/received first
//			unsigned short DataLength;
//			// Amount of optional data bytes to be received
//			unsigned char OptionLength;
//			// Packe type code
//			unsigned char Type;
//			// Data buffer: raw data + optional bytes
//			unsigned char DataBuffer[SER_MAX_PAYLOAD];
//		};
//		unsigned char Raw[SER_MAX_PAYLOAD+4];
//	} PACKET_SERIAL_TYPE;
//
//	typedef struct {
//		std::string Name;
//		DataPacket * (EnOceanreader::*p) (const PACKET_SERIAL_TYPE *Packet, const unsigned char buf_size);
//	} FUNCTION_TABLE;
//
//public:
//	EnOceanreader(void);
//	virtual ~EnOceanreader(void);
//
//private:
//	// Arguments
//	GIBN_REQUIRED std::string devname;							// Name of serial port for reading
//	GIBN_REQUIRED_VEC Vector< Vector<unsigned int> > devlist;	// Array with the IDs of the sensors, E.g., [[1,2],[3]]
//	GIBN_REQUIRED_VEC Vector< std::string > devtype;			// Array with the type of the sensors. E.g., ["FTK", "FIH63"]
//	GIBN_OPTIONAL_VEC Vector<unsigned int> destinationid;		// Array with destination IDs for filtering of telegrams. Default is no filtering
//
//	// Global variables
//	SerialDevice *serial;
//	const static unsigned char u8CRC8Table[256];
//	const static FUNCTION_TABLE decoderTableFuncPtr[];
//	Vector<unsigned char> decoder_map;
//
//	// Decoder functions
//	DataPacket * decoder_FTK(const PACKET_SERIAL_TYPE *Packet, const unsigned char buf_size);
//	DataPacket * decoder_FT4F(const PACKET_SERIAL_TYPE *Packet, const unsigned char buf_size);
//	DataPacket * decoder_FIH63(const PACKET_SERIAL_TYPE *Packet, const unsigned char buf_size);
//	DataPacket * decoder_FBH63(const PACKET_SERIAL_TYPE *Packet, const unsigned char buf_size);
//	DataPacket * decoder_SR_MDS(const PACKET_SERIAL_TYPE *Packet, const unsigned char buf_size);
//	DataPacket * decoder_RAW(const PACKET_SERIAL_TYPE *Packet, const unsigned char buf_size);
//	DataPacket * decoder_Teach_in(const PACKET_SERIAL_TYPE *Packet, const unsigned char buf_size);
//	DataPacket * decoder_FSR61VA(const PACKET_SERIAL_TYPE *Packet, const unsigned char buf_size);
//
//	// EnOcean protocol methods
//	bool init_decoder_map(void);
//	void init_port(void);
//	bool sample_sensor(void);
//	DataPacket * decoder(const PACKET_SERIAL_TYPE *buf, const unsigned char buf_size);
//	int stream_data(const PACKET_SERIAL_TYPE *buf, const unsigned char buf_size, const int port);
//	int getOutputForDev(const unsigned int DeviceID);
//	bool idOfInterest(const unsigned int DeviceID);
//	unsigned char uart_getPacket(const unsigned char RxByte, PACKET_SERIAL_TYPE *pPacket, const unsigned int BufferLength);
//	void paramsChanged(void);
//
//	// Standard task methods
//	void cancelAllBlockingCalls();
//	void run(void);
//
//	// Helper functions
//	inline unsigned char crc8(const unsigned char CRC, const unsigned char Data);
//	inline void getstate(int code,int &lr, int &rr);
//	inline long time(void);
//	inline void swap(unsigned char *a, unsigned char *b);
//};
//
//#endif //EnOceanreader_H
