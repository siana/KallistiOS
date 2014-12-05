#!/usr/bin/env bash

# Re-creates the banner.h file for each compilation run

echo 'static const char banner[] = ' > banner.h

echo -n '"KallistiOS ' >> banner.h
if [ -d "$KOS_BASE/.git" ]; then
    echo -n 'Git revision ' >> banner.h
    echo -n `git rev-list --full-history --all --abbrev-commit | head -1` >> banner.h
    echo -n ': ' >> banner.h
else
    echo -n '##version##: ' >> banner.h
fi
echo -n `date` >> banner.h
echo '\n"' >> banner.h

echo -n '"  ' >> banner.h
echo -n `whoami` >> banner.h
echo -n '@' >> banner.h
if [ `uname` = Linux ]; then
	echo -n `hostname -f` >> banner.h
else
	echo -n `hostname` >> banner.h
fi
echo -n ':' >> banner.h
echo -n $KOS_BASE >> banner.h
echo '\n"' >> banner.h

echo ';' >> banner.h

