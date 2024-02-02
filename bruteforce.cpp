#include <iostream>
#include <argparse/argparse.hpp>
#include <H5Cpp.h>
#include <queue>
#include <cassert>

using namespace std;

typedef pair<int, int> result_t;

const uint32_t m1  = 0x55555555; //binary: 0101...
const uint32_t m2  = 0x33333333; //binary: 00110011..
const uint32_t m4  = 0x0f0f0f0f; //binary:  4 zeros,  4 ones ...
const uint32_t m8  = 0x00ff00ff; //binary:  8 zeros,  8 ones ...
const uint32_t m16 = 0x0000ffff; //binary: 16 zeros, 16 ones ...

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

	cout << "Reading data from file: " << datafile << endl;
	cout << "Reading queries from file: " << queryfile << endl;
	cout << "Finding " << k << " nearest neighbors" << endl;


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

	result_t *result = bruteforce(data, rows, cols, k, queries);

	cout << "Nearest neighbors found:" << endl;
	for (int i = 0; i < k; i++) {
		cout << result[i].second << " with distance " << result[i].first << endl;
	}
}
