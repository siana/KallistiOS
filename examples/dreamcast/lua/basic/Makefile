# KallistiOS ##version##
#
# lua/basic/Makefile
# (c)2002 Dan Potter
#

all: rm-elf lua.elf

KOS_LOCAL_CFLAGS := -I$(KOS_BASE)/addons/lua/include


include $(KOS_BASE)/Makefile.rules

OBJS = lua.o

clean:
	-rm -f lua.elf $(OBJS)

rm-elf:
	-rm -f lua.elf

lua.elf: $(OBJS) 
	$(KOS_CC) $(KOS_CFLAGS) $(KOS_LDFLAGS) -o lua.elf $(KOS_START) $(OBJS) $(DATAOBJS) \
		$(OBJEXTRA) -llua -llualib -lconio $(KOS_LIBS) -lm

run: lua.elf
	$(KOS_LOADER) lua.elf

dist:
	-rm -f $(OBJS)
	$(KOS_STRIP) lua.elf
