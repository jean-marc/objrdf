/*
Resource type Class; comment ; c_index 0 .
Class type Class; subClassOf Resource; comment ; c_index 1 .
Literal type Class; subClassOf Resource; comment ; c_index 2 .
String type Class; subClassOf Literal; comment ; c_index 3 .
Short type Class; subClassOf Literal; comment ; c_index 4 .
Property type Class; subClassOf Resource; comment ; c_index 5 .
Int type Class; subClassOf Literal; comment ; c_index 6 .
p0 type Property; domain Test; range Int .
p1 type Property; domain Test; range String .
p2 type Property; domain Test1; range Test .
Test type Class; comment ; c_index 7 .
Test1 type Class; comment ; c_index 8 .
*/
#include "objrdf.h"
using namespace objrdf;
PROPERTY(p0,int);
PROPERTY(p1,string);
CLASS2(Test,p0,p1);
PROPERTY(p2,Test*);
CLASS1(Test1,p2);
int main(){}
