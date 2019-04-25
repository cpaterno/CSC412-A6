//
//  main.c
//  Final Project CSC412
//
//  Created by Jean-Yves Hervé on 2017-05-01.
//  Copyright © 2017 Jean-Yves Hervé. All rights reserved.
//

#include <iostream>
#include <string>
#include <vector> // added
//
#include <cstdio>
#include <cstdlib>
#include <time.h>
#include <dirent.h> // added
//
#include "gl_frontEnd.h"
#include "imageIO_TGA.h"


//==================================================================================
//	Function prototypes
//==================================================================================
void myKeyboard(unsigned char c, int x, int y);
void initializeApplication(void);
void readInFiles(std::vector<std::string>& fileList, const std::string& dirPath);


//==================================================================================
//	Application-level global variables
//==================================================================================

//	Don't touch. These are defined in the front-end source code
extern int	gMainWindow;


//	Don't rename any of these variables/constants
//--------------------------------------------------
unsigned int numLiveFocusingThreads = 0;		//	the number of live focusing threads

//	An array of C-string where you can store things you want displayed in the state pane
//	that you want the state pane to display (for debugging purposes?)
//	Don't change the dimensions as this may break the front end
//	I preallocate the max number of messages at the max message
//	length.  This goes against some of my own principles about
//	good programming practice, but I do that so that you can
//	change the number of messages and their content "on the fly,"
//	at any point during the execution of your program, whithout
//	having to worry about allocation and resizing.
const int MAX_NUM_MESSAGES = 8;
const int MAX_LENGTH_MESSAGE = 32;
char** message;
int numMessages;
time_t launchTime;

//	This is the image that you should be writing into.  In this
//	handout, I simply read one of the input images into it.
//	You should not rename this variable unless you plan to mess
//	with the display code.
ImageStruct* imageOut;
std::vector<std::string> images;

//------------------------------------------------------------------
//	The variables defined here are for you to modify and add to
//------------------------------------------------------------------
const std::string IN_PATH = "/home/dev/Assignments/a6/Handout - Data 2/Series02/";
const std::string OUT_PATH = "./Output/";
int imageIndex = 0;
//#define IN_PATH		"/home/dev/Assignments/a6/Handout - Data 2/Series02/"
//#define OUT_PATH	"./Output/"

//------------------------------------------------------------------------
//	You shouldn't have to change anything in the main function.
//------------------------------------------------------------------------
int main(int argc, char** argv) {
	//	Now we can do application-level initialization
	initializeApplication();

	//	Even though we extracted the relevant information from the argument
	//	list, I still need to pass argc and argv to the front-end init
	//	function because that function passes them to glutInit, the required call
	//	to the initialization of the glut library.
	initializeFrontEnd(argc, argv, imageOut);
	
	//==============================================
	//	This is OpenGL/glut magic.  Don't touch
	//==============================================
	//	Now we enter the main loop of the program and to a large extend
	//	"lose control" over its execution.  The callback functions that
	//	we set up earlier will be called when the corresponding event
	//	occurs
	glutMainLoop();
	
	//	Free allocated resource before leaving (not absolutely needed, but
	//	just nicer.  Also, if you crash there, you know something is wrong
	//	in your code.
	for (int k=0; k<MAX_NUM_MESSAGES; k++)
		free(message[k]);
	free(message);
	
	//	This will probably never be executed (the exit point will be in one of the
	//	call back functions).
	return 0;
}

//==================================================================================
//	These are the functions that tie the computation with the rendering.
//	Some parts are "don't touch."  Other parts need your intervention
//	to make sure that access to critical section is properly synchronized
//==================================================================================

//	I can't see any reason why you may need/want to change this
//	function
void displayImage(GLfloat scaleX, GLfloat scaleY) {
	//==============================================
	//	This is OpenGL/glut magic.  Don't touch
	//==============================================
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPixelZoom(scaleX, scaleY);

	//--------------------------------------------------------
	//	stuff to replace or remove.
	//	Here I assign a random color to a few random pixels
	//--------------------------------------------------------
	/*for (int k=0; k<100; k++) {
		int i = random() % imageOut->height;
		int j = random() % imageOut->width;
		//	I make sure that my alpha channel is 255
		int newCol = (int)(random() % 0x100000000) | 0xFF000000;
		int* dest = (int*) imageOut->raster;
		dest[i*imageOut->width + j] = newCol;
	}*/

	//==============================================
	//	This is OpenGL/glut magic.  Don't touch
	//==============================================
	glDrawPixels(imageOut->width, imageOut->height,
				  GL_RGBA,
				  GL_UNSIGNED_BYTE,
				  imageOut->raster);
	// modify this for cycleing^^^^^

}


