<?xml version="1.0"?>
<xsl:stylesheet version="1.0" 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:s='http://www.w3.org/2005/sparql-results#'
	xmlns='http://www.w3.org/1999/xhtml'
	xmlns:xlink='http://www.w3.org/1999/xlink'
	xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'
>
<!--
	should be applied to result of

	SELECT * WHERE {?p rdfs:domain <type>; . ?p rdfs:range ?r .}
	problem: we don't know the type, and we need it, we can use xslt-parameter but not supported by Chrome
	could use URL parameter

-->
<xsl:param name='type'/>
<xsl:variable
	name='literals'
	select="document('/sparql?query=select * where {?s rdfs:subClassOf rdf:Literal .}')/s:sparql/s:results/s:result/s:binding/s:uri"
/>
<!--
	could also query rdfs:comment for properties but we need support for optional in sparql first
-->
<xsl:template match="s:results">
<html>
<head>
<script src="jquery.js" type="text/javascript"></script>
<link rel='stylesheet' type='text/css' href='inventory.css'/>
</head>
<body>
<div id='top'>
<a href="/sparql?query=select * where %7B?x a rdfs:Class .%7D order by ?x&amp;xsl=inventory.xsl">home</a> | 
search by id: <form id='by_id'><input type='text'/></form> |
search by site: <form id='by_site'><input type='text'/></form> |
</div>
<div class='jm'>
<div class='header'><b><xsl:value-of select="substring-after($type,'#')"/></b> </div>
<form name='add' method='get'>
<table>
<tr>
<td>ID:</td><td><input type='text' name='ID'/></td></tr>
<xsl:apply-templates/>
<tr><input type='submit' value='create'/></tr>
</table>
</form>
</div>
<!--
	insert data {<id> a <some_type>;<p_0> <v_0>;... .}
	need to go through all the input
	if id is not defined we have to use blank node, often we don't want to create new id
-->
<script type="text/javascript">
$("form").submit(function(){
	id=$('.jm form input:first').val();
	var s;
	if(id)
		s='insert data {&lt;'+id+'&gt; a &lt;<xsl:value-of select='$type'/>&gt;';
	else
		s='insert data {[] a &lt;<xsl:value-of select='$type'/>&gt;';
	$('.jm form input[type!=submit][name!=ID]').each(function(){
		if($(this).attr('value')!=$(this).val())
			s+=';&lt;'+$(this).attr('name')+'&gt; "'+$(this).val()+'"';
	});	
	$('.jm form select').each(function(){
		if($('option[selected=true]').attr('value')!=$(this).val())
			s+=';&lt;'+$(this).attr('name')+'&gt; &lt;'+$(this).val()+'&gt;';
	});
	s+=' .}'
	alert(s);
	$.post('/',s,function(data,status){
		//what would be the right thing to do after creating new resource?
		//we could do a describe unless it is blank node: we don't know the ID
		alert(status);
	})
})
</script>
</body>
</html>
</xsl:template>
<xsl:template name='escape'>
</xsl:template>
<xsl:template match="s:result">
<!--
	we want to create an INSERT DATA { } query but we need to know the type

	the HTML way to send a form is 
	http://server/page?a=0&b=1&c=2&...
	we could handle that on the server and create the SPARQL query or use javascript

-->
<xsl:variable name='tmp' select='.'/>
<xsl:variable name='range' select="s:binding[@name='r']/s:uri"/>
<tr>
<td><xsl:value-of select="substring-after(s:binding[@name='p']/s:uri,'#')"/>:</td>
<td>
<xsl:choose>
	<xsl:when test="$literals[text()=$tmp/s:binding[@name='r']/s:uri/text()]">
		<input type='text' name="{$tmp/s:binding[@name='p']/s:uri}" value=""/><!-- interesting browser escapes characters here -->
	</xsl:when>
	<xsl:otherwise>
		<select name="{$tmp/s:binding[@name='p']/s:uri}">
		<!-- we also ask for the type because of entailment we will get sub-classes --> 
		<!--<xsl:for-each select="document(concat('/sparql?query=select * where {?s a &lt;',substring-before($range,'#'),'%23',substring-after($range,'#'),'&gt;;a ?t .}'))/s:sparql/s:results/s:result/s:binding/s:uri">-->
		<option value='--' selected='true'>--</option>
		<xsl:for-each select="document(concat('/sparql?query=select * where {?s a &lt;',substring-before($range,'#'),'%23',substring-after($range,'#'),'&gt; .}'))/s:sparql/s:results/s:result/s:binding/s:uri">
			<!--<option value='{.}'><xsl:value-of select="substring-after(.,'#')"/></option>-->
			<option value='{.}'><xsl:value-of select="."/></option>
		</xsl:for-each>
		</select>
	</xsl:otherwise>
</xsl:choose>
</td>
</tr>
</xsl:template>
<xsl:template match="s:result[s:binding[@name='p']/s:uri='http://www.w3.org/1999/02/22-rdf-syntax-ns#type']"/>
<xsl:template match="s:result[s:binding[@name='p']/s:uri='http://www.example.org/objrdf#id']"/>
<xsl:template match="s:result[s:binding[@name='p']/s:uri='http://www.example.org/objrdf#self']"/>
<xsl:template match="s:result[s:binding[@name='p']/s:uri='http://www.example.org/objrdf#prev']"/>
<xsl:template match="s:result[s:binding[@name='p']/s:uri='http://www.example.org/objrdf#next']"/>
</xsl:stylesheet>
