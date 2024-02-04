# Mosiah Beal
# 2024-01-31
# ECE 362 HW 2


# Implicit rule for .c to .o
%.o: %.c
	gcc -c $< -o $@


# Targets for the executables
TARGETS = mywrite myterminalwrite

all: $(TARGETS)

clean:
	rm -f $(TARGETS) mywrite.txt *.o

# Also make sure that the .txt file exists before running mywrite
mywrite: mywrite.o
	touch mywrite.txt
	gcc -o mywrite mywrite.o

myterminalwrite: myterminalwrite.o
	gcc -o myterminalwrite myterminalwrite.o



