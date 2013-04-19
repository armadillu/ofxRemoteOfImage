#include "testApp.h"


void testApp::setup(){

	ofSetVerticalSync(true);
	ofSetFrameRate(60);
	ofBackground(0);

	client.setDesiredFramerate(60);
	client.setNetworkProtocol(REMOTE_OF_IMAGE_TCP);
	client.connect(&receivedImage, "10.0.0.109");
	ofSetWindowShape(640, 480);
}


void testApp::draw(){

	client.begin(); //call begin() before drawing the image, to avoid tearing

		client.updatePixels();	//if you need to draw the image on screen, call updatePixels() before drawing it
								//otherwise the texture will not be allocated to save on GPU time

		receivedImage.draw(0, 0); //draw your image normally

	client.end(); //call end() after drawing the image, to avoid tearing

	ofDrawBitmapString("CLIENT " + ofToString(ofGetFrameRate(),1), 20, 20);
}

void testApp::exit(){
	client.stop();
}



void testApp::keyPressed(int key){

}

