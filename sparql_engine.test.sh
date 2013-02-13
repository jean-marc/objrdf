#sparql_engine test
set -e
#echo "SELECT * WHERE {?x <http://www.w3.org/1999/02/22-rdf-syntax-ns#type> ?y .}" | ./sparql_engine.test sparql_engine.1.rdf
#echo "SELECT * WHERE {?x a ?y .}" | ./sparql_engine.test sparql_engine.1.rdf
#echo "SELECT * WHERE {?x ?y ?z .}" | ./sparql_engine.test
#test prefix
#echo "PREFIX  rdf:  <http://www.w3.org/1999/02/22-rdf-syntax-ns#> PREFIX : <http://www.w3.org/1999/02/22-rdf-syntax-ns#> SELECT * WHERE {:type rdf:type ?y;rdf:type ?z;:j \"fdfsdf\";:k 'qwe';:l 1234.0 .}" | ./sparql_engine.test
#echo "PREFIX ex:<http://www.example.org/> SELECT * WHERE{?x ex:p_0 ?a;ex:p_1 ?b;ex:p_2 ?c .}" | ./sparql_engine.test sparql_engine.1.rdf
#echo "PREFIX ex:<http://www.example.org/> SELECT ?x ?a WHERE{?x ex:p_0 ?a;ex:p_1 ?b;ex:p_2 ?c .?x ex:p_2 ?d .}" | ./sparql_engine.test sparql_engine.1.rdf
#echo "PREFIX ex:<http://www.example.org/#> INSERT{?x ex:p_2 3 .} WHERE{?x a ex:C .}" | ./sparql_engine.test sparql_engine.1.rdf
echo "PREFIX ex:<http://www.example.org/#> INSERT DATA {<jb> a ex:C .}" | ./sparql_engine.test
#echo "PREFIX ex:<http://www.example.org/> PREFIX :<> INSERT DATA{<1> a ex:C .}" | ./sparql_engine.test sparql_engine.1.rdf
#echo "PREFIX ex:<http://www.example.org/> DELETE DATA{ex:jm ex:p_2 3 .}" | ./sparql_engine.test sparql_engine.1.rdf

#echo $?
