#include "ofApp.h"




int deferfps = 0;

// function prototypes:

//caitao information
int caitao_toollist[4] = { 1,1,2,3 };



//ofxCvGrayscaleImage IRimagePrevQueue[5];

ofxCvColorImage outloadImage;






//====================Brian==============
vector <dustParticle> dusts;
vector <dirtParticle> dirts;

int dustIndex;
int dirtIndex;

communicator com;

float startTime;
float timer;

bool brushDown;
bool scrapeDown;

bool emitP;
bool found;

bool disableForce;

ofPoint mouse;
ofPoint *mPos;

std::string TCHAR2STRING(TCHAR *STR)

{

	int iLen = WideCharToMultiByte(CP_ACP, 0, STR, -1, NULL, 0, NULL, NULL);

	char* chRtn = new char[iLen * sizeof(char)];

	WideCharToMultiByte(CP_ACP, 0, STR, -1, chRtn, iLen, NULL, NULL);

	std::string str(chRtn);

	return str;

}




//--------------------------------------------------------------
void ofApp::setup() {
	ofSetVerticalSync(true);

	/*********************game data setup**********************/

	caitao.setup(4, "caitao", caitao_toollist);

	setForce(20, 14, 28, 24);//Brian!! fill in these 4 parameters. The first one is the maximum force magnitude you may get from the gage sensor on the knife, the second is the safe threshold for the knife and the other two are for the brush.

	//knife sensor max force:208
	//knife safe threshold: 204
	//brush sensor max force: 
	//brush safe threshold:

	/****************widgets setup******************/
	// should be after the data setup
	caitaoWidgets.setup();
	caitaoWidgets.setThres(safeThres1 / forceTotal1, safeThres2 / forceTotal2);
	ToolSwitchSetup();


	/***********************     Button Setup    ****************************/
	ButtonSetup();

	/**********************For Motion Experiment gui**********************/
	Experiment.setup("Parameters", "settings.xml");
	Experiment.add(Amplify.setup("Amplify", 5.0, 1.0, 10.0));
	Experiment.add(Damping.setup("Damping", 0.7, 0.0, 1.0));
	Experiment.add(Threshold.setup("Threshold", 15, 0, 255));
	Experiment.add(Adap.setup("Adaptive", false));
	Experiment.add(AdaptiveThreshold.setup("AdaptiveThreshold", 30, 0, 255));
	//Experiment.add(outlineth.setup("outlineThreshold", 128, 0, 255));
	//Experiment.add(IRthreshold.setup("IRthreshold", 90, 0, 255));
//	Experiment.add(IRfanwei.setup("IRfanwei", 25, 0, 50));
	//************   For Mask Shader   ***********/
#ifdef TARGET_OPENGLES
	shaderMask.load("shadersES2/shader");
#else
	if (ofIsGLProgrammableRenderer()) {
		shaderMask.load("shadersGL3/shaderMask");
		shaderMotion.load("shadersGL3/shaderMotion");
	}
	else {
		shaderMask.load("shadersGL2/shaderMask");
		shaderMotion.load("shadersGL2/shaderMotion");
	}
#endif
	shiftx = 160;
	shifty = 0;
	times = 6;
	IR_mtx.lock();
	IRimagePrev.allocate(160, 120);
	IRimage.allocate(160, 120);
	IR_mtx.unlock();
	diff.allocate(160, 120);
	//diffFloat.allocate(160, 120);
	//binaryMotion.allocate(160, 120);
	/******************          image loading           *****************/
	startbackground.loadImage("interface/startbackground.png");//should be startbackground.png
	endbackground.loadImage("interface/endbackground.png");
	gameoverbackground.loadImage("interface/gameoverbackground.png");

	backgroundImage.loadImage("caitao/1.jpg");
	foregroundImage.loadImage("caitao/0.jpg");
	appfont.load("HYQuHeiW 2.ttf", 30);


	/******************************      SOUND   ************************************/
	shali.load("audio/shali.mp3");
	shali.setVolume(0.7);
	saotu.load("audio/saotu.mp3");
	saotu.setVolume(0.65);

	kouxue.load("audio/kouxue.mp3");






	shali.setMultiPlay(true);
	saotu.setMultiPlay(true);




	//pixelBuffer.allocate(FrameWidth, FrameHeight, OF_IMAGE_GRAYSCALE);
	for (int i = 0; i < FrameSize; i++) { pixelBuffer[i] = 0; }
	int width = ofGetWidth();
	int height = ofGetHeight();
	existFbo.allocate(IRimage_w, IRimage_h);
	maskFbo.allocate(width, height);
	fbo.allocate(width, height);

	// Clear the FBO's
	// otherwise it will bring some junk with it from the memory
	existFbo.begin();
	ofClear(0, 0, 0, 255);
	existFbo.end();
	maskFbo.begin();
	ofClear(0, 0, 0, 255);
	maskFbo.end();

	fbo.begin();
	ofClear(0, 0, 0, 0);
	fbo.end();

	//Init(160, 120, 2);//for ipc frame  w,h,depth

	/*For IPC Connection*/
	//SetImagerIPCCount(1);
	InitIPC();   //这个里面有 init和run
	SetIPCMode(0, 1);
	

	//deferfps = 0;
	//ofEnableAlphaBlending();

	//gamelogic setup
	stage = START;


	//============Brian==================
	//setup particles
	int numDust = 750;
	int numDirt = 750;

	dusts.assign(numDust, dustParticle());
	dirts.assign(numDirt, dirtParticle());
	dustIndex = 0;
	dirtIndex = 0;

	brushDown = false;
	scrapeDown = false;
	emitP = false;

	resetParticles();
	resetTimer();

	mouse = ofPoint(200, 200);
	mPos = &mouse;


	//setup comms
	com.setup(9600, "COM3");
	com.reset();



}

