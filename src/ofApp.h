#pragma once

#include "ofMain.h"
#include "ofxFaceTracker.h"
#include "ofxOpenCv.h"
#include "ofxKinect.h"
#include "ofxDelaunay.h"
#include "ofxPostProcessing.h"
#include "ofxGUI.h"
#include "ofxCameraSaveLoad.h"

class ofApp : public ofBaseApp{

	public:
		void setup();
        void initCamera(int num);
        void initBG();
        void initGUI();
        void initPostFX();
        void initKinect();
    
        void updateFaceGrabber();
        void updateDelaunay();
        void modulateDelaunay();
		void update();
    
        void drawDebug();
        void drawAxis();
        void drawDelaunay();
		void draw();
    
        void captureFace();
    
        void theDirector();
        void updateCamera();
        void changeCamera(int num);
        void saveCamera(int num);

		void keyPressed(int key);
        void push();
        void pop();
    
    // Face tracking and kinect
    ofxKinect kinect;
    ofxFaceTracker tracker;
    
    int cropX;
    int cropY;
    int cropW;
    int cropH;
    
    bool bFaceCaptured;
    int captureFaceTimer;
    int captureFaceTimerMax;
    
    ofxCvColorImage kinectColor;
    ofxCvGrayscaleImage kinectDepth;
    ofImage capturedFaceColor;
    ofImage capturedFaceDepth;
    ofImage blob;
    
    const float ratio = 1.4;
    const float pixelScale = 55.0;
    const float fudge = 7.0;
    const bool USE_KINECT_DEPTH = false;
    int depthNear = 5.0;
    int depthFar = 1000;
    int angle = 0.0;
    
    ofEasyCam cam;

    ofMesh generatedMesh;
    ofMesh mesh;
    
    ofxDelaunay del;
    ofVboMesh delaunayMesh;
    ofVboMesh wireframeMesh;
    
    // Lights
    ofLight pointLight;
    
    // FX
    ofxPostProcessing postfx;
    
    // Noise
    float noiseRadius;
    float noiseScale;
    float noiseAmt;
    
    int spacing = 3;
    int timer = 0;
    
    // Palette
    ofColor *colors;
    int desatVal;
    
    // Functional Booleans
    bool bIsRealTime; // If not real time, then portrait mode
    bool bWireframe;
    bool bFaces;
    bool bPoints;
    bool bNoiseMode;
    bool bEnableFX;
    int camNum = 0;
    
    bool bPresentationMode;
    bool bDelusion; // 1 == Capgras, 0 == Cotard;
    bool bSceneChanged;
    
    // Helpers
    bool bDrawDebug;
    bool bDrawAxis;
    
    ofxFloatSlider focus;
    ofxFloatSlider aperture;
    ofxFloatSlider maxBlur;
    ofxFloatSlider focalLength;
    ofxFloatSlider focalDepth;
    ofxFloatSlider fStop;
    ofxFloatSlider cam1X;
    ofxFloatSlider cam1Y;
    ofxFloatSlider cam1Z;
    ofxFloatSlider cam1Rot;
    ofxPanel gui;
    
};
