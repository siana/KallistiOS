#!/bin/sh

# Re-creates the banner.h file for each compilation run

printf 'static const char banner[] = \n' > banner.h

printf '"KallistiOS ' >> banner.h
if [ -d "$KOS_BASE/.git" ]; then
    printf 'Git revision ' >> banner.h
    printf `git rev-list --full-history --all --abbrev-commit | head -1` >> banner.h
    printf ': ' >> banner.h
else
    printf '##version##: ' >> banner.h
fi

tmp=`date`
printf "$tmp" >> banner.h
printf '\\n"\n' >> banner.h

printf '"  ' >> banner.h
tmp=`whoami`
printf "$tmp" >> banner.h
printf '@' >> banner.h

if [ `uname` = Linux ]; then
    tmp=`hostname -f`
else
    tmp=`hostname`
fi

printf "$tmp" >> banner.h

printf ':' >> banner.h
printf "$KOS_BASE" >> banner.h
printf '\\n"\n' >> banner.h

printf ';\n' >> banner.h
