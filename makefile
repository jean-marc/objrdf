CC = g++ -O3 
ARM =  /opt/ioplex_mx/usr/bin/arm-linux-gnueabihf-g++ -O3

#CFLAGS = -Wall -Wno-invalid-offsetof -Xlinker -zmuldefs -DOBJRDF_VERB
CFLAGS = -std=c++0x -I. -w -Wno-invalid-offsetof -Xlinker -zmuldefs -DOBJRDF_VERB -DPERSISTENT -UREF_COUNT
#CFLAGS = -w -Wno-invalid-offsetof -DOBJRDF_VERB -DPERSISTENT
OBJ1 = objrdf.o uri.o
OBJ5 = Sockets.o
OBJ7 = sparql_engine.o
OBJ6 = httpd.o $(OBJ5) $(OBJ7)
OBJ8 = rdf_xml_parser.o
OBJ9 = ebnf.o
OBJS = $(OBJ1) $(OBJ2) $(OBJ3)
%.o:%.cpp %.h
	$(CC) -c $(CFLAGS) $< -o $@
%.pic.o:%.cpp %.h
	$(CC) -c $(CFLAGS) -fpic $< -o $@
%.arm.o:%.cpp %.h
	$(ARM) -c $(CFLAGS) $< -o $@
%.arm.pic.o:%.cpp %.h
	$(ARM) -c $(CFLAGS) -fpic $< -o $@
#all:test0 test1
#clean:
#	rm $(OBJS)
test%:test%.cpp $(OBJ1) $(OBJ8) objrdf.h
	$(CC) $(CFLAGS) $< $(OBJ1) $(OBJ8) -o $@ 
examples/%:examples/%.cpp $(OBJ1) $(OBJ7) $(OBJ8) $(OBJ9) objrdf.h
	$(CC) $(CFLAGS) $< $(OBJ1) $(OBJ7) $(OBJ8) $(OBJ9) -o $@ 
_example.%:example.%.cpp libobjrdf.so
	$(CC) $(CFLAGS) $< libobjrdf.so -o $@ 
example%:example%.cpp $(OBJ1) $(OBJ6) $(OBJ8) $(OBJ9) objrdf.h
	$(CC) $(CFLAGS) $< $(OBJ1) $(OBJ6) $(OBJ8) $(OBJ9) -lpthread -o $@ 
tests = $(basename $(wildcard test*.cpp))
examples = $(basename $(wildcard example.*.cpp))

dump:dump.cpp $(OBJ1)
	$(CC) $(CFLAGS) dump.cpp $(OBJ1) schema.so -o dump
dump_schema:dump_schema.cpp $(OBJ1)
	$(CC) $(CFLAGS) dump_schema.cpp $(OBJ1) schema.so -o dump_schema
objrdf_ctl:objrdf_ctl.cpp $(OBJ1) $(OBJ6) $(OBJ8) $(OBJ9) 
	#$(CC) $(CFLAGS) objrdf_ctl.cpp $(OBJ1) $(OBJ6) $(OBJ8) $(OBJ9) schema.so -lpthread -o objrdf_ctl
	$(CC) -Os $(CFLAGS) objrdf_ctl.cpp $(OBJ1) $(OBJ6) $(OBJ8) $(OBJ9) inventory.schema.pic.o -lpthread -o objrdf_ctl
	ln -fs run_objrdf_ctl.sh dump
	ln -fs run_objrdf_ctl.sh dump_schema
	ln -fs run_objrdf_ctl.sh parser
	ln -fs run_objrdf_ctl.sh query
	ln -fs run_objrdf_ctl.sh server

all:$(examples) $(tests)

test7:test7.cpp $(OBJ1) $(OBJ2) objrdf.h obj_xml_parser.h
	$(CC) $(CFLAGS) test7.cpp $(OBJ1) $(OBJ2) -o test7 
#object file order matters!
test10:test10.cpp $(OBJ1) $(OBJ3) objrdf.h obj_xml_parser.h
	#$(CC) $(CFLAGS) test10.cpp $(OBJ3) $(OBJ1) -o test10 
	$(CC) $(CFLAGS) test10.cpp $(OBJ1) $(OBJ3) -o test10 
test13:test13.cpp $(OBJ1) $(OBJ8) objrdf.h rdf_xml_parser.h
	$(CC) $(CFLAGS) test13.cpp $(OBJ1) $(OBJ8) -o test13 
wbdf_message.o:wbdf_message.cpp wbdf_message.h
	$(CC) $(CFLAGS) -c $< -o $@
xml_parser.test:xml_parser.test.cpp xml_parser.h ebnf.h char_iterator.h uri.o
	$(CC) $(CFLAGS) xml_parser.test.cpp uri.o -o xml_parser.test