//--------------------------------------------------------------
void ofApp::update() {
	if (IR_mtx.try_lock()) {
		IRtoMotion(IRimage, IRimagePrev);
		IR_mtx.unlock();
	}

	//if (binaryMotion.bAllocated) {
	//	maskShaderUpdate();
	//}

	switch (stage) {
	case START: {

		//getSwitchStage();
		if (getButtonState(ButtonStart)) {

			//----------------------------------------
			stage = TUTORIAL;
			resetGameData();
			gamelogicsound.load("audio/start.mp3");
			gamelogicsound.setVolume(0.7);
			gamelogicsound.play();
		}
		break;
	}
	case TUTORIAL: {
		if (getButtonState(ButtonContinue)) {
			stage = PROCESS;
		}
	}
	case PROCESS: {
		if (getButtonState(ButtonRestartpro)) {
			stage = START;
		}
		if (processing_status == starttext) {

			//insert the starttext part here!!

		}
		else if (processing_status == processing) {

			//printf("first time process update\n");
			if (firsttimehere) { //update the step data
				workingLeft = workingTotal[caitao.currentStep];
				caitaoWidgets.update(caitao, stepend);
				backgroundImage = caitao.ProcessImages[caitao.currentStep + 1];
				foregroundImage = caitao.ProcessImages[caitao.currentStep];

				for (int i = 0; i < FrameSize; i++) { pixelBuffer[i] = 0; }//clean the pixel Buffer for recording a new step motion
				firsttimehere = false;
			}




			ToolSwitchUpdate();//Brian part

			ToolNow = caitao.Toollist[caitao.currentStep];
			if (ToolNow == caitao.Toollist[caitao.currentStep]) { //If user is not using the right tool,then nothing updates.
				maskShaderUpdate();//workingleft is calculated here.
			}
			else {
				currentForce = 0;
				if(ToolNow!= none)
					errTooltime++; 
			}

			if (caitao.Toollist[caitao.currentStep] == knife) {
				
				caitaoWidgets.toolparaPercent = ofMap(currentForce, 0.0, forceTotal1, 0.0, 1.0, true);
				if (Moved && currentForce > safeThres1) {
					healthLeft = healthLeft - 5; 
				}//如果力大于thres1 用刀 并且 motion有数
			}
			else if (caitao.Toollist[caitao.currentStep] == brush) {
				
				caitaoWidgets.toolparaPercent = ofMap(currentForce, 0.0, forceTotal2, 0.0, 1.0, true);
				if (Moved  && currentForce > safeThres2) {
					healthLeft = healthLeft - 5;
				}//如果力大于thres2 用刷 并且 motion有数
			}
			else {
				//if the tool is dropper , no health deduction wil be done.
			}
			

		caitaoWidgets.healthPercent = ofMap(healthLeft, 0.0, healthTotal, 0.0, 1.0, true);
		caitaoWidgets.workingPercent = ofMap((float)workingLeft, 0.0, (float)workingTotal[caitao.currentStep], 0.0, 1.0, true);


		//screenshot.grabScreen(0, 0, ofGetWidth(), ofGetHeight());
		if (caitaoWidgets.workingPercent < 0.1 && caitao.Toollist[caitao.currentStep] == knife) {
			stepend = true;
			firsttimeend = true;
			//printf("%f\n", caitaoWidgets.workingPercent);
		}
		else if (caitaoWidgets.workingPercent < 0.05 && caitao.Toollist[caitao.currentStep] == brush) {
			stepend = true;
			firsttimeend = true;
		}
		else if (caitaoWidgets.workingPercent < 0.01 && caitao.Toollist[caitao.currentStep] == dropper) {
			stepend = true;
			firsttimeend = true;
		}


	}
	else {


		if (firsttimeend) {
			gamelogicsound.load("audio/stepend.mp3");
			gamelogicsound.setVolume(0.7);
			gamelogicsound.play();
			caitaoWidgets.update(caitao, stepend);
			changingstarttime = ofGetElapsedTimeMillis();
			firsttimeend = false;
			//printf("tukuaiend!\n");
		}
		changingtimer = ofGetElapsedTimeMillis();
		if (changingtimer - changingstarttime > changingtimeLimit) {
			caitao.currentStep++;
			firsttimehere = true;
			stepend = false;
			if (caitao.currentStep > 4) {
				gamelogicsound.load("audio/success.mp3");
				gamelogicsound.setVolume(0.7);
				gamelogicsound.play();
				stage = END;
			}
		}

	}

	//printf("Moved = %i \n", Moved);
	emitP = Moved;
	if (Moved) {
		//ofPoint temp = GetMotionCenter();
		mouse = GetMotionCenter();
		//printf("x = %f y = %f", temp.x, temp.y);
	}

	break;
	}
	case END: {

		if (getButtonState(ButtonRestartend)) {
			stage = START;
		}
		break;
	}

	default:
		printf("something wrong ...stage!=any of the game stages\n"); break;
}




//===================Brian=============================
timer = ofGetElapsedTimeMillis() - startTime;


particleEffects();
// set emitParticles to true when dirt is being removed to allow particles to generate

com.update();







}

