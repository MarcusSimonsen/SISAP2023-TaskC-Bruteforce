# Performance testing notes on CPP bruteforce implementation

| Testing                    | \#Queries | Size | Time    |
|----------------------------|-----------|------|---------|
| cosine (double)            | 100       | 10M  | 1006.46 |
| hamming - uint64 (ui64)    | 100       | 10M  | 874.33  |
| hamming - uint16 (ui16)    | 100       | 10M  | 873.185 |
| hamming - ui64, no pop(np) | 100       | 10M  | 313.141 |
| hamming - ui64,O2,np       | 10000     | 100K | 93.1629 |
| hamming - ui64,O2,np, simd | 10000     | 100K | 45.6273 |
