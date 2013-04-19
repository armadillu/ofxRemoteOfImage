//
//  ofxRemoteOfImage.h
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 18/04/13.
//
//

#ifndef __emptyExample__ofxRemoteOfImage__
#define __emptyExample__ofxRemoteOfImage__

#include <iostream>
#include "ofMain.h"

#include "ofxNetwork.h"

#define OFX_REMOTE_OF_IMAGE_DEFAULT_PORT	11999
#define MAX_MSG_PAYLOAD						4096
#define BLOCKING							true
#define SIZE_L								5

enum ofxRemoteOfImageMode{ REMOTE_OF_IMAGE_SERVER, REMOTE_OF_IMAGE_CLIENT };

class ofxRemoteOfImage: public ofThread{

public:

	ofxRemoteOfImage();

	//server
	void startServer(ofImage * image, int port = OFX_REMOTE_OF_IMAGE_DEFAULT_PORT);
	void imageChanged(); //flag image as updated, to send thorugh network

	//client
	void connect(ofImage * image, string host, int port = OFX_REMOTE_OF_IMAGE_DEFAULT_PORT);
	void updatePixels();

	//for both
	void setFrameRate(float rate);
	void begin(); // sync
	void end();

	void stop();


private:

	void updateThread();
	int bytesPerPixel(ofImageType type);
	void threadedFunction();
	void update();

	ofxRemoteOfImageMode mode;
	ofxUDPManager udpConnection;
	ofImage * img;

	bool debug;
	float frameRate;

	char* data; // client

};

#endif