//--------------------------------------------------------------
void ofApp::draw() {

	ofBackground(0, 0, 0); //Set up white background
	ofSetColor(255, 255, 255); //Set color for image drawing



	switch (stage) {
	case START: {
		startbackground.draw(0, 0, 1280, 800);
		ButtonStart.draw();
		break;
	}
	case TUTORIAL: {
		tutorialbackground.draw(0, 0, 1280, 800);
		ButtonContinue.draw();
		break;
	}
	case PROCESS: {
		// FIRST draw the background image
		//printf("first time process draw\n");
		foregroundImage.draw(0, 0);
		// THEN draw the masked fbo on top
		fbo.draw(0, 0);
		caitaoWidgets.draw();
		ToolSwitchDraw();
		ButtonRestartpro.draw();
		
		if (ToolNow != caitao.Toollist[caitao.currentStep] && ToolNow != none) {
			string wrongstr = "Wrong Tool!";
			appfont.drawString(wrongstr, 1280 / 2 - 50, 415);
		}

		break;
	}
	case END: {

		endbackground.draw(0, 0, 1280, 800);
		ButtonRestartend.draw();

		string scorestr;
		scorestr = ofToString((int)round(caitaoWidgets.healthPercent * 100));
		scorestr += "%";
		//appfont.setSpaceSize(100);
		appfont.drawString(scorestr, 1280 / 2 - 30, 200);

		break;
	}

	default:
		printf("something wrong ...stage!=any of the game stages\n");
	}



	//----------------------------------------------------------
	//Here is For Test 

	//// FIRST draw the background image
	//foregroundImage.draw(0, 0);
	//// THEN draw the masked fbo on top
	//fbo.draw(0, 0);

	//MotionDraw();
	//existFbo.draw(3*IRimage_w,  3*IRimage_h);
	//Experiment.draw();

	//IRimage.draw(1600-320, 900-240,FrameWidth*2,FrameHeight*2); //Draw image
	//ofDrawBitmapString(TCHAR2STRING(labelFrameCounter), 100, 800);
	//printf("drawing\n");

	//ofBackgroundGradient(ofColor(60, 60, 60), ofColor(10, 10, 10));





	//=================Brian======================
	//drawParticles();



	//ofSetColor(190);
	//LASTLY draw the particle effects
	drawParticles();
}

