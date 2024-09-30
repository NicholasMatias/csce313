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
#include <fstream>
#include <sys/wait.h>
#include <unistd.h>
#include <cstring>

using namespace std;

void requestDataPoint(FIFORequestChannel &chan, int patient, double time, int ecg) {
    datamsg dmsg(patient, time, ecg);
    chan.cwrite(&dmsg, sizeof(datamsg));

    double response;
    chan.cread(&response, sizeof(double));
    cout << "ECG " << ecg << " value for patient " << patient << " at time " << time << " is " << response << endl;
}

void requestFile(FIFORequestChannel &chan, const string &filename, int buffer_capacity) {
    // Send file length request
    filemsg fm(0, 0);
    int len = sizeof(filemsg) + filename.size() + 1;
    char* buf = new char[len];

    memcpy(buf, &fm, sizeof(filemsg));
    strcpy(buf + sizeof(filemsg), filename.c_str());
    chan.cwrite(buf, len);

    // Receive file length
    __int64_t file_length;
    chan.cread(&file_length, sizeof(file_length));
    cout << "File size: " << file_length << " bytes" << endl;

    // Open file to write chunks
    ofstream ofs("received/" + filename, ios::binary);
    if (!ofs.is_open()) {
        cerr << "Error opening file for writing." << endl;
        delete[] buf;
        return;
    }

    // Request file in chunks
    __int64_t offset = 0;
    while (offset < file_length) {
        int chunk_size = min((__int64_t)buffer_capacity, file_length - offset);
        filemsg chunk_fm(offset, chunk_size);
        memcpy(buf, &chunk_fm, sizeof(filemsg));
        chan.cwrite(buf, len);

        // Read the chunk and write to file
        char* recv_buffer = new char[chunk_size];
        chan.cread(recv_buffer, chunk_size);
        ofs.write(recv_buffer, chunk_size);
        delete[] recv_buffer;

        offset += chunk_size;
    }

    ofs.close();
    delete[] buf;
    cout << "File transfer completed." << endl;
}

FIFORequestChannel* openNewChannel(FIFORequestChannel &chan) {
    MESSAGE_TYPE nch_msg(NEWCHANNEL_MSG);
    chan.cwrite(&nch_msg, sizeof(MESSAGE_TYPE));

    char new_channel_name[MAX_MESSAGE];
    chan.cread(new_channel_name, sizeof(new_channel_name));
    return new FIFORequestChannel(new_channel_name, FIFORequestChannel::CLIENT_SIDE);
}

void closeChannel(FIFORequestChannel &chan) {
    MESSAGE_TYPE quit_msg(QUIT_MSG);
    chan.cwrite(&quit_msg, sizeof(MESSAGE_TYPE));
}

int main(int argc, char *argv[]) {
    // Variables to store arguments
    int patient = 1;
    double time = 0.0;
    int ecg = 1;
    string filename = "";
    int buffer_capacity = MAX_MESSAGE;
    bool new_channel = false;

    // Parse command-line arguments
    int opt;
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
                cerr << "Invalid argument!" << endl;
                return -1;
        }
    }

    // Task 1: Run the server as a child process
    pid_t pid = fork();
    if (pid == 0) {
        char* args[] = {"./server", nullptr};
        execvp(args[0], args);
        exit(0);
    }

    // Parent process: Connect to server
    FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);

    // Task 4: Open new channel if requested
    FIFORequestChannel* data_channel = &chan;
    if (new_channel) {
        data_channel = openNewChannel(chan);
    }

    // Task 2: Request data points
    if (!filename.empty()) {
        requestFile(*data_channel, filename, buffer_capacity);
    } else {
        requestDataPoint(*data_channel, patient, time, ecg);
    }

    // Task 5: Close channels
    closeChannel(*data_channel);
    if (data_channel != &chan) {
        closeChannel(chan);
        delete data_channel;
    }

    wait(NULL);  // Wait for the server to exit
    return 0;
}
