#include "data.hpp"
#include "/mnt/d/_external_lib/spatial_2.1.8/src/idle_point_multiset.hpp"
#include "/mnt/d/_external_lib/spatial_2.1.8/src/bits/spatial_neighbor.hpp"
#include "/mnt/d/_external_lib/spatial_2.1.8/src/bits/spatial_region.hpp"
#include <random>
#include <chrono>
#include <cfloat>


// log
std::chrono::system_clock::time_point start, end;
double time_local_density_update = 0;
double time_dependency_update = 0;
double time_local_density_update_avg = 0;
double time_dependency_update_avg = 0;
double time_total = 0;
double time_avg = 0;


// definiton of kdtree type
typedef spatial::idle_point_multiset<dimensionality, point> container_type;


// definiton of D-DPC class
class D_DPC {

    // local density array
    std::vector<float> local_density_array;

    // dependent point-distance array
    std::vector<std::pair<unsigned int, float>> dependency_array;

    // kd-tree
    container_type kdtree;

    // max local density & its identifier
    std::pair<float, unsigned int> rho_max = {0,0};

public:

    // constructor
    D_DPC() {}

    // incremental kNN search
    void incremental_knn_search(const unsigned int count_deletion, const unsigned int size, const point &p, bool f) {

        // get id
        const unsigned int idx = p.identifier;

        // init dependent point
        dependency_array[idx] = {idx, FLT_MAX};

        // init count
        unsigned int count = 200;

        // get NN
        spatial::neighbor_iterator<container_type> iter = neighbor_begin(kdtree, p);
                
        while (count > 0) {

            // get id
            const unsigned int id = iter->identifier;

            if (local_density_array[idx] < local_density_array[id]) {
                dependency_array[idx] = {id, distance(iter)};
                break;
            }

            // get next NN
            ++iter;

            // decrement count
            --count;
        }

        // could not find from x-NN -> linear scan
        if (dependency_array[idx].first == idx) {

            for (unsigned int j = 0; j < size; ++j) {

                if (local_density_array[idx] < local_density_array[j]) {

                    // distance computation
                    const float distance = compute_distance(dataset[idx], dataset[j]);
                            
                    // update dependency
                    if (dependency_array[idx].second > distance) dependency_array[idx] = {j, distance};
                    if (f && distance <= cutoff) break;
                }
            }
        }
    }

    // init function
    void initialization() {

        // size
        const unsigned int size = dataset.size() - workload_size;

        // resize & init
        local_density_array.resize(size);
        dependency_array.resize(size);

        // build kd-tree
        start = std::chrono::system_clock::now();

        for (unsigned int i = 0; i < size; ++i) kdtree.insert(dataset[i]);

        end = std::chrono::system_clock::now();
        double t = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	    std::cout << " kd-tree build time: " << t / 1000 << "[msec]\n\n";


        //-------------------------------//
        //---- compute local density ----//
        //-------------------------------//
        start = std::chrono::system_clock::now();

        #pragma omp parallel
	    {
            #pragma omp for schedule(static)
            for (unsigned int i = 0; i < size; ++i) {

                // init local density 
                std::mt19937 mt(i);
                std::uniform_real_distribution<> rnd(0, 0.9999);
                local_density_array[i] = rnd(mt);

                // make MBR
                point low, high;
                for (unsigned int j = 0; j < dimensionality; ++j) {
                    low[j] = dataset[i][j] - cutoff;
                    high[j] = dataset[i][j] + cutoff;
                }

                // range search
                spatial::region_iterator<container_type> itr = region_begin(kdtree, low, high);
                while (itr != region_end(kdtree, low, high)) {
                    if (compute_distance(dataset[i], dataset[itr->identifier]) < cutoff) local_density_array[i] = local_density_array[i] + 1.0;
                    ++itr;
                }
                local_density_array[i] = local_density_array[i] - 1.0;
            }
        }

        end = std::chrono::system_clock::now();
        t = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	    std::cout << " local density comp. time: " << t / 1000 << "[msec]\n\n";

        // update local density max
        for (unsigned int i = 0; i < size; ++i) {
            if (local_density_array[i] > rho_max.first) rho_max = {local_density_array[i], i};
        }


        //---------------------------------//
        //---- compute dependent point ----//
        //---------------------------------//
        start = std::chrono::system_clock::now();

        #pragma omp parallel
	    {
            #pragma omp for schedule(static)
            for (unsigned int i = 0; i < size; ++i) {

                // inc kNN search
                incremental_knn_search(0, size, dataset[i], 0);
                //incremental_knn_search(0, size, dataset[i], 1);
            }
        }

        end = std::chrono::system_clock::now();
        t = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
	    std::cout << " dependent point comp. time: " << t / 1000 << "[msec]\n\n";
	}

