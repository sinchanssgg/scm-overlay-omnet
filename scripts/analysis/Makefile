all: process visualize

process:
	python3 process_results.py ${resultdir}

visualize:
	python3 ../visualization/plot_metrics.py ${resultdir}

clean:
	rm -f ${resultdir}/*.csv ${resultdir}/*.png