

////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                            //
//                                   C A P G R A S                                            //
//                                 D E L U S I O N S                                          //
//                                                                                            //
//                            By Joseph Rodrigues Marsh 2018                                  //
//                    Workshops in Creative Coding Term 2 Assignment                          //
//                                                                                            //
//              A grotesque computer mirror which distorts all it reflects.                   //
//                                                                                            //
// --- --- --- --- --- --- --- --- --- ------ --- --- --- ------ --- --- --- ------ --- --- - //
// - ----- -- - --- - --- -- - - -- ---- -- --- - - -- - - -- - ------ - ----- ---- --- - --- //
//                                                                                            //
//              The program is set to run in 'Presentation Mode', where it will               //
//              run autonomously, however it is also possible to enter 'Debug mode'           //
//              in order to install for an installation.                                      //
//                                                                                            //
//              Hit 'Enter' to switch between Presentation / Debug modes.                     //
//              More comprehensive input notes can be found in keyPressed().                  //
//                                                                                            //
//              Currently, the viewer is required to be positioned around 1.5 feet            //
//              from the kinect in order to be in focus and for cameras to be                 //
//              correctly positioned, but this can be tweaked depending on the space.         //
//                                                                                            //
//                                                                                            //
////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                            //
//                                    References                                              //
//                                                                                            //
// KinectDelaunay by Kamen Dimitrov https://bit.ly/2K7U9VN                                    //
// Face Capturing Panopticon by Zach Rispoll https://bit.ly/2HHwL32                           //
// Animated looping noise https://bit.ly/2jczrao                                              //
//                                                                                            //
////////////////////////////////////////////////////////////////////////////////////////////////


#include "ofApp.h"

using namespace ofxCv;
using namespace cv;


                                /////////////////
                                //  S E T U P  //
                                //  S E T U P  //
                                //  S E T U P  //
                                /////////////////
// Let's begin.

void ofApp::setup(){
    
    ofSetVerticalSync(true);
    ofSetBackgroundAuto(true);
    
    //Initialise the kinect
    initKinect();
    
    // Set up the tracker
    tracker.setup();
    tracker.setRescale(.5);
    captureFaceTimer = 0;
    
    // Initialise the scene, GUI and postFX
    initCamera(camNum);
    initBG();
    initGUI();
    initPostFX();
    
    // Set initial booleans
    bFaceCaptured = false;
    bEnableFX = true;
    bIsRealTime = false;
    bWireframe = true;
    bFaces = true;
    bPoints = false;
    bNoiseMode = true;
    bPresentationMode = false;
    bSceneChanged = false;
    captureFaceTimerMax = 60; // Amount of time to take a portrait and for each scene,
                              // default 60 but should increase if i'm able to increase the frameRate
                              // which is currently +/- 10fps :(
}

                                /////////////////////
                                // U P D A T I N G //
                                // U P D A T I N G //
                                // U P D A T I N G //
                                /////////////////////

void ofApp::update(){
    
    // Update the kinect images
    kinect.update();
    
    // If there is a new frame and we are connected...
    if(kinect.isFrameNew()) {
        kinectDepth.setFromPixels(kinect.getDepthPixels());
        kinectDepth.flagImageChanged();
        kinectColor.setFromPixels(kinect.getPixels());
        kinectColor.flagImageChanged();
    }
    
    // Make meaningful choices...
    theDirector();
    
    // Update the face grabber
    updateFaceGrabber();
    
    // In realtime mode, update the mesh every frame
    if (bIsRealTime){
        updateDelaunay();
    }
    
    // Modulate each vertex of the Delaunay mesh every frame
    modulateDelaunay();
    
    // Increment the timer
    timer++;
}

void ofApp::updateFaceGrabber(){
    // Update the tracker with the kinect's RGB image
    tracker.update(toCv(kinectColor));

    cropX = tracker.getPosition().x;
    cropY = tracker.getPosition().y;
    float faceSize = tracker.getScale();
    cropW = faceSize * pixelScale;
    cropH = faceSize * pixelScale * ratio;
    cropY -= cropH/fudge;
    
    captureFaceTimer++;
    
    if (timer % captureFaceTimerMax == 0){
        captureFace();
        captureFaceTimer = 0;
        
        // In portrait mode, only update whenever the timer resets
        if (!bIsRealTime){
            updateDelaunay();
        }
    }
}

