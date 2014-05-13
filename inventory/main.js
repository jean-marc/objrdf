/*
 *	attached to main.html, query the db and populate div's
 *
 *
 */
var ns = {
	'xhtml' : 'http://www.w3.org/1999/xhtml',
	'mathml': 'http://www.w3.org/1998/Math/MathML',
	'rdfs'  : 'http://www.w3.org/2000/01/rdf-schema#',
	'rdf'   : 'http://www.w3.org/1999/02/22-rdf-syntax-ns#',
	'g'	: 'http://www.sat.com/2011/generate#',
	's'	: 'http://www.w3.org/2005/sparql-results#'
};

function nsResolver(prefix) {return ns[prefix] || null;}

var sites_xsl,site_presentation_xsl,sets_xsl,edit_xsl,current_site;
function live_sites(){
//query the db to find if any site came on-line
	$.post('/','prefix :<http://monitor.unicefuganda.org/#> select ?site where {?s a :Set;:on 1;:located ?site .}_buggy_',function(data,status){
		iterator=data.evaluate("//s:uri/text()",data,nsResolver,XPathResult.UNORDERED_NODE_ITERATOR_TYPE,null);
		var node = iterator.iterateNext();
		//need to remove if not in the list
		$('#left a.on').removeClass('on');
		while (node) {
			$('#left a[href='+node.textContent+']').addClass('on');
			node = iterator.iterateNext();
		}
	});
}
function refresh_site(){
	$.post('/','describe <'+current_site+'>',function(data,status){
		xsltProcessor=new XSLTProcessor();
		xsltProcessor.importStylesheet(site_presentation_xsl);
		resultDocument = xsltProcessor.transformToFragment(data,document);
		$('#current_site :nth-child(2)').replaceWith(resultDocument);
	});
}
function refresh_sets(){
	$.post('/','prefix :<http://monitor.unicefuganda.org/#> describe * where {?s a :Set;:located <'+current_site+'> .}_buggy_',function(data,status){
		xsltProcessor=new XSLTProcessor();
		xsltProcessor.importStylesheet(sets_xsl);
		resultDocument = xsltProcessor.transformToFragment(data,document);
		$('#main_top :nth-child(2)').replaceWith(resultDocument);
		$("#main_top div.set a:contains('edit')").click(edit_set);
		$("#main_top div.set a:contains('delete')").click(delete_set);
	});
}
function edit_set(){
	//find the report id, going up
	id=$(this).parents('div.set').attr('id');
	//jquery does not like # in id
	//$(document.getElementById(report_id)).find('div.text').hide();	
	$.post('/','prefix :<http://monitor.unicefuganda.org/#> select * where {<'+id+'> ?p ?v .}_buggy_',function(data,status){
		xsltProcessor=new XSLTProcessor();
		xsltProcessor.importStylesheet(edit_xsl);
		resultDocument = xsltProcessor.transformToFragment(data,document);
		$(document.getElementById(id)).find('div.kkk').replaceWith(resultDocument);	
		$(document.getElementById(id)).find('form').submit({subject:id,callback:refresh_sets},edit_handler);
	});	
	return false;
}

function edit_report(){
	//find the report id, going up
	report_id=$(this).parents('div.report').attr('id');
	//jquery does not like # in id
	//$(document.getElementById(report_id)).find('div.text').hide();	
	$.post('/','prefix :<http://monitor.unicefuganda.org/#> select * where {<'+report_id+'> ?p ?v .}_buggy_',function(data,status){
		xsltProcessor=new XSLTProcessor();
		xsltProcessor.importStylesheet(edit_xsl);
		resultDocument = xsltProcessor.transformToFragment(data,document);
		$(document.getElementById(report_id)).find('div.text').replaceWith(resultDocument);	
		$(document.getElementById(report_id)).find('form').submit({subject:report_id,callback:refresh_reports},edit_handler);
	});	
	return false;
}
function delete_report(){
	$.post('/','prefix :<http://monitor.unicefuganda.org/#> delete data {<'+current_site+'> :report <'+$(this).parents('div.report').attr('id')+'> .}_buggy_',function(data,status){
		refresh_reports();
	});
	return false;
}
function add_contact(){


}
function edit_contact(){
	//would be nice to toggle
	person_id=$(this).parents('div.person').attr('id');
	//jquery does not like # in id
	//$(document.getElementById(report_id)).find('div.text').hide();	
	$.post('/','prefix :<http://monitor.unicefuganda.org/#> select * where {<'+person_id+'> ?p ?v .}_buggy_',function(data,status){
		xsltProcessor=new XSLTProcessor();
		xsltProcessor.importStylesheet(edit_xsl);
		resultDocument = xsltProcessor.transformToFragment(data,document);
		$(document.getElementById(person_id)).find('div.phone').replaceWith(resultDocument);	
		$(document.getElementById(person_id)).find('form').submit({subject:person_id,callback:refresh_contacts},edit_handler);
		//we need to update the current site in add form
		//$('#add_contact select option[value='+current_site+']').attr('selected','true');	
	});	
	return false;
}
function delete_contact(){
	$.post('/','prefix :<http://monitor.unicefuganda.org/#> delete data {<'+$(this).parents('div.person').attr('id')+'> :site <'+current_site+'> .}_buggy_',function(data,status){
		refresh_contacts();
	});
	return false;
}
/*
 *	generic form handler
 */
