//
//  ofxRemoteOfImage.cpp
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 18/04/13.
//
//

#include "ofxRemoteOfImage.h"

ofxRemoteOfImage::ofxRemoteOfImage(){
	int UDPbuffer = 1000;
	udpConnection.SetReceiveBufferSize(MAX_MSG_PAYLOAD_UDP * UDPbuffer);
	udpConnection.SetSendBufferSize(MAX_MSG_PAYLOAD_UDP * UDPbuffer);
	udpConnection.SetTimeoutReceive(1);
	udpConnection.SetTimeoutSend(1);
	debug = true;
	img = NULL;
	frameRate = 60;
	connected = false;
	//protocol = REMOTE_OF_IMAGE_UDP;
	protocol = REMOTE_OF_IMAGE_TCP;
}


void ofxRemoteOfImage::startServer(ofImage * image, int port_){

	port = port_;
	mode = REMOTE_OF_IMAGE_SERVER;
	img = image;
	if (protocol == REMOTE_OF_IMAGE_UDP){
		udpConnection.Create();
		printf("serving ofImage at %d\n", port);
		udpConnection.Bind(port);
		udpConnection.SetNonBlocking(!BLOCKING);
	}else{
		bool ok = tcpServer.setup(port, BLOCKING);
		if (!ok) ofExit();
	}
	startThread();
}


void ofxRemoteOfImage::connect(ofImage * image, string host_, int port_){

	host = host_;
	port = port_;
	mode = REMOTE_OF_IMAGE_CLIENT;
	img = image;
	if (protocol == REMOTE_OF_IMAGE_UDP){
		udpConnection.Create();
		printf("connecting to %s : %d\n", host.c_str(), port);
		udpConnection.Connect(host.c_str(), port);
		udpConnection.SetNonBlocking(!BLOCKING);
	}else{
		connected = tcpClient.setup(host, port, BLOCKING);
		if (!connected) ofExit();
	}
	startThread();
}


void ofxRemoteOfImage::setDesiredFramerate(float r){
	frameRate = r;
}

void ofxRemoteOfImage::setNetworkProtocol(ofxRemoteOfImageNetworkProtocol p){
	protocol = p;
}

void ofxRemoteOfImage::stop(){

	stopThread();
	if (protocol == REMOTE_OF_IMAGE_UDP){

		udpConnection.Close(); //rude! close while receiving

	}else{

		if(mode == REMOTE_OF_IMAGE_CLIENT){ // TCP CLIENT

			tcpClient.close();

		}else{								// TCP SERVER

			for(int i = 0; i < tcpServer.getLastID(); i++){
				if( tcpServer.isClientConnected(i) ){
					tcpServer.disconnectClient(i);
				}
			}
			ofSleepMillis(250);
			tcpServer.close();
		}
	}

	ofSleepMillis(250);
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
		if (protocol == REMOTE_OF_IMAGE_UDP)
			updateUDP();
		else
			updateTCP();

		//stick to desired fps
		float duration = ofGetElapsedTimef() - tStart;
		float remaining = 1. / frameRate - duration;

		//printf("duration: %f total: %f remaining %f\n", duration, 1/frameRate, remaining );
		if (remaining < 0) remaining = 0;
		if (remaining > 0){
			ofSleepMillis( 1000.0f * remaining);
		}
	}
	cout << "ending ThreadedFunction()!" << endl;
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

void ofxRemoteOfImage::updateUDP(){

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
		if (w == 0 || h == 0){
			unlock();
			return;
		}
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
			//if (data) free(data);
			//data = (char*) malloc( sizeof(char) * w * h * bpp);
		}
		int numBytes = w * h * bpp;
		char * pix = (char*)img->getPixels();

		//udpConnection.Receive( (char*)img->getPixels(), numBytes);

		int maxSize = MAX_MSG_PAYLOAD_UDP; //udpConnection.GetMaxMsgSize();
		if(maxSize > numBytes) maxSize = numBytes; // for very tiny images!
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
		int maxSize = MAX_MSG_PAYLOAD_UDP; //udpConnection.GetMaxMsgSize();
		if(maxSize > numBytes) maxSize = numBytes; // for very tiny images!
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



