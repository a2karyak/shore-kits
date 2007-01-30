
#ifndef _ATTRIBUTE_OFFSET_H
#define _ATTRIBUTE_OFFSET_H

#include <cstdlib>

#define OFFSET_OF(struct_type, field) ( (size_t)(&((struct_type *)0)->field) )
#define ATTRIBUTE_OFFSET(tuple_struct, attribute) OFFSET_OF(tuple_struct, attribute)


#endif
