./vmsim -n 8 -a nru -r 40 bzip.trace > bzip.8.nru.40.txt
./vmsim -n 16 -a nru -r 132 bzip.trace > bzip.16.nru.132.txt
./vmsim -n 32 -a nru -r 320 bzip.trace > bzip.32.nru.320.txt
./vmsim -n 64 -a nru -r 630 bzip.trace > bzip.64.nru.630.txt
./vmsim -n 128 -a nru -r 2050 bzip.trace > bzip.128.nru.2050.txt

echo "PERIOD,FRAMES,PAGE FAULTS,DISK WRITES" >> bzip.nru.csv

tail -n 1 bzip.8*.txt >> bzip.nru.csv
tail -n 1 bzip.16*.txt >> bzip.nru.csv
tail -n 1 bzip.32*.txt >> bzip.nru.csv
tail -n 1 bzip.64*.txt >> bzip.nru.csv
tail -n 1 bzip.128*.txt >> bzip.nru.csv

