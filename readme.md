# Objrdf library
## Introduction

In the objrdf framework RDF classes are mapped to C++ classes, RDF properties are mapped to C++ class members. 
RDF classes and properties are defined in the application code, using a syntax similar to standard class definitions.
It implements a RDF datastore with a SPARQL end point.

## Examples
### Serializing
In the following a class `Test` and two properties `a` and `b` are defined in the http://test.example.org/# namespace, an instance `t` is created, properties are set and the resource is serialized to RDF/XML. Note: 

* properties must be defined before the domain class, because C++ does not allow to add members after the class has been defined
* properties are packed in a `std::tuple` (if the class has no properties use `std::tuple<>`)
* the class defines an allocator to create and destroy instances (it can be customized see later), following examples used the default heap allocator `std::allocator<>`.

```cpp
#include <objrdf.h>	/* doc/example.0.cpp */
using namespace objrdf;
RDFS_NAMESPACE("http://test.example.org/#","test")
typedef property<rdfs_namespace,str<'a'>,int> a;
typedef property<rdfs_namespace,str<'b'>,double> b;
typedef resource<rdfs_namespace,str<'T','e','s','t'>,std::tuple<a,b>> Test;
int main(){
	Test t(uri("test"));
	base_resource::do_index(&t);
	t.get<a>().t=123;
	t.get<b>().t=.123;
	to_rdf_xml(cout);
}
```
Running the program will print:

```xml
<rdf:RDF
	xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'
	xmlns:test='http://test.example.org/#'
>
	<test:Test rdf:ID='test'>
		<rdf:type rdf:resource='http://test.example.org/#Test'/>
		<test:a>123</test:a>
		<test:b>0.123</test:b>
	</test:Test>
	<!-- -->
</rdf:RDF>
```
### Parsing
Let us use the same class and parse an RDF file. 

```cpp
#include <objrdf.h>	/* doc/example.1.cpp */
#include <rdf_xml_parser.h>
using namespace objrdf;
RDFS_NAMESPACE("http://test.example.org/#","test")
typedef property<rdfs_namespace,str<'a'>,int> a;
typedef property<rdfs_namespace,str<'b'>,double> b;
typedef resource<rdfs_namespace,str<'T','e','s','t'>,std::tuple<a,b>> Test;
int main(){
	Test::get_class();
	rdf_xml_parser r(cin);
	r.go();
	to_rdf_xml(cout);
}
```
Note the call `Test::get_class()` is required to register the class so that the parser can retrieve the class's information (including methods to instantiate).

### Links
So far we have used literal properties (`int` and `double`) let us now use pointers to connect resources.

```cpp
#include <objrdf.h>	/* doc/example.2.cpp */
using namespace objrdf;
RDFS_NAMESPACE("http://test.example.org/#","test")
typedef resource<rdfs_namespace,str<'T','a','r','g','e','t'>,std::tuple<>> Target;
typedef property<rdfs_namespace,str<'a'>,Target*> a;
typedef resource<rdfs_namespace,str<'T','e','s','t'>,std::tuple<a>> Test;
int main(){
	Test t(uri("test"));
	base_resource::do_index(&t);
	Target d(uri("dest"));
	base_resource::do_index(&d);
	t.get<a>().t=&d;
	to_rdf_xml(cout);
}
```
Running the program will print:

```xml
<?xml version='1.0'?>
<rdf:RDF
	xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'
	xmlns:test='http://test.example.org/#'
>
	<test:Target rdf:ID='dest'>
		<rdf:type rdf:resource='http://test.example.org/#Target'/>
	</test:Target>
	<test:Test rdf:ID='test'>
		<rdf:type rdf:resource='http://test.example.org/#Test'/>
		<test:a rdf:resource='#dest'/>
	</test:Test>
	<!-- -->
</rdf:RDF>
```

### Multiple instances of same property 

```cpp
#include <objrdf.h>	/* doc/example.3.cpp */
using namespace objrdf;
RDFS_NAMESPACE("http://test.example.org/#","test")
typedef property<rdfs_namespace,str<'a'>,int> a;
typedef resource<rdfs_namespace,str<'T','e','s','t'>,std::tuple<objrdf::array<a>>> Test;
int main(){
	Test t(uri("test"));
	base_resource::do_index(&t);
	t.get<objrdf::array<a>>().push_back(a(123));
	t.get<objrdf::array<a>>().push_back(a(456));
	to_rdf_xml(cout);
}
```
where `objrdf::array<>` is a thin wrapper around `std::vector<>`. Running the program will print:

