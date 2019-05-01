#include <iostream>
#include <string>
//
#include <cstdio>
#include <cstdlib>
#include <time.h>
// for opening directories
#include <dirent.h>
//
#include "gl_frontEnd.h"
#include "imageIO_TGA.h"
// for threading
#include "imageThread.h" 


//==================================================================================
//	Function prototypes
//==================================================================================
void myKeyboard(unsigned char c, int x, int y);
void initializeApplication(void);
void readInNames(std::vector<std::string>& fileList, const std::string& dirPath);
void readInImages(std::vector<ImageStruct*>& series, std::vector<std::string>& names);
void globalsCleanup();
void createThreads(std::vector<ImageStruct*>* series, ImageStruct* output, std::vector<ImageThread>& info);


//==================================================================================
//	Application-level global variables
//==================================================================================

//	Don't touch. These are defined in the front-end source code
extern int	gMainWindow;


//	Don't rename any of these variables/constants
//--------------------------------------------------

//	the number of live focusing threads
unsigned int numLiveFocusingThreads = 0;		

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

//	This is the image that you should be writing into. In this
//	handout, I simply read one of the input images into it.
//	You should not rename this variable unless you plan to mess
//	with the display code.
ImageStruct* imageOut = nullptr;
// globals for cycling through images in the GUI
std::vector<ImageStruct*> imageSeries;
const ImageStruct* currentImage = nullptr;
int imageIndex = 0;
// thread structs array
std::vector<ImageThread> threadInfo;

//------------------------------------------------------------------
//	The variables defined here are for you to modify and add to
//------------------------------------------------------------------
const std::string IN_PATH = "./Handout - Data 1/Series01/";
const std::string OUT_PATH = "./Output/inFocus.tga";

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
	//--------------------------------------------------------

	// Join all done threads
	for (std::size_t i = 0; i < numThreads; ++i) {
		if (threadInfo[i].status == DONE) {
			if (pthread_join(threadInfo[i].threadID, nullptr) != 0) {
				std::cerr << "Thread can not be joined" << std::endl;
				exit(EXIT_FAILURE);
			}
			threadInfo[i].status = JOINED;
			--numLiveFocusingThreads;
		}
	}

	//==============================================
	//	This is OpenGL/glut magic.  Don't touch
	//==============================================

	// modified this function so one can cycle through the input images for comparison
	glDrawPixels(currentImage->width, currentImage->height, GL_RGBA, GL_UNSIGNED_BYTE,currentImage->raster);
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
	if (currentImage == imageOut) {
		sprintf(message[2], "Output Image Mode");
	} else {
		sprintf(message[2], "Input Image Mode");
		sprintf(message[3], "Index: %d", imageIndex);
		++numMessages;
	}
	
	
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
	switch (c) {
		//	'esc' to quit
		case 27:
			// only write out image if all the threads are already joined
			if (numLiveFocusingThreads == 0)
				writeTGA(OUT_PATH.c_str(), imageOut);
			// join all threads if they are not already joined
			for (std::size_t i = 0; i < numThreads; ++i) {
				if (threadInfo[i].status != JOINED) {
					if (pthread_join(threadInfo[i].threadID, nullptr) != 0) {
						std::cerr << "Thread can not be joined" << std::endl;
						exit(EXIT_FAILURE);
					}
					threadInfo[i].status = JOINED;
					--numLiveFocusingThreads;
				}
			}
			//	Free allocated resource before leaving (not absolutely needed, but
			//	just nicer.  Also, if you crash there, you know something is wrong
			//	in your code.
			globalsCleanup();
			exit(0);
			break;
		//	Feel free to add more keyboard input, but then please document that in the report.
		// show next image
		case 'd':
		case 'D':
			// no effect if in output mode
			if (currentImage != imageOut) {
				++imageIndex;
				// bounds checking
				if (static_cast<std::size_t>(imageIndex) == imageSeries.size())
					imageIndex = 0;
				// update image to be displayed
				currentImage = imageSeries[imageIndex];
			}
			break;
		// show previous image
		case 'a':
		case 'A':
			// no effect if in output mode
			if (currentImage != imageOut) {
				--imageIndex;
				// bounds checking
				if (imageIndex < 0)
					imageIndex = imageSeries.size() - 1;
				// update image to be displayed
				currentImage = imageSeries[imageIndex];
			}
			break;
		// toggle between output and input image mode
		case 's':
		case 'S':
			// update image to be displayed
			if (currentImage == imageOut)
				currentImage = imageSeries[imageIndex];
			else 
				currentImage = imageOut;
			break;
		default:
			std::cout << "Invalid key, please try again" << std:: endl;
			std::cout << "The following keys are valid:\n \
							+ esc -> quits program\n \
							+ d or D -> view next input image\n \
							+ a or A -> view previous input image\n \
							+ s or S switch view from input to output image or vice versa" << std::endl;
			break;
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
	// read in file names
	std::vector<std::string> imageNames;
	readInNames(imageNames, IN_PATH);
	// open the TGA images into ImageStructs
	readInImages(imageSeries, imageNames);
	// if no images are read in, no point in running the program
	if (imageSeries.empty()) {
		std::cerr << "No TGA files in input directory" << std::endl;
		exit(EXIT_FAILURE);
	}
	// initialize global image pointers
	imageOut = new ImageStruct(imageSeries[0]->width, imageSeries[0]->height, imageSeries[0]->type, 1);
	currentImage = imageOut;
	// begin stack focussing
	createThreads(&imageSeries, imageOut, threadInfo);
	launchTime = time(NULL);
}

