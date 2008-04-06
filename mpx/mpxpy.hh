#include <boost/python.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/cstdint.hpp>
#include <Python.h>
#include "mpx/types.hh"
#include "mpx.hh"
#include "mcs/mcs.h"
using namespace boost::python;

namespace mpxpy
{
	std::string
	variant_repr(MPX::Variant &self);
	
	void
	variant_setint(MPX::Variant  &self, gint64 value);

	gint64
	variant_getint(MPX::Variant  &self);

	std::string	
	variant_getstring(MPX::Variant  &self);

	void
	variant_setstring(MPX::Variant  &self, std::string const& value);

	double
	variant_getdouble(MPX::Variant  &self);

	void
	variant_setdouble(MPX::Variant  &self, double const& value);

	std::string
	opt_repr(MPX::OVariant &self);

	void
	opt_init(MPX::OVariant &self);
}

namespace MPX
{
	void
	mpx_py_init();
}
