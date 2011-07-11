CC = g++ -g -std=c++0x #-O3
IMPL=IMPL_IN_HEADER
CFLAGS = -D$(IMPL) -DBIND_TEST -Wall -Wno-invalid-offsetof -Xlinker -zmuldefs -D$(IMPL)  -I../../common -DVERBOSE
OBJ1 = objrdf.o uri.o
OBJ2 = sigmon.o
OBJ3 = wbdf_message.o
OBJ5 = ../../common/Sockets.o
OBJ6 = httpd.o
OBJ7 = sparql_engine.o
OBJ8 = rdf_xml_parser.o
OBJ9 = ../../../license/Md5.o
OBJS = $(OBJ1) $(OBJ2) $(OBJ3)
%.o:%.cpp %.h
	$(CC) -c $(CFLAGS) $< -o $@
#all:test0 test1
#clean:
#	rm $(OBJS)
test%:test%.cpp $(OBJ1) $(OBJ8) objrdf.h
	$(CC) $(CFLAGS) $< $(OBJ1) $(OBJ8) -o $@ 
example%:example%.cpp $(OBJ1) $(OBJ8) objrdf.h rdf_xml_parser.h
	$(CC) $(CFLAGS) $< $(OBJ1) $(OBJ8) -o $@ 
tests = $(basename $(wildcard test*.cpp))
examples = $(basename $(wildcard example.*.cpp))

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
xml_parser.test:xml_parser.test.cpp xml_parser.h ebnf_template.h char_iterator.h uri.o
	$(CC) $(CFLAGS) xml_parser.test.cpp uri.o -o xml_parser.test
rdf_xml_parser.test:rdf_xml_parser.test.cpp rdf_xml_parser.o objrdf.o xml_parser.h ebnf_template.h char_iterator.h uri.o
	$(CC) $(CFLAGS) rdf_xml_parser.test.cpp uri.o rdf_xml_parser.o objrdf.o -o rdf_xml_parser.test
httpd.o:httpd.cpp httpd.h http_parser.h
	$(CC) -c $(CFLAGS) $< -o $@
http_parser.test:http_parser.test.cpp http_parser.h ebnf_template.h
	$(CC) $(CFLAGS) http_parser.test.cpp $(OBJ5) -o http_parser.test 
httpd.test:$(OBJ1) $(OBJ5) $(OBJ6) $(OBJ7) httpd.test.cpp httpd.h http_parser.h ebnf_template.h
	$(CC) $(CFLAGS) httpd.test.cpp $(OBJ1) $(OBJ5) $(OBJ6) $(OBJ7) -lpthread -o httpd.test 
turtle_parser.test:$(OBJ1) ebnf_template.h turtle_parser.h
	$(CC) $(CFLAGS) turtle_parser.test.cpp -o turtle_parser.test 
sparql_parser.test:$(OBJ1) $(OBJ7) sparql_parser.test.cpp sparql_parser.cpp sparql_parser.h ebnf_template.h jm.h
	$(CC) $(CFLAGS) sparql_parser.test.cpp $(OBJ1) $(OBJ7) -o sparql_parser.test 
sparql_engine.test: objrdf.o uri.o sparql_engine.o rdf_xml_parser.o sparql_engine.test.cpp ebnf_template.h turtle_parser.h char_iterator.h
	$(CC) $(CFLAGS) sparql_engine.test.cpp objrdf.o uri.o sparql_engine.o rdf_xml_parser.o -o sparql_engine.test 
sparql_parser.1.test:$(OBJ1) sparql_parser.1.test.cpp ebnf_template.h jm.h
	$(CC) $(CFLAGS) sparql_parser.1.test.cpp $(OBJ1) -o sparql_parser.1.test 
xml_signature.test:$(OBJ9) xml_signature.h uri.o
	$(CC) $(CFLAGS) -I../../../license xml_signature.test.cpp $(OBJ9) uri.o -o xml_signature.test
persistence.objrdf: persistence.objrdf.cpp objrdf.o uri.o rdf_xml_parser.o ebnf_template.h char_iterator.h
	$(CC) $(CFLAGS) persistence.objrdf.cpp objrdf.o uri.o rdf_xml_parser.o -o persistence.objrdf
clean:
	rm -f $(OBJS) $(OBJ8)

