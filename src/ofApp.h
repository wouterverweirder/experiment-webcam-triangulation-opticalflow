#pragma once

#include "ofMain.h"
#include "ofxCv.h"
#include "ofxPoly2Tri.h"

class ofApp : public ofBaseApp {
public:
  void setup();
  void update();
  void draw();

  int videoWidth;
  int videoHeight;
  
  ofVideoGrabber videoGrabber;
  ofxPoly2Tri  triangulator;
  ofPolyline bounds;
  ofMesh  polypoints;
  vector<ofPoint> steinerPoints;

  ofxCv::FlowFarneback flow;
  ofMesh mesh;
  int stepSize, xSteps, ySteps;
};