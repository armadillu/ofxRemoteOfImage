#include "testApp.h"

int imgW = 640;
int imgH = 480;


void testApp::setup(){

	ofSetVerticalSync(true);
	ofSetFrameRate(60);
	ofBackground(0);
	ofSetWindowPosition(640 + 60, 10);

	// alloc the img we will be serving
	servedImage.allocate(imgW, imgH, OF_IMAGE_GRAYSCALE);
	server.startServer(&servedImage);
}


void testApp::update(){

	server.begin(); //call begin() before editing the image, to avoid tearing when sending it to clients

		unsigned char * pix = servedImage.getPixels();
		for(int i = 0; i < imgW; i+= 1){
			for(int j = 0; j < imgH; j+= 1){
				pix[(j * imgW + i)] = (ofGetFrameNum() * 15 + i + j * 2 )%255;
			}
		}
		servedImage.update(); //update the image after editing its piexls, to see results on screen

	server.end(); //call end() after editing the image, to avoid tearing when sending it to clients
	
}


void testApp::draw(){
	servedImage.draw(0, 0);
	ofDrawBitmapString("SERVER " + ofToString(ofGetFrameRate(),1), 20, 20);
}



void testApp::exit(){
	server.stop();
}



void testApp::keyPressed(int key){

}
