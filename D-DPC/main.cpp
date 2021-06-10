#include "ddpc.hpp"


const unsigned int interval = 100;


int main() {
    
    // input parameter
    input_parameter();

    // input data
    input_data();

    std::cout << " ------------------\n";
	std::cout << " data id: " << dataset_id << "\n";
	std::cout << " dimensionality: " << dimensionality << "\n";
	std::cout << " cardinality: " << dataset.size() << "\n";
	std::cout << " cutoff-disntance: " << cutoff << "\n";
	std::cout << " #threads: " << thread_number << "\n";
    std::cout << " deletion rate: " << deletion_rate << "\n";
	std::cout << " ------------------\n\n";

    // display current time
    get_current_time();

    // make D-DPC instance
    D_DPC ddpc;

    //--------------------------//
    //----- Initialization -----//
    //--------------------------//
    ddpc.initialization();


    //---------------------------//
    //---- test dynamic data ----//
    //---------------------------//
    unsigned int count_insertion = 0;
    unsigned int count_deletion = 0;

    // random generator
    std::mt19937 mt(0);
	std::uniform_real_distribution<> rnd(0, 1.0);

    while (count_deletion + count_insertion < workload_size) {

        // determine insertion or deletion
        bool flag = 1;
        if (rnd(mt) <= deletion_rate) flag = 0;

        if (flag) {

            // increment count_insertion
            ++count_insertion;

            // call insertion case
            ddpc.insertion(count_insertion, count_deletion);
        }
        else {

            // increment count_deletion
            ++count_deletion;

            // call deletion case
            ddpc.deletion(count_insertion, count_deletion);
        }

        // file output
        if ((count_deletion + count_insertion) % interval  == 0) {

            std::cout << " " << count_deletion + count_insertion << " updates are over at";
            get_current_time();
            
            ddpc.output_file(1, count_deletion + count_insertion, interval);
        }
    }

    // file output
    ddpc.output_file(0, 0, 0);


    return 0;
}