TARGET=motion
include ../shared/Makefile.shared

$(TARGET): $(TARGET).o
	$(CC) $(TARGET).cpp -lstdc++ -lm $(SNIPER_LDFLAGS) -o $(TARGET)

run_$(TARGET):
	$(MAKE) -C ../../
	../../run-sniper -c base.cfg --cache-only --roi -- ./motion frames/img-005.bmp frames/img-006.bmp outputs/output.bmp

CLEAN_EXTRA=output.bmp