void ofApp::updateDelaunay(){
    
    del.reset();
    
    // Create a pixel array to store our depth values.
    unsigned char* pix = new unsigned char[640*480];
    
    // Loop through every pixel of the kinect and check the distance
    
    for(int x = 0; x < kinect.width; x++) {
        for(int y = 0; y < kinect.height; y++) {
            
            float distance = kinect.getDistanceAt(x, y);
            int pIndex = x + y * 640;
            pix[pIndex] = 0;
            
            if(distance > depthNear && distance < depthFar) {
                
                // If it's within the threshold, mark the pixel as white
                pix[pIndex] = 255;
            }
        }
    }
    
    
    // Set the blob image to be from the grayscale pix array.
    blob.setFromPixels(pix, kinect.width,kinect.height, OF_IMAGE_GRAYSCALE);
    
    int numPoints = 0;
    
    // Loop through the whole kinect image
    for(int x = 0; x < kinect.width; x += spacing) {
        for(int y = 0; y < kinect.height; y += spacing) {
            int pIndex = x + 640 * y;
            
            // If there is a pixel at the index
            if(blob.getPixels()[pIndex] > 0) {
                
                // Create a temp vector at that pixels world position
                ofVec3f wc = kinect.getWorldCoordinateAt(x, y);
                
                // Subtract the depth image w/h
                wc.x = x - 320.0;
                wc.y = y - 240.0;
                
                // If it's within the threshold...
                if(abs(wc.z) > depthNear && abs(wc.z ) < depthFar) {
                    
                    // flip the Z axis
                    wc.z = -wc.z;
                    // And add the point to the delaunay
                    del.addPoint(wc);
                }
                numPoints++;
            }
        }
    }
    
    // If we have more than 0 points, triangulate
    if(numPoints > 0)
        del.triangulate();
    
    // Initialise the delaunay with a colour
    for(int i=0;i<del.triangleMesh.getNumVertices();i++) {
        del.triangleMesh.addColor(ofColor(0,0,0));
    }

    // And set that colour to the corresponding vertices' colour
    for(int i=0;i<del.triangleMesh.getNumIndices()/3;i+=1) {
        ofVec3f v = del.triangleMesh.getVertex(del.triangleMesh.getIndex(i*3));
        
        v.x = ofClamp(v.x, -319,319);
        v.y = ofClamp(v.y, -239, 239);
        
        ofColor c = kinect.getColorAt(v.x+320.0, v.y+240.0);
        c.a = 255;
        
        del.triangleMesh.setColor(del.triangleMesh.getIndex(i*3),c);
        del.triangleMesh.setColor(del.triangleMesh.getIndex(i*3+1),c);
        del.triangleMesh.setColor(del.triangleMesh.getIndex(i*3+2),c);
    }
    
    // Clear both meshes
    delaunayMesh.clear();
    wireframeMesh.clear();
    wireframeMesh.setMode(OF_PRIMITIVE_TRIANGLES);
    
    for(int i=0;i<del.triangleMesh.getNumIndices()/3;i+=1) {
        
        // Create indices and points from the triangulated mesh
        
        int indx1 = del.triangleMesh.getIndex(i*3);
        ofVec3f p1 = del.triangleMesh.getVertex(indx1);
        int indx2 = del.triangleMesh.getIndex(i*3+1);
        ofVec3f p2 = del.triangleMesh.getVertex(indx2);
        int indx3 = del.triangleMesh.getIndex(i*3+2);
        ofVec3f p3 = del.triangleMesh.getVertex(indx3);
        
        ofVec3f triangleCenter = (p1+p2+p3)/3.0; // Determine the centre of the triangle
        triangleCenter.x += 320; // Depth image width
        triangleCenter.y += 240; // Depth image height
        
        triangleCenter.x = floor(ofClamp(triangleCenter.x, 0,640));
        triangleCenter.y = floor(ofClamp(triangleCenter.y, 0, 480));
        
        int pixIndex = triangleCenter.x + triangleCenter.y * 640;
        if(pix[pixIndex] > 0) {
            
                // Add vertices to the face mesh from the triangulted vertices
                // and slightly desaturate...
                desatVal = 1.9;
                delaunayMesh.addVertex(p1);
                ofColor dC1 = del.triangleMesh.getColor(indx1);
                dC1.setSaturation(dC1.getSaturation() / desatVal);
                delaunayMesh.addColor(dC1);
                
                delaunayMesh.addVertex(p2);
                ofColor dC2 = del.triangleMesh.getColor(indx2);
                dC2.setSaturation(dC2.getSaturation() / desatVal);
                delaunayMesh.addColor(dC2);
                
                delaunayMesh.addVertex(p3);
                ofColor dC3 = del.triangleMesh.getColor(indx3);
                dC3.setSaturation(dC3.getSaturation() / desatVal);
                delaunayMesh.addColor(dC3);
            
                // Do the same for the wireframe mesh
                wireframeMesh.addVertex(p1);
                ofColor wC1 = del.triangleMesh.getColor(indx1);
                wC1.setSaturation(wC1.getSaturation() / desatVal);
                wireframeMesh.addColor(wC1);
            
                wireframeMesh.addVertex(p2);
                ofColor wC2 = del.triangleMesh.getColor(indx2);
                wC2.setSaturation(wC2.getSaturation() / desatVal);
                wireframeMesh.addColor(wC2);
            
                wireframeMesh.addVertex(p3);
                ofColor wC3 = del.triangleMesh.getColor(indx3);
                wC3.setSaturation(wC3.getSaturation() / desatVal);
                wireframeMesh.addColor(wC3);
        }
    }
    
    // And delete the pixel array now we are done with it.
    delete[] pix;
    
}