```xml
<rdf:RDF
	xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'
	xmlns:test='http://test.example.org/#'
>
	<test:Test rdf:ID='test'>
		<rdf:type rdf:resource='http://test.example.org/#Test'/>
		<test:a>123</test:a>
		<test:a>456</test:a>
	</test:Test>
	<!-- -->
</rdf:RDF>
```

### Inheritance 

```cpp
#include <objrdf.h>	/* doc/example.4.cpp */
using namespace objrdf;
RDFS_NAMESPACE("http://test.example.org/#","test")
typedef property<rdfs_namespace,str<'a'>,int> a;
typedef resource<rdfs_namespace,str<'A'>,std::tuple<a>> A;
typedef property<rdfs_namespace,str<'b'>,double> b;
typedef resource<rdfs_namespace,str<'B'>,std::tuple<b>,objrdf::NIL,A> B;
int main(){
	B t(uri("test"));
	base_resource::do_index(&t);
	t.get<a>().t=123;
	t.get<b>().t=.123;
	to_rdf_xml(cout);
}
```
The output is:

```xml
<rdf:RDF
	xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'
	xmlns:rdfs='http://www.w3.org/2000/01/rdf-schema#'
	xmlns:test='http://test.example.org/#'
>
	<test:B rdf:ID='test'>
		<rdf:type rdf:resource='http://test.example.org/#B'/>
		<test:a>123</test:a>
		<test:b>0.123</test:b>
	</test:B>
	<!-- -->
	<rdfs:Class rdf:about='http://test.example.org/#A'>
		<rdf:type rdf:resource='http://www.w3.org/2000/01/rdf-schema#Class'/>
	</rdfs:Class>
	<rdfs:Class rdf:about='http://test.example.org/#B'>
		<rdf:type rdf:resource='http://www.w3.org/2000/01/rdf-schema#Class'/>
		<rdfs:subClassOf rdf:resource='http://test.example.org/#A'/>
	</rdfs:Class>
</rdf:RDF>
```
The RDFS description of classes `A` and `B` was created through [introspection](introspection).

## More Advanced Examples
### Custom Class
```cpp
#include <objrdf.h>	/* doc/example.5.cpp */
using namespace objrdf;
RDFS_NAMESPACE("http://test.example.org/#","test")
typedef property<rdfs_namespace,str<'a'>,int> a;
typedef property<rdfs_namespace,str<'b'>,double> b;
struct Test:resource<rdfs_namespace,str<'T','e','s','t'>,std::tuple<a,b>,Test>{
	Test(uri id):SELF(id){
		get<a>().t=1;
		get<b>().t=0.1;
		base_resource::do_index(this);
	}
};
int main(){
	Test t(uri("test"));
	to_rdf_xml(cout);
}
```
### Pseudo Properties
```cpp

#include <objrdf.h>	/* doc/example.6.cpp */
using namespace objrdf;
RDFS_NAMESPACE("http://test.example.org/#","test")
typedef property<rdfs_namespace,str<'a'>,int> a;
typedef property<rdfs_namespace,str<'b'>,double> b;
typedef property<rdfs_namespace,str<'c'>,std::string> c;//pseudo-property
struct Test:resource<rdfs_namespace,str<'T','e','s','t'>,std::tuple<a,b>,Test>{
	Test(uri id):SELF(id){
		get<a>().t=1;
		get<b>().t=0.1;
		base_resource::do_index(this);
	}
	static void patch(V& _v){
		function_table t;
		t.get_size=[](CONST_RESOURCE_PTR){return size_t(1);};
		t.out=[](CONST_RESOURCE_PTR subject,ostream& os,size_t index){os<<"Hello RDF!";};	
		_v.push_back(property_info(c::get_property(),t));
	}
};
int main(){
	Test t(uri("test"));
	to_rdf_xml(cout);
}
```
Each property (real and pseudo-) has an entry in the property table (`base_resource::V`), the table is filled at runtime through introspection (by visiting the `std::tuple<>`) and via a user supplied `resource<...>::patch()` function.
The output is:

