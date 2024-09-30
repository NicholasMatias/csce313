/*
	Author of the starter code
    Yifan Ren
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 9/15/2024
	
	Please include your Name, UIN, and the date below
	Name: Nicholas Matias	
	UIN: 232006560
	Date: 9/29/24
*/
#include "common.h"
#include "FIFORequestChannel.h"

using namespace std;


// int main (int argc, char *argv[]) {
// 	int opt;
// 	int p = 1;
// 	double t = 0.0;
// 	int e = 1;
// 	string filename = "";

// 	//Add other arguments here
// 	while ((opt = getopt(argc, argv, "p:t:e:f:")) != -1) {
// 		switch (opt) {
// 			case 'p':
// 				p = atoi (optarg);
// 				break;
// 			case 't':
// 				t = atof (optarg);
// 				break;
// 			case 'e':
// 				e = atoi (optarg);
// 				break;
// 			case 'f':
// 				filename = optarg;
// 				break;
// 		}
// 	}

// 	//Task 1:
// 	//Run the server process as a child of the client process

//     FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);

// 	//Task 4:
// 	//Request a new channel
	
// 	//Task 2:
// 	//Request data points
//     char buf[MAX_MESSAGE];
//     datamsg x(1, 0.0, 1);
	
// 	memcpy(buf, &x, sizeof(datamsg));
// 	chan.cwrite(buf, sizeof(datamsg));
// 	double reply;
// 	chan.cread(&reply, sizeof(double));
// 	cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	
// 	//Task 3:
// 	//Request files
// 	filemsg fm(0, 0);
// 	string fname = "1.csv";
	
// 	int len = sizeof(filemsg) + (fname.size() + 1);
// 	char* buf2 = new char[len];
// 	memcpy(buf2, &fm, sizeof(filemsg));
// 	strcpy(buf2 + sizeof(filemsg), fname.c_str());
// 	chan.cwrite(buf2, len);

// 	delete[] buf2;
// 	__int64_t file_length;
// 	chan.cread(&file_length, sizeof(__int64_t));
// 	cout << "The length of " << fname << " is " << file_length << endl;
	
// 	//Task 5:
// 	// Closing all the channels
//     MESSAGE_TYPE m = QUIT_MSG;
//     chan.cwrite(&m, sizeof(MESSAGE_TYPE));
// }

#include <iostream>
#include <string>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    // Process command-line arguments
    int opt;
    int patient = 0;
    double time = 0.0;
    int ecg = 0;
    std::string filename;
    int buffer_capacity = MAX_MESSAGE;
    bool new_channel = false;

    while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
        switch (opt) {
            case 'p':
                patient = atoi(optarg);
                break;
            case 't':
                time = atof(optarg);
                break;
            case 'e':
                ecg = atoi(optarg);
                break;
            case 'f':
                filename = optarg;
                break;
            case 'm':
                buffer_capacity = atoi(optarg);
                break;
            case 'c':
                new_channel = true;
                break;
            default:
                std::cerr << "Unknown option" << std::endl;
                return -1;
        }
    }

    // Start the server as a child process
    pid_t pid = fork();
    if (pid == 0) {
        // Child process: Run the server
        char* args[] = {"./server", NULL};
        execvp(args[0], args);
        exit(0);
    }

    // Parent process: Communicate with server
    FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);

    // Handle data point request (-p, -t, -e)
    if (patient > 0 && time >= 0 && (ecg == 1 || ecg == 2)) {
        datamsg dmsg(patient, time, ecg);
        chan.cwrite(&dmsg, sizeof(datamsg));
        
        double response;
        chan.cread(&response, sizeof(double));
        std::cout << "ECG Value: " << response << std::endl;
    }

    // Handle file request (-f)
    if (!filename.empty()) {
        // Send a file message to get file length
        filemsg fmsg(0, 0);
        int len = sizeof(filemsg) + filename.size() + 1;
        char buf[len];
        memcpy(buf, &fmsg, sizeof(filemsg));
        strcpy(buf + sizeof(filemsg), filename.c_str());

        chan.cwrite(buf, len);

        __int64_t filesize;
        chan.cread(&filesize, sizeof(filesize));
        std::cout << "File size: " << filesize << " bytes" << std::endl;

        // Request the file in chunks
        __int64_t offset = 0;
        char recvbuf[buffer_capacity];
        while (offset < filesize) {
            int remaining = std::min((__int64_t)buffer_capacity, filesize - offset);
            filemsg chunkmsg(offset, remaining);
            memcpy(buf, &chunkmsg, sizeof(filemsg));
            strcpy(buf + sizeof(filemsg), filename.c_str());

            chan.cwrite(buf, len);
            chan.cread(recvbuf, remaining);
            // Write received chunk to file (you can use fwrite for binary data)
            offset += remaining;
        }
    }

    // Handle new channel request (-c)
    if (new_channel) {
        // Request a new channel
        MESSAGE_TYPE nch_msg(NEWCHANNEL_MSG);
        chan.cwrite(&nch_msg, sizeof(MESSAGE_TYPE));

        char new_channel_name[256];
        chan.cread(new_channel_name, sizeof(new_channel_name));
        FIFORequestChannel new_chan(new_channel_name, FIFORequestChannel::CLIENT_SIDE);
        
        // Use the new channel for further communication (optional logic)
        // After use, close it properly with QUIT_MSG
    }

    // Send QUIT_MSG to close communication
    MESSAGE_TYPE quit_msg(QUIT_MSG);
    chan.cwrite(&quit_msg, sizeof(MESSAGE_TYPE));

    // Wait for the server to exit
    wait(NULL);

    return 0;
}