// get file names from a directory
void readInNames(std::vector<std::string>& fileList, const std::string& dirPath) {
	const char* dataPath = dirPath.c_str();
	
    DIR* directory = opendir(dataPath);
    // ensure the directory is valid
    if (!directory) {
		std::cout << "Program aborted data folder " << dataPath << " not found" << std::endl;
		exit(EXIT_FAILURE);
	}

	struct dirent* entry;
	std::string fileExtension;
	std::string name;
    // while there are contents of the directory traverse it
    while ((entry = readdir(directory))) {
        name = std::string(entry->d_name);
        // ignore . and ..
        if (name[0] != '.') {
            // update filelist
			fileExtension = (name.length() > 4) ? name.substr(name.length() - 4) : "";
            if (entry->d_type == DT_REG && fileExtension == ".tga")
			    fileList.push_back(dirPath + name);   
        }
        
    }
	closedir(directory);
}

// read and store TGA images from a list of file names
void readInImages(std::vector<ImageStruct*>& series, std::vector<std::string>& names) {
	if (!series.empty())
		series.clear();
	std::string fileExtension;
	for (const auto& s : names) {
		fileExtension = (s.length() > 4) ? s.substr(s.length() - 4) : "";
		if (fileExtension == ".tga")
			series.push_back(readTGA(s.c_str()));
	}		
}

// cleanup global variables with dynamically allocated memory
void globalsCleanup() {
	// free messages (code moved from main)
	for (int k=0; k<MAX_NUM_MESSAGES; k++)
		free(message[k]);
	free(message);
	message = nullptr;

	// free imageOut
	delete imageOut;
	imageOut = nullptr;

	// free input images
	for (auto& i : imageSeries) {
		delete i;
		i = nullptr;
	}
}

// divide the image into parts and create a thread to process each part
void createThreads(std::vector<ImageStruct*>* series, ImageStruct* output, std::vector<ImageThread>& info) {
	info.resize(numThreads);
	// calculate how many rows to give each thread
	std::size_t rowsPerThread = output->height / numThreads;
	if (output->height % numThreads != 0)
		++rowsPerThread;
	// setup ImageThread structs and create threads
	for (std::size_t i = 0; i < numThreads; ++i) {
		// intialize the ith thread struct
		info[i].imageStack = series;
		info[i].outputImage = output;
		info[i].startRow = i * rowsPerThread;
		info[i].endRow = info[i].startRow + rowsPerThread - 1;
		if (info[i].endRow >= output->height)
			info[i].endRow = output->height - 1;
		info[i].status = RUNNING;

		// create the ith thread
		if (pthread_create(&info[i].threadID, nullptr, imageThreadFunc, static_cast<void*>(&info[i])) != 0) {
			std::cerr << "Could not create thread" << std::endl;
			exit(EXIT_FAILURE);
		}
		++numLiveFocusingThreads;
	}
}