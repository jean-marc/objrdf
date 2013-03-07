<?xml version="1.0"?>
<xsl:stylesheet version="1.0" 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:s='http://www.w3.org/2005/sparql-results#'
	xmlns='http://www.w3.org/1999/xhtml'
	xmlns:xlink='http://www.w3.org/1999/xlink'
	xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'
>
<!--
	how do we get here? describe or select?

	describe <id>

	select * where {<id> ?p ?v .} 

	this is not going to show properties that are not set

	alternatively we could have the server create the forms (some extra-work but would be really fast and robust)

-->
<!--
	we can extract the type from the query result
-->
<!--
<xsl:param name='id' select="string('?')"/>
-->
<!-- is it always the first result ? -->
<xsl:variable name='type' select="s:sparql/s:results/s:result[1]/s:binding[@name='v']/s:uri"/>
<xsl:variable name='id' select="s:sparql/s:results/s:result[3]/s:binding[@name='v']/s:literal"/>
<xsl:variable name='esc_type' select="concat(substring-before($type,'#'),'%23',substring-after($type,'#'))"/>
<!-- 
	could be more specific, we don't need all the literal properties, only the ones relevant to
	the current class
	we could also split literal and non-literal properties
	a lot of queries could be cached on server

-->
<xsl:variable
	name='literals'
	select="document('/sparql?query=select * where {?s rdfs:subClassOf rdf:Literal .}')/s:sparql/s:results/s:result/s:binding/s:uri"
/>
<!-- query rdfs:subPropertyOf to find out if it is a sub-class of rdfs:member -->
<xsl:variable
	name='properties'
	select="document(concat('/sparql?query=select * where {?p rdfs:domain &lt;',$esc_type,'&gt; . ?p rdfs:range ?r;rdfs:subPropertyOf ?s .}'))"
/>
<xsl:variable
	name='values'
	select='.'
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
<a href="/sparql?query=select * where %7B?x a rdfs:Class .%7D&amp;xsl=inventory.xsl">home</a> | 
search by id: <form id='by_id'><input type='text'/></form> |
search by site: <form id='by_site'><input type='text'/></form> |
</div>
<div class='jm'>
<div class='header'><b><xsl:value-of select="concat(substring-after($type,'#'),' ',$id)"/></b> </div>
<form name='add' method='get'>
<table>
<xsl:call-template name='form'>
<xsl:with-param name='properties' select='$properties'/>
</xsl:call-template>
<tr><input type='submit' value='submit'/></tr>
</table>
</form>
</div>
<!--
[<a href='/sparql?query=describe &lt;{$id}&gt;'>rdf</a>]
-->
<script type="text/javascript">
<!--
	we must make the distinction between insert data and delete/insert data

