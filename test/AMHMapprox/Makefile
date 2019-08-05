TARGET=AMHMapprox
include ../shared/Makefile.shared

$(TARGET): $(TARGET).o
	$(CC) $(TARGET).o -lm $(SNIPER_LDFLAGS) -o $(TARGET)

run_$(TARGET):
	$(MAKE) -C ../../
	../../run-sniper -c gainestown.cfg -- ./AMHMapprox

CLEAN_EXTRA=viz Output.txt
