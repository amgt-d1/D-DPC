## Introduction
* This repository provides implementations of D-DPC.
* This is a fast algorithm for [density-peaks clustering](https://science.sciencemag.org/content/344/6191/1492.full) (proposed in Science) on fully dynamic data.
* As for the details about this algorithm, please read our paper, [Fast Density-Peaks Clustering for Static and Dynamic Data in
Euclidean Spaces](https://).

## Requirement
*  [spatial library](http://spatial.sourceforge.net/)
	* We used version 2.1.8.
	* Path to this library should be changed based on your environment, see `ddpc.hpp`.
* The source codes of D-DPC have to be changed based on your paths of the above library.

## How to use
* We prepared codes for Ubuntu (20.04 LTS).
* We assume low-dimensional datasets, as we use a kd-tree.
	* Set dimensionality in `file_input.hpp`.
* Compile: `g++ -O3 main.cpp -o ddpc.out -fopenmp`.
* Run: `./ddpc.out`.

### Datasets
* Our code inputs `csv` file, where each row shows a vector whose elements are separeted by `,`.
  * For example, one 3-dimensional data is described by `1.0,10,2.5`.
* For your data, assign `dataset_id` as written in `input_data()` function of `data.hpp` to read your dataset.
* The path of your dataset can be freely changed in `data.hpp`, see `input_data()` function.
* To simulate random point insertions and deletions, we sorted a given dataset by a random order. If you need to remove arbitrary points, please change our codes.

### Parameters
* Set some value in the corresponding txt file in `parameter`.

### Notes
* Initialization is done by Ex-DPC+ like algorithm (the dependent point of each point is computed by incremental kNN search).
	* `initialization()` function is much similar to Ex-DPC+ (the difference is that we do not compute the kNNs of each point offline).
	* Therefore, coding Ex-DPC+ from `initialization()` is straightforward.

### Citation
If you use our implementation, please cite the following paper.
``` 
@article{amagata2021ddpc,  
    title={Fast Density-Peaks Clustering for Static and Dynamic Data in Euclidean Spaces},  
    author={Amagata, Daichi and Hara, Takahiro},  
    booktitle={},  
    pages={xxx--xxx},  
    year={xxx}  
}
```

## License
Copyright (c) 2021 Daichi Amagata  
This software is released under the [MIT license](https://github.com/amgt-d1/DPC/blob/main/LICENSE).
