#!/bin/bash

# opt
rm *.csv
echo "Running all algorithms on all trace files...\n"
for TRACEFILE in bzip swim gcc
do
	echo "Trace file: $TRACEFILE.trace"
	for ALGORITHM in opt rand nru aging
	do
		echo "  Running $ALGORITHM..."
		if [ $ALGORITHM == "opt" ] || [ $ALGORITHM == "rand" ] ; then
			echo "FRAMES, PAGE FAULTS, DISK WRITES" >> $TRACEFILE.$ALGORITHM.csv
			for frames in 8 16 32 64 128
			do
				echo "    -- Frames: $frames --"
				./vmsim -n $frames -a $ALGORITHM $TRACEFILE.trace > $TRACEFILE.$frames.$ALGORITHM.txt
				tail -n 1 $TRACEFILE.$frames.$ALGORITHM.txt >> $TRACEFILE.$ALGORITHM.csv
				rm $TRACEFILE.$frames.$ALGORITHM.txt
			done
		else
			echo "PERIOD, FRAMES, PAGE FAULTS, DISK WRITES" >> $TRACEFILE.$ALGORITHM.csv
			for frames in 8 16 32 64 128
			do
                                echo "    -- Frames: $frames --"
				for period in 25 50 100 150 200 250 300 350 400 450 500 550 600 650 700 750 800 850 900 950 1000 
				do
					echo "      -- Period: $period --"
					./vmsim -n $frames -a $ALGORITHM -r $period $TRACEFILE.trace > $TRACEFILE.$frames.$ALGORITHM.$period.txt
					tail -n 1 $TRACEFILE.$frames.$ALGORITHM.$period.txt >> $TRACEFILE.$ALGORITHM.csv
					rm $TRACEFILE.$frames.$ALGORITHM.$period.txt
				done
			done
		fi
	done
done

echo "Done.\n"

