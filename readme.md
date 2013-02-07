# Objrdf library
## Introduction
In the objrdf framework RDF classes are mapped to C++ classes, RDF properties are mapped to C++ class members. 
RDF classes and properties are defined in the application code, using a syntax similar to standard class definitions.
It implements a RDF datastore with a SPARQL end point.

## Example
In the following a class 'Test' and two properties 'a' and 'b' are defined in the 'http://test.example.org/#' namespace, an instance 'test' is created, properties are set and the resource is serialized to XML. Note: 
* properties must be defined before the domain class, because C++ does not allow to add members after the class has been defined	
* properties are packed in a std::tuple (if the class has no properties use std::tuple<>)
* the class defines an allocator to create and destroy instances (it can be customized see later)
```
#include "objrdf.h"
using namespace objrdf;
RDFS_NAMESPACE("http://test.example.org/#","test")
PROPERTY(a,int);
PROPERTY(b,double);
CLASS(Test,std::tuple<a,b>);
int main(){
	Test::allocator al;
	auto ptr=al.allocate(1);
	al.construct(ptr,uri("test"));
	ptr->get<a>().t=123;
	ptr->get<b>().t=.123;
	to_rdf_xml(ptr,cout);
	al.destroy(ptr);
	al.deallocate(ptr,1);
}
```
## Implementation
The code relies heavily on the latest language features brought in C++0x

## Reference

Once classes and members are defined in code by specializing templates any new object becomes part of a RDF document, that can be published (RDFXML) and 
modified (SPARQL).

Persistence

When allocating an object on the persistent store guarantees that the pointer will always be valid, including between runs of the programs.
The only way the pointer would become invalid is if the program is recompiled and causes the pool index to change, obviously changing the pointed-to class will also cause problems.
Safeguards mechanisms can be put in place to prevent strange behaviours (hashing the file, ...).
Modifying the class is a normal evolution of a program and updating the database can be done through RDF dumps and -possibly- XML transformations (XSLT).
Pointers have to be stored from one run to the next, they can be put in an array which also gets persisted, an alternative is to use the pool itself, iterated through each cells.
Alternatively an object can be allocated at a set address.
The set of pools can be seen as the RDF document.

Parameter

When creating a new class it is useful to know how many instances N to expect.
The pointer size in bits will have to be > log2 N, it is also good to know how many references will be made to each instance to size the reference counter (how connected the graph is)

Robustness

As many error as possible should be caught at compile time, static_assert can help us detect semantic problems: for instance casting between unrelated types.
To preserve the database it should be possible to make the files read-only so - when testing - no crash can corrupt the data.

Rules

The template mechanism can help us enforce rules:eg. if an object is persisted, all its properties must be persisted as well otherwise pointers will be wrong on the next run.

## Pseudo-properties

Pseudo-properties do not have any memory allocated to them, it is up to the user to provide the value on demand (when the object is serialized or queried), that value will usually be based on one or more other properties and can be used for instance to display data in a user-friendly format or provide some kind of summary of a set of data. They are similar to database views. They can not be used to enter data and are read-only.

## Trigger

Trigger are used to notify a resource that one property will be modified (add, delete or edit), the user-provided handler receives the new value, test the value and decides whether to accept or reject it.

## Versioning

It might be desirable to record versions of a predicate, one solution is to stow away a copy of the entire resource before removing a predicate (it means the resource and the copy will only differ by one predicate), the new - modified - resource will have a reference to the copy e.g:

	<inv:Set rdf:ID='test'>
		<inv:located rdf:resource='#naguru'/>
		<inv:on>0</inv:on>
		<obj:prev rdf:nodeID='ae2'/>
	</inv:Set>
	<inv:Set rdf:nodeID='ae2'>
		<inv:located rdf:resource='#kololo'/>
		<inv:on>0</inv:on>
	</inv:Set>
We modified the property inv:located in the resource test from 'kololo' to 'naguru', note that the copy is a blank node and can only be reached through the objrdf:prev property of the resource, the copy must not be modified. 

## Privileges

All modification to the document done through SPARQL and RDF parsing are done through the class s function table (virtual functions are not used) which provide a generic interface to all the properties. That function table can be modified to change behaviour.