void displayState(void) {
	//==============================================
	//	This is OpenGL/glut magic.  Don't touch
	//==============================================
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();


	//--------------------------------------------------------
	//	stuff to replace or remove.
	//--------------------------------------------------------
	//	Here I hard-code a few messages that I want to see displayed in my state
	//	pane.  The number of live focusing threads will always get displayed
	//	(as long as you update the value stored in the.  No need to pass a message about it.
	time_t currentTime = time(NULL);
	numMessages = 3;
	sprintf(message[0], "System time: %ld", currentTime);
	sprintf(message[1], "Time since launch: %ld", currentTime-launchTime);
	std::string strIndex = std::to_string(imageIndex) + '\x0';
	sprintf(message[2], strIndex.c_str());
	
	
	//---------------------------------------------------------
	//	This is the call that makes OpenGL render information
	//	about the state of the simulation.
	//	You may have to synchronize this call if you run into
	//	problems, but really the OpenGL display is a hack for
	//	you to get a peek into what's happening.
	//---------------------------------------------------------
	drawState(numMessages, message);
}

//	This callback function is called when a keyboard event occurs
//	You can change things here if you want to have keyboard input
//
void handleKeyboardEvent(unsigned char c, int x, int y) {
	int ok = 0;
	
	switch (c) {
		//	'esc' to quit
		case 27:
			//	If you want to do some cleanup, here would be the time to do it.
			exit(0);
			break;
		//	Feel free to add more keyboard input, but then please document that
		//	in the report.
		// right arrow
		case 'd':
		case 'D':
			++imageIndex;
			if (static_cast<std::size_t>(imageIndex) == images.size())
				imageIndex = 0;
			break;
		// left arrow
		case 'a':
		case 'A':
			--imageIndex;
			if (imageIndex < 0)
				imageIndex = images.size() - 1;
			break;
		default:
			ok = 1;
			break;
	}
	if (!ok) {
		//	do something?
	}
}

//==================================================================================
//	This is a part that you have to edit and add to, for example to
//	load a complete stack of images and initialize the output image
//	(right now it is initialized simply by reading an image into it.
//==================================================================================

void initializeApplication(void) {

	//	I preallocate the max number of messages at the max message
	//	length.  This goes against some of my own principles about
	//	good programming practice, but I do that so that you can
	//	change the number of messages and their content "on the fly,"
	//	at any point during the execution of your program, without
	//	having to worry about allocation and resizing.
	message = (char**) malloc(MAX_NUM_MESSAGES*sizeof(char*));
	for (int k=0; k<MAX_NUM_MESSAGES; k++)
		message[k] = (char*) malloc((MAX_LENGTH_MESSAGE+1)*sizeof(char));
	
	//---------------------------------------------------------------
	//	All the code below to be replaced/removed
	//	I load an image to have something to look at
	//---------------------------------------------------------------
	//	Yes, I am using the C random generator, although I usually rant on and on
	//	that the C/C++ default random generator is junk and that the people who use it
	//	are at best fools.  Here I am not using it to produce "serious" data (as in a
	//	simulation), only some color, in meant-to-be-thrown-away code
	
	//	seed the pseudo-random generator
	srand((unsigned int) time(NULL));

	//	right now I read *one* hardcoded image, into my output
	//	image. This is definitely something that you will want to
	//	change.
	readInFiles(images, IN_PATH);
	imageOut = readTGA(images[imageIndex].c_str());
	launchTime = time(NULL);
}

void readInFiles(std::vector<std::string>& fileList, const std::string& dirPath) {
	const char* dataPath = dirPath.c_str();
	
    DIR* directory = opendir(dataPath);
    // insure the directory is valid
    if (!directory) {
		std::cout << "Program aborted data folder " << dataPath << " not found" << std::endl;
		exit(EXIT_FAILURE);
	}

	struct dirent* entry;
    // while there are contents of the directory traverse it
    while ((entry = readdir(directory))) {
        std::string name = std::string(entry->d_name);
        // ignore . and ..
        if (name[0] != '.') {
            // update filelist
            if (entry->d_type == DT_REG)
			    fileList.push_back(dirPath + name);   
        }
        
    }
	closedir(directory);
}