    // insertion case function
    void insertion(const unsigned int count_insertion, const unsigned int count_deletion) {

        std::cout << " Insertion\n";

        // size
        const unsigned int size = dataset.size() - workload_size + count_insertion;


        //------------------------------//
        //---- update local density ----//
        //------------------------------//
        start = std::chrono::system_clock::now();

        // update local density & dependency arrays
        std::mt19937 mt(size);
		std::uniform_real_distribution<> rnd(0, 0.9999);
        local_density_array.push_back(rnd(mt));
        dependency_array.push_back({size - 1, FLT_MAX});

        // flag
        std::vector<bool> flag_set(size);
        flag_set[size - 1] = 1;

        // make MBR
        point low, high;
        for (unsigned int j = 0; j < dimensionality; ++j) {
            low[j] = dataset[size - 1][j] - cutoff;
            high[j] = dataset[size - 1][j] + cutoff;
        }

        float local_density_max = 0;
        float local_density_min = FLT_MAX;
        std::vector<unsigned int> neighbor;

        // range search
        spatial::region_iterator<container_type> itr = region_begin(kdtree, low, high);
        while (itr != region_end(kdtree, low, high)) {
            if (compute_distance(dataset[size - 1], dataset[itr->identifier]) < cutoff) {

                // insert into neighbor
                neighbor.push_back(itr->identifier);

                // update flag
                flag_set[itr->identifier] = 1;

                // update local density (min, max)
                local_density_array[itr->identifier] = local_density_array[itr->identifier] + 1.0;
                if (local_density_max < local_density_array[itr->identifier]) local_density_max = local_density_array[itr->identifier];
                if (local_density_min > local_density_array[itr->identifier]) local_density_min = local_density_array[itr->identifier];
                if (rho_max.first < local_density_array[itr->identifier]) rho_max = {local_density_array[itr->identifier], itr->identifier};

                // update local density of new point
                local_density_array[size - 1] = local_density_array[size - 1] + 1.0;
            }
            ++itr;
        }
        if (local_density_max < local_density_array[size - 1]) local_density_max = local_density_array[size - 1];
        if (local_density_min > local_density_array[size - 1]) local_density_min = local_density_array[size - 1];
        if (rho_max.first < local_density_array[size - 1]) rho_max = {local_density_array[size - 1], size - 1};
        neighbor.push_back(size - 1);

        // get local density of new point
        const float rho = local_density_array[size - 1];

        // update kd-tree
        kdtree.insert(dataset[size - 1]);

        end = std::chrono::system_clock::now();
        double t = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        time_local_density_update += t / 1000;
        time_local_density_update_avg += t / 1000;
        time_total += t / 1000;
        time_avg += t / 1000;
        std::cout << " " << count_deletion + count_insertion << "-th local density update time: " << t / 1000 << "[msec]\t";
        std::cout << "local density: " << local_density_array[size - 1] << "\n";


        //--------------------------------//
        //---- update dependent point ----//
        //--------------------------------//
        start = std::chrono::system_clock::now();

        #pragma omp parallel num_threads(thread_number)
	    {
            #pragma omp for schedule(static)
            for (unsigned int i = count_deletion; i < size; ++i) {

                if (flag_set[i]) {

                    // inside case
                    
                    // get dependent point
                    const unsigned int idx = dependency_array[i].first;

                    bool f = 0;
                    if (i == idx) {
                        f = 1;
                    }
                    else {
                        if (local_density_array[i] >= local_density_array[idx]) f = 1;
                    }

                    if (f) {

                        // inc kNN search
                        if (i != rho_max.second) {

                            // inc kNN search
                            incremental_knn_search(count_deletion, size, dataset[i], 0);
                        }
                    }
                    else {

                        if (local_density_array[i] < rho) {

                            // distance computation
                            const float distance = compute_distance(dataset[i], dataset[size - 1]);

                            // update dependency
                            if (dependency_array[i].second > distance) dependency_array[i] = {size - 1, distance};
                        }
                    }
                }
                else {

                    // outside case
                    bool f = 1;
                    const float rho_ = local_density_array[i];
                    if (rho_ >= local_density_max) {
                        f = 0;
                    }
                    else if (rho_ < local_density_min - 1.0) {
                        f = 0;
                    }

                    if (f) {

                        // try triangle inequality
                        const float dist = compute_distance(dataset[i], dataset[size - 1]);
                        
                        if (dependency_array[i].second > dist - cutoff) {
 
                            for (unsigned int j = 0; j < neighbor.size(); ++j) {

                                // get idx
                                const unsigned int idx = neighbor[j];

                                if (rho_ < local_density_array[idx]) {

                                    // distance computation
                                    const float distance = compute_distance(dataset[i], dataset[idx]);

                                    // update dependency
                                    if (dependency_array[i].second > distance) dependency_array[i] = {idx, distance};
                                }
                            }
                        }
                    }
                }
            }
        }

        end = std::chrono::system_clock::now();
        t = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        time_dependency_update += t / 1000;
        time_dependency_update_avg += t / 1000;
        time_total += t / 1000;
        time_avg += t / 1000;
        std::cout << " " << count_deletion + count_insertion << "-th dependency update time: " << t / 1000 << "[msec]\n\n";
    }

