#pragma once

#include "ofMain.h"

#include <stdio.h>
#include <math.h>

#include <vector>
#include <map>
#include <cstdlib>


#include "ofxGui.h"



#include "dustParticle.h"
#include "dirtParticle.h"
#include "communicator.h"

#include "cameraipc.h"

enum ToolStyle { none = 0, knife, brush, dropper };
enum GameStage { START = 0, TUTORIAL, STARTTEXT, PROCESS, STEPEND, END };
enum RelicType { CAITAO = 0, HUASHI };


struct RelicPackage {//for each relic package's files management
	string imageFolder;
	int imageNum;
	//ofImage outlineImage;//160*120 pixels  the file should be named as imageFolder_mask.jpg
	//map<string,ToolStyle> outlinePair;
	RelicPackage(string a, int b) { imageFolder = a; imageNum = b; }
};


//______________________________CLASSES_____________________________________

class Tool {
public:
	ToolStyle ID;
	ofImage icon_on;
	ofImage icon_off;
	ofImage icon_err;
	Tool() { ID = none; }
	void setToolStyle(int num) { ID = (ToolStyle)num; }
};

class button {
public:
	bool state = false;
	int x, y;
	int w, h;
	ofImage icon;
	string name;
	ofPoint toucharea;
	void setposition(int x1, int y1, int w1, int h1) { x = x1; y = y1; w = w1; h = h1; }
	void setposition(int x1, int y1) { x = x1; y = y1; }
	void draw() {icon.draw(x, y, w, h); }
};

class Process {
public:
	int TotalStepsNum;
	int TotalImgsNum;
	string imgdirectory;
	int currentStep;
	//ofImage temp;
	vector<ofImage> ProcessImages;
	vector<ofImage> OutlineImages;
	vector<ToolStyle> Toollist;

	void setup(int stepsnum, string imgfolder, int * toollist);
};

class Widgets {//the widgets drawn on the top layer
public:
	ofImage instruction;
	ofImage tips;
	ofImage finishpercent;
	ofImage teachingtext;
	ofImage review[3];//0 for record; 1 for  profession ; 2 for manipulation

	//ofImage health;
	//ofImage toolpara;
	ofTrueTypeFont  font;
	float thres1;//for knife
	float thres2;//for brush
	float workingPercent = 1.0;//working left percent
	float healthPercent = 1.0;
	float recordPercent = 1.0;
	float manipPercent = 1.0;
	bool  bMidtext = false;  //when need to draw mid-process text, make it true

	//float toolparaPercent = 0.0;
	// int healthBarWidth = 700;
	 int workingBarWidth = 500;
	 int workingBarHeight = 30;
	 int scoreBarWidth = 100;
	 int scoreBarHeight = 30;
	// int toolparaBarWidth = 260;
	ToolStyle currentToolStyle;
	void setup();
	void reset() {
		 workingPercent = 1.0;//working left percent
		 healthPercent = 1.0;
		 //toolparaPercent = 0.0;

	}
	void setscore(float a, float b, float c) {
		recordPercent = a;
		healthPercent = b;
		manipPercent = c;
	}
	void update(Process cai, GameStage status);
	void setThres(float x, float y) { thres1 = x; thres2 = y; }
	void draw(GameStage status);
};


