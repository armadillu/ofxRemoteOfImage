//
//  ofxRemoteOfImage.h
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 18/04/13.
//
//

#ifndef __ofxRemoteOfImage__
#define __ofxRemoteOfImage__

#include <iostream>
#include "ofMain.h"

#include "ofxNetwork.h"

#define OFX_REMOTE_OF_IMAGE_DEFAULT_PORT	45744
#define MAX_MSG_PAYLOAD_UDP					512
#define MAX_MSG_PAYLOAD_TCP					500
#define BLOCKING							true
#define SIZE_L								5
#define STARTUP_MSG							"_::_"
#define ACK_MSG								"OK"
#define	ACK_MID_MSG							"!+!"
#define	LOOP_MSG							"!L!"
#define RECONNECT_INTERVAL					5000
#define STRING_DELIMITER					"_"
#define ZERO_PAD_LEN						6
#define FORMAT_STR_LEN						(ZERO_PAD_LEN * 3 + 4)
#define TIME_OUT_MILIS						1000

enum ofxRemoteOfImageMode{ REMOTE_OF_IMAGE_SERVER, REMOTE_OF_IMAGE_CLIENT };
enum ofxRemoteOfImageNetworkProtocol{ REMOTE_OF_IMAGE_UDP, REMOTE_OF_IMAGE_TCP };

class ofxRemoteOfImage: public ofThread{

public:

	ofxRemoteOfImage();


	//TCP OR UDP.
	//UDP allows for one client per server only.
	//apply same protocol to both server and client
	void setNetworkProtocol(ofxRemoteOfImageNetworkProtocol p);

	//how often the image will be sent through the net
	void setDesiredFramerate(float fr); //apply same value to both client and server

	// server /////////
	void startServer(ofImage * image, int port = OFX_REMOTE_OF_IMAGE_DEFAULT_PORT);

	// client ////////
	void connect(ofImage * image, string host, int port = OFX_REMOTE_OF_IMAGE_DEFAULT_PORT);
	void updatePixels();

	//these 2 are meant to used together, to avoid tearing.
	//You must use them both, in client AND server
	void begin();	// call before editing img pixels in server, and before drawing in client
	void end();		// call once you are done editing pixels in server, and after drawing in client

	//call on exit, on both client and server.
	void stop();

private:

	void updateThread();
	int bytesPerPixel(ofImageType type);
	void threadedFunction();
	void update();
	int send(ofxTCPManager & tcp, const unsigned char * bytes, unsigned long numBytes);
	int receive(ofxTCPManager & tcp, const unsigned char * buffer, unsigned long numBytes);

	string zeroPadNumber(int num);

	void updateTCP();
	void updateUDP();

	ofxRemoteOfImageMode				mode; //server / client
	ofxRemoteOfImageNetworkProtocol		protocol;	// TCP / UDP

	ofxUDPManager						udpConnection;
	ofxTCPServer						tcpServer;
	ofxTCPClient						tcpClient;

	ofImage								*img;

	bool								debug;
	float								frameRate; //in fps
	int									port;
	string								host;

	// TCP only //////
	bool								connected;
	float								counter;
	int									connectTime;
	int									deltaTime;

};

#endif
