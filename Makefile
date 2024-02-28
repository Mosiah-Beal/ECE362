# Mosiah Beal
# 2024-02-28
# ECE 362 HW 3


# Implicit rule for .c to .o
%.o: %.c
	gcc -c $< -o $@


# Targets for the executables
TARGETS = timer alarmSig

all: $(TARGETS)

clean:
	rm -f $(TARGETS) *.o

timer: timer.o
	gcc -o $@ $^

alarmSig: alarmSig.o
	gcc -o $@ $^