rdf_xml_parser.test:rdf_xml_parser.test.cpp rdf_xml_parser.o objrdf.o xml_parser.h ebnf.h char_iterator.h uri.o
	$(CC) $(CFLAGS) rdf_xml_parser.test.cpp uri.o rdf_xml_parser.o objrdf.o -o rdf_xml_parser.test
httpd.o:httpd.cpp httpd.h http_parser.h
	$(CC) -c $(CFLAGS) $< -o $@
http_parser.test:http_parser.test.cpp http_parser.h ebnf.h
	$(CC) $(CFLAGS) http_parser.test.cpp $(OBJ5) -o http_parser.test 
httpd.test:$(OBJ1) $(OBJ5) $(OBJ6) $(OBJ7) httpd.test.cpp httpd.h http_parser.h ebnf.h
	$(CC) $(CFLAGS) httpd.test.cpp $(OBJ1) $(OBJ5) $(OBJ6) $(OBJ7) -lpthread -o httpd.test 
turtle_parser.test:$(OBJ1) ebnf.h turtle_parser.h
	$(CC) $(CFLAGS) turtle_parser.test.cpp -o turtle_parser.test 
sparql_engine.test: objrdf.o uri.o sparql_engine.o ebnf.o sparql_engine.test.cpp turtle_parser.h char_iterator.h
	$(CC) $(CFLAGS) sparql_engine.test.cpp objrdf.o uri.o sparql_engine.o ebnf.o -o sparql_engine.test 
sparql_parser.1.test:$(OBJ1) sparql_parser.1.test.cpp ebnf.h jm.h
	$(CC) $(CFLAGS) sparql_parser.1.test.cpp $(OBJ1) -o sparql_parser.1.test 
xml_signature.test:$(OBJ9) xml_signature.h uri.o
	$(CC) $(CFLAGS) -I../../../license xml_signature.test.cpp $(OBJ9) uri.o -o xml_signature.test
persistence.objrdf: persistence.objrdf.cpp objrdf.o uri.o rdf_xml_parser.o ebnf.h char_iterator.h
	$(CC) $(CFLAGS) persistence.objrdf.cpp objrdf.o uri.o rdf_xml_parser.o -o persistence.objrdf

#shared library experiment, total size is 25% bigger, could be improved
#	ls -l libobjrdf.so example.inventory _example.inventory 
#	-rwxrwxr-x 1 user user 1557269 Apr 18 10:56 _example.inventory
#	-rwxrwxr-x 1 user user 1946582 Apr 18 10:21 example.inventory
#	-rwxrwxr-x 1 user user  939032 Apr 18 10:55 libobjrdf.so
#	run with /lib64/ld-linux-x86-64.so.2 --library-path /home/user/projects/objrdf/ _example.inventory

libobjrdf.so:objrdf.pic.o uri.pic.o sparql_engine.pic.o ebnf.pic.o httpd.pic.o rdf_xml_parser.pic.o Sockets.pic.o
	$(CC) $(CFLAGS) objrdf.pic.o uri.pic.o sparql_engine.pic.o ebnf.pic.o httpd.pic.o rdf_xml_parser.pic.o Sockets.pic.o -lpthread -shared -o $@
libobjrdf.arm.so:objrdf.arm.pic.o uri.arm.pic.o sparql_engine.arm.pic.o ebnf.arm.pic.o httpd.arm.pic.o rdf_xml_parser.arm.pic.o Sockets.arm.pic.o
	$(ARM) $(CFLAGS) objrdf.arm.pic.o uri.arm.pic.o sparql_engine.arm.pic.o ebnf.arm.pic.o httpd.arm.pic.o rdf_xml_parser.arm.pic.o Sockets.arm.pic.o -lpthread -shared -o $@
#too many include files, need to reorganize the code
install:libobjrdf.so
	cp libobjrdf.so /usr/local/lib/
	cp char_iterator.h http_parser.h result.h turtle_parser.h ifthenelse.hpp uri.h ebnf.h objrdf.h Sockets.h xml_parser.h geo.h sparql_engine.h httpd.h rdf_xml_parser.h tuple_helper.h /usr/local/include/objrdf/
arm_install:libobjrdf.arm.so
	cp libobjrdf.arm.so /opt/ioplex_mx/usr/arm-buildroot-linux-gnueabihf/sysroot/usr/lib/libobjrdf.so
	cp char_iterator.h http_parser.h result.h turtle_parser.h ifthenelse.hpp uri.h ebnf.h objrdf.h Sockets.h xml_parser.h geo.h sparql_engine.h httpd.h rdf_xml_parser.h tuple_helper.h /opt/ioplex_mx/usr/arm-buildroot-linux-gnueabihf/sysroot/usr/include/objrdf/
%.schema.so:%.schema.pic.o objrdf.o
	$(CC) $(CFLAGS) $< -shared -o $@ 
clean:
	rm -f $(OBJS) $(OBJ8)

