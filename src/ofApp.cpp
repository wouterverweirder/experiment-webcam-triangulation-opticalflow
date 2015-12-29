#include "ofApp.h"

using namespace ofxCv;
using namespace cv;

void ofApp::setup() {
	videoWidth = 320;
	videoHeight = 240;
	ofSetVerticalSync(true);
	ofSetFrameRate(60);
	videoGrabber.setup(videoWidth, videoHeight);

	//CREATE A RECTANGLE with ofPolyLine with multiple points on each side
	int numberofverticesperside = 8;
	bounds.addVertex(ofPoint(0, 0));
	bounds.bezierTo(ofPoint(0, videoHeight / 3), ofPoint(0, videoHeight - (videoHeight / 3)), ofPoint(0, videoHeight), numberofverticesperside);
	bounds.bezierTo(ofPoint(videoWidth / 3, videoHeight), ofPoint(videoWidth - (videoWidth / 3), videoHeight), ofPoint(videoWidth, videoHeight), numberofverticesperside);
	bounds.bezierTo(ofPoint(videoWidth, videoHeight - (ofGetHeight() / 3)), ofPoint(videoWidth, videoHeight / 3), ofPoint(videoWidth, 0), numberofverticesperside);
	bounds.bezierTo(ofPoint(videoWidth - (videoWidth / 3), 0), ofPoint(videoWidth / 3, 0), ofPoint(0, 0), numberofverticesperside);

	//COPY THE VERTICES form ofPolyLine into ofMesh
	//We dot this te easily remove double vertices created by the bezierTo
	for (int i = 0; i<bounds.size(); ++i) {
		polypoints.addVertex(ofVec3f(bounds[i].x, bounds[i].y, 0));
	}
	polypoints.setMode(OF_PRIMITIVE_POINTS);
    
	//REMOVE DOUBLE VERTICES
	//POLY2TRI DOES NOT LIKE THAT
	polypoints.removeVertex(0);
	polypoints.removeVertex(numberofverticesperside - 1);
	polypoints.removeVertex((numberofverticesperside - 1) * 2);
	polypoints.removeVertex((numberofverticesperside - 1) * 3);
	polypoints.removeVertex((numberofverticesperside - 1) * 4);

    //generate random steinerpoints to generate a bunch of triangles
    int numSteinerPoints = 400;
	for (int i = 0; i<numSteinerPoints; ++i) {
		ofPoint newpoint;
		newpoint.x = ofRandom(videoWidth);
		newpoint.y = ofRandom(videoHeight);
		steinerPoints.push_back(newpoint);
	}

	triangulator.setSteinerPoints(steinerPoints);
}

void ofApp::update() {
	videoGrabber.update();
	if(videoGrabber.isFrameNew()) {
		flow.setWindowSize(8);
		flow.calcOpticalFlow(videoGrabber);

		//set the vertex colors
		int red = 0, green = 0, blue = 0;
		int radius = 3;
		int numSamples = 0;
		ofPixels videoPixels = videoGrabber.getPixels();
		ofColor clr;
		
		//execute triangulation
		triangulator.triangulate(polypoints);

        //map which will store the translations per vertex x,y position
        //we have duplicate vertices, we need to make sure they share the translations
        //this map is a lookup map for those translations
		std::unordered_map<float, std::unordered_map<float, ofVec3f>> vertexTranslations;
        
        float stepSize = 8.0;
        float distortionStrength = 4.0;

		//update the vertex positions
		for (int i = 0; i < triangulator.triangulatedMesh.getNumVertices(); i++) {
			ofVec3f vertex = triangulator.triangulatedMesh.getVertex(i);
            ofVec3f translation = vertex;
			//only calculate a diff on unique vertices
			if (vertexTranslations.count(vertex.x) == 0){
                
                ofRectangle area(vertex - ofVec2f(stepSize, stepSize) / 2, stepSize, stepSize);
                if(area.x > 0 && area.x + stepSize < videoWidth && area.y > 0 && area.y + stepSize < videoHeight) {
                    translation = flow.getAverageFlowInRegion(area);
                    translation *= distortionStrength;
                }
                
				vertexTranslations[vertex.x] = { { vertex.y, translation } };
			}
			translation = vertexTranslations[vertex.x][vertex.y];
			vertex = vertex + translation;
			triangulator.triangulatedMesh.setVertex(i, vertex);
		}

		//set the colors
   		triangulator.triangulatedMesh.clearColors();
		for (int i = 0; i < triangulator.triangulatedMesh.getNumVertices() / 3; i += 1) {

			ofVec3f p1 = triangulator.triangulatedMesh.getVertex(i*3);
			ofVec3f p2 = triangulator.triangulatedMesh.getVertex(i * 3 + 1);
			ofVec3f p3 = triangulator.triangulatedMesh.getVertex(i * 3 + 2);

			ofVec3f triangleCenter = (p1 + p2 + p3) / 3.0;

			triangleCenter.x = floor(ofClamp(triangleCenter.x, 0, videoWidth));
			triangleCenter.y = floor(ofClamp(triangleCenter.y, 0, videoHeight));

			//calculate average color
			red = 0;
			green = 0;
			blue = 0;
			numSamples = 0;
			for (int x = triangleCenter.x - radius; x < triangleCenter.x + radius; x++) {
                if (x <= 0 || x >= videoWidth) {
					continue;
				}
				for (int y = triangleCenter.y - radius; y < triangleCenter.y + radius; y++) {
					if (y <= 0 || y >= videoHeight) {
						continue;
					}
					clr = videoPixels.getColor(x, y);
					red += clr.r;
					green += clr.g;
					blue += clr.b;
					numSamples++;
				}
			}

			numSamples = max(1, numSamples);

			ofColor c = ofColor(red / numSamples, green / numSamples, blue / numSamples);

			triangulator.triangulatedMesh.addColor(c);
			triangulator.triangulatedMesh.addColor(c);
			triangulator.triangulatedMesh.addColor(c);
		}
	}
}

void ofApp::draw() {
	ofBackground(0);

    float scale = 1.0f * ofGetWidth() / videoWidth;
    float vScale = 1.0f * ofGetHeight() / videoHeight;
    if(vScale < scale) {
        scale = vScale;
    }
    ofPushMatrix();
    ofScale(scale, scale);
	triangulator.triangulatedMesh.drawFaces();
    ofPopMatrix();
    
    ofDrawBitmapString(ofGetFrameRate(), 10, 10);
}