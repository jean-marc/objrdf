#include "introspect.h"
#include "test.h"
namespace objrdf{
	template struct introspect<test::Test>;
	template struct introspect<test::Testd>;
	template struct introspect<test::p_0>;
	template struct introspect<test::p_1>;
}
