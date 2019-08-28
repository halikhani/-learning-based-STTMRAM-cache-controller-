TARGET=test
include ../shared/Makefile.shared

$(TARGET): $(TARGET).o
	$(CXX) $(TARGET).o -lm $(SNIPER_LDFLAGS) -o $(TARGET)

run_$(TARGET):
	$(MAKE) -C ../../
	../../run-sniper -c cacheonly.cfg --cache-only --roi -- ./test

CLEAN_EXTRA=viz
