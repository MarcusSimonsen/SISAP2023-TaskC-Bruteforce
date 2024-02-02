#include <iostream>
#include <argparse/argparse.hpp>
#include <H5Cpp.h>
#include <queue>
#include <cassert>
#include <filesystem>
#include <string>
#include <vector>

using namespace std;
using namespace std::filesystem;

typedef pair<int, int> result_t;

const uint32_t m1  = 0x55555555; //binary: 0101...
const uint32_t m2  = 0x33333333; //binary: 00110011..
const uint32_t m4  = 0x0f0f0f0f; //binary:  4 zeros,  4 ones ...
const uint32_t m8  = 0x00ff00ff; //binary:  8 zeros,  8 ones ...
const uint32_t m16 = 0x0000ffff; //binary: 16 zeros, 16 ones ...
								 //
const vector<string> sizes{"10K", "100K", "1M", "30M", "10M", "100M"};

/* 
 * See wiki page for Hamming weight
 * https://en.wikipedia.org/wiki/Hamming_weight
 */
int count(float x) {
	uint32_t n = *(reinterpret_cast<uint32_t *>(&x));
	n = (n & m1 ) + ((n >>  1) & m1 );
	n = (n & m2 ) + ((n >>  2) & m2 );
	n = (n & m4 ) + ((n >>  4) & m4 );
	n = (n & m8 ) + ((n >>  8) & m8 );
	n = (n & m16) + ((n >> 16) & m16);
	return n;
}

float distance(float *a, float *b, int n) {
	int dist = 0;
	for (int i = 0; i < n; i++) {
		dist += count((*(reinterpret_cast<uint32_t *>(&(a[i])))) ^ (*(reinterpret_cast<uint32_t *>(&(b[i])))));
	}
	return dist;
}

result_t *bruteforce(float *data, hsize_t rows, hsize_t cols, int k, float *query) {
	priority_queue<pair<float, int>> pq;
	for (int i = 0; i < rows; i++) {
		float dist = distance(data + i * cols, query, cols);
		pq.push({dist, i});
		if (pq.size() > k) {
			pq.pop();
		}
	}

	result_t *result = new result_t[k];
	for (int i = k - 1; i >= 0; i--) {
		result[i] = pq.top();
		pq.pop();
	}

	return result;
}

int main(int argc, char *argv[]) {
	argparse::ArgumentParser program("bruteforce-simple");

	program.add_argument("--datafile", "-df")
		.required()
		.help("Path to the data file");
	program.add_argument("--queryfile", "-qf")
		.required()
		.help("Path to the query file");
	program.add_argument("-k")
		.default_value(30)
		.scan<'i', int>()
		.help("Number of nearest neighbors to be found");
	program.add_argument("--size", "-s")
		.required()
		.help("Size of the dataset");

	try {
		program.parse_args(argc, argv);
	}
	catch (const exception& err) {
		cerr << err.what() << endl;
		cerr << program;
		exit(1);
	}

	auto datafile = program.get<string>("--datafile");
	auto queryfile = program.get<string>("--queryfile");
	auto k = program.get<int>("-k");
	auto size = program.get<string>("--size");

	assert(filesystem::exists(datafile));
	assert(filesystem::exists(queryfile));
	assert(k > 0);
	assert(find(sizes.begin(), sizes.end(), size) != sizes.end());

	cout << "Reading data from file: " << datafile << endl;
	cout << "Reading queries from file: " << queryfile << endl;
	cout << "Finding " << k << " nearest neighbors" << endl;
	cout << "Size of the dataset: " << size << endl;


	H5::H5File h5datafile(datafile, H5F_ACC_RDONLY);
	H5::DataSet dataset = h5datafile.openDataSet("hamming");
	H5::DataSpace dataspace = dataset.getSpace();

	H5::H5File h5queryfile(queryfile, H5F_ACC_RDONLY);
	H5::DataSet queryset = h5queryfile.openDataSet("hamming");
	H5::DataSpace queryspace = queryset.getSpace();

	hsize_t dims_out[2];
	hsize_t query_dims_out[2];

	dataspace.getSimpleExtentDims(dims_out, NULL);
	queryspace.getSimpleExtentDims(query_dims_out, NULL);

	hsize_t rows = dims_out[0];
	hsize_t cols = dims_out[1];
	hsize_t query_rows = query_dims_out[0];
	hsize_t query_cols = query_dims_out[1];

	cout << "Data has " << rows << " rows and " << cols << " columns" << endl;
	cout << "Queries have " << query_rows << " rows and " << query_cols << " columns" << endl;

	assert(cols == query_cols);

	float *data = new float[rows * cols];
	dataset.read(data, H5::PredType::NATIVE_FLOAT);
	float *queries = new float[query_rows * query_cols];
	queryset.read(queries, H5::PredType::NATIVE_FLOAT);

	cout << "Files read successfully" << endl;

	int knns[query_rows];
	int dists[query_rows];

	for (int i = 0; i < query_rows; i++) {
		if (i % 100 == 0) {
			cout << "Processed " << i << " queries" << endl;
		}
		result_t *result = bruteforce(data, rows, cols, k, &queries[i * query_cols]);
		for (int j = 0; j < k; j++) {
			dists[i * query_cols + j] = result[j].first;
			knns[i * query_cols + j] = result[j].second + 1;
		}
	}

	if (!filesystem::exists("results/"))
		filesystem::create_directory("results/");

	H5::H5File result_file("results/result.h5", H5F_ACC_TRUNC);
	
	H5::Attribute data_attr = result_file.createAttribute("data", H5::StrType(H5::PredType::C_S1, 256), H5::DataSpace(H5S_SCALAR));
	data_attr.write(H5::StrType(H5::PredType::C_S1, 256), "hamming");
/*
	H5::Attribute size_attr = result_file.createAttribute("size", H5::PredType::NATIVE_INT, H5::DataSpace(H5S_SCALAR));
	size_attr.write(H5::StrType(H5::PredType::C_S1), string{size}.c_str());
*/
	H5::Attribute algo_attr = result_file.createAttribute("algo", H5::StrType(H5::PredType::C_S1, 256), H5::DataSpace(H5S_SCALAR));
	algo_attr.write(H5::StrType(H5::PredType::C_S1, 256), "bruteforce");

	H5::Attribute buildtime_attr = result_file.createAttribute("buildtime", H5::PredType::NATIVE_FLOAT, H5::DataSpace(H5S_SCALAR));
	buildtime_attr.write(H5::PredType::NATIVE_FLOAT, "0.0");

	H5::Attribute querytime_attr = result_file.createAttribute("querytime", H5::PredType::NATIVE_FLOAT, H5::DataSpace(H5S_SCALAR));
	querytime_attr.write(H5::PredType::NATIVE_FLOAT, "0.0");

	H5::Attribute params_attr = result_file.createAttribute("params", H5::StrType(H5::PredType::C_S1, 256), H5::DataSpace(H5S_SCALAR));
	params_attr.write(H5::StrType(H5::PredType::C_S1, 256), to_string(k).c_str());

	hsize_t knn_dims[2] = {query_rows, k};
	H5::DataSet knn_set = result_file.createDataSet("knn", H5::PredType::NATIVE_INT, H5::DataSpace(2, knn_dims));
	
	knn_set.write(knns, H5::PredType::NATIVE_INT);
}
