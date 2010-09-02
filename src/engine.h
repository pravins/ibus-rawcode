/* vim:set et sts=4: */
/* 
* Copyright (C) 2009-2010 Pravin Satpute <psatpute@redhat.com>
* Copyright (C) 2009-2010 Red Hat, Inc.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; version 2 only
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Library General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __ENGINE_H__
#define __ENGINE_H__

#include <ibus.h>

#define IBUS_TYPE_RAWCODE_ENGINE	\
	(ibus_rawcode_engine_get_type ())

GType   ibus_rawcode_engine_get_type    (void);

#endif