-->
$("form").submit(function(){
	$('input.edit[type!=submit][name!=ID]').each(function(){
		if($(this).attr('value')!=$(this).val()){
			var s='delete data {&lt;<xsl:value-of select='$id'/>&gt; &lt;'+$(this).attr('name')+'&gt; "'+$(this).attr('value')+'" .};insert data {&lt;<xsl:value-of select='$id'/>&gt; &lt;'+$(this).attr('name')+'&gt; "'+$(this).val()+'" .}';
			//alert(s);
			$.post('/',s,function(){})
		}
	});	

	$('textarea.edit[type!=submit][name!=ID]').each(function(){
		var s='insert data {&lt;<xsl:value-of select='$id'/>&gt; &lt;'+$(this).attr('name')+'&gt; "'+$(this).val()+'" .}';
		//alert(s);
		$.post('/',s,function(){})
	});
	$('textarea.add[type!=submit][name!=ID]').each(function(){
		var s='insert data {&lt;<xsl:value-of select='$id'/>&gt; &lt;'+$(this).attr('name')+'&gt; "'+$(this).val()+'" .}';
		//alert(s);
		$.post('/',s,function(){})
	});
	$('input.add[type!=submit][name!=ID]').each(function(){
		if($(this).attr('value')!=$(this).val()){
			var s='insert data {&lt;<xsl:value-of select='$id'/>&gt; &lt;'+$(this).attr('name')+'&gt; "'+$(this).val()+'" .}';
			//alert(s);
			$.post('/',s,function(){})
		}
	});	
	$('select.edit').each(function(){
		if($(this).find('option[selected=true]').attr('value')!=$(this).val()){
			var s='delete data {&lt;<xsl:value-of select='$id'/>&gt; &lt;'+$(this).attr('name')+'&gt; &lt;'+$(this).find('option[selected=true]').attr('value')+'&gt; .};insert data {&lt;<xsl:value-of select='$id'/>&gt; &lt;'+$(this).attr('name')+'&gt; &lt;'+$(this).val()+'&gt; .}';
			//alert(s);
			$.post('/',s,function(){})
		}
	});
	$('select.add').each(function(){
		if($(this).find('option[selected=true]').attr('value')!=$(this).val()){
			var s='insert data {&lt;<xsl:value-of select='$id'/>&gt; &lt;'+$(this).attr('name')+'&gt; &lt;'+$(this).val()+'&gt; .}';
			//alert(s);
			$.post('/',s,function(){})
		}
	});
	$('input.delete[type=checkbox]:checked').each(function(){
		var s='delete data {&lt;<xsl:value-of select='$id'/>&gt; &lt;'+$(this).attr('name')+'&gt; &lt;'+$(this).attr('value')+'&gt; .};insert data {}';//hack because of parser limitation
		//alert(s);
		$.post('/',s,function(){})
	});
	$('input.delete_literal[type=checkbox]:checked').each(function(){
		var s='delete data {&lt;<xsl:value-of select='$id'/>&gt; &lt;'+$(this).attr('name')+'&gt; "'+$(this).attr('value')+'" .};insert data {}';//hack because of parser limitation
		//alert(s);
		$.post('/',s,function(){})
	});
})
</script>
</body>
</html>
</xsl:template>

<xsl:template name='form'>
<!--
	3 parameters
	select * where {?p rdfs:domain <type> .?p rdfs:range ?r .}
	select * where {<id> ?p ?v .}
	select * where {?p rdfs:subClassOf rdf:Literal .}
-->
<xsl:param name='properties'/>
<xsl:apply-templates select='$properties/s:sparql/s:results/*'/>
</xsl:template>

<xsl:template match="s:result">
<xsl:variable name='current_property' select='.'/>
<xsl:variable name='range' select="s:binding[@name='r']/s:uri"/>
<xsl:variable name='esc_range' select="concat(substring-before($range,'#'),'%23',substring-after($range,'#'))"/>
<!--
	we have to look-up the value
	there might be 0, 1 or more matches
