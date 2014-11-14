#!/bin/bash
# run a sparql SELECT query on database and format result as spreadsheet
# query could be GET or POST
# could use mktemp -d

#DB=http://inventory.unicefuganda.org
DB=localhost:1080
#QUERY="prefix :<http://inventory.unicefuganda.org/#> 
	#select * where {
		#?x a :Equipment;a ?type;:partOf ?partOf;:model ?model;:status ?status .
	#}order by ?x E"
QUERY="prefix :<http://inventory.unicefuganda.org/#> prefix geo:<http://www.w3.org/2003/01/geo/wgs84_pos#>
	select * where {
		?x :organization <#Warchild>;geo:lat ?lat;geo:long ?long .
	}order by ?x E"
curl --data-binary "$QUERY" $DB | xsltproc calc.xsl - > content.xml
zip -r - content.xml settings.xml styles.xml META-INF/manifest.xml  > sparql.ods
