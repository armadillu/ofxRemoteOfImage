//
//  ofxRemoteOfImage.cpp
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 18/04/13.
//
//

#include "ofxRemoteOfImage.h"

ofxRemoteOfImage::ofxRemoteOfImage(){
	int buffer = 1000;
	udpConnection.SetReceiveBufferSize(MAX_MSG_PAYLOAD * buffer);
	udpConnection.SetSendBufferSize(MAX_MSG_PAYLOAD * buffer);
	udpConnection.SetTimeoutReceive(1);
	udpConnection.SetTimeoutSend(1);
	debug = false;
	img = NULL;
	frameRate = 60;
	data = NULL;
}


void ofxRemoteOfImage::startServer(ofImage * image, int port){

	mode = REMOTE_OF_IMAGE_SERVER;
	img = image;
	udpConnection.Create();
	printf("serving ofImage at %d\n", port);
	udpConnection.Bind(port);
	udpConnection.SetNonBlocking(!BLOCKING);
	startThread();
}


void ofxRemoteOfImage::connect(ofImage * image, string host, int port){

	mode = REMOTE_OF_IMAGE_CLIENT;
	img = image;
	udpConnection.Create();
	printf("connecting to %s : %d\n", host.c_str(), port);
	udpConnection.Connect(host.c_str(), port);
	udpConnection.SetNonBlocking(!BLOCKING);
	startThread();
}


void ofxRemoteOfImage::setFrameRate(float r){
	frameRate = r;
}

void ofxRemoteOfImage::stop(){
	stopThread();
	udpConnection.Close(); //rude! close while receiving
	waitForThread();
}


int ofxRemoteOfImage::bytesPerPixel(ofImageType type){

	switch (type) {
		case OF_IMAGE_GRAYSCALE: return 1;
		case OF_IMAGE_COLOR: return 3;
		case OF_IMAGE_COLOR_ALPHA: return 4;
	}
}


void ofxRemoteOfImage::threadedFunction(){

	while (isThreadRunning()) {

		float tStart = ofGetElapsedTimef();
		update();
		
		//stick to desired fps
		float duration = ofGetElapsedTimef() - tStart;
		float remaining = 1. / frameRate - duration;

		//printf("duration: %f total: %f remaining %f\n", duration, 1/frameRate, remaining );
		if (remaining < 0) remaining = 0;
		if (remaining > 0){
			ofSleepMillis( 1000.0f * remaining);
		}
	}
}


void ofxRemoteOfImage::begin(){
	lock();
}


void ofxRemoteOfImage::end(){
	unlock();
}


void ofxRemoteOfImage::updatePixels(){
	//memcpy(img->getPixels(), (const char*) pix, (int)(img->getWidth() * img->getHeight() * bytesPerPixel(img->getPixelsRef().getImageType())) );
	img->setUseTexture(true);
	img->update();
}


void ofxRemoteOfImage::update(){

	char ww[SIZE_L];
	char hh[SIZE_L];
	char imgMode[SIZE_L];
	int bpp = 1;

	if(mode == REMOTE_OF_IMAGE_CLIENT){

		int sent = udpConnection.Send( "_::_", SIZE_L );

		lock(); //////////////////////////////////////////////////////////////////////

		udpConnection.Receive(ww,SIZE_L);
		udpConnection.Receive(hh,SIZE_L);
		udpConnection.Receive(imgMode,SIZE_L);

		int w = atoi( ww );
		int h = atoi( hh );
		ofImageType imgType = (ofImageType)atoi(imgMode);
		bpp = bytesPerPixel(imgType);
		if(debug) printf("client received %d - %d (%s - %s) (%d)\n",w,h, ww, hh, bpp);

		if (
			img->getWidth() == w &&
			img->getHeight() == h &&
			imgType == img->getPixelsRef().getImageType() ){ //no change

		}else{ //server image changed dimensions!
			img->clear();
			img->setUseTexture(false);
			img->allocate(w, h, imgType);
			img->setUseTexture(true);
			if (data) free(data);
			data = (char*) malloc( sizeof(char) * w * h * bpp);
		}
		int numBytes = w * h * bpp;
		char * pix = (char*)img->getPixels();

		//udpConnection.Receive( (char*)img->getPixels(), numBytes);

		int maxSize = MAX_MSG_PAYLOAD; //udpConnection.GetMaxMsgSize();
		int loopTimes = floor((float)numBytes/maxSize);
		int reminder = numBytes%maxSize;

		if(debug) printf("will receive %d x %d + %d\n", loopTimes, maxSize, reminder);
		for(int i = 0; i < loopTimes; i++){
			udpConnection.Receive( pix  + i * maxSize, maxSize);
			if(debug) printf("send ACK\n");
			udpConnection.Send( "ok", SIZE_L );
		}

		if (reminder > 0){
			if(debug) printf("will Receive reminder of %d\n", reminder);
			udpConnection.Receive( pix  + numBytes - reminder, reminder);
			if(debug) printf("received reminder of %d\n", sent);
		}

		unlock(); //////////////////////////////////////////////////////////////////////

	}else{			//SERVER
		
		int imgW = img->getWidth();
		int imgH = img->getHeight();
		sprintf(ww, "%d", (int) imgW);
		sprintf(hh, "%d", (int) imgH);
		int m = (int)img->getPixelsRef().getImageType();
		sprintf(imgMode, "%d", m);
		int sent = 0;
		bpp = bytesPerPixel(img->getPixelsRef().getImageType());
		if(debug) printf("server about to send %s - %s (%d)\n",ww,hh, bpp);
		char * pix = (char*)img->getPixels();

		char aux[SIZE_L];
		int err = udpConnection.Receive(aux,SIZE_L);
		if (err == SOCKET_ERROR){return;} // most likely we are trying to exit app
		//printf("sync: %s\n", aux);

		lock(); ////////////////////////////////////////////////////////////////////////

		sent = udpConnection.Send( ww, SIZE_L );
		sent = udpConnection.Send( hh, SIZE_L );
		sent = udpConnection.Send( imgMode, SIZE_L );

		int numBytes = img->getWidth() * img->getHeight() * bpp;
		int maxSize = MAX_MSG_PAYLOAD; //udpConnection.GetMaxMsgSize();
		int loopTimes = floor((float)numBytes/maxSize);
		int reminder = numBytes%maxSize;

		if(debug) printf("will send %d x %d + %d\n", loopTimes, maxSize, reminder);
		sent = 0;
		char ack[SIZE_L];
		for(int i = 0; i < loopTimes; i++){
			sent += udpConnection.Send( pix + i * maxSize, maxSize);
			udpConnection.Receive(ack,SIZE_L);
			if(debug) printf("ack: %s\n", ack);
		}

		if(debug) printf("sent %d bytes total\n", sent);

		if (reminder > 0){
			if(debug) printf("will send reminder of %d\n", reminder);
			sent = udpConnection.Send( pix + numBytes - reminder, reminder);
			if(debug) printf("sent reminder of %d\n", sent);
		}
		unlock(); ////////////////////////////////////////////////////////////////////////
	}
}