void ofApp::modulateDelaunay(){
    
    // For each vertex in the mesh...
    for (int i = 0; i < delaunayMesh.getVertices().size(); i++){
        ofVec3f vecMod = delaunayMesh.getVertex(i);
        float t = 1.0 * (ofGetElapsedTimef()) / 24;
        
        // Add 4D noise to our temporary vectors x/y position
        if (bNoiseMode){
            
            vecMod.x += ofMap(ofSignedNoise(noiseScale * vecMod.x, noiseScale * vecMod.z, noiseRadius * sin(TWO_PI * t), noiseRadius * cos(TWO_PI * t)), -1, 1, -noiseAmt, noiseAmt);
            vecMod.y += ofMap(ofSignedNoise(noiseScale * vecMod.y, noiseScale * vecMod.z, noiseRadius * sin(TWO_PI * t), noiseRadius * cos(TWO_PI * t)), -1, 1, -noiseAmt, noiseAmt);
            
            // And set the vertex add the index to our modulated vertex
            delaunayMesh.setVertex(i, vecMod);
            wireframeMesh.setVertex(i, vecMod);
        }
        else if (!bNoiseMode){
            
            vecMod.x += ofMap(ofSignedNoise(noiseScale * vecMod.x, noiseScale * vecMod.y, noiseRadius * sin(TWO_PI * t), noiseRadius * cos(TWO_PI * t)), -1, 1, -noiseAmt, noiseAmt);
            vecMod.y += ofMap(ofSignedNoise(noiseScale * vecMod.x, noiseScale * vecMod.y, noiseRadius * sin(TWO_PI * t), noiseRadius * cos(TWO_PI * t)), -1, 1, -noiseAmt, noiseAmt);
            
            delaunayMesh.setVertex(i, vecMod);
            wireframeMesh.setVertex(i, vecMod);
        }
    }
    
    bSceneChanged = false;
}

