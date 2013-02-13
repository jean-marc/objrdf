<?xml version="1.0"?>
<xsl:stylesheet version="1.0" 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:s='http://www.w3.org/2005/sparql-results#'
	xmlns='http://www.w3.org/1999/xhtml'
	xmlns:xlink='http://www.w3.org/1999/xlink'
	xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'
	xmlns:obj='http://www.example.org/objrdf#'
>
<xsl:template match='/'>
<html>
<head>
<style type='text/css'>
body{
	font-family:helvetica;
}
</style>

</head>
<body>
<div id='jm'>
<table>
<xsl:apply-templates/>
</table>
</div>
</body>
</html>
</xsl:template>
<xsl:template match='s:sparql'>
	<table>
	<xsl:apply-templates select='s:head'/>
	<xsl:apply-templates select='s:results'/>
	</table>
</xsl:template>
<xsl:template match='s:head'>
	<tr><xsl:apply-templates select='s:variable'/></tr>
</xsl:template>
<xsl:template match='s:variable'><th><xsl:value-of select='@name'/></th>
</xsl:template>
<xsl:template match='s:results[not(node())]'>
No result!
</xsl:template>
<xsl:template match='s:results'>
<xsl:apply-templates/>
</xsl:template>
<xsl:template match='s:result'><tr><xsl:apply-templates/></tr></xsl:template>
<xsl:template match='s:binding'><xsl:apply-templates/></xsl:template>
<xsl:template match="s:uri[contains(.,'#')]"><td>
<xsl:variable name='esc_uri' select="concat(substring-before(.,'#'),'%23',substring-after(.,'#'))"/>
<!-- send to editing form -->
<a href="{concat('/sparql?query=select * where {&lt;',$esc_uri,'&gt; ?p ?v .}')}&amp;xsl=edit_form.xsl"><xsl:value-of select='.'/></a>|
<!--<a href="{concat('/sparql?query=DESCRIBE &lt;',$esc_uri,'&gt;')}"><xsl:value-of select='.'/></a>|-->
<!--
	only makes sense if the URI is a Class, can we pass parameters to style sheet? yes!
	we can look up the type of the current objects
-->
<a href="{concat('/sparql?query=SELECT * WHERE {?x a &lt;',$esc_uri,'&gt; .}&amp;xsl=describe.xsl')}">list</a>|
<!--
	a form to add a new element, need to list all the properties of the Class
	how do we separate literal from non literal
	should look into caching common queries
-->
<a href="{concat('/sparql?query=SELECT * WHERE {?p rdfs:domain &lt;',$esc_uri,'&gt; . ?p rdfs:range ?r .}&amp;xsl=add_form.xsl&amp;xslt-param-name=type&amp;xslt-param-value=',$esc_uri)}">add</a>
</td></xsl:template>

<xsl:template match="s:uri"><td>
<a href="{concat('/sparql?query=select * where {&lt;',.,'&gt; ?p ?v .}&amp;xsl=edit_form.xsl&amp;xslt-param-name=id&amp;xslt-param-value=',.)}"><xsl:value-of select='.'/></a>|
<!--
<a href="{concat('/sparql?query=DESCRIBE &lt;',.,'&gt;')}"><xsl:value-of select='.'/></a>|
-->
<!--
	only makes sense if the URI is a Class
-->
<a href="{concat('/sparql?query=SELECT * WHERE {?x a &lt;',.,'&gt; .}&amp;xsl=describe.xsl')}">list</a>
</td></xsl:template>

<xsl:template match='s:literal'><td><xsl:apply-templates/></td></xsl:template>
<xsl:template match='rdf:RDF'>
	<xsl:apply-templates mode='resource'/>
</xsl:template>
<xsl:template match='*' mode='resource'>
	<xsl:apply-templates select='.' mode='uri'/>
	<xsl:apply-templates select='*' mode='property'/>
</xsl:template>
<xsl:template match='obj:id' mode='property'/>
<xsl:template match='*[@rdf:about]' mode='uri'>
	<tr style='background-color:#eeeeee;'>
	<td>
	<b><xsl:value-of select='@rdf:about'/></b>
	</td>
	<td/>
	<td/>
	<td><a href=''>edit</a></td>
	</tr>
</xsl:template>
<xsl:template match='*[@rdf:ID]' mode='uri'>
	<tr style='background-color:#eeeeee;'>
	<td>
	<b><xsl:value-of select='@rdf:ID'/></b> 
	<!--<a href="{concat('/sparql?query=select * where {&lt;',substring-before(@rdf:ID,'#'),'%23',substring-after(@rdf:ID,'#'),'&gt; ?p ?v .}&amp;xsl=edit_form.xsl')}">edit</a>-->
	</td>
	<td/>
	<td/>
	<td><a href="{concat('/sparql?query=select * where {&lt;',@rdf:ID,'&gt; ?p ?v .}&amp;xsl=edit_form.xsl')}">edit</a></td>
	</tr>
</xsl:template>
<xsl:template match='*[@rdf:resource]' mode='property'>
<!--
	we need to get URI for property
	INSERT DATA 
-->
	<xsl:variable name='object'><xsl:apply-templates select='@rdf:resource' mode='uri'/></xsl:variable>
	<tr>
	<td/>
	<!--<td><xsl:value-of select="concat(namespace-uri(),local-name())"/></td>-->
	<td><xsl:value-of select='name()'/></td>
	<td><a href="{concat('/sparql?query=describe &lt;',substring-before(@rdf:resource,'#'),'%23',substring-after(@rdf:resource,'#'),'&gt;&amp;xsl=describe.xsl')}"><xsl:value-of select='@rdf:resource'/></a></td>
	<td/>
	</tr>
</xsl:template>
<xsl:template match='*' mode='property'>
	<tr>
	<td/>
	<td><xsl:value-of select='name()'/></td>
	<td><xsl:apply-templates/></td>
	</tr>
</xsl:template>	
<xsl:template match='node()'>
	<xsl:copy-of select='.'/>
</xsl:template>
</xsl:stylesheet>
