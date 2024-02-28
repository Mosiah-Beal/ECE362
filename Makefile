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


# Generate rules for all targets in TARGETS
define TARGET_RULE
$1: $1.o
    gcc -o $$@ $$^
endef

# Evaluate the rules for all targets in TARGETS
$(foreach tgt, $(TARGETS), $(eval $(call TARGET_RULE, $(tgt))))