//====================================BASEAPP========================================
class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();
		
		void resetTimer();

		void resetParticles();
		void drawParticles();
		//void brushParticleEffects();
		//void scrapeParticleEffects();
		void particleEffects();
		void emitParticles();
		

		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		void exit();


		//Kilo start from here

		/*For Arduino Serial Communication*/
		
		//string str;
		ToolStyle ToolNow;
		int IRimage_w = 160;
		int IRimage_h = 120;

		/*************     For Motion Detection      *************/
		ofxCvGrayscaleImage diff;		//Absolute difference of the frames
		ofxCvFloatImage diffFloat;		//Amplified difference images
		ofxCvFloatImage bufferFloat;	//Buffer image
		ofxCvGrayscaleImage binaryMotion; //binary mask of motion
		void IRtoMotion(ofxCvGrayscaleImage IR, ofxCvGrayscaleImage IRprev);
		bool Moved;
		void MotionDraw();
		bool newMotion;

		//GUI  for motion detecting experiments
		ofxPanel Experiment;
		ofxFloatSlider Amplify;
		ofxFloatSlider Damping;
		ofxIntSlider Threshold;
		ofxIntSlider AdaptiveThreshold;
		ofxToggle Adap;
		ofxIntSlider outlineth;
		ofxIntSlider IRthreshold;
		//ofxIntSlider IRfanwei;
		
		/****************************       For mask shader         *******************/
		ofShader shaderMask;
		ofShader shaderMotion;
		ofImage screenshot;
		ofImage backgroundImage;
		ofImage foregroundImage;
		ofImage bF;
		ofxCvGrayscaleImage outlineImage;     // outlineImage is a const image for each map/stage
		ofxCvGrayscaleImage outlineImageTBD;// this one is used in shader to detect whether 
		//ofImage brushImage; //these three haven't been used
		//ofImage knifeImage;
		//ofImage waterImage;
		ofFbo maskFbo;
		ofFbo existFbo;
		ofFbo fbo;
		void  maskShaderUpdate();
		float times = 6;
		float shiftx = 160;
		float shifty = 0;
		/*****************               PREPARE FOR PARTICLE SYS                       *******************/
		ofxCvContourFinder contourFinder;
		ofPoint GetMotionCenter();

		/**************************************   Tool Switch   *******************************************/
		Tool Dropper, Knife, Brush;
		ofPoint TSPosition;
		void ToolSwitchSetup();
		void ToolSwitchUpdate();
		void ToolSwitchDraw();

		/***************************************   Game Logic   ********************************************/
		
		bool firsttimehere = true;
		bool firsttimeend = false;
		bool bRecording = false;
		//int GameStage;
		GameStage stage;
		RelicType relic;
		Process caitao;

		ofSoundPlayer shali;
		ofSoundPlayer saotu;
		ofSoundPlayer kouxue;
		//ofSoundPlayer baojing;
		//ofSoundPlayer daojishi;
		ofSoundPlayer gamelogicsound;
		

		//START
		ofImage startbackground;
		//TUTORIAL
		ofImage tutorialbackground;
		//PROCESS
		Widgets caitaoWidgets;

		//END
		ofImage endbackground;

		//GAMEOVER
		ofImage gameoverbackground;
		ofTrueTypeFont  appfont;
		/************************************   Game Data    ***************************************/
		void resetGameData();
		const float healthTotal = 5000;
		float healthLeft=5000;
		int workingTotal[4] = { 4151,4151,1449,4151 };
		int workingLeft;
		float recordTotal = 6; //if a user record the picture for more than 6 times, he will get full score in the final recording judgement. 2/4/6 bad/medium/good
		float recordtimes = 0;

		float forceTotal1= 30;
		float forceTotal2 = 100;
		float currentForce=0;
		float safeThres1 = 15;
		float safeThres2 = 50;
		void setForce(float total1, float thres1, float total2, float thres2) {
			forceTotal1 = total1;
			forceTotal2 = total2;
			safeThres1 = thres1;
			safeThres2 = thres2;
		}
		float timeLimit = 120.0; //120 seconds
		float errTooltime=0;
		float tempbegin=0;

		/********************************** time keeper ******************************************/
		float tutoLimit = 6000;//2seconds 2000millisecs
		float tutostarttime = 0;
		float tutotimer = 0;

		float stependLimit = 4000;//2seconds 2000millisecs
		float stependstarttime =0;
		float stependtimer =0;
		
		float starttextLimit = 6000;//2seconds 2000millisecs
		float starttextstarttime = 0;
		float starttexttimer = 0;

		float recordLimit = 1000;//2seconds 2000millisecs
		float recordstarttime = 0;
		float recordtimer = 0;

		/****************************************    Button    ********************************************/

		//button ButtonCaitao;
		//button ButtonHuashi;
		button ButtonStart;
		button ButtonContinue;
		button ButtonRecord;
		button ButtonRestartpro;
		button ButtonRestartend;
		//button ButtonRestartgameover;
		//button ButtonHelp;
		//void getSwitchStage();
		void ButtonSetup();
		bool getButtonState(button bu);

		//void RelicPackageSetup();
		
		
		//Brian start from here

		
};
