#include "testApp.h"

//--------------------------------------------------------------
void testApp::setup(){

	ofSetVerticalSync(true);
	ofSetFrameRate(60);
	ofBackground(0);

	client.connect(&receivedImage, "127.0.0.1");
	ofSetWindowShape(640, 480);

}

//--------------------------------------------------------------
void testApp::update(){

	//client.update();
	//ofSetWindowShape(receivedImage.getWidth(), receivedImage.getHeight());
}


//--------------------------------------------------------------
void testApp::draw(){
	client.begin();
	client.updatePixels();
	receivedImage.draw(0, 0);
	client.end();

	ofDrawBitmapString("CLIENT " + ofToString(ofGetFrameRate(),1), 20, 20);
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){

}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}