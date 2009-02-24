/* vim:set et sts=4: */
#ifndef __ENGINE_H__
#define __ENGINE_H__

#include <ibus.h>

#define IBUS_TYPE_RAWCODE_ENGINE	\
	(ibus_rawcode_engine_get_type ())

GType   ibus_rawcode_engine_get_type    (void);

#endif
