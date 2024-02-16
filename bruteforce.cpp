#include <iostream>
#include <argparse/argparse.hpp>
#include <H5Cpp.h>
#include <queue>
#include <cassert>
#include <filesystem>
#include <string>
#include <vector>
#include <bits/stdc++.h>
#include <chrono>
#include <immintrin.h>

#define KNNS_LABEL "knns"
#define DIST_LABEL "dist"
typedef uint64_t dist_t;

using namespace std;
using namespace std::filesystem;
using namespace std::chrono;

typedef pair<dist_t, int> result_t;

const vector<string> sizes{"10K", "100K", "1M", "30M", "10M", "100M"};

uint64_t distance(uint64_t *a, uint64_t *b, int n) {
	uint64_t dist = 0;
	for (int i = 0; i < n; i++) {
		dist += __builtin_popcountll(a[i] ^ b[i]);
	}
	return dist;
}

uint64_t sumOfCounts(__m256i vec) {
	uint64_t dists[4];
	_mm256_store_si256((__m256i*)dists, vec);
	uint64_t sum{0};
	sum += __builtin_popcountll(dists[0]);
	sum += __builtin_popcountll(dists[1]);
	sum += __builtin_popcountll(dists[2]);
	sum += __builtin_popcountll(dists[3]);
	return sum;
}
uint64_t distance_simd(uint64_t *a, uint64_t *b, int n) {
	// Load data into vectors
	__m256i a1 = _mm256_loadu_si256((__m256i *)&(a[4 * 0]));
	__m256i a2 = _mm256_loadu_si256((__m256i *)&(a[4 * 1]));
	__m256i a3 = _mm256_loadu_si256((__m256i *)&(a[4 * 2]));
	__m256i a4 = _mm256_loadu_si256((__m256i *)&(a[4 * 3]));

	__m256i b1 = _mm256_loadu_si256((__m256i *)&(b[4 * 0]));
	__m256i b2 = _mm256_loadu_si256((__m256i *)&(b[4 * 1]));
	__m256i b3 = _mm256_loadu_si256((__m256i *)&(b[4 * 2]));
	__m256i b4 = _mm256_loadu_si256((__m256i *)&(b[4 * 3]));

	// a XOR b
	__m256i d1 = _mm256_xor_si256(a1, b1);
	__m256i d2 = _mm256_xor_si256(a2, b2);
	__m256i d3 = _mm256_xor_si256(a3, b3);
	__m256i d4 = _mm256_xor_si256(a4, b4);

	// Sum popcounts
	uint64_t dist{0};
	dist += sumOfCounts(d1);
	dist += sumOfCounts(d2);
	dist += sumOfCounts(d3);
	dist += sumOfCounts(d4);

	return dist;
}

double cosine_distance(uint64_t *a, uint64_t *b, int n) {
	uint32_t prod = 0;
	uint32_t lenA = 0;
	uint32_t lenB = 0;
	for (int i = 0; i < n; i++) {
		prod += __builtin_popcountll(a[i] & b[i]);
		lenA += __builtin_popcountll(a[i]);
		lenB += __builtin_popcountll(b[i]);
	}

	double P = (double)prod;
	double A = (double)lenA;
	double B = (double)lenB;
	return 1.0 - P / (sqrt(A) * sqrt(B));
}

result_t *bruteforce(uint64_t *data, hsize_t rows, hsize_t cols, int k, uint64_t *query) {
	priority_queue<result_t, std::vector<result_t>, std::greater<result_t>> pq;
	for (int i = 0; i < rows; i++) {
		dist_t dist = distance(&data[i * cols], query, cols);
		result_t res(dist, i);
		pq.push(res);
	}

	result_t *result = (result_t*)malloc(sizeof(result_t) * k);
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

	uint64_t *data = new uint64_t[rows * cols];
	dataset.read(data, H5::PredType::NATIVE_UINT64);

	uint64_t *queries = new uint64_t[query_rows * query_cols];
	queryset.read(queries, H5::PredType::NATIVE_UINT64);

	cout << "Files read successfully" << endl;

	uint64_t *knns = (uint64_t *)malloc(query_rows * k * sizeof(uint64_t));
	dist_t *dist = (dist_t *)malloc(query_rows * k * sizeof(dist_t));

	auto start = high_resolution_clock::now();

	for (int i = 0; i < query_rows; i++) {
		if (i % 100 == 0) {
			cout << "Processed " << i << " queries" << endl;
		}
		result_t *result = bruteforce(data, rows, cols, k, &(queries[i * query_cols]));
		for (int j = 0; j < k; j++) {
			dist[i * k + j] = result[j].first;
			knns[i * k + j] = result[j].second + 1;
		}
		free(result);
	}

	auto stop = high_resolution_clock::now();
	auto querytime = duration_cast<microseconds>(stop - start);

	if (!filesystem::exists("results/"))
		filesystem::create_directory("results/");

	H5::H5File result_file("results/result.h5", H5F_ACC_TRUNC);
	
	H5::Attribute data_attr = result_file.createAttribute("data", H5::StrType(H5::PredType::C_S1, 256), H5::DataSpace(H5S_SCALAR));
	data_attr.write(H5::StrType(H5::PredType::C_S1, 256), "hamming");

	H5::Attribute size_attr = result_file.createAttribute("size", H5::StrType(H5::PredType::C_S1, 256), H5::DataSpace(H5S_SCALAR));
	size_attr.write(H5::StrType(H5::PredType::C_S1, 256), size);

	H5::Attribute algo_attr = result_file.createAttribute("algo", H5::StrType(H5::PredType::C_S1, 256), H5::DataSpace(H5S_SCALAR));
	algo_attr.write(H5::StrType(H5::PredType::C_S1, 256), "bruteforce");

	H5::Attribute buildtime_attr = result_file.createAttribute("buildtime", H5::PredType::NATIVE_DOUBLE, H5::DataSpace(H5S_SCALAR));
	buildtime_attr.write(H5::PredType::NATIVE_DOUBLE, to_string(0.0));
	cout << "Build time: " << 0.0 << endl;

	H5::Attribute querytime_attr = result_file.createAttribute("querytime", H5::PredType::NATIVE_DOUBLE, H5::DataSpace(H5S_SCALAR));
	querytime_attr.write(H5::PredType::NATIVE_DOUBLE, to_string(querytime.count() / 1000000.0));
	cout << "Query time: " << querytime.count() / 1000000.0 << endl;

	H5::Attribute params_attr = result_file.createAttribute("params", H5::StrType(H5::PredType::C_S1, 256), H5::DataSpace(H5S_SCALAR));
	params_attr.write(H5::StrType(H5::PredType::C_S1, 256), to_string(k).c_str());

	hsize_t knns_dims[2] = {query_rows, (hsize_t)k};
	hsize_t dist_dims[2] = {query_rows, (hsize_t)k};
	H5::DataSet knns_set = result_file.createDataSet(KNNS_LABEL, H5::PredType::NATIVE_UINT64, H5::DataSpace(2, knns_dims));
	H5::DataSet dist_set = result_file.createDataSet(DIST_LABEL, H5::PredType::NATIVE_UINT64, H5::DataSpace(2, dist_dims));
	knns_set.write(knns, H5::PredType::NATIVE_UINT64);
	dist_set.write(dist, H5::PredType::NATIVE_UINT64);
}
