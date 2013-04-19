#include "testApp.h"

int imgW = 640;
int imgH = 480;

//--------------------------------------------------------------
void testApp::setup(){

	ofSetVerticalSync(true);
	ofSetFrameRate(60);
	ofBackground(0);

	ofSetWindowPosition(640 + 60, 10);
	ofSetWindowShape(640, 480);
	//servedImage.setUseTexture(false);
	servedImage.allocate(imgW, imgH, OF_IMAGE_GRAYSCALE);
	server.startServer(&servedImage);
}

//--------------------------------------------------------------
void testApp::update(){

	//change image
	server.begin();
	unsigned char * pix = servedImage.getPixels();
	for(int i = 0; i < imgW; i+= 1){
		for(int j = 0; j < imgH; j+= 1){
//			pix[3 * (j * w + i)] = j;
//			pix[3 * (j * w + i) + 1] = ofRandom(255);
//			pix[3 * (j * w + i) + 2] = ofGetFrameNum()%255;
			pix[(j * imgW + i)] = (ofGetFrameNum() + i)%255;
		}
	}
	servedImage.update();
	//server.update();
	server.end();
}


//--------------------------------------------------------------
void testApp::draw(){
	servedImage.draw(0, 0);
	ofDrawBitmapString("SERVER " + ofToString(ofGetFrameRate(),1), 20, 20);
}


void testApp::exit(){
	server.stop();
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