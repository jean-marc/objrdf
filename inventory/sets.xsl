<?xml version="1.0"?>
<xsl:stylesheet version="1.0" 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:s='http://www.w3.org/2005/sparql-results#'
	xmlns='http://www.w3.org/1999/xhtml'
	xmlns:xlink='http://www.w3.org/1999/xlink'
	xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'
	xmlns:mon='http://monitor.unicefuganda.org/#'
	xmlns:svg='http://www.w3.org/2000/svg'
>
<!--
	should be applied to result of
	 * where {?x a :Site .} order by ?x
	could also use describe to give more info
-->
<xsl:template match='*' mode='id'><xsl:value-of select='@rdf:ID'/></xsl:template>
<xsl:template match='*[@rdf:nodeID]' mode='id'><xsl:value-of select="concat('blank#',@rdf:nodeID)"/></xsl:template>
<!-- handle empty reply -->
<xsl:template match='rdf:RDF'>
<div><xsl:apply-templates/></div>
</xsl:template>
<xsl:template match='mon:Kiosk|mon:DDrum|mon:DDoorway|mon:Uniport'>
<xsl:variable name='id'><xsl:apply-templates mode='id' select='.'/></xsl:variable> 
<div id="{$id}">
	<xsl:attribute name='class'>set <xsl:if test='mon:ip_add'>on</xsl:if></xsl:attribute>
	<div class='hh'><xsl:value-of select='@rdf:ID|@rdf:nodeID'/> | <a href='#'>edit</a> | <a href='#'>delete</a></div>
	<div class='kkk'>
		<a target='_blank' title='historical data' href="{concat('/sparql?query=prefix :&lt;http://monitor.unicefuganda.org/%23&gt; select ?time ?uptime ?volt ?client ?data where{&lt;',$id,'&gt; :logger ?x .?x :time_stamp_v ?time;:uptime_v ?uptime;:volt ?volt;:n_client ?client;:data ?data .}&amp;xsl=table.xsl')}">
			<xsl:copy-of select='./mon:plot/svg:svg'/>
		</a>
		<p>Latest Webalizer <a href="{concat('/monitor/webalizer/',@rdf:ID,'/index.html')}">report</a></p>
		<xsl:if test='mon:ip_add'><p>VPN IP address:<xsl:value-of select='mon:ip_add'/></p></xsl:if>
	</div>
</div>
</xsl:template>
<xsl:template match='mon:Set'>
<div>--</div>
</xsl:template>
<!-- mon:Persons -->
<xsl:template match='mon:Person'>
<div class='person'>
	<xsl:attribute name='id'><xsl:apply-templates mode='id' select='.'/></xsl:attribute> 
	<div class='hh'>
	<xsl:choose>
		<xsl:when test='mon:first_name'><xsl:value-of select="concat(mon:first_name,' ',mon:last_name)"/></xsl:when> 	
		<xsl:otherwise><xsl:value-of select='@rdf:ID|@rdf:nodeID'/></xsl:otherwise>
	</xsl:choose>
	&#160;<xsl:value-of select='mon:email'/> | <a href='#'>edit</a> | <a href='#'>delete</a>
	</div>
	<div class='phone'>
	<ul>
		<xsl:for-each select='mon:phone'>
			<li><xsl:value-of select='.'/></li>
		</xsl:for-each>
	</ul>	
	</div>
</div>
</xsl:template>
<!-- 
     id gets a bit complicated because of blank nodes	
     maybe server could come up with its own obj:ID type
     problem: it confuses html DOM, so we should escape
-->
<xsl:template match='mon:Report'>
<div class='report'>
	<xsl:attribute name='id'><xsl:apply-templates mode='id' select='.'/></xsl:attribute> 
	<div class='hh'><xsl:value-of select="substring-before(mon:time_stamp_v,'T')"/> | <a href='#' title='modify text and date'>edit</a> | <a href='#'>delete</a></div>
	<div class='text'><xsl:value-of select='mon:text'/></div>
</div>
</xsl:template>


</xsl:stylesheet>
