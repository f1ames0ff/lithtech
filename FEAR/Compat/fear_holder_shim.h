#ifndef FEAR_HOLDER_SHIM_H
#define FEAR_HOLDER_SHIM_H

#include "ltmodule.h"

#undef define_interface
#undef define_holder

#define define_interface(impl_class, interface_class) \
    instantiate_interface(impl_class, interface_class, Fear);

#define define_holder(interface_name, ptr_var) \
    define_holder_to_instance(interface_name, ptr_var, Fear)

#endif
