build:
	gcc process_generator.c -o process_generator.out -lm
	gcc clk.c -o clk.out
	gcc srtn.c -o srtn.out -lm
	gcc process.c -o process.out
	gcc test_generator.c -o test_generator.out

clean:
	rm -f *.out

all: build run clean

run:
	./process_generator.out