    // deletion case function
    void deletion(const unsigned int count_insertion, const unsigned int count_deletion) {

        std::cout << " Deletion\n";

        // size
        const unsigned int size = dataset.size() - workload_size + count_insertion;


        //------------------------------//
        //---- update local density ----//
        //------------------------------//
        start = std::chrono::system_clock::now();

        // flag
        std::vector<bool> flag_set(size);

        // get idx
        const unsigned int idx = count_deletion - 1;

        // remove from kd-tree
        auto iter = kdtree.find(dataset[idx]);
		while (1) {
			if (iter->identifier == idx) {
				kdtree.erase(iter);
				break;
			}
			++iter;
		}        

        // make MBR
        point low, high;
        for (unsigned int j = 0; j < dimensionality; ++j) {
            low[j] = dataset[idx][j] - cutoff;
            high[j] = dataset[idx][j] + cutoff;
        }

        // range search
        bool flag = 0;
        unsigned int cnt = 0;
        spatial::region_iterator<container_type> itr = region_begin(kdtree, low, high);
        while (itr != region_end(kdtree, low, high)) {
            if (compute_distance(dataset[idx], dataset[itr->identifier]) < cutoff) {

                if (itr->identifier == rho_max.second) flag = 1;

                // update flag
                flag_set[itr->identifier] = 1;

                // update local density (min, max)
                local_density_array[itr->identifier] = local_density_array[itr->identifier] - 1.0;
                ++cnt;
            }
            ++itr;
        }

        if (flag) {
            rho_max = {0,0};
            for (unsigned int i = count_deletion; i < size; ++i) {
                if (rho_max.first < local_density_array[i]) rho_max = {local_density_array[i], i};
            }
        }

        // init local density
        local_density_array[idx] = 0;

        end = std::chrono::system_clock::now();
        double t = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        time_local_density_update += t / 1000;
        time_local_density_update_avg += t / 1000;
        time_total += t / 1000;
        time_avg += t / 1000;
        std::cout << " " << count_deletion + count_insertion << "-th local density update time: " << t / 1000 << "[msec]\t";
        std::cout << "local density: " << cnt << "\n";


        //--------------------------------//
        //---- update dependent point ----//
        //--------------------------------//
        start = std::chrono::system_clock::now();

        #pragma omp parallel num_threads(thread_number)
	    {
            #pragma omp for schedule(static)
            for (unsigned int i = count_deletion; i < size; ++i) {

                // get dependent point
                const unsigned int id = dependency_array[i].first;

                bool f = 0;
                if (flag_set[i] && id != idx) f = 1;

                if (f) {
                        
                    // make MBR
                    const float dep_dist = dependency_array[i].second;
                    point l, h;
                    for (unsigned int j = 0; j < dimensionality; ++j) {
                        l[j] = dataset[i][j] - dep_dist;
                        h[j] = dataset[i][j] + dep_dist + 0.1;
                    }

                    // range search
                    spatial::region_iterator<container_type> iter = region_begin(kdtree, l, h);
                    while (itr != region_end(kdtree, l, h)) {

                        const float distance = compute_distance(dataset[i], dataset[iter->identifier]);
                        if (distance <= dep_dist) {
                            if (local_density_array[i] < local_density_array[iter->identifier]) {
                                if (distance < dep_dist) dependency_array[i] = {iter->identifier, distance};
                            }
                        }
                        ++itr;
                    }
                }
                else {

                    if (id == idx) {
                        f = 1;
                    }
                    else if (flag_set[id]) {
                        if (local_density_array[i] >= local_density_array[id]) {
                            if (i != rho_max.second) f = 1;
                        }
                    }

                    if (f) {

                        incremental_knn_search(count_deletion, size, dataset[i], 0);
                    }
                }
            }
        }

        end = std::chrono::system_clock::now();
        t = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        time_dependency_update += t / 1000;
        time_dependency_update_avg += t / 1000;
        time_total += t / 1000;
        time_avg += t / 1000;
        std::cout << " " << count_deletion + count_insertion << "-th dependency update time: " << t / 1000 << "[msec]\n\n";
    }

    // file output
    void output_file(bool flag, const unsigned int count, const unsigned int interval) {

        std::string f_name = "result/id(" + std::to_string(dataset_id) + ")_cutoff(" + std::to_string(cutoff) + ")_deletion-rate(" + std::to_string(deletion_rate) + ")_thread-number(" + std::to_string(thread_number) + ")_dexdpc.txt";
        std::ofstream file;
        file.open(f_name.c_str(), std::ios::out | std::ios::app);

        if (file.fail()) {
            std::cerr << " cannot open the output file." << std::endl;
            file.clear();
            return;
        }

        if (flag) {
            file << count / interval << "," << time_avg / interval << "," << time_local_density_update_avg / interval << "," << time_dependency_update_avg / interval << "\n";
        }
        else {
            file << count << "," << time_total << "," << time_local_density_update << "," << time_dependency_update << "\n";
        }

        time_avg = 0;
        time_local_density_update_avg = 0;
        time_dependency_update_avg = 0;

        file.close();
    }

};