void ofApp::theDirector(){
    
    // If presenting...
    if (bPresentationMode){
        cam.disableMouseInput();
        
        // theDirector cycles through each camera sequentially
        // and picks the corresponding colour from the array
        if (timer % captureFaceTimerMax == 0){
            
            changeCamera(camNum + 1);
            ofSetBackgroundColor(colors[camNum]);
            if (camNum >= 4) camNum = 0;
            updateCamera();
        }
    }
    
    if (bIsRealTime & bSceneChanged){
        
        // Pick semi-random values for each new scene
        noiseScale = ofRandom(0.005, 0.015);    // Choose the noise scale
        noiseRadius = ofRandom(0.50, 4);        // Choose the noise radius
        noiseAmt = 30.0;                        // Choose the amount of noise
        bNoiseMode = (int)ofRandom(2);          // Choose noise mode
        bDelusion = (int)ofRandom(2);           // Choose delusion
        depthFar = 780;
    }
    
    else if (!bIsRealTime & bSceneChanged){
        noiseScale = ofRandom(0.01, 0.015);
        noiseRadius = ofRandom(0.5, 5);
        noiseAmt = 2.0;
        bNoiseMode = (int)ofRandom(2);
        bDelusion = (int)ofRandom(2);
        depthFar = 1300;
    }
    
    if (!bPresentationMode){
        bDrawDebug = true;
        cam.enableMouseInput();
    }
    else if (bPresentationMode){
        bDrawDebug = false;
    }
}





                                ///////////////////
                                // D R A W I N G //
                                // D R A W I N G //
                                // D R A W I N G //
                                ///////////////////

void ofApp::draw(){

    if (bEnableFX) postfx.begin(cam);
    if (bDrawAxis) drawAxis();
    
    drawDelaunay();
    
    if (bEnableFX) postfx.end();
    if (bDrawDebug) drawDebug(); ofSetWindowTitle(ofToString(ofGetFrameRate()));
    
}

void ofApp::drawDelaunay(){
    
    cam.begin();
    
    push();
    
    ofSetColor(255,255,255);
    ofTranslate(0,  -580, 400);
    ofScale(ofPoint(3));
    ofFill();
    
    if (bFaces || bDelusion){
        bPoints = false;
        bWireframe = false;
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glShadeModel(GL_FLAT);
        glProvokingVertex(GL_FIRST_VERTEX_CONVENTION);
        delaunayMesh.drawFaces();
        glShadeModel(GL_SMOOTH);
        glPopAttrib();
    }

    if (bWireframe || !bDelusion){
        bFaces = false;
        ofPushMatrix();
        ofTranslate(0, 0,0.5);
        glLineWidth(3);
        wireframeMesh.drawWireframe();
        ofPopMatrix();
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPointSize(5);
        glEnable(GL_POINT_SMOOTH);
        delaunayMesh.drawVertices();
        glPopAttrib();
    }
    
    pop();
    
    cam.end();
}

void ofApp::captureFace() {
    
    // A remnant of when I was trying to capture only the face...
    // I'm reluctant to remove as I think I will be able to achieve
    // better performance by only capturing the face mask.
    // The idea was to use the brighness pixels of the captured image to
    // extrude a plane in Z space, and manipulate from there.
    
    // Causes occasional OfPixel errors....
    //capturedFaceColor.setFromPixels(kinectColor.getPixels());
    //capturedFaceDepth.setFromPixels(kinectDepth.getPixels());
//
    capturedFaceColor.crop(cropX-cropW/2.0,
                           cropY-cropH/2.0,
                           cropW,
                           cropH);
    capturedFaceDepth.crop(cropX-cropW/2.0,
                           cropY-cropH/2.0,
                           cropW,
                           cropH);
    
    // Rather than looking thorugh the whole kinect image,
    // need to just grab the face pixels
    
    for(int x = 0; x < kinect.width; x += spacing) {
        for(int y = 0; y < kinect.height; y += spacing) {
            
            float distance = kinect.getDistanceAt(x, y);

            if(distance > depthNear && distance < depthFar)
            {
                ofVec3f wc = kinect.getWorldCoordinateAt(x, y);
                generatedMesh.addColor(kinect.getColorAt(x, y));
                wc.z = -wc.z;
                
                mesh.addVertex(wc);

            }
        }
    }
    bFaceCaptured = true;
    tracker.reset();
}






                                ///////////////
                                // S C E N E //
                                // S C E N E //
                                // S C E N E //
                                ///////////////


//--------------------------------------------------------------
void ofApp::initCamera(int num){
    
    // Initialise cam by loading camera settings from file
    cam.resetTransform();
    ofxLoadCamera(cam, "cam" + ofToString(num));
}

//--------------------------------------------------------------
void ofApp::changeCamera(int num){
    
    // Load new camera from saved settings
    cam.resetTransform();
    ofxLoadCamera(cam, "cam" + ofToString(num));
    camNum = num;
    bSceneChanged = true; // Flag the scene has changed in order to change background
}

//--------------------------------------------------------------
void ofApp::saveCamera(int num){
    ofxSaveCamera(cam, "cam" + ofToString(num));
}

//--------------------------------------------------------------
void ofApp::updateCamera(){
    // Add rotation / swing?
}

//--------------------------------------------------------------
void ofApp::initBG(){
    colors = new ofColor[5];
    
    // Blue
    colors[0].r = 145;
    colors[0].g = 209;
    colors[0].b = 218;
    
    // Pink
    colors[1].r = 224;
    colors[1].g = 123;
    colors[1].b = 141;
    
    // Green
    colors[2].r = 76;
    colors[2].g = 195;
    colors[2].b = 174;
    
    // Grey
    colors[3].r = 175;
    colors[3].g = 186;
    colors[3].b = 199;
    
    // Lellow
    colors[4].r = 226;
    colors[4].g = 189;
    colors[4].b = 72;
}







                            ///////////////////
                            // H E L P E R S //
                            // H E L P E R S //
                            // H E L P E R S //
                            ///////////////////

void ofApp::initKinect(){
    
    angle = 0;
    kinect.setRegistration(true);
    kinect.init();
    kinect.open();
    kinect.setDepthClipping(depthNear, depthFar);
    kinect.setCameraTiltAngle(angle);
    
    kinectColor.allocate(kinect.width, kinect.height);
    kinectDepth.allocate(kinect.width, kinect.height);
}

void ofApp::initGUI(){
    gui.setup();
    gui.add(focus.setup("focus", 0.9999, 0.9500, 0.9999));
    gui.add(aperture.setup("aperture", 0.8, 0, 1));
    gui.add(maxBlur.setup("maxBlur", 0.6, 0, 1));
    gui.add(focalLength.setup("focalLength", 1000.f, -2000, 2000));
    gui.add(focalDepth.setup("focalDepth", 1.5f, 0., 10.));
    gui.add(fStop.setup("fStop", 5.6, 0, 22));
    gui.add(cam1X.setup("cam1: X", 0, -2000, 2000));
    gui.add(cam1Y.setup("cam1: Y", -580, -2000, 2000));
    gui.add(cam1Z.setup("cam1: Z", 2400, -4000, 4000));
    gui.add(cam1Rot.setup("cam1: Rotation", 0, -180, 180));
}

void ofApp::initPostFX(){
    
    // This is VERY fiddly, the blur effect is too strong for my liking
    // and its parameters are rather unintuitive. Currently requires
    // the user to be in a very specific location in Z space which would be
    // marked in the installation space.
    
    postfx.init(ofGetWidth(), ofGetHeight());
    postfx.createPass<DofPass>()->setEnabled(true);
    postfx.createPass<DofPass>()->setFocus(0.9899); // Very, very fine focal length.
    postfx.createPass<DofPass>()->setAperture(.02f);
    postfx.createPass<DofPass>()->setMaxBlur(0.0005);
    postfx.setFlip(false);
}

void ofApp::drawDebug(){
    
    // Fairly self explanatory debug display.
    
    ofDisableDepthTest();
    push();
    
    ofSetColor(255, 255, 255);
    ofScale(.4, .4);
    kinectColor.draw(0, 0);     // Raw kinect RGB image
    kinectDepth.draw(0, 480);   // Raw kinect depth image
    blob.draw(0, 960);          // Draw the image we are using to calculate the delaunay
    
    if(bFaceCaptured) {
        //ofSetColor(0, 0, 0, 150);
        //ofDrawBitmapString("Captured face RGB", 200, 1540);
        //ofDrawBitmapString("Captured face Depth", 200, 1720);
        //ofSetColor(255);
        //capturedFaceColor.draw(0, 1440);
        //capturedFaceDepth.draw(0, 1620);
    }

    ofSetColor(255, 0, 0, 150);
    ofSetRectMode(OF_RECTMODE_CENTER);
    ofDrawRectangle(cropX, cropY, cropW, cropH);// Draw the face capture
    ofSetColor(255, 255, 255, 100);
    tracker.draw();                             // Draw the tracker
    pop();

    push();
    int nudgeX = 150;
    int nudgeY = 15;
    
    ofTranslate(ofGetWidth() - 300, ofGetHeight() - 180);
    ofSetColor(0, 0, 0, 150);
    ofDrawBitmapString("Drawing Mode:", 0, 0);
    ofDrawBitmapString("Delusion:", 0, nudgeY);
    
    if (bIsRealTime) ofDrawBitmapString("R E A L T I M E", nudgeX, 0);
    else if (!bIsRealTime) ofDrawBitmapString("P O R T R A I T", nudgeX, 0);
    if (bDelusion) ofDrawBitmapString("C A P G R A S", nudgeX, nudgeY);
    else if (!bDelusion) ofDrawBitmapString("C O T A R D", nudgeX, nudgeY);
    
    // Display variable info for debugging
    
    String noiseMode;
    if (bNoiseMode) noiseMode = "X, Z, Y, Z";
    else{ noiseMode = "X, Y, X, Y"; }
    ofTranslate(0, nudgeY);
    ofDrawBitmapString("numVerts:", 0, nudgeY);
    ofDrawBitmapString(ofToString(delaunayMesh.getVertices().size()), nudgeX, nudgeY);
    ofDrawBitmapString("noiseScale:", 0, nudgeY*2);
    ofDrawBitmapString(ofToString(noiseScale), nudgeX, nudgeY*2);
    ofDrawBitmapString("noiseRadius:", 0, nudgeY*3);
    ofDrawBitmapString(ofToString(noiseRadius), nudgeX, nudgeY*3);
    ofDrawBitmapString("noiseAmt:", 0, nudgeY*4);
    ofDrawBitmapString(ofToString(noiseAmt), nudgeX, nudgeY*4);
    ofDrawBitmapString("faceTimer:", 0, nudgeY*5);
    ofDrawBitmapString(ofToString(captureFaceTimer), nudgeX, nudgeY*5);
    ofDrawBitmapString("noiseMode: ", 0, nudgeY*6); ofDrawBitmapString(noiseMode, nudgeX, nudgeY*6);
    ofDrawBitmapString("Cam:", 0, nudgeY*7);
    ofDrawBitmapString(ofToString(camNum), nudgeX, nudgeY*7);
    ofDrawBitmapString("KinectDepthNear:", 0, nudgeY*8);
    ofDrawBitmapString(ofToString(depthNear), nudgeX, nudgeY*8);
    ofDrawBitmapString("KinectDepthFar:", 0, nudgeY*9);
    ofDrawBitmapString(ofToString(depthFar), nudgeX, nudgeY*9);
    
    pop();
    
    gui.setPosition(ofGetWidth() - 300, 10);
    gui.draw();
    
    ofEnableDepthTest();
}

void ofApp::drawAxis(){
    
        // Draw an axis for debugging
        push();
        // X
        ofSetLineWidth(3);
        ofSetColor(255, 0, 0);
        ofDrawLine(0, 0, 0, 100, 0, 0);
        ofDrawSphere(100, 0, 0, 3);
        // Y
        ofSetColor(0, 255, 0);
        ofDrawLine(0, 0, 0, 0, 100, 0);
        ofDrawSphere(0, 100, 0, 3);
        // Z
        ofSetColor(0, 0, 255);
        ofDrawLine(0, 0, 0, 0, 0, 100);
        ofDrawSphere(0, 0, 100, 3);
        pop();
}

