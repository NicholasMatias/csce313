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
#include <sys/time.h>

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = -1;
	double t = -1;
	int e = -1;
	string filename = "";
	int m = MAX_MESSAGE;
	bool newchan = false;
	vector<FIFORequestChannel*> channels;

	//Add other arguments here
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
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
			case 'm':
				m = atoi (optarg);
				break;
			case 'c':
				newchan = true;
				break;
		}
	}

	//Task 1:
	// Run the server process as a child of the client process
	pid_t pid = fork();
	if (pid == 0){

		// give arguments for the server
		
		// server needs './server', '-m', '<val for -m arg>', 'NULL'
		
		char* args[] = {
			(char*)"./server",
			(char*)"-m",
			(char*)to_string(m).c_str(),
			NULL
		};

		// in the child, run execvp using the server arguments
		
		execvp(args[0], args);
		return(1);
	}

    FIFORequestChannel cont_chan("control", FIFORequestChannel::CLIENT_SIDE);
	channels.push_back(&cont_chan);

	//Task 4:
	
	//Request a new channel
	
	if (newchan) {
	
		// send newchannel request to the server
	
		MESSAGE_TYPE nc = NEWCHANNEL_MSG;
		cont_chan.cwrite(&nc, sizeof(MESSAGE_TYPE));

		// create a variable to hold the name
	
		char newchan_name[1024];

		// create a response from the server
	
		cont_chan.cread(newchan_name, sizeof(newchan_name));
		
		// call the FIFORequestChannel constructor with the name from the server
	
		FIFORequestChannel *newchan = new FIFORequestChannel(newchan_name, FIFORequestChannel::CLIENT_SIDE);

		// push the new channel into the vector
	
		channels.push_back(newchan);
	}

	FIFORequestChannel chan = *(channels.back());

	//Task 2:
	
	//Request data points	
	
	if (!(p == -1 || t == -1 || e == -1)){
		char buf[MAX_MESSAGE];	
		datamsg x(p, t, e);
		
		memcpy(buf, &x, sizeof(datamsg));
		chan.cwrite(buf, sizeof(datamsg));
		double reply;
		chan.cread(&reply, sizeof(double));
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	} else if (p != -1){

		ofstream out_file("received/x1.csv", ios::app);

		// loop over first 1000 lines
	
		for (double i = 0; i < 1000; ++i){
			out_file << (i*.004);

			for (int j = 1; j <= 2; ++j){

				// send requests for ecgs 1 and 2
	
				char buf[MAX_MESSAGE];
				datamsg x(p, (i*.004), j);

				memcpy(buf, &x, sizeof(datamsg));
				chan.cwrite(buf, sizeof(datamsg));
				double reply;
				chan.cread(&reply, sizeof(double));

				//write line to received/x1.csv
	
				out_file << "," << reply;
			}
			out_file << endl;
		}
		out_file.close();
		
	//Task 3:
	
	//Request files
	
	} else if (filename != ""){
		struct timeval start;
		struct timeval end;
		gettimeofday(&start, NULL);
		filemsg fm(0, 0);
		string fname = filename;
		
		int len = sizeof(filemsg) + (fname.size() + 1);
		char* buf2 = new char[len];
		memcpy(buf2, &fm, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), fname.c_str());
		chan.cwrite(buf2, len);

		__int64_t file_length;
		chan.cread(&file_length, sizeof(__int64_t));
		cout << "The length of " << fname << " is " << file_length << endl;

		char* buf3 = new char[m]; // create buffer of size buff capacity (m)

		// loop over the segments in the file file_length / buff capacity(m).
	
		for (__int64_t offset = 0; offset < file_length; offset += m){

			// set filemsg instance
	
			filemsg* file_req = (filemsg*)buf2;

			file_req->offset = offset;//set offset in the file
			file_req->length = min(static_cast<__int64_t>(m), file_length - offset);// set the length

			// send the request (buf2)
	
			chan.cwrite(buf2, len);

			// receive the response

			// cread into buf3 length file_req->length

			chan.cread(buf3, file_req->length);

			// write buf3 into file: received/filename

			ofstream out_file("received/" + fname, ios::binary | ios::app);
			out_file.write(buf3, file_req->length);
			out_file.close();
		}

		delete[] buf2;
		delete[] buf3;
		gettimeofday(&end, NULL);
		double timeTaken;
		timeTaken = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) * 1e-6;
		cout << "Time taken to transfer file: " << timeTaken << " seconds." << endl;
	}
	
	//Task 5:

	// Closing all the channels

	MESSAGE_TYPE q = QUIT_MSG;

    // if necessary, close and delete the new channel

	if (newchan) {
		// do closes and deletes

		channels.back()->cwrite(&q, sizeof(MESSAGE_TYPE));
		delete channels.back();
		channels.pop_back();
	}

    chan.cwrite(&q, sizeof(MESSAGE_TYPE));
}