void ofApp::exit() {
	Experiment.saveToFile("settings.xml");
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {
	//brushDown = true;
	//scrapeDown = true;

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
	//brushDown = false;
	//scrapeDown = false;

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {

}




void ofApp::ButtonSetup()
{

	ButtonStart.icon.loadImage("interface/start.png");
	ButtonStart.setposition(ofGetWindowWidth() / 2 - ButtonStart.icon.getWidth() / 2, ofGetWindowHeight() - 80 - ButtonStart.icon.getHeight(), ButtonStart.icon.getWidth(), ButtonStart.icon.getHeight());
	ButtonStart.name = "Start";
	ButtonStart.toucharea.set((ButtonStart.x + ButtonStart.w / 2 - shiftx) / 6, (ButtonStart.y + ButtonStart.h / 2 - shifty) / 6);
	
	ButtonContinue.icon.loadImage("interface/Continue.png");
	ButtonContinue.setposition(ofGetWindowWidth() / 2 - ButtonContinue.icon.getWidth() / 2, ofGetWindowHeight() - 80 - ButtonContinue.icon.getHeight(), ButtonContinue.icon.getWidth(), ButtonContinue.icon.getHeight());
	ButtonContinue.name = "Continue";
	ButtonContinue.toucharea.set((ButtonContinue.x + ButtonContinue.w / 2 - shiftx) / 6, (ButtonContinue.y + ButtonContinue.h / 2 - shifty) / 6);

	ButtonRestartpro.icon.loadImage("interface/restart.png");
	ButtonRestartpro.setposition(shiftx + 10, shifty + 30, ButtonRestartpro.icon.getWidth() + 30, ButtonRestartpro.icon.getHeight() + 30);
	ButtonRestartpro.name = "Restart";
	ButtonRestartpro.toucharea.set((ButtonRestartpro.x + ButtonRestartpro.w / 2 - shiftx) / 6, (ButtonRestartpro.y + ButtonRestartpro.h / 2 - shifty) / 6);
	//	ButtonHelp.setposition(ofGetWindowSize().x / 2, ofGetWindowSize().y / 2, 200, 100);
	//	ButtonHelp.icon.loadImage("interface/help.png");
	//	ButtonHelp.name = "Help";
	ButtonRestartend.icon.loadImage("interface/restartgameover.png");
	ButtonRestartend.setposition(ofGetWindowWidth() / 2 - ButtonRestartend.icon.getWidth() / 2, 578 - ButtonRestartend.icon.getHeight() / 2, ButtonRestartend.icon.getWidth(), ButtonRestartend.icon.getHeight());
	ButtonRestartend.name = "Restart";
	ButtonRestartend.toucharea.set((ButtonRestartend.x + ButtonRestartend.w / 2 - shiftx) / 6, (ButtonRestartend.y + ButtonRestartend.h / 2 - shifty) / 6);
	ButtonRecord.icon.loadImage("interface/record.png");
	ButtonRecord.setposition(shiftx + 800, shifty + 30, ButtonRecord.icon.getWidth() + 30, ButtonRecord.icon.getHeight() + 30);
	ButtonRecord.name = "Record";
	ButtonRecord.toucharea.set((ButtonRecord.x + ButtonRecord.w / 2 - shiftx) / 6, (ButtonRecord.y + ButtonRecord.h / 2 - shifty) / 6);
}
	
	
bool ofApp::getButtonState(button bu) {

	if (Moved) {
		ofPoint touch = GetMotionCenter();
		if (touch.x > bu.x&&touch.x<bu.x + bu.w&&touch.y>bu.y&&touch.y < bu.y + bu.h) {
			return true;
		}
		else {
			return false;
		}
	}
	else {
		return false;
	}
	//if (binaryMotion.bAllocated) {
	//	ofPixels temp = binaryMotion.getPixels();

	//	//unsigned char value;
	//	int pixNo = roundf(bu.toucharea.x + bu.toucharea.y * 160);
	//	if (pixNo > FrameSize - 1) { printf("Array Bounds Write! Error!\n"); return false; }//
	//	if (temp[pixNo] > 240) { return true; }
	//	else { return false; }
	//}
	//else
	//{
	//	return false;
	//}

}

ofPoint ofApp::GetMotionCenter()
{

	float tempmax = 0;
	int tempi = 0;
	for (int i = 0; i < contourFinder.nBlobs; i++) {
		if (tempmax < contourFinder.blobs.at(i).area) {
			tempmax = contourFinder.blobs.at(i).area;
			tempi = i;
		}
	}
	ofPoint a = ofPoint(contourFinder.blobs.at(tempi).centroid.x * 6 + 160, contourFinder.blobs.at(tempi).centroid.y * 6);
	return a;
}

void ofApp::ToolSwitchSetup()
{
	Knife.icon_off.loadImage("interface/chan_off.png");
	Knife.icon_on.loadImage("interface/chan_on.png");
	Knife.ID = knife;
	Brush.icon_off.loadImage("interface/brush_off.png");
	Brush.icon_on.loadImage("interface/brush_on.png");
	Brush.ID = brush;
	Dropper.icon_off.loadImage("interface/dropper_off.png");
	Dropper.icon_on.loadImage("interface/dropper_on.png");
	Dropper.ID = dropper;
	TSPosition.set(1180, 268);

}

void ofApp::ToolSwitchUpdate()
{
	//ToolNow = ??
	//currentForce=??
	//For Brian
	//printf("%c", com.whatTool());
	if (disableForce) {
		disableForce = false;
	}

	switch (com.whatTool()) {
	case 'b':
		ToolNow = brush;
		break;
	case 'k':
		ToolNow = knife;
		break;
	case 'p':
		ToolNow = dropper;
		break;
	case '0': {
		ToolNow = none;
		disableForce = true;
		break;
	}
	default: {
		disableForce = true;
		ToolNow = none;
	}
	}

	//get force
	if (disableForce) {
		currentForce = 0;
	}
	else if (ToolNow == brush) {
		// max force = 400
		// normal force = 374
		// diff = 20
		currentForce = abs(394 - com.getBrushForce());
	}
	else if (ToolNow == knife) {

		// max force 236
		// normal force 228
		// diff = 8
		currentForce = abs(232 - com.getKnifeForce());
	}
	else {

		currentForce = 0;
	}



}

void ofApp::ToolSwitchDraw()
{
	ofSetColor(255, 255, 255);
	switch (ToolNow) {
	case none: {
		Knife.icon_off.draw(TSPosition);
		Brush.icon_off.draw(TSPosition.x, TSPosition.y + Knife.icon_off.getHeight());
		Dropper.icon_off.draw(TSPosition.x, TSPosition.y + Knife.icon_off.getHeight() * 2);
		break;
	}
	case knife: {
		Knife.icon_on.draw(TSPosition);
		Brush.icon_off.draw(TSPosition.x, TSPosition.y + Knife.icon_off.getHeight());
		Dropper.icon_off.draw(TSPosition.x, TSPosition.y + Knife.icon_off.getHeight() * 2);
		break;
	}
	case brush: {
		Knife.icon_off.draw(TSPosition);
		Brush.icon_on.draw(TSPosition.x, TSPosition.y + Knife.icon_off.getHeight());
		Dropper.icon_off.draw(TSPosition.x, TSPosition.y + Knife.icon_off.getHeight() * 2);
		break;
	}
	case dropper: {
		Knife.icon_off.draw(TSPosition);
		Brush.icon_off.draw(TSPosition.x, TSPosition.y + Knife.icon_off.getHeight());
		Dropper.icon_on.draw(TSPosition.x, TSPosition.y + Knife.icon_off.getHeight() * 2);
		break;
	}
	}
}

void ofApp::resetGameData()
{
	stepend = false;
	firsttimehere = true;
	firsttimeend = false;
	caitao.currentStep = 0;
	healthLeft = healthTotal;
	workingLeft = workingTotal[0];

	existFbo.begin();
	ofClear(0, 0, 0, 255);
	existFbo.end();
	maskFbo.begin();
	ofClear(0, 0, 0, 255);
	maskFbo.end();

	fbo.begin();
	ofClear(0, 0, 0, 0);
	fbo.end();
}

void ofApp::IRtoMotion(ofxCvGrayscaleImage IR, ofxCvGrayscaleImage IRprev)
{
	/*
	binaryMotion = IRimage;
	binaryMotion.threshold(Threshold); //we need to experiment
	for (int i = 0; i < FrameSize; i++) {
		if (binaryMotion.getPixels()[i] > 128) { Moved = true; break; }
		Moved = false;
	}
	newIR = false;
	newMotion = true;
	*/
	//printf("%d  ---  %d\n", IRimage.getPixels()[17680],IRimagePrev.getPixels()[17680]);
	if (IRprev.bAllocated) {
		//for (int i = 0; i < FrameSize; i++) {
		//	pixels[i] = (unsigned char)clip((int)(IRimage.getPixels()[i] - IRimagePrev.getPixels()[i]));
		//}
		binaryMotion.absDiff(IR, IRprev);
		binaryMotion.threshold(Threshold);
		contourFinder.findContours(binaryMotion, 3, (160 * 120) / 4, 4, false, true);
		if (contourFinder.nBlobs > 0) {
			Moved = true;
		}
		else {
			Moved = false;
		}

	}
	else { printf("IRimagePrev does not exist\n"); }
	//printf("%d\n", binaryMotion.getPixels()[17680]);



	/*
	if (newIR) {
		//Store the previous frame, if it exists till now


		//Do processing if grayImagePrev is inited
		if (IRimagePrev.bAllocated) {
			//Get absolute difference
			diff.absDiff(IRimage, IRimagePrev);

			//We want to amplify the difference to obtain
			//better visibility of motion
			//We do it by multiplication. But to do it, we
			//need to convert diff to float image first
			diffFloat = diff;	//Convert to float image
			diffFloat *= Amplify;	//Amplify the pixel values

									//Update the accumulation buffer
			if (!bufferFloat.bAllocated) {
				//If the buffer is not initialized, then
				//just set it equal to diffFloat
				bufferFloat = diffFloat;
			}
			else {
				//Slow damping the buffer to zero
				bufferFloat *= Damping;
				//Add current difference image to the buffer
				bufferFloat += diffFloat;
			}
			//get binary image
			binaryMotion = bufferFloat;


			if (Adap) {
				binaryMotion.adaptiveThreshold(AdaptiveThreshold);
			}
			else {
				binaryMotion.threshold(Threshold); //we need to experiment
			}

			for (int i = 0; i < FrameSize; i++) {
				if (binaryMotion.getPixels()[i] > 128) { Moved = true; break; }
				Moved = false;
			}
			newIR = false;
			newMotion = true;
		}
	}
	*/
	//return binaryMotion;
}

void ofApp::maskShaderUpdate()
{
	float times = 6; //finally we make the touch area 960*720, 6 times to IRimage 
					 //here the drawing parameters decide the camera view on the whole screen

					 //------------------------------------------------------------------------
					 // we need to accumulate the motion areas .here we store all the motions into existFbo
//	ofxCvGrayscaleImage abc;
	/*
	abc = IRimage;
	abc.threshold(IRthreshold);

	*/
	//	abc = binaryMotion;
	ofPixels ab;
	//ab.allocate(160, 120,OF_IMAGE_GRAYSCALE);
	ab = binaryMotion.getPixels();


	//unsigned char value;
	int counter = 0;
	//printf("%d\n", caitao.OutlineImages[caitao.currentStep].getPixels()[2]);
	//printf("%d\n", caitao.OutlineImages[caitao.currentStep].getPixels()[9600]);
	for (int i = 0; i < FrameSize; i++) {
		if (pixelBuffer[i] < ab[i]) {
			//printf("ab[i]  %d\n",ab[i]);
			pixelBuffer[i] = ab[i];
			if (caitao.OutlineImages[caitao.currentStep].getPixels()[i] > 0)
			{
				workingLeft--;

			}
		}
	}

	ofxCvGrayscaleImage pBimage;
	pBimage.allocate(160, 120);
	pBimage.setFromPixels(pixelBuffer, FrameWidth, FrameHeight);
	ofSetColor(255);
	existFbo.begin();
	pBimage.draw(0, 0);
	existFbo.end();
	//----------------------------------------------------------
	//then draw 160-120 size existFbo expand it "times" times and put it on the middletop of maskFbo
	maskFbo.begin();
	//ofClear(0, 0, 0, 0);
	existFbo.draw(160, 0, IRimage_w * times, IRimage_h * times);
	maskFbo.end();
	//----------------------------------------------------------
	// HERE the shader-masking happends
	fbo.begin();
	// Cleaning everthing with alpha mask on 0 in order to make it transparent by default
	ofClear(0, 0, 0, 0);

	shaderMask.begin();
	// here is where the fbo is passed to the shader
	shaderMask.setUniformTexture("maskTex", maskFbo.getTextureReference(), 1);

	backgroundImage.draw(0, 0);

	shaderMask.end();
	fbo.end();
	//ofEnableAlphaBlending();

}
void ofApp::MotionDraw()
{
	//if (diffFloat.bAllocated) {

	if (IRimage.bAllocated && IR_mtx.try_lock()) {
		//Get image dimensions
		int w = IRimage.width;
		int h = IRimage.height;

		//Set color for images drawing
		//ofSetColor(255, 255, 255);

		//Draw images grayImage,  diffFloat, bufferFloat
		IRimage.draw(0, 0, 3 * w, 3 * h);
		//diffFloat.draw(0, h, w, h);
		//bufferFloat.draw(0, 2 * h, w, h);
		binaryMotion.draw(0, 3 * h, 3 * w, 3 * h);
		binaryMotion.draw(3 * w, 0, 3 * w, 3 * h);
		for (int i = 0; i < contourFinder.nBlobs; i++) {
			contourFinder.blobs[i].draw(3 * w, 0);
		}
		IR_mtx.unlock();
	}

}

void Process::setup(int stepsnum, string imgfolder, int *toollist) {
	TotalImgsNum = stepsnum + 1;
	TotalStepsNum = stepsnum;
	currentStep = 0;
	string imgdirectory;
	ofImage temp;
	for (int i = 0; i < TotalStepsNum; i++)
	{
		imgdirectory = imgfolder + "/" + ofToString(i) + ".jpg";
		if (temp.loadImage(imgdirectory)) ProcessImages.push_back(temp);
		imgdirectory = imgfolder + "/outline" + ofToString(i) + ".jpg";
		if (temp.loadImage(imgdirectory)) OutlineImages.push_back(temp);
		imgdirectory = imgfolder + "/starttext" + ofToString(i) + ".jpg";
		if (temp.loadImage(imgdirectory)) starttexts.push_back(temp);
		imgdirectory = imgfolder + "/midtext" + ofToString(i) + ".jpg"; // now we only have one stage(stage 2, midtext1) that contains midtext
		if (temp.loadImage(imgdirectory)) midtexts.push_back(temp);
		Toollist.push_back((ToolStyle)toollist[i]);
	}
	imgdirectory = imgfolder + "/" + ofToString(TotalStepsNum) + ".jpg";
	temp.loadImage(imgdirectory);
	ProcessImages.push_back(temp);
}


void Widgets::setup()
{
	font.load("HYQuHeiW 2.ttf", 20);
	finishpercent.loadImage("interface/finishpercent.png");
	health.loadImage("interface/health.png");
}

void Widgets::update(Process cai, bool finish)
{

	//toolpara.loadImage("interface/toolpara" + ofToString(cai.currentStep) + ".png");
	currentToolStyle = cai.Toollist[cai.currentStep];
	if (finish) {
		instruction.loadImage("interface/stepend" + ofToString(cai.currentStep) + ".png");
		tips.loadImage("interface/tipsend" + ofToString(cai.currentStep) + ".png");

	}
	else {

		instruction.loadImage("interface/step" + ofToString(cai.currentStep) + ".png");
		tips.loadImage("interface/tips" + ofToString(cai.currentStep) + ".png");
	}
}

void Widgets::draw()
{
	instruction.draw(50, 720);
	tips.draw(900, 720);
	finishpercent.draw(260, 80);
	//health.draw(260, 30);
	//toolpara.draw(715, 80);
	//drawing the health bar
	//ofSetColor(255 * (1 - healthPercent), 255 * healthPercent, 30);
	//ofRect(380, 30, healthBarWidth*healthPercent, 30);
	//ofSetColor(255, 255, 255);
	string str;
	//str = ofToString((int)round(healthPercent * 100));
	//str += "%";
	//font.drawString(str, 380 + healthBarWidth + 5, 55);
	//draw the working bar
	ofSetColor(134, 216, 63);
	ofRect(380, 80, workingBarWidth*workingPercent, 30);
	ofSetColor(255, 255, 255);
	str = ofToString((int)round(workingPercent * 100));
	str += "%";
	font.drawString(str, 380 + workingBarWidth + 5, 105);
	ofNoFill();
	//ofRect(379, 29, healthBarWidth + 2, 32);
	ofRect(379, 79, workingBarWidth + 2, 32);
	//ofRect(819, 79, toolparaBarWidth + 2, 30 + 2);
	ofFill();
	/*
	switch (currentToolStyle) {
	case knife:
	{
		//ofSetColor(255, 255, 255);

		if (toolparaPercent > thres1) {
			ofSetColor(255, 0, 0);
			ofRect(820, 80, toolparaBarWidth*toolparaPercent, 30);
			font.drawString("X", 820 + toolparaBarWidth + 5, 105);
		}
		else {
			ofSetColor(134, 216, 63);
			ofRect(820, 80, toolparaBarWidth*toolparaPercent, 30);
			ofSetColor(255, 255, 255);
			font.drawString("O", 820 + toolparaBarWidth + 5, 105);
		}
		ofLine(ofPoint(820 + toolparaBarWidth*thres1, 80), ofPoint(820 + toolparaBarWidth*thres1, 80 + 30));
		break;
	}
	case brush:
	{
		if (toolparaPercent > thres2) {
			ofSetColor(255, 0, 0);
			ofRect(820, 80, toolparaBarWidth*toolparaPercent, 30);
			font.drawString("X", 820 + toolparaBarWidth + 5, 105);
		}
		else {
			ofSetColor(134, 216, 63);
			ofRect(820, 80, toolparaBarWidth*toolparaPercent, 30);
			ofSetColor(255, 255, 255);
			font.drawString("O", 820 + toolparaBarWidth + 5, 105);
		}
		ofLine(ofPoint(820 + toolparaBarWidth*thres2, 80), ofPoint(820 + toolparaBarWidth*thres2, 80 + 30));
		break;
	}
	case dropper:
	{
		ofSetColor(134, 216, 63);
		ofRect(820, 80, toolparaBarWidth*toolparaPercent, 30);
		break;
	}

	}
	*/

}




//=====================Brian==========================

// A class to describe a group of Particles
// An ArrayList is used to manage the list of Particles 
void ofApp::resetParticles() {
	//reset dust
	for (unsigned int i = 0; i < dusts.size(); i++) {
		dusts[i].reset();
	}

	//reset dirt
	for (unsigned int i = 0; i < dirts.size(); i++) {
		dirts[i].reset();
	}
}

void ofApp::drawParticles() {
	for (unsigned int i = 0; i < dusts.size(); i++) {
		dusts[i].draw();
	}

	for (unsigned int i = 0; i < dirts.size(); i++) {
		dirts[i].draw();
	}
}

void ofApp::resetTimer() {
	startTime = ofGetElapsedTimeMillis();
	timer = ofGetElapsedTimeMillis() - startTime;
}

/*
void ofApp::brushParticleEffects() {
//brush effects
for (unsigned int i = 0; i < dusts.size(); i++) {
dusts[i].update();
}

if (ToolNow == knife && timer > 80 && emitParticles) {
bool found = false;
resetTimer();

for (unsigned int i = 0; i < dusts.size(); i++) {
if (!dusts[i].isAlive()) {
//find index where there is unutilized particle objects
dustIndex = i;
found = true;
break;
}
}

if (!found) {
dusts[0].reset();
dustIndex = 0;
}

dusts[dustIndex].emit();
}
}

void ofApp::scrapeParticleEffects() {
//dirt effects
for (unsigned int i = 0; i < dirts.size(); i++) {
dirts[i].update();
}





if (ToolNow == knife && timer > 80 && emitParticles) {
bool found = false;
resetTimer();

for (unsigned int i = 0; i < dirts.size(); i++) {
if (!dirts[i].isAlive()) {
//find index where there is unutilized particle objects
dirtIndex = i;
found = true;
break;
}
}

if (!found) {
dirts[0].reset();
dirtIndex = 0;
}

dirts[dirtIndex].emit();
}

if (Moved) {
if (ToolNow == knife && !shali.isPlaying()) {
shali.play();
}
else if (ToolNow == brush && !shali.isPlaying()) {
saotu.play();
}
}

}
*/
void ofApp::emitParticles() {
	if (true) {
		//printf("emitParticles \n");

		switch (ToolNow) {
		case knife:
			found = false;
			resetTimer();
			for (unsigned int i = 0; i < dirts.size() - 2; i++) {
				if (!dirts[i].isAlive() && !dirts[i + 1].isAlive() && !dirts[i + 2].isAlive()) {
					//find index where there is unutilized particle objects
					dirtIndex = i;
					found = true;
					break;
				}
			}

			if (!found) {
				dirts[0].reset();
				dirtIndex = 0;
			}
			dirts[dirtIndex].emit(mPos);
			dirts[dirtIndex + 1].emit(mPos);
			dirts[dirtIndex + 2].emit(mPos);
			break;

		case brush:
			found = false;
			resetTimer();
			for (unsigned int i = 0; i < dusts.size(); i++) {
				if (!dusts[i].isAlive()) {
					//find index where there is unutilized particle objects
					dustIndex = i;
					found = true;
					break;
				}
			}

			if (!found) {
				dusts[0].reset();
				dustIndex = 0;
			}

			dusts[dustIndex].emit(mPos);
			break;
		case none:

			break;
		default:
			break;


		}

	}

}

void ofApp::particleEffects() {

	for (unsigned int i = 0; i < dusts.size(); i++) {
		dusts[i].update(mPos);
	}

	for (unsigned int i = 0; i < dirts.size(); i++) {
		dirts[i].update(mPos);
	}

	if (Moved) {
		//printf("emitP \n");
		emitParticles();
		float playPos = ofRandom(0.05, 0.2);
		if (ToolNow == knife) {
			if (shali.getPosition() > playPos || !shali.isPlaying()) {
				shali.play();
			}

		}
		else if (ToolNow == brush) {
			if (saotu.getPosition() > playPos || !saotu.isPlaying()) {
				saotu.play();
			}
		}
	}
}