```xml
<rdf:RDF
	xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'
	xmlns:test='http://test.example.org/#'
>
	<test:Test rdf:ID='test'>
		<rdf:type rdf:resource='http://test.example.org/#Test'/>
		<test:a>1</test:a>
		<test:b>0.1</test:b>
		<test:c>Hello RDF!</test:c>
	</test:Test>
	<!-- -->
</rdf:RDF>
```
Everytime the resource is serialized or queried (see sparql), the function stored in `Test::v[i].t.out` is invoked where `i` is the index in the function table. This mechanism can be used to parse obsolete properties (after schema has been modified).


## Queries

The objects and properties being mapped to a RDF model, they can also be queried through SPARQL, [sparql_engine.cpp](sparql_engine.cpp) offers a limitedimplementation of the query language as well as SPARQL Update. 


## Implementation

The code relies heavily on the latest C++0x language features, note that because of limited support in MSVC 2010 work around are used in the code enclosed in `#ifndef __GNUG__` ...`#endif`.

## Pool allocator

Resources are allocated on a pool, each class has its own pool, this offers multiple advantages:

* all instances of a given class can be listed by iterating the corresponding pool
* a pool can be made persistent so all instances are preserved between runs
* the maximum pool size is set at compile time so determining the pointer size to reference every instances 
* access control can be added to track memory access errors
* resources can be allocated at a set address

## Reference

Once classes and members are defined in code by specializing templates any new object becomes part of a RDF document, that can be published (RDFXML) and modified (SPARQL).

## Persistence

Allocating an object on the persistent store guarantees that the pointer will always be valid, including between runs of the programs.
The only way the pointer would become invalid is if the program is recompiled and causes the pool index to change, obviously changing the pointed-to class will also cause problems.
Safeguards mechanisms can be put in place to prevent strange behaviours (hashing the file, ...).
Modifying the class is a normal evolution of a program and updating the database can be done through RDF dumps and -possibly- XML transformations (XSLT).
Pointers have to be stored from one run to the next, they can be put in an array which also gets persisted, an alternative is to use the pool itself, iterated through each cells.
Alternatively an object can be allocated at a set address.
The set of pools can be seen as the RDF document.
Persistence is implemented by memory mapped files, the synchronization is controlled by the kernel and might cause a lot of disk writes, a simple solution is to use a file in RAM using tmpfs for instance.
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

Trigger are used to notify a resource that one property will be modified (add, delete or edit), in the SPARQL Update syntax an edit is done by deleting than adding the modified triplet, the user-provided handler receives the new value, test the value and decides whether to accept or reject it.

## Versioning

It might be desirable to record versions of a predicate, one solution is to stow away a copy of the entire resource before removing a predicate (it means the resource and the copy will only differ by one predicate), the new - modified - resource will have a reference to the copy e.g:

```xml
<inv:Set rdf:ID='test'>
	<inv:located rdf:resource='#naguru'/>
	<inv:on>0</inv:on>
	<obj:prev rdf:nodeID='ae2'/>
</inv:Set>
<inv:Set rdf:nodeID='ae2'>
	<inv:located rdf:resource='#kololo'/>
	<inv:on>0</inv:on>
</inv:Set>
```
We modified the property inv:located in the resource test from 'kololo' to 'naguru', note that the copy is a blank node and can only be reached through the objrdf:prev property of the resource, the copy must not be modified.

## Privileges

Any modification to the document through SPARQL or RDF parsing is done through the class s function table which provide a generic interface to all the properties (similar to virtual functions). That function table can be modified to change behaviour e.g:

* disable editing of resources allocated on the persistent store mapped to a read-only file 
* hide properties 	
* create pseudo-properties (e.g: rdf:type)
* obfuscate/encrypt properties 
Rather than modify the existing table we can extend it and use offset based on user, 'root' has access to the original table and can do anything, other users will use a filtered copy of the original.

## Data structure alignment

Alignment requirements might necessitate padding between members, this will increase the memory usage (bigger database files), the framework offers some information about members (RDF properties) offset and size and can help reorder to limit the padding. Note that data mis-alignment results in slower memory access (more pointer operation) and could be a problem in CPU intensive operation (see here for instance)


##questions/ideas

comparison with relational db: is a pool similar to a table?...
