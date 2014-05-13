<?xml version="1.0"?>
<xsl:stylesheet version="1.0" 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:s='http://www.w3.org/2005/sparql-results#'
	xmlns='http://www.w3.org/1999/xhtml'
	xmlns:xlink='http://www.w3.org/1999/xlink'
	xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'
	xmlns:mon='http://monitor.unicefuganda.org/#'
	xmlns:geo='http://www.w3.org/2003/01/geo/wgs84_pos#'
>
<!--
	should be applied to result of
	select * where {?x a :Site .} order by ?x
-->
<xsl:template match='s:results'>
	<a href='https://maps.google.com/?q=http://monitor.unicefuganda.org:1080/cgi-bin/test.sh?1=2' target='_blank'>map</a>
	<ul><xsl:apply-templates/></ul>
</xsl:template>
<xsl:template match='s:uri'>
	<li><a href='{.}'><xsl:value-of select='.'/></a></li>
</xsl:template>
<!-- 
	applied to describe * where {?x a :Site .} order by ?x 
	so we can test if the mon:name attribute is present, the problem with that is that a lot of data is sent (but only once),
	without compression, causing a delay when first loading the page.
	we need support for OPTIONAL in sparql
	should be simpler than this 
-->
<xsl:template match='*' mode='id'><xsl:value-of select='@rdf:ID'/></xsl:template>
<xsl:template match='*[@rdf:nodeID]' mode='id'><xsl:value-of select="concat('blank#',@rdf:nodeID)"/></xsl:template>
<xsl:template match='rdf:RDF'>
	<a title='map all sites' href='https://maps.google.com/?q=http://monitor.unicefuganda.org:1080/cgi-bin/test.sh?1=2' target='_blank'>map</a>
	<ul><xsl:apply-templates/></ul>
</xsl:template>
<xsl:template match='mon:Site[mon:name]'>
	<li><a>
		<xsl:attribute name='href'><xsl:apply-templates mode='id' select='.'/></xsl:attribute>
		<xsl:value-of select='mon:name'/>
	</a></li>
</xsl:template>
<xsl:template match='mon:Site'>
	<li><a>
		<xsl:attribute name='href'><xsl:apply-templates mode='id' select='.'/></xsl:attribute>
		<xsl:value-of select='@rdf:ID|@rdf:nodeID'/>
	</a></li>
</xsl:template>

</xsl:stylesheet>
