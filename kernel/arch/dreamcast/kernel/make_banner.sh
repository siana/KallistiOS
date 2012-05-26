#!/bin/bash

# Re-creates the banner.c file for each compilation run

echo 'char banner[] = ' > banner.c

echo -n '"KallistiOS ' >> banner.c
if [ -d ".svn" ]; then
    echo -n 'SVN r' >> banner.c
    echo -n `svnversion` >> banner.c
    echo -n ': ' >> banner.c
else
    echo -n '##version##: ' >> banner.c
fi
echo -n `date` >> banner.c
echo '\n"' >> banner.c

echo -n '"  ' >> banner.c
echo -n `whoami` >> banner.c
echo -n '@' >> banner.c
if [ `uname` = Linux ]; then
	echo -n `hostname -f` >> banner.c
else
	echo -n `hostname` >> banner.c
fi
echo -n ':' >> banner.c
echo -n $KOS_BASE >> banner.c
echo '\n"' >> banner.c

echo ';' >> banner.c