//--------------------------------------------------------------
void ofApp::push(){
    ofPushMatrix();
    ofPushStyle();
    glPushAttrib(GL_ALL_ATTRIB_BITS);
}

//--------------------------------------------------------------
void ofApp::pop(){
    ofPopMatrix();
    ofPopStyle();
    glPopAttrib();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    
    // Inputs for debugging / set up, only if we're not presenting
    
    if (key == OF_KEY_RETURN){
        bPresentationMode = !bPresentationMode;
    }
    
    if (!bPresentationMode){
        
        // ONLY FOR DEBUG / INSTALL
        
        switch (key) {
                
        case 'd':
            bDrawDebug = !bDrawDebug;
            break;
                
        case 'a':
            bDrawAxis = !bDrawAxis;
            break;
                
        case 'r': // To switch between portrait and realtime mode
            bIsRealTime = !bIsRealTime;
            bSceneChanged = true;
            break;
                
        case 'e': // Enable / disable PostFX
            bEnableFX = !bEnableFX;
            break;
                
        case 'f': // Draw faces on / off
            bFaces = !bFaces;
            break;
                
        case 'w': // Draw wireframe on / off
            bWireframe = !bWireframe;
            break;
            
        case '.': // Draw points on / off
            bPoints = !bPoints;
            break;
                
            case 'n': // Switch noise mode (X, Y, X, Y) || (X, Z, Y, Z)
            bNoiseMode = !bNoiseMode;
            break;

        case '=':
            noiseScale += 0.005; // Increase / Decrease the noise scale
            if (noiseScale >= 1.0) noiseScale = 1.0;
            break;
            
        case '-':
            noiseScale -= 0.005;
            if (noiseScale <= 0.0) noiseScale = 0.0;
            break;
            
        case ']':
            noiseRadius += 0.5; // Increase / Decrease the noise radius
            if (noiseRadius >= 10) noiseRadius = 10;
            break;
            
        case '[':
            noiseRadius -= 0.5;
            if (noiseRadius <= 0) noiseRadius = 0;
            break;
                
        case 'o': // Open the connection to the kinect (in case it bugs out)
            kinect.setCameraTiltAngle(angle); // go back to prev tilt
            kinect.open();
            break;
                
        case 'c': // Close the connection to the kinect (refresh the image)
            kinect.setCameraTiltAngle(0); // zero the tilt
            kinect.close();
            break;
                
        case OF_KEY_UP: // Increase / decrease tilt of the kinect, ideally keep this at eye level
            angle++;
            if(angle > 30) angle = 30;
            kinect.setCameraTiltAngle(angle);
            break;
            
        case OF_KEY_DOWN:
            angle--;
            if(angle < -30) angle = -30;
            kinect.setCameraTiltAngle(angle);
            break;
            
        case OF_KEY_LEFT: // Clip the depth image near / far
            depthFar -= 10;
            if(depthFar<50) depthFar=50;
            break;
            
        case OF_KEY_RIGHT:
            depthFar += 10;
            if(depthFar>4000) depthFar=4000;
            break;
                
        // Saving and loading cameras for debug
        case '0':
            camNum = 0;
            saveCamera(camNum);
            break;
        case '1':
            camNum = 1;
            saveCamera(camNum);
            break;
        case '2':
            camNum = 2;
            saveCamera(camNum);
            break;
        case '3':
            camNum = 3;
            saveCamera(camNum);
            break;
        case '4':
            camNum = 4;
            saveCamera(camNum);
            break;
        case '5':
            camNum = 0;
            initCamera(camNum);
            break;
        case '6':
            camNum = 1;
            initCamera(camNum);
            break;
        case '7':
            camNum = 2;
            initCamera(camNum);
            break;
        case '8':
            camNum = 3;
            initCamera(camNum);
            break;
        case '9':
            camNum = 4;
            initCamera(camNum);
            break;
                

    }

    }
}