void ofxRemoteOfImage::updateTCP(){

	int bpp = 1;

	if(mode == REMOTE_OF_IMAGE_CLIENT){			// CLIENT

		if(connected){

			tcpClient.send(STARTUP_MSG); //////////////////////////////////////////////////////////		<<	HI
			string startMSG = tcpClient.receive(); ////////////////////////////////////////////////		>>	HI
			if(debug) cout << "CLIENT got Start MSG: " << startMSG << endl;

			if( STARTUP_MSG == startMSG ){

				lock(); //////////////////////////////////////////////////////////////////////////////////////////////////
				
				string ww = tcpClient.receive();	///////////////////////////////////////////////		<<	W
				if(debug) cout << "ww: " << ww <<endl;
				if(ww.length() != 0){
					
					string hh = tcpClient.receive();	///////////////////////////////////////////		<<	H
					if(debug) cout << "hh: " << hh <<endl;
					if(hh.length() != 0){

						string imgMode = tcpClient.receive();	///////////////////////////////////		<<	MODE
						if(debug) cout << "imgMode: " << imgMode <<endl;
						if(imgMode.length() != 0){

							int w = atoi( ww.c_str() );
							int h = atoi( hh.c_str() );
							ofImageType imgType = (ofImageType)atoi(imgMode.c_str());
							bpp = bytesPerPixel( imgType );

							if (img->getWidth() == w &&
								img->getHeight() == h &&
								imgType == img->getPixelsRef().getImageType()
								){
								//OK
							}else{ //server image changed dimensions!
								img->clear();
								img->setUseTexture(false);
								img->allocate(w, h, imgType);
								img->setUseTexture(true);
							}

							int numBytes = w * h * bpp;
							char * pix = (char*)img->getPixels();
							tcpClient.send(ACK_MSG);	///////////////////////////////////////////		>>	ACK

							int maxSize = MAX_MSG_PAYLOAD_TCP;
							if(maxSize > numBytes) maxSize = numBytes; // for very tiny images!
							int loopTimes = floor((float)numBytes/maxSize);
							int reminder = numBytes%maxSize;

							if(debug) printf("will receive %d x %d + %d\n", loopTimes, maxSize, reminder);
							int nr = 0;
							for(int i = 0; i < loopTimes; i++){
								nr = tcpClient.receiveRawBytes( pix + i * maxSize, maxSize);///////		<<	DATA_PART_N
								//if(debug) cout << "got " << nr << " bytes" << endl;
								if ( nr == 0 ) break;
								tcpClient.send(ACK_MID_MSG);///////////////////////////////////////		>> MID_ACK
							}

							if (reminder > 0 && nr > 0){
								//if(debug) printf("will Receive reminder of %d\n", reminder);
								tcpClient.receiveRawBytes( pix + numBytes - reminder, reminder);///		<< DATA_REMAINDER
							}
							//if(debug) printf("did Receive reminder of %d\n", reciv);

							tcpClient.send(LOOP_MSG);
						}
					}
				}

				unlock(); ///////////////////////////////////////////////////////////////////////////////////////////////

			}else{
				if(debug) cout << "bad STARTUP MSG" << endl;
				if(!tcpClient.isConnected()){
					if(debug) cout << "disconnected!" << endl;
					connected = false;
				}
			}
		}else{
			//if we are not connected lets try and reconnect every 5 seconds
			deltaTime = ofGetElapsedTimeMillis() - connectTime;

			if( deltaTime > RECONNECT_INTERVAL ){
				connected = tcpClient.setup(host, port);
				connectTime = ofGetElapsedTimeMillis();
			}
		}

	}else{			//SERVER

		for(int i = 0; i < tcpServer.getLastID(); i++){

			if( !tcpServer.isClientConnected(i) ) continue; //clanup at some point?

			string port = ofToString( tcpServer.getClientPort(i) );
			string ip   = tcpServer.getClientIP(i);

			string startMSG = tcpServer.receive(i); ////////////////////////////////////////		<< HI
			if(debug) cout << "SERVER got Start MSG: " << startMSG << endl;
			
			if (startMSG.length() > 0 ){

				tcpServer.send(i, STARTUP_MSG);	////////////////////////////////////////////		>>	HI

				lock(); ////////////////////////////////////////////////////////////////////////////////////////

				int m = (int)img->getPixelsRef().getImageType();
				bpp = bytesPerPixel(img->getPixelsRef().getImageType());
				char * pix = (char*)img->getPixels();
				int numBytes = img->getWidth() * img->getHeight() * bpp;

				string ww = ofToString((int)img->getWidth());
				tcpServer.send(i, ww);	////////////////////////////////////////////////////		>>	W

				string hh = ofToString((int)img->getHeight());
				tcpServer.send(i, hh);	////////////////////////////////////////////////////		>>	H

				tcpServer.send(i, ofToString(m));	////////////////////////////////////////		>>	MODE

				string ack = tcpServer.receive(i);	////////////////////////////////////////		<<	ACK
				if(debug) cout << "ack: " << ack << endl;
				if ( ack.length() == 0 ) continue;

				int maxSize = MAX_MSG_PAYLOAD_TCP; //udpConnection.GetMaxMsgSize();
				if(maxSize > numBytes) maxSize = numBytes; // for very tiny images!
				int loopTimes = floor((float)numBytes/maxSize);
				int remainder = numBytes%maxSize;

				if(debug) printf("will sent total of %d bytes\n", numBytes);
				
				for(int j = 0; j < loopTimes; j++){
					tcpServer.sendRawBytes( i, pix + j * maxSize, maxSize);/////////////////		>>	DATA_PART_N
					string midACK = tcpServer.receive(i);////////////////////////////////////		<<	MID_ACK
					if ( midACK.length() == 0 ){
						remainder = 0; // to skip remainder part
						break;
					}
					//if(debug) printf("midAck: %s\n", midACK.c_str());
				}

				if (remainder > 0){
					if(debug) printf("will send reminder of %d\n", remainder);
					tcpServer.sendRawBytes( i, pix + numBytes - remainder, remainder);////////		>> DATA_REMAINDER
				}
			}

			string loopMSG = tcpServer.receive(i);
			if(debug) printf("loopMSG: %s\n", loopMSG.c_str());

			unlock(); ///////////////////////////////////////////////////////////////////////////////////////

		}
	}
}

