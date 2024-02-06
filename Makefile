clean:
	rm -f bruteforce
	rm -f bruteforce.o
	rm -rf results/
	rm -f result_*.png

clean-py:
	rm -f results/results.csv
	rm -f result_*.png

test-cpp:
	h5c++ bruteforce.cpp -o bruteforce
	./bruteforce -df ../datasets/laion2B-en-hammingv2-n=100K.h5 -qf ../datasets/public-queries-10k-hammingv2.h5 -k 100 -s 100K

test-py:
	python3 eval/eval.py results/results.csv
	python3 eval/plot.py results/results.csv

test:
	h5c++ bruteforce.cpp -o bruteforce
	./bruteforce -df ../datasets/laion2B-en-hammingv2-n=100K.h5 -qf ../datasets/public-queries-10k-hammingv2.h5 -k 100 -s 100K
	python3 eval/eval.py results/results.csv
	python3 eval/plot.py results/results.csv
