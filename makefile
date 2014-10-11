raspi_osc_test: raspi_osc_test.o blit_saw_oscillator.o
	g++ raspi_osc_test.o blit_saw_oscillator.o -o raspi_osc_test -O3 -lasound

raspi_osc_test.o: raspi_osc_test.cpp
	g++ raspi_osc_test.cpp -c -std=c++0x

blit_saw_oscillator.o: blit_saw_oscillator.cpp blit_saw_oscillator.h
	g++ blit_saw_oscillator.cpp -c -std=c++0x
