CC = g++ -g
IMPL=IMPL_IN_HEADER
CFLAGS = -D$(IMPL) -DBIND_TEST -Wall -Wno-invalid-offsetof -Xlinker -zmuldefs -D$(IMPL) -O3 -I../../common
OBJ1 = objrdf.o
OBJ2 = sigmon.o
OBJ3 = wbdf_message.o
OBJ5 = ../../common/Sockets.o
OBJ6 = httpd.o
OBJ7 = sparql_parser.o
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
http_parser.test:http_parser.test.cpp http_parser.h ebnf_template.h
	$(CC) $(CFLAGS) http_parser.test.cpp $(OBJ5) -o http_parser.test 
httpd.test:$(OBJ1) $(OBJ5) $(OBJ6) $(OBJ7) httpd.test.cpp httpd.h http_parser.h ebnf_template.h
	$(CC) $(CFLAGS) httpd.test.cpp $(OBJ1) $(OBJ5) $(OBJ6) $(OBJ7) -lpthread -o httpd.test 
turtle_parser.test:$(OBJ1) ebnf_template.h turtle_parser.h
	$(CC) $(CFLAGS) turtle_parser.test.cpp -o turtle_parser.test 
sparql_parser.test:$(OBJ1) $(OBJ7) sparql_parser.test.cpp sparql_parser.cpp sparql_parser.h ebnf_template.h jm.h
	$(CC) $(CFLAGS) sparql_parser.test.cpp $(OBJ1) $(OBJ7) -o sparql_parser.test 
xml_signature.test:$(OBJ9) xml_signature.h
	$(CC) $(CFLAGS) -I../../../license xml_signature.test.cpp $(OBJ9) -o xml_signature.test
clean:
	rm -f $(OBJS) $(OBJ8)

