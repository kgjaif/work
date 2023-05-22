LIBNAME1=ds18b20
LIBNAME2=socket
LIBNAME3=database
LIBNAME4=syslog
LIBNAME5=packet
INSTPATH=`pwd`/../lib/
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

all:static_lib 
	@make clear
	@make install
	${CC} ${CFLAGS} socket_ds18b20.c -l sqlite3  -o ${APPNAME} ${LDFLAGS} 	 	       

static_lib:
	${CC} -c *.c
	${AR} -rcs lib${LIBNAME1}.a ${LIBNAME1}.o
	${AR} -rcs lib${LIBNAME2}.a ${LIBNAME2}.o
	${AR} -rcs lib${LIBNAME3}.a ${LIBNAME3}.o
	${AR} -rcs lib${LIBNAME4}.a ${LIBNAME4}.o
	${AR} -rcs lib${LIBNAME5}.a ${LIBNAME5}.o

install:
	cp -rf lib*.a ${INSTPATH}
	cp -rf *.h ${INSTPATH}



uninstall:
	rm -rf ${INSTPATH}/lib*.a
	rm -rf ${INSTPATH}/*.h



clean: 
	rm -rf lib*.a
	rm -rf *.db
