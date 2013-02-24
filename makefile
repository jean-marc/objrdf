CC = g++ -std=c++0x -I . -O3 
#CFLAGS = -Wall -Wno-invalid-offsetof -Xlinker -zmuldefs -DOBJRDF_VERB
CFLAGS = -w -Wno-invalid-offsetof -Xlinker -zmuldefs -DOBJRDF_VERB -DPERSISTENT #-DTEST_STRING
#CFLAGS = -w -Wno-invalid-offsetof -DOBJRDF_VERB -DPERSISTENT
OBJ1 = objrdf.o uri.o pseudo_ptr.o
OBJ5 = Sockets.o
OBJ7 = sparql_engine.o
OBJ6 = httpd.o $(OBJ5) $(OBJ7)
OBJ8 = rdf_xml_parser.o
OBJ9 = ebnf.o
OBJS = $(OBJ1) $(OBJ2) $(OBJ3)
%.o:%.cpp %.h
	$(CC) -c $(CFLAGS) $< -o $@
#all:test0 test1
#clean:
#	rm $(OBJS)
test%:test%.cpp $(OBJ1) $(OBJ8) objrdf.h
	$(CC) $(CFLAGS) $< $(OBJ1) $(OBJ8) -o $@ 
examples/%:examples/%.cpp $(OBJ1) $(OBJ8) objrdf.h
	$(CC) $(CFLAGS) $< $(OBJ1) $(OBJ8) -o $@ 
#example.%:example.%.cpp libobjrdf.so
	#$(CC) $(CFLAGS) $< libobjrdf.so -o $@ 
#example%:example%.cpp $(OBJ1) $(OBJ8) objrdf.h rdf_xml_parser.h
example%:example%.cpp $(OBJ1) $(OBJ6) $(OBJ7) $(OBJ8) $(OBJ9) objrdf.h
	$(CC) $(CFLAGS) $< $(OBJ1) $(OBJ6) $(OBJ7) $(OBJ8) $(OBJ9) -lpthread -o $@ 
tests = $(basename $(wildcard test*.cpp))
examples = $(basename $(wildcard example.*.cpp))

dump:dump.cpp $(OBJ1) pseudo_ptr.h
	$(CC) $(CFLAGS) dump.cpp $(OBJ1) schema.so -o dump
dump_schema:dump_schema.cpp $(OBJ1) pseudo_ptr.h
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
sparql_engine.test: objrdf.o uri.o sparql_engine.o sparql_engine.test.cpp ebnf.h turtle_parser.h char_iterator.h
	$(CC) $(CFLAGS) sparql_engine.test.cpp objrdf.o uri.o sparql_engine.o -o sparql_engine.test 
sparql_parser.1.test:$(OBJ1) sparql_parser.1.test.cpp ebnf.h jm.h
	$(CC) $(CFLAGS) sparql_parser.1.test.cpp $(OBJ1) -o sparql_parser.1.test 
xml_signature.test:$(OBJ9) xml_signature.h uri.o
	$(CC) $(CFLAGS) -I../../../license xml_signature.test.cpp $(OBJ9) uri.o -o xml_signature.test
persistence.objrdf: persistence.objrdf.cpp objrdf.o uri.o rdf_xml_parser.o ebnf.h char_iterator.h
	$(CC) $(CFLAGS) persistence.objrdf.cpp objrdf.o uri.o rdf_xml_parser.o -o persistence.objrdf
%.pic.o:%.cpp %.h
	$(CC) -c $(CFLAGS) -fpic $< -o $@
libobjrdf.so:objrdf.pic.o uri.pic.o
	$(CC) $(CFLAGS) objrdf.pic.o uri.pic.o -shared -o libobjrdf.so 
%.schema.so:%.schema.pic.o objrdf.o
	$(CC) $(CFLAGS) $< -shared -o $@ 
clean:
	rm -f $(OBJS) $(OBJ8)

