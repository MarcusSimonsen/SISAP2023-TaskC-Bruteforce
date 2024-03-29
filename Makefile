clean:
	rm -f bruteforce
	rm -f bruteforce.o
	sudo rm -rf results/
	rm -f result_*.png
	rm -f perf.data

build:
	h5c++ bruteforce.cpp -o bruteforce -O2 -march=native

test:
	make build
	./bruteforce -df ../datasets/laion2B-en-hammingv2-n=100K.h5 -qf ../datasets/public-queries-10k-hammingv2.h5 -k 10 -s 100K

py:
	python3 eval/eval.py results/results.csv
	python3 eval/plot.py results/results.csv

run-10M:
	make build
	./bruteforce -df ../datasets/laion2B-en-hammingv2-n=10M.h5 -qf ../datasets/public-queries-10k-hammingv2.h5 -k 10 -s 10M

run-30M:
	make build
	./bruteforce -df ../datasets/laion2B-en-hammingv2-n=30M.h5 -qf ../datasets/public-queries-10k-hammingv2.h5 -k 10 -s 30M

run-100M:
	make build
	./bruteforce -df ../datasets/laion2B-en-hammingv2-n=100M.h5 -qf ../datasets/public-queries-10k-hammingv2.h5 -k 10 -s 100M

record:
	make build
	sudo perf record -g ./bruteforce -df ../datasets/laion2B-en-hammingv2-n=100K.h5 -qf ../datasets/public-queries-10k-hammingv2.h5 -k 10 -s 100K

report:
	sudo perf report -g 'graph,0.5,caller'
