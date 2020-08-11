TARGET=motion
DEP1=image_processing
DEP2=CImageProcessing
include ../shared/Makefile.shared

$(TARGET): $(TARGET).o
	$(CC) $(TARGET).cc $(DEP1).cc $(DEP2).cc -lstdc++ -lm $(SNIPER_LDFLAGS) -o $(TARGET)

run_$(TARGET):
	$(MAKE) -C ../../
	../../run-sniper -c cacheonly.cfg --cache-only --roi -- ./motion videoframe1.bmp videoframe2.bmp output.bmp

CLEAN_EXTRA=output.bmp
