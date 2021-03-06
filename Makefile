all:
	cd samtools-0.1.18; make
	cd liblinear-1.96; make
	g++ -Wall -I ./samtools-0.1.18 -L./samtools-0.1.18 -c DNaseFlash.c -lbam -lz -lm -o DNaseFlash.o
	g++ countCoverage.c DNaseFlash.o -L./samtools-0.1.18 -lbam -lz -lm -o countCoverage
	g++ extractFeature.c DNaseFlash.o -L./samtools-0.1.18 -lbam -lz -lm -o extractFeature
	g++ predictModel.c DNaseFlash.o -L./samtools-0.1.18 -lbam -lz -lm -o predictModel
	g++ trainModel.c DNaseFlash.o -L./samtools-0.1.18 -lbam -lz -lm -o trainModel
	g++ -Wall -o predictModelSelectedRegions predictModelSelectedRegions.c ./liblinear-1.96/tron.o ./liblinear-1.96/linear.o ./DNaseFlash.o ./liblinear-1.96/blas/blas.a -L./samtools-0.1.18 -lbam -lz
	g++ -Wall -o predictModelWholeGenome predictModelWholeGenome.c ./liblinear-1.96/tron.o ./liblinear-1.96/linear.o ./DNaseFlash.o ./liblinear-1.96/blas/blas.a -L./samtools-0.1.18 -lbam -lz
	g++ -Wall -o predictModelWholeGenome_multithread predictModelWholeGenome_multithread.c ./liblinear-1.96/tron.o ./liblinear-1.96/linear.o ./DNaseFlash.o ./liblinear-1.96/blas/blas.a -L./samtools-0.1.18 -lbam -lz -lpthread
	\rm *.o
	echo Clean done
