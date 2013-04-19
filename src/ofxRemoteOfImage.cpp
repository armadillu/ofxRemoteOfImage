//
//  ofxRemoteOfImage.cpp
//  emptyExample
//
//  Created by Oriol Ferrer Mesià on 18/04/13.
//
//

#include "ofxRemoteOfImage.h"

ofxRemoteOfImage::ofxRemoteOfImage(){
	int buffer = 10000;
//	udpConnection.SetReceiveBufferSize(MAX_MSG_PAYLOAD_UDP * buffer);
//	udpConnection.SetSendBufferSize(MAX_MSG_PAYLOAD_UDP * buffer);
	udpConnection.SetTimeoutReceive(1);
	udpConnection.SetTimeoutSend(1);
	debug = true;
	img = NULL;
	frameRate = 60;
	connected = false;
//	tcpClient.TCPClient.SetReceiveBufferSize(MAX_MSG_PAYLOAD_TCP * buffer);
//	tcpClient.TCPClient.SetSendBufferSize(MAX_MSG_PAYLOAD_TCP * buffer);
//	tcpServer.TCPServer.SetReceiveBufferSize(MAX_MSG_PAYLOAD_TCP * buffer);
//	tcpServer.TCPServer.SetSendBufferSize(MAX_MSG_PAYLOAD_TCP * buffer);
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
		bool ok = tcpServer.setup(port, false);
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
		connected = tcpClient.setup(host, port, true);
		if (!connected) ofExit();
	}
	startThread();
}

string ofxRemoteOfImage::zeroPadNumber(int num){
    std::ostringstream ss;
    ss << std::setw( ZERO_PAD_LEN ) << std::setfill( '0' ) << num;
    return ss.str();
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

int ofxRemoteOfImage::send(ofxTCPManager & tcp, const unsigned char * bytes, unsigned long numBytes){

	int dataSent = 0;
	int lastTime = ofGetElapsedTimeMillis();

	cout << " send " << numBytes << " bytes" << endl;

	while (dataSent < numBytes) {

		int result = tcp.Send((const char *) bytes + dataSent, numBytes - dataSent);
		cout << "result: " << result << endl;

		if( (result < 0 && errno != EAGAIN ) || result == 0 ){
			if(debug) cout << "SEND ERROR : errno = " << errno << endl;
			return -1;
		}else{
			if(result > 0){
				dataSent += result;
			}
		}
		if (ofGetElapsedTimeMillis() - lastTime > TIME_OUT_MILIS) {
			if( TIME_OUT_MILIS > 0) cout << "SEND TIMEOUT" << endl;
			return 0;
		}
	}
	return 0;
}


int ofxRemoteOfImage::receive(ofxTCPManager &manager, const unsigned char* buffer, unsigned long numBytes){

	int dataReceived = 0;
	int result;
	int lastTime = ofGetElapsedTimeMillis();

	cout << " receive " << numBytes << " bytes" << endl;

	while( dataReceived < numBytes ){

		result = manager.Receive((char*)buffer + dataReceived, numBytes - dataReceived);
		if( (errno != EAGAIN && result < 0) || result == 0 ){
			if(debug) cout << "RECIEVE ERROR : errno = " << errno<< endl;
			return -1;
		}else{

			if(result>0){
				dataReceived += result;
				if (dataReceived == numBytes){
					dataReceived = 0;
					return numBytes;
				}
			}
		}
		if (ofGetElapsedTimeMillis() - lastTime > TIME_OUT_MILIS) {
			if(TIME_OUT_MILIS > 0 && debug) cout << "RECEIVE TIMEOUT" << endl;
			return 0;
		}
	}
	return dataReceived;
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

	if(mode == REMOTE_OF_IMAGE_CLIENT){			// CLIENT ///////////////////////////////////////////////////////////////

		if(connected){

			lock(); ///////////////////////////////////////////////////////////////

			char format[256];
			int r1 = receive(tcpClient.TCPClient , (unsigned char*)format, FORMAT_STR_LEN);
			if( r1 < FORMAT_STR_LEN){
				cout << "err reading format" << endl;
				connected = false;
				unlock();
				return;
			}

			printf("format: %s (r1: %d)\n", format, r1);

			vector<string> strElements = ofSplitString(string(format), STRING_DELIMITER);
			int w = atoi( strElements[0].c_str() );
			int h = atoi( strElements[1].c_str() );
			ofImageType imgType = (ofImageType)atoi( strElements[2].c_str() );
			bpp = bytesPerPixel( imgType );
			printf("%d %d %d\n", w, h, bpp);

//			int w = 640;
//			int h = 480;
//			ofImageType imgType = OF_IMAGE_GRAYSCALE;

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

			unsigned long numBytes = bpp * w * h;
			if(debug) printf("will receive total of %lu bytes\n", numBytes);
			int r = receive(tcpClient.TCPClient, (unsigned char*) img->getPixels(), numBytes);
			if( r < numBytes){
				cout << "err reading img data" << endl;
				connected = false;
			}

			unlock(); ///////////////////////////////////////////////////////////////

		}else{
			//if we are not connected lets try and reconnect every 5 seconds
			deltaTime = ofGetElapsedTimeMillis() - connectTime;

			if( deltaTime > RECONNECT_INTERVAL ){
				connected = tcpClient.setup(host, port, true);
				connectTime = ofGetElapsedTimeMillis();
			}
		}

	}else{			// SERVER //////////////////////////////////////////////////////////////////////////////////////

		for(int i = 0; i < tcpServer.getLastID(); i++){

			if( !tcpServer.isClientConnected(i) ) continue; 

			lock(); /////////////////////////////////////////////////////////////////

			int m = (int)img->getPixelsRef().getImageType();
			bpp = bytesPerPixel(img->getPixelsRef().getImageType());
			char * pix = (char*)img->getPixels();
			int ww = (int)img->getWidth();
			int hh = (int)img->getHeight();
			unsigned long numBytes = ww * hh * bpp;

			string format = zeroPadNumber(ww) + STRING_DELIMITER + zeroPadNumber(hh) + STRING_DELIMITER + zeroPadNumber(m) + STRING_DELIMITER; //
			send(tcpServer.TCPConnections[i].TCPClient , (unsigned char*)format.c_str(), FORMAT_STR_LEN);

			if(debug) printf("will sent total of %lu bytes\n", numBytes);
			send(tcpServer.TCPConnections[i].TCPClient , img->getPixels(), numBytes);

			unlock(); ///////////////////////////////////////////////////////////////
			
		}
		
	}
}

