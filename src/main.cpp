#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( ){
    
	ofSetupOpenGL(1280,960,OF_FULLSCREEN);  // Trying window as the same ratio as the kinect,
                                        // Having ofxPostProcessing aspect ratio issue...
	ofRunApp(new ofApp());

}
