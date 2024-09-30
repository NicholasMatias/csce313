/*
	Author of the starter code
    Yifan Ren
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 9/15/2024
	
	Added edits as an example to base starter code
	Kyle Lang
	Date: 9/18/2024
*/
#include "common.h"
#include "stdlib.h"
#include "FIFORequestChannel.h"

using namespace std;

// Task 2.1: Single Data Point transfer
// Base code already provides an implementation for this task
void requestDataPoint() {
    char buf[MAX_MESSAGE];
	datamsg x(1, 0.0, 1);		// Request patient data point
	
    memcpy(buf, &x, sizeof(datamsg));   // Can either copy datamsg into separate buffer then write buffer into pipe,
	chan.cwrite(&x, sizeof(datamsg));   // or just directly write datamsg into pipe
	double reply;
	chan.cread(&reply, sizeof(double));

	cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
}

// Task 2.2: 1000 Data Point transfer into received/x1.csv
// Request 1000 data points from server and write results into received/x1.csv with same file format as the patient {1-15}.csv files
void requestData() {
    // Open x1.csv file under ./received/
	// Can use any method for opening a file, ofstream is one method
	ofstream ofs();

	// Iterate 1000 times
	for (0..1000) {
		// Write time into x1.csv (Time is 0.004 second deviations)

		msg = datamsg(<ecg 1>); // Request ecg1
		// Write ecg1 datamsg into pipe
		// Read response for ecg1 from pipe
        // Write ecg1 value into x1.csv
		
		msg = datamsg(<ecg 2>); // Request ecg2
		// Write ecg2 datamsg into pipe
        // Read response for ecg2 from pipe
        // Write ecg2 value into x1.csv

        // Increment time
	}

	// CLOSE YOUR FILE
}

// Task 3: Request File from server
// Request a file under BIMDC/ given the file name, by sequentially requesting data chunks from the file and copying into a new file of the same name into received/
void requestFile() {
	filemsg fm(0, 0);	// Request file length message
	string fname = "1.csv"; // Make sure to read filename from command line arguments (given to -f)

	// Calculate file length request message and set up buffer
	int len = sizeof(filemsg) + (fname.size() + 1);
	char* buf2 = new char[len];

	// Copy filemsg fm into msgBuffer, attach filename to the end of filemsg fm in msgBuffer, then write msgBuffer into pipe
	memcpy(buf2, &fm, sizeof(filemsg));
	strcpy(buf2 + sizeof(filemsg), fname.c_str());
	chan.cwrite(buf2, len);
    delete[] buf2;

	// Read file length response from server for specified file
	__int64_t file_length;
	chan->cread(&file_length, sizeof(__int64_t));
	cout << "The length of " << fname << " is " << file_length << endl;

	// Set up output file under received folder
	// Can use any file opening method
	ofstream ofs();
	
	// Request data chunks from server and output into file
	// Loop from start of file to file_length
	for ( 0..file_length ) {
		// Create filemsg for data chunk range
        // Assign data chunk range properly so that the data chunk to fetch from the file does NOT exceed the file length (i.e. take minimum between the two)
		fm = filemsg(<some offset>, <data chunk range>);

		// Copy filemsg into buf2 buffer and write into pipe
		// File name need not be re-copied into buf2, as filemsg struct object is staticly sized and therefore the file name is unchanged when filemsg is re-copied into buf2

		// Read data chunk response from server into separate data buffer
		
		// Write data chunk into new file
		ofs.write();
	}

	// CLOSE YOUR FILE
}

// Task 4: Request a new FIFORequestChannel
// Send a request to the server to establish a new FIFORequestChannel, and use the servers response to create the client's FIFORequestChannel
// Client must now communicate over this new RequestChannel for any data point or file transfers
void openChannel() {
	MESSAGE_TYPE m = NEWCHANNEL_MSG;
	// Write new channel message into pipe

    // Read response from pipe (Can create any static sized char array that fits server response, e.g. MAX_MESSAGE)

    // Create a new FIFORequestChannel object using the name sent by server
	chan2 = FIFORequestChannel(newPipeName, FIFORequestChannel::CLIENT_SIDE);
}

int main (int argc, char *argv[]) {
	int opt;

    // Can add boolean flag variables if desired
	int p = 1;
	double t = 0.0;
	int e = 1;
	string filename = "";

	// Add other arguments here   |   Need to add -c, -m flags. BE CAREFUL OF getopt() NOTATION
	while ((opt = getopt(argc, argv, "p:t:e:f:")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				p_Flag = true;
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
            /*
                Add -c and -m flag cases
            */
		}
	}

    // NOT NECESSARY: Check for p flag XOR f flag; That is, either p flag or f flag should be given not both
    if ( p == f )
        EXITONERROR() 
    
	// Task 1:
	// Run the server process as a child of the client process
	char* cmd[] = { server, args..., nullptr };
    // Fork child process
	if ( fork() < 0 ) 
		EXITONERROR();

    // Run server
    // SECOND CONDITIONAL NOT EVALUATED BY PARENT
	if ( pid == 0 && ( execvp(server) < 0 ) ) 
		EXITONERROR();

	// Set up FIFORequestChannel
    FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);

	// Task 4:
	// Request a new channel (e.g. -c)
	// Should use this new channel to communicate with server (still use control channel for sending QUIT_MSG)
	if ( c )
		openChannel();

	// Task 2.1 + 2.2:
	// Request data points
	if ( p ) {
		if ( t ) requestDataPoint(); // (e.g. './client -p 1 -t 0.000 -e 1')
		else requestData();	// (e.g. './client -p 1')
	}
	//Task 3:
	//Request files (e.g. './client -f 1.csv')
	else if ( f ) {
		requestFile();
	}
	
	//Task 5:
	// Closing all the channels
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite(&m, sizeof(MESSAGE_TYPE));
	
	// Wait on children
}
