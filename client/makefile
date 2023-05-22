LIBNAME1=ds18b20
LIBNAME2=socket
LIBNAME3=database
LIBNAME4=syslog
LIBNAME5=packet
LIBPATH=`pwd`/lib/
APPNAME=socket_ds18b20
CFLAGS+=-I ${LIBPATH}
LDFLAGS+=-L ${LIBPATH}
LDFLAGS+=-l${LIBNAME1}
LDFLAGS+=-l${LIBNAME2}
LDFLAGS+=-l${LIBNAME3}
LDFLAGS+=-l${LIBNAME4}
LDFLAGS+=-l${LIBNAME5}

CC=gcc
AR=ar

all:  
	${CC} ${CFLAGS} client.c -o ${APPNAME} ${LDFLAGS} -l sqlite3
	export LD_LIBRARY_PATH=/home/lanli/work1/work/client/      	  

clean:
	rm -rf ${APPNAME}
	rm -rf *.db
	rm -rf *.dv
	rm -rf Log/

distclean: clean