function edit_handler(e){
	var n_query=0;
	function test(data,status){
		--n_query;
		if(n_query==0){
			e.data.callback();
		}
	}
	$('input.edit[type!=submit][name!=ID]').each(function(){
		if($(this).attr('value')!=$(this).val()){
			var s='delete data {<'+e.data.subject+'> <'+$(this).attr('name')+'> "'+$(this).attr('value')+'" .};insert data {<'+e.data.subject+'> <'+$(this).attr('name')+'> "'+$(this).val()+'" .}';
			++n_query;
			$.post('/',s,test);
		}
	});
	$('input.add[type!=submit][name!=ID]').each(function(){
		if($(this).attr('value')!=$(this).val()){
			var s='insert data {<'+e.data.subject+'> <'+$(this).attr('name')+'> "'+$(this).val()+'" .}';
			++n_query;
			$.post('/',s,test)
		}
	});
	$('textarea.edit[type!=submit][name!=ID]').each(function(){
		var s='insert data {<'+e.data.subject+'> <'+$(this).attr('name')+'> "'+$(this).val()+'" .}';
		++n_query;
		$.post('/',s,test)
	});
	$('textarea.add[type!=submit][name!=ID]').each(function(){
		var s='insert data {<'+e.data.subject+'> <'+$(this).attr('name')+'> "'+$(this).val()+'" .}';
		++n_query;
		$.post('/',s,test)
	});
	$('select.edit').each(function(){
		if($(this).find('option[selected=true]').attr('value')!=$(this).val()){
			var s='delete data {<'+e.data.subject+'> <'+$(this).attr('name')+'> <'+$(this).find('option[selected=true]').attr('value')+'> .};insert data {<'+e.data.subject+'> <'+$(this).attr('name')+'> <'+$(this).val()+'> .}';
			++n_query;
			$.post('/',s,test)
		}
	});
	$('select.add').each(function(){
		if($(this).find('option[selected=true]').attr('value')!=$(this).val()){
			var s='insert data {<'+e.data.subject+'> <'+$(this).attr('name')+'> <'+$(this).val()+'> .}';
			++n_query;
			$.post('/',s,test)
		}
	});
	$('input.delete[type=checkbox]:checked').each(function(){
		var s='delete data {<'+e.data.subject+'> <'+$(this).attr('name')+'> <'+$(this).attr('value')+'> .};insert data {}';//hack because of parser limitation
		++n_query;
		$.post('/',s,test)
	});
	$('input.delete_literal[type=checkbox]:checked').each(function(){
		var s='delete data {<'+e.data.subject+'> <'+$(this).attr('name')+'> "'+$(this).attr('value')+'" .};insert data {}';//hack because of parser limitation
		++n_query;
		$.post('/',s,test)
	});
	return false;
}
function refresh_reports(){
	//order by time_stamp
	$.post('/','prefix :<http://monitor.unicefuganda.org/#> describe * where {<'+current_site+'> :report ?r .} _buggy_',function(data,status){
		xsltProcessor=new XSLTProcessor();
		xsltProcessor.importStylesheet(sets_xsl);
		resultDocument = xsltProcessor.transformToFragment(data,document);
		$('#main_middle :nth-child(2)').replaceWith(resultDocument);
		$("#main_middle div.report a:contains('edit')").click(edit_report);
		$("#main_middle div.report a:contains('delete')").click(delete_report);
	});
}
function refresh_contacts(){
	$.post('/','prefix :<http://monitor.unicefuganda.org/#> describe * where {?p a :Person;:site <'+current_site+'> .}_buggy_',function(data,status){
		xsltProcessor=new XSLTProcessor();
		xsltProcessor.importStylesheet(sets_xsl);
		resultDocument = xsltProcessor.transformToFragment(data,document);
		//$('#right :nth-child(2)').replaceWith(resultDocument);
		$('#right div.payload :nth-child(1)').replaceWith(resultDocument);
		$("#right a:contains('edit')").click(edit_contact);
		$("#right a:contains('delete')").click(delete_contact);
		$("#add_contact select").val(current_site);	
	});
}
function add_handler(e){
	s='insert data {[] a <'+e.data.type+'>';
	$('#add_contact form input[type!=submit][name!=ID]').each(function(){
		if($(this).attr('value')!=$(this).val())
			s+=';<'+$(this).attr('name')+'> "'+$(this).val()+'"';
	});	
	$('#add_contact form select').each(function(){
		if($('option[selected=true]').attr('value')!=$(this).val())
			s+=';<'+$(this).attr('name')+'> <'+$(this).val()+'>';
	});
	$('#add_contact form textarea').each(function(){
		s+=';<'+$(this).attr('name')+'> "'+$(this).val()+'"';
	});
	s+=' .}'
	$(this).closest('form').find("input[type=text], textarea").val("");	
	$.post('/',s,function(data,status){
		e.data.callback();
	})
	return false;
}
function list_sites(){
	//$.post('/','prefix :<http://monitor.unicefuganda.org/#> select * where {?s a :Site .} order by ?s _buggy_',function(data,status){	
	//$.post('/','prefix :<http://monitor.unicefuganda.org/#> describe ?s where {?s a :Site;:name ?n .} order by ?n _buggy_',function(data,status){	
	$.post('/','prefix :<http://monitor.unicefuganda.org/#> describe * where {?s a :Site;:name ?n .} order by ?n _buggy_',function(data,status){	
		xsltProcessor=new XSLTProcessor();
		xsltProcessor.importStylesheet(sites_xsl);
		resultDocument = xsltProcessor.transformToFragment(data,document);
		$('#left :nth-child(2)').replaceWith(resultDocument);
		//attach callbacks to newly created links
		$('#left ul a').click(function(){
			//send a query to get all sets attached to site
			current_site=$(this).attr('href');
			//update title and site info
			$('#current_site h1').text($(this).text());
			refresh_site();
			refresh_sets();
			refresh_reports();
			refresh_contacts();
			return false;
		});
		$('#main_bottom form').submit(function(){
			$.post('/','prefix :<http://monitor.unicefuganda.org/#> insert data {<'+current_site+'> :report [:text "'+$('#main_bottom form textarea').val()+'"] .}_buggy_',function(data,status){
				//clear form
				$('#main_bottom form textarea').val('');
				refresh_reports();
			});
			return false;
		});
		//live_sites();
	});
}
function start(){
	$.get('sites.xsl',function(data){
		sites_xsl=data;	
		list_sites();
	});
	//is a site given in the URL
	//alert(window.location.match(/site=([^&]+)/)[1])
	//alert(location)
	$.get('site_presentation.xsl',function(data){site_presentation_xsl=data;});
	$.get('sets.xsl',function(data){sets_xsl=data;});
	$.get('edit.xsl',function(data){edit_xsl=data;});
	//add contact
	$.get('add.xsl',function(data){
		add_xsl=data;
		$.post('/','describe <http://monitor.unicefuganda.org/#Person>',function(data,status){
			xsltProcessor=new XSLTProcessor();
			xsltProcessor.importStylesheet(add_xsl);
			resultDocument = xsltProcessor.transformToFragment(data,document);
			$('#add_contact :nth-child(1)').replaceWith(resultDocument);
			$('#add_contact input[type=submit]').val('add contact');
			$("#add_contact form").submit({type:'http://monitor.unicefuganda.org/#Person',callback:refresh_contacts},add_handler);
		});
	});
	setInterval(live_sites,5000);//query db every 5 sec for new sites
}
