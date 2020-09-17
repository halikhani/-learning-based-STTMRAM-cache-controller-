TARGET=test
include ../shared/Makefile.shared

$(TARGET): $(TARGET).o
	$(CXX) $(TARGET).o -lm $(SNIPER_LDFLAGS) -o $(TARGET)

run_$(TARGET):
	$(MAKE) -C ../../
	../../run-sniper -c base.cfg -- ./test

CLEAN_EXTRA=viz
