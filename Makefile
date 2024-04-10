all:
	make -C decoder_8086
	make -C libdecoder_8086
	make -C simulator_8086
	make -C rdtsc
	make -C data_gen
	make -C rep_tester
	make -C page_faults


clean:
	make -C data_gen clean
	make -C decoder_8086 clean
	make -C libdecoder_8086 clean
	make -C page_faults clean
	make -C rdtsc clean
	make -C rep_tester clean
	make -C simulator_8086 clean

