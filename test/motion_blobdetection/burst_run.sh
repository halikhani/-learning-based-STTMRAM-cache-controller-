#!/bin/sh
#AMHM batch file to run Sniper for motion_blobdetection
tm="$(date +%s)"
echo "####################################################################################"
echo "#          Script For Running Motion Blob-Detection in the Sniper Simulator        #"
echo "#                            By Amir Mahdi Hosseini Monazzah                       #"
echo "####################################################################################"

for InbgImage in 0 0.001 0.0001 0.00001 0.000001 0.0000001 0.00000001 0.000000001
do
	for bgGreyImage in 0 0.001 0.0001 0.00001 0.000001 0.0000001 0.00000001 0.000000001
	do
		for outputImage in 0 0.001 0.0001 0.00001 0.000001 0.0000001 0.00000001 0.000000001
		do
			for greyImage in 0 0.001 0.0001 0.00001 0.000001 0.0000001 0.00000001 0.000000001
			do
				for motionImage in 0 0.001 0.0001 0.00001 0.000001 0.0000001 0.00000001 0.000000001
				do
					for blobImage in 0 0.001 0.0001 0.00001 0.000001 0.0000001 0.00000001 0.000000001
					do
						for erosionImage in 0 0.001 0.0001 0.00001 0.000001 0.0000001 0.00000001 0.000000001
						do
							for edgeImage in 0 0.001 0.0001 0.00001 0.000001 0.0000001 0.00000001 0.000000001
							do

								sed -i "10s/.*/double ber[8]={${InbgImage},${bgGreyImage},${outputImage},${greyImage},${motionImage},${blobImage},${erosionImage},${edgeImage}\};/g" motion.cc
								make run > terminal_output.log
								mv output.bmp ./burst_output/output_${InbgImage}_${bgGreyImage}_${outputImage}_${greyImage}_${motionImage}_${blobImage}_${erosionImage}_${edgeImage}.bmp
								mv terminal_output.log ./burst_output/terminal_output_${InbgImage}_${bgGreyImage}_${outputImage}_${greyImage}_${motionImage}_${blobImage}_${erosionImage}_${edgeImage}.log
								make clean

							done
						done
					done
				done
			done
		done
	done
done

echo "All Simulation Run Was Finished for All Combinations"
tm="$(($(date +%s)-tm))"
tm="$((tm/60))"
echo "Time in minutes: ${tm}





