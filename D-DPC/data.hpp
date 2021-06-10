#include <vector>
#include <array>
#include "file_input.hpp"


// definition of my point
class point : public std::array<float, dimensionality> {

public:

	// identifier
	unsigned int identifier = 0;
};



// definition of dataset
std::vector<point> dataset;

// definition of workload
std::vector<point> workload;


// input data
void input_data() {

	// id variable
	unsigned int id = 0;

	// point coordinates variable
	point pt;
    //pt.resize(dimensionality);

    // dataset input
	std::string f_name = "/mnt/d/_dataset/";
	if (dataset_id == 0) f_name += "airline_3d.txt";
    if (dataset_id == 1) f_name += "household_4d.txt";
	if (dataset_id == 2) f_name += "pamap2_4d.txt";
	if (dataset_id == 3) f_name += "sensor_8d.txt";

    // file input
	std::ifstream ifs_file(f_name);
    std::string full_data;

	// error check
	if (ifs_file.fail()) {
		std::cout << " data file does not exist." << std::endl;
		std::exit(0);
	}

	// read data
	while (std::getline(ifs_file, full_data)) {

		std::string meta_info;
		std::istringstream stream(full_data);
		std::string type = "";

		for (unsigned int i = 0; i < dimensionality; ++i) {

			std::getline(stream, meta_info, ',');
			std::istringstream stream_(meta_info);
			long double val = std::stold(meta_info);
			pt[i] = val;
		}

		// update id
		pt.identifier = id;

		// insert into dataset
		dataset.push_back(pt);

		// increment identifier
		++id;
	}

	// get workload
	for (unsigned int i = dataset.size() - workload_size; i < dataset.size(); ++i) workload.push_back(dataset[i]);
}


// distance computation
float compute_distance(const point &p, const point &q) {

	float distance = 0;
	for (unsigned int i = 0; i < dimensionality; ++i) {

		const float temp = p[i] - q[i];
		distance += temp * temp;
	}

	return sqrt(distance);
}