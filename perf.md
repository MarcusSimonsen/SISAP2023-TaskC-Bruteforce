# Performance testing notes on CPP bruteforce implementation

| Testing            | \#Queries | Size | Time    |
|--------------------|-----------|------|---------|
| cosine (double)    | 100       | 10M  | 1006.46 |
| hamming (uint64)   | 100       | 10M  | 874.33  |
| hamming (uint16)   | 100       | 10M  | 873.185 |
| hamming (no pop)   | 100       | 10M  | 313.141 |
| hamming - O2       | 10000     | 100K | 94.1264 |
| hamming - O2,nheap | 10000     | 100K | 70.2554 |
