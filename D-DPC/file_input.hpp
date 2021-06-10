#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <random>
#include <time.h>


// dataset identifier
unsigned int dataset_id = 0;

// data dimensionality
const unsigned int dimensionality = 8;

// cutoff
float cutoff = 0;

// #threads
unsigned int thread_number = 0;

// deletion rate
float deletion_rate = 0;

// workload size
const unsigned int workload_size = 20000;


// get current time
void get_current_time() {

	time_t t = time(NULL);
	printf(" %s\n\n", ctime(&t));
}

// parameter input
void input_parameter() {

	std::ifstream ifs_dataset_id("parameter/dataset_id.txt");
	std::ifstream ifs_cutoff("parameter/cutoff.txt");
	std::ifstream ifs_thread("parameter/thread_number.txt");
	std::ifstream ifs_deletion("parameter/deletion_rate.txt");

	if (ifs_dataset_id.fail()) {
		std::cout << " dataset_id.txt does not exist." << std::endl;
		std::exit(0);
	}
	if (ifs_cutoff.fail()) {
		std::cout << " cutoff.txt does not exist." << std::endl;
		std::exit(0);
	}
	if (ifs_thread.fail()) {
		std::cout << " thread_num.txt does not exist." << std::endl;
		std::exit(0);
	}
	if (ifs_deletion.fail()) {
		std::cout << " deletion_rate.txt does not exist." << std::endl;
		std::exit(0);
	}

	while (!ifs_dataset_id.eof()) { ifs_dataset_id >> dataset_id; }
	while (!ifs_cutoff.eof()) { ifs_cutoff>> cutoff; }
	while (!ifs_thread.eof()) { ifs_thread >> thread_number; }
	while (!ifs_deletion.eof()) { ifs_deletion >> deletion_rate; }

	// set dimensionality
	//if (dataset_id == 0) dimensionality = 3;
	//if (dataset_id == 1) dimensionality = 4;
	//if (dataset_id == 2) dimensionality = 4;
	//if (dataset_id == 3) dimensionality = 8;
}