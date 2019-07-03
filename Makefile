#  @author: Michael De Angelis
#  @mat: 560049
#  @project: Sistemi Operativi e Laboratorio [SOL]
#  @A.A: 2018 / 2019

# ---------------------------------------------------------------------------
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2 as 
#  published by the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
#  As a special exception, you may use this file as part of a free software
#  library without restriction.  Specifically, if other files instantiate
#  templates or use macros or inline functions from this file, or you compile
#  this file and link it with other files to produce an executable, this
#  file does not by itself cause the resulting executable to be covered by
#  the GNU General Public License.  This exception does not however
#  invalidate any other reasons why the executable file might be covered by
#  the GNU General Public License.
#
# ---------------------------------------------------------------------------

CC 			  = gcc
AR            = ar
CFLAGS 		  += -std=c99 -Wall -pedantic -g
ARFLAGS       = rvs
INCLUDES      = -I./include
LDFLAGS       = -L./lib/
OPTFLAGS	  = -O3
LIBS 		  = -lpthread -lcom_protocol

TARGETS 	  = object_store    \
				client

INCLUDE_FILES = ./include/client_op.h     \
				./include/com_protocol.h  \
				./include/error_control.h \
				./include/server_op.h     

.PHONY: all clean test

.SUFFIXES: .c .h

vpath %.c ./src

%: ./src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $< $(LDFLAGS) $(LIBS)

./obj/%.o: ./src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -c -o $@ $<

all : $(TARGETS)

object_store: ./obj/server.o ./lib/libserver_op.a ./lib/libcom_protocol.a ./lib/libhash_table.a
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS) -lserver_op

./lib/libserver_op.a: ./obj/server_op.o $(INCLUDE_FILES)
	$(AR) $(ARFLAGS) $@ $<

./obj/server_op.o: ./src/server_op.c $(INCLUDE_FILES)

./lib/libcom_protocol.a: ./obj/com_protocol.o $(INCLUDE_FILES)
	$(AR) $(ARFLAGS) $@ $<

./obj/com_protocol.o: ./src/com_protocol.c $(INCLUDE_FILES)

./lib/libhash_table.a: ./obj/hash_table.o ./include/hash_table.h
	$(AR) $(ARFLAGS) $@ $<

./obj/hash_table.o: ./src/hash_table.c ./include/hash_table.h

./obj/server.o: ./src/server.c $(INCLUDE_FILES)

client: ./obj/client.o ./lib/libclient_op.a ./lib/libcom_protocol.a
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS) -lclient_op

./lib/libclient_op.a: ./obj/client_op.o $(INCLUDE_FILES)
	$(AR) $(ARFLAGS) $@ $<

./obj/client_op.o: ./src/client_op.c $(INCLUDE_FILES)

./obj/client.o: ./src/client.c $(INCLUDE_FILES)

clean :
	\rm -f $(TARGETS)
	\rm -f *.o *~ *.a
	\rm -f ./lib/*.o ./lib/*~ ./lib/*.a
	\rm -f ./src/*.o ./src/*~ ./src/*.a
	\rm -f ./obj/*.o
	\rm -f ./scripts/*.log

test : client
	chmod +x ./scripts/test.sh
	./scripts/test.sh 1 > ./scripts/testout.log 