-->
<xsl:variable name='p' select="$values/s:sparql/s:results/s:result[s:binding[@name='p']/s:uri=$current_property/s:binding[@name='p']/s:uri]"/>
<xsl:for-each select="$p">
<xsl:variable name='current_instance' select='.'/>
<tr>
<td><xsl:value-of select="substring-after(s:binding[@name='p']/s:uri,'#')"/>:</td>
<xsl:choose>
	<xsl:when test="$literals[text()=$current_property/s:binding[@name='r']/s:uri/text()]">
		<!-- 
			how to customize forms by property type? 
			would be a lot easier with specialized templates
		-->
		<td>
			<xsl:choose>
			<xsl:when test="$current_property/s:binding[@name='p']/s:uri='http://inventory.unicefuganda.org/#text'">
			<textarea class='edit' name="{$current_property/s:binding[@name='p']/s:uri}">
				<xsl:value-of select="$current_instance/s:binding[@name='v']/s:literal"/>
			</textarea>
			</xsl:when>
			<xsl:otherwise>
			<input class='edit' type='text' name="{$current_property/s:binding[@name='p']/s:uri}" value="{$current_instance/s:binding[@name='v']/s:literal}"/>
			</xsl:otherwise>
			</xsl:choose>
		</td>
		<td><input class='delete_literal' type='checkbox' name="{$current_property/s:binding[@name='p']/s:uri}" value="{$current_instance/s:binding[@name='v']/s:literal}"/></td>
	</xsl:when>
	<xsl:otherwise>
		<td>
		<select class='edit' name="{$current_property/s:binding[@name='p']/s:uri}">
		<option value='--'>--</option>
		<xsl:for-each select="document(concat('/sparql?query=select * where {?s a &lt;',$esc_range,'&gt; .}'))/s:sparql/s:results/s:result/s:binding/s:uri">
			<option value='{.}'>
			<xsl:if test="text()=$current_instance/s:binding[@name='v']/s:uri/text()"><xsl:attribute name='selected'>true</xsl:attribute></xsl:if>
			<xsl:value-of select="."/>
			</option>
		</xsl:for-each>
		</select>
		</td>
		<td><input class='delete' type='checkbox' name="{$current_property/s:binding[@name='p']/s:uri}" value="{$current_instance/s:binding[@name='v']/s:uri/text()}"/></td>
	</xsl:otherwise>
</xsl:choose>
</tr>
</xsl:for-each>
<!-- the property is sub-Property of rdfs:member -->
<!-- would be cleaner with OPTIONAL sparql -->
<xsl:if test="not($p) or $current_property/s:binding[@name='s']/s:uri='http://www.w3.org/2000/01/rdf-schema#member'">
<tr>
<td><xsl:value-of select="substring-after(s:binding[@name='p']/s:uri,'#')"/>:</td>
<td>
	<xsl:choose>
	<xsl:when test="$literals[text()=$current_property/s:binding[@name='r']/s:uri/text()]">
		<xsl:choose>
		<xsl:when test="$current_property/s:binding[@name='p']/s:uri='http://inventory.unicefuganda.org/#text'">
		<textarea class='add' name="{$current_property/s:binding[@name='p']/s:uri}"/>
		</xsl:when>
		<xsl:otherwise>
		<input class='add' type='text' name="{$current_property/s:binding[@name='p']/s:uri}" value=""/>
		</xsl:otherwise>
		</xsl:choose>
	</xsl:when>
	<xsl:otherwise>
		<select class='add' name="{$current_property/s:binding[@name='p']/s:uri}">
			<option selected='true' value='--'>--</option>
			<xsl:for-each select="document(concat('/sparql?query=select * where {?s a &lt;',$esc_range,'&gt; .}'))/s:sparql/s:results/s:result/s:binding/s:uri">
				<option value='{.}'><xsl:value-of select="."/></option>
			</xsl:for-each>
		</select>
	</xsl:otherwise>
	</xsl:choose>
</td>
<td/>
</tr>
</xsl:if>

</xsl:template>
<xsl:template match="s:result[s:binding[@name='p']/s:uri='http://www.w3.org/1999/02/22-rdf-syntax-ns#type']"/>
<xsl:template match="s:result[s:binding[@name='p']/s:uri='http://www.example.org/objrdf#self']"/>
<xsl:template match="s:result[s:binding[@name='p']/s:uri='http://www.example.org/objrdf#prev']"/>
<xsl:template match="s:result[s:binding[@name='p']/s:uri='http://www.example.org/objrdf#next']"/>
<xsl:template match="s:result[s:binding[@name='p']/s:uri='http://inventory.unicefuganda.org/#logger']"/>
<xsl:template match="s:result[s:binding[@name='p']/s:uri='http://inventory.unicefuganda.org/#logger']"/>
<xsl:template match="s:result[s:binding[@name='p']/s:uri='http://inventory.unicefuganda.org/#time_stamp']"/>
<xsl:template match="s:result[s:binding[@name='p']/s:uri='http://inventory.unicefuganda.org/#time_stamp_v']"/>
</xsl:stylesheet>
