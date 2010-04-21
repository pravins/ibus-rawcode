/* vim:set et sts=4: */

#include <ibus.h>
#include <ibusengine.h>
#include <string.h>
#include "engine.h"
#include <glib.h>

typedef struct _IBusRawcodeEngine IBusRawcodeEngine;
typedef struct _IBusRawcodeEngineClass IBusRawcodeEngineClass;


struct _IBusRawcodeEngine {
	IBusEngine parent;

    /* members */
    GString *buffer;
    int maxpreeditlen;

    IBusLookupTable *table;
    IBusProperty    *rawcode_mode_prop;
    IBusPropList    *prop_list;
};

struct _IBusRawcodeEngineClass {
	IBusEngineClass parent;
};

/* functions prototype */
static void	ibus_rawcode_engine_class_init   (IBusRawcodeEngineClass  *klass);
static void	ibus_rawcode_engine_init		    (IBusRawcodeEngine		*rawcode);
static GObject*
            ibus_rawcode_engine_constructor  (GType                   type,
                                             guint                   n_construct_params,
                                             GObjectConstructParam  *construct_params);
static void	ibus_rawcode_engine_destroy		(IBusRawcodeEngine		*rawcode);
static gboolean
			ibus_rawcode_engine_process_key_event
                                            (IBusEngine             *engine,
                                             guint               	 keyval,
                                             guint               	 keycode,
                                             guint               	 modifiers);
static void ibus_rawcode_engine_focus_in     (IBusEngine             *engine);
static void ibus_rawcode_engine_focus_out    (IBusEngine             *engine);
static void ibus_rawcode_engine_reset        (IBusEngine             *engine);
static void ibus_rawcode_engine_enable       (IBusEngine             *engine);
static void ibus_rawcode_engine_disable      (IBusEngine             *engine);
#if 0
static void ibus_engine_set_cursor_location (IBusEngine             *engine,
                                             gint                    x,
                                             gint                    y,
                                             gint                    w,
                                             gint                    h);
static void ibus_hangul_engine_set_capabilities
                                            (IBusEngine             *engine,
                                             guint                   caps);
#endif
static void ibus_rawcode_engine_page_up      (IBusEngine             *engine);
static void ibus_rawcode_engine_page_down    (IBusEngine             *engine);
static void ibus_rawcode_engine_cursor_up    (IBusEngine             *engine);
static void ibus_rawcode_engine_cursor_down  (IBusEngine             *engine);
#if 0
static void ibus_hangul_property_activate   (IBusEngine             *engine,
                                             const gchar            *prop_name,
                                             gint                    prop_state);
static void ibus_hangul_engine_property_show
											(IBusEngine             *engine,
                                             const gchar            *prop_name);
static void ibus_hangul_engine_property_hide
											(IBusEngine             *engine,
                                             const gchar            *prop_name);
#endif

static void ibus_rawcode_engine_flush        (IBusRawcodeEngine       *rawcode);
static void ibus_rawcode_engine_update_preedit_text
			(IBusRawcodeEngine       *rawcode);
static void ibus_rawcode_engine_process_preedit_text
			(IBusRawcodeEngine       *rawcode);
static gunichar rawcode_get_unicode_value (const GString *preedit);
static int rawcode_ascii_to_hex (int ascii);
static int rawcode_hex_to_ascii (int hex);
static int create_rawcode_lookup_table(IBusRawcodeEngine *rawcode);
static void commit_buffer_to_ibus(IBusRawcodeEngine *rawcode);

static IBusEngineClass *parent_class = NULL;

GType
ibus_rawcode_engine_get_type (void)
{
	static GType type = 0;

	static const GTypeInfo type_info = {
		sizeof (IBusRawcodeEngineClass),
		(GBaseInitFunc)		NULL,
		(GBaseFinalizeFunc) NULL,
		(GClassInitFunc)	ibus_rawcode_engine_class_init,
		NULL,
		NULL,
		sizeof (IBusRawcodeEngine),
		0,
		(GInstanceInitFunc)	ibus_rawcode_engine_init,
	};

	if (type == 0) {
		type = g_type_register_static (IBUS_TYPE_ENGINE,
									   "IBusRawcodeEngine",
									   &type_info,
									   (GTypeFlags) 0);
	}

	return type;
}

static void
ibus_rawcode_engine_class_init (IBusRawcodeEngineClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
	IBusObjectClass *ibus_object_class = IBUS_OBJECT_CLASS (klass);
	IBusEngineClass *engine_class = IBUS_ENGINE_CLASS (klass);

	parent_class = (IBusEngineClass *) g_type_class_peek_parent (klass);

    object_class->constructor = ibus_rawcode_engine_constructor;
	ibus_object_class->destroy = (IBusObjectDestroyFunc) ibus_rawcode_engine_destroy;

    engine_class->process_key_event = ibus_rawcode_engine_process_key_event;

    engine_class->reset = ibus_rawcode_engine_reset;
    engine_class->enable = ibus_rawcode_engine_enable;
    engine_class->disable = ibus_rawcode_engine_disable;

    engine_class->focus_in = ibus_rawcode_engine_focus_in;
    engine_class->focus_out = ibus_rawcode_engine_focus_out;

    engine_class->page_up = ibus_rawcode_engine_page_up;
    engine_class->page_down = ibus_rawcode_engine_page_down;

    engine_class->cursor_up = ibus_rawcode_engine_cursor_up;
    engine_class->cursor_down = ibus_rawcode_engine_cursor_down;
}

static void
ibus_rawcode_engine_init (IBusRawcodeEngine *rawcode)
{
    rawcode->buffer = g_string_new ("");
    rawcode->rawcode_mode_prop = ibus_property_new ("rawcode_mode_prop",
                                           PROP_TYPE_NORMAL,
                                           NULL,
                                           NULL,
                                           NULL,
                                           TRUE,
                                           FALSE,
                                           0,
                                           NULL);
    g_object_ref_sink (rawcode->rawcode_mode_prop);

    rawcode->prop_list = ibus_prop_list_new ();
    g_object_ref_sink (rawcode->prop_list);
    ibus_prop_list_append (rawcode->prop_list,  rawcode->rawcode_mode_prop);
    rawcode->table = ibus_lookup_table_new (16, 0, TRUE, TRUE);
    g_object_ref_sink (rawcode->table);
    rawcode->maxpreeditlen = 8;
}

static GObject*
ibus_rawcode_engine_constructor (GType                   type,
                                guint                   n_construct_params,
                                GObjectConstructParam  *construct_params)
{
    IBusRawcodeEngine *rawcode;

    rawcode = (IBusRawcodeEngine *) G_OBJECT_CLASS (parent_class)->constructor (type,
                                                       n_construct_params,
                                                       construct_params);

    return (GObject *)rawcode;
}


static void
ibus_rawcode_engine_destroy (IBusRawcodeEngine *rawcode)
{
    if (rawcode->prop_list) {
        g_object_unref (rawcode->prop_list);
        rawcode->prop_list = NULL;
    }

    if (rawcode->rawcode_mode_prop) {
        g_object_unref (rawcode->rawcode_mode_prop);
        rawcode->rawcode_mode_prop = NULL;
    }

    if (rawcode->table) {
        g_object_unref (rawcode->table);
        rawcode->table = NULL;
    }

    if (rawcode->buffer) {
        g_string_free (rawcode->buffer, TRUE);
        rawcode->buffer = NULL;
    }

	IBUS_OBJECT_CLASS (parent_class)->destroy ((IBusObject *)rawcode);
}

static void
ibus_rawcode_engine_update_preedit_text (IBusRawcodeEngine *rawcode)
{
    IBusText *text;
   const gchar *str;
    str= rawcode->buffer->str;

		if (str != NULL) {
			        text = ibus_text_new_from_string (str);
			        ibus_text_append_attribute (text, IBUS_ATTR_TYPE_FOREGROUND, 0x00ffffff, 0, -1);
			        ibus_text_append_attribute (text, IBUS_ATTR_TYPE_BACKGROUND, 0x00000000, 0, -1);
			        ibus_engine_update_preedit_text ((IBusEngine *)rawcode,
						                                         text,
						                                         ibus_text_get_length (text),
				                                         TRUE);
		}
	      else {
			        text = ibus_text_new_from_static_string ("");
			        ibus_engine_update_preedit_text ((IBusEngine *)rawcode, text, 0, FALSE);
		    } 
}

static gboolean
ibus_rawcode_engine_process_key_event (IBusEngine     *engine,
                                      guint           keyval,
                                      guint               	 keycode,
                                      guint           modifiers)
{
    IBusRawcodeEngine *rawcode = (IBusRawcodeEngine *) engine;
   const gchar *keyval_name;

    if (modifiers & IBUS_RELEASE_MASK)
        return FALSE;

    if (modifiers & (IBUS_CONTROL_MASK | IBUS_MOD1_MASK))
        return FALSE;

    if (keyval == IBUS_BackSpace && rawcode->buffer->len!=0) {
	g_string_truncate(rawcode->buffer, (rawcode->buffer->len)-1);
	ibus_rawcode_engine_update_preedit_text (rawcode);
	ibus_rawcode_engine_process_preedit_text (rawcode);
        return TRUE;
    }

    if (keyval == IBUS_Escape) {
	ibus_rawcode_engine_reset ( engine);
        return TRUE;
    } 

    if (keyval == IBUS_space && rawcode->buffer->len!=0) {
	       commit_buffer_to_ibus(rawcode);
	       return TRUE;
    }

    if (((keyval >= IBUS_0 && keyval <= IBUS_9) || 
	(keyval >= IBUS_A && keyval <= IBUS_F) || 
	(keyval >= IBUS_a && keyval <= IBUS_f)) &&
	((rawcode->buffer->len < rawcode->maxpreeditlen)) ) {

	if(rawcode->buffer->len==0){
		ibus_engine_show_preedit_text((IBusEngine *)rawcode);
	}

	keyval_name = ibus_keyval_name(keyval);
	g_string_append (rawcode->buffer, keyval_name);
	ibus_rawcode_engine_update_preedit_text (rawcode);
	ibus_rawcode_engine_process_preedit_text(rawcode);
	return TRUE;
    }

  if(keyval==IBUS_Up) {
	if(rawcode->table->candidates->len != 0) {
//                g_debug("print len %d", rawcode->table->len);
		ibus_lookup_table_cursor_up(rawcode->table);
	        ibus_engine_update_lookup_table ((IBusEngine *)rawcode, rawcode->table, TRUE);
		return TRUE;
	}
  }  
  

  if(keyval==IBUS_Down) {
	if(rawcode->table->candidates->len != 0) {
		ibus_lookup_table_cursor_down(rawcode->table);
	        ibus_engine_update_lookup_table ((IBusEngine *)rawcode, rawcode->table, TRUE);
		return TRUE;
	}
   }

  if(keyval==IBUS_Return) {
	if(rawcode->table->candidates->len != 0) {
	        IBusText *text;
		text = ibus_lookup_table_get_candidate(rawcode->table, rawcode->table->cursor_pos);
		ibus_engine_commit_text ((IBusEngine *)rawcode, text);
	        ibus_lookup_table_clear (rawcode->table);
	        ibus_engine_hide_lookup_table((IBusEngine *)rawcode);

		g_string_assign (rawcode->buffer, "");
		text = ibus_text_new_from_static_string ("");
		ibus_engine_update_preedit_text ((IBusEngine *)rawcode, text, 0, FALSE);

	       return TRUE;
	}
  }

// other keys will not allowed in preedit
	if(rawcode->buffer->len>0)
		return TRUE;

    ibus_rawcode_engine_flush (rawcode);
    return FALSE;
}

static void
ibus_rawcode_engine_flush (IBusRawcodeEngine *rawcode)
{
    IBusText *text;

	text = ibus_text_new_from_static_string ("");
	ibus_engine_update_preedit_text ((IBusEngine *)rawcode, text, 0, FALSE);

//    ibus_engine_hide_preedit_text ((IBusEngine *) rawcode);

}


static void
ibus_rawcode_engine_focus_in (IBusEngine *engine)
{
    IBusRawcodeEngine *rawcode = (IBusRawcodeEngine *) engine;

    ibus_engine_register_properties (engine, rawcode->prop_list);

    parent_class->focus_in (engine);
  if(rawcode->buffer->len){
        ibus_rawcode_engine_update_preedit_text (rawcode);
    if(rawcode->table){
	ibus_engine_update_lookup_table ((IBusEngine *)rawcode, rawcode->table, TRUE);
        }
  }
  
}

static void
ibus_rawcode_engine_focus_out (IBusEngine *engine)
{
    IBusRawcodeEngine *rawcode = (IBusRawcodeEngine *) engine;

    ibus_rawcode_engine_flush (rawcode);

    parent_class->focus_out ((IBusEngine *) rawcode);
}

static void
ibus_rawcode_engine_reset (IBusEngine *engine)
{
    IBusRawcodeEngine *rawcode = (IBusRawcodeEngine *) engine;
     g_string_assign (rawcode->buffer, "");

    ibus_rawcode_engine_flush (rawcode);
    ibus_engine_hide_lookup_table((IBusEngine *)rawcode);
    parent_class->reset (engine);
}

static void
ibus_rawcode_engine_enable (IBusEngine *engine)
{
    parent_class->enable (engine);
}

static void
ibus_rawcode_engine_disable (IBusEngine *engine)
{
    ibus_rawcode_engine_focus_out (engine);
    parent_class->disable (engine);
}

static void
ibus_rawcode_engine_page_up (IBusEngine *engine)
{
    parent_class->page_up (engine);
}

static void
ibus_rawcode_engine_page_down (IBusEngine *engine)
{
    parent_class->page_down (engine);
}

static void
ibus_rawcode_engine_cursor_up (IBusEngine *engine)
{
    parent_class->cursor_up (engine);
}

static void
ibus_rawcode_engine_cursor_down (IBusEngine *engine)
{
    parent_class->cursor_down (engine);
}

static void
ibus_rawcode_engine_process_preedit_text (IBusRawcodeEngine *rawcode)
{
  int MAXLEN = 6;
	if(rawcode->buffer->len==0){
                ibus_engine_hide_preedit_text((IBusEngine *)rawcode);
	        ibus_engine_hide_lookup_table((IBusEngine *)rawcode);
	}

        if (rawcode->buffer->len > 0) {
            if (rawcode->buffer->str[0] == '0')
                MAXLEN = 4;
            else if (rawcode->buffer->str[0] == '1')
                MAXLEN = 6;
            else
                MAXLEN = 5;
        }

	if((rawcode->buffer->len>=3) && (rawcode->buffer->len < MAXLEN)){
        	create_rawcode_lookup_table(rawcode);
	} else if(rawcode->buffer->len==MAXLEN){
			commit_buffer_to_ibus(rawcode);
		       	      }


}

static gunichar rawcode_get_unicode_value (const GString *preedit)
{
    gunichar code = 0;
    int i;
    for (i=0; i<preedit->len; ++i) {
        code = (code << 4) | (rawcode_ascii_to_hex ((int) preedit->str[i]) & 0x0f);
    }
    return code;
}

static int rawcode_ascii_to_hex (int ascii)
{
    if (ascii >= '0' && ascii <= '9')
        return ascii - '0';
    else if (ascii >= 'a' && ascii <= 'f')
        return ascii - 'a' + 10;
    else if (ascii >= 'A' && ascii <= 'F')
        return ascii - 'A' + 10;
    return 0;
}


static int rawcode_hex_to_ascii (int hex)
{
    hex %= 16;

    if (hex >= 0 && hex <= 9)
        return hex + '0';

    return hex - 10 + 'a';
}

static int create_rawcode_lookup_table(IBusRawcodeEngine *rawcode)
{

gunichar c, trail;
int i;
IBusText *text;
	if(rawcode->table)
		ibus_lookup_table_clear (rawcode->table);
//		ibus_lookup_table_set_page_size(rawcode->table,10);
// adding space key character in lookuptable
/*		c = rawcode_get_unicode_value (rawcode->buffer);
		if (c >0x0 && c < 0x10FFFF){
			text = ibus_text_new_from_unichar(c);			
			ibus_lookup_table_append_candidate (rawcode->table, text);
			ibus_engine_update_lookup_table ((IBusEngine *)rawcode, rawcode->table, TRUE);
			} */

	for (i=1; i<10; ++i) {
		trail =(gchar) rawcode_hex_to_ascii (i);
		g_string_append_c (rawcode->buffer, trail);		
		c = rawcode_get_unicode_value (rawcode->buffer);
		if ((c >0x0 && c < 0x10FFFF) && g_unichar_validate (c)){		
        		text = ibus_text_new_from_unichar(c);			
	        	ibus_lookup_table_append_candidate (rawcode->table, text);
	        	ibus_engine_update_lookup_table ((IBusEngine *)rawcode, rawcode->table, TRUE);
		}
		g_string_truncate(rawcode->buffer, rawcode->buffer->len-1);

	}
		g_string_append_c (rawcode->buffer, '0');
		c = rawcode_get_unicode_value (rawcode->buffer);
		if ((c >0x0 && c < 0x10FFFF) && g_unichar_validate (c)){		
	        	text = ibus_text_new_from_unichar(c);			
	        	ibus_lookup_table_append_candidate (rawcode->table, text);
	        	ibus_engine_update_lookup_table ((IBusEngine *)rawcode, rawcode->table, TRUE);
		}
	        	g_string_truncate(rawcode->buffer, rawcode->buffer->len-1);

	for (i=10; i<16; ++i) {
		trail =(gchar) rawcode_hex_to_ascii (i);
		g_string_append_c (rawcode->buffer, trail);		
		c = rawcode_get_unicode_value (rawcode->buffer);
		if ((c >0x0 && c < 0x10FFFF)&& g_unichar_validate (c)){		
        		text = ibus_text_new_from_unichar(c);			
	        	ibus_lookup_table_append_candidate (rawcode->table, text);
	        	ibus_engine_update_lookup_table ((IBusEngine *)rawcode, rawcode->table, TRUE);
		}
		g_string_truncate(rawcode->buffer, rawcode->buffer->len-1);

	} 



//	ibus_engine_hide_lookup_table((IBusEngine *)rawcode);
//	text =  ibus_text_new_from_string (rawcode->table->candidates->data);
//	ibus_engine_update_auxiliray_text((IBusEngine *)rawcode, text, TRUE)  ;

	
return rawcode->table->candidates->len;
}

static void commit_buffer_to_ibus(IBusRawcodeEngine *rawcode)
{
	gunichar c;
	IBusText *text;
	c = rawcode_get_unicode_value (rawcode->buffer);
	if ((c >0x0 && c < 0x10FFFF) && g_unichar_validate (c)){		
		text = ibus_text_new_from_unichar(c);
		ibus_engine_commit_text ((IBusEngine *)rawcode, text);
	}

	g_string_assign (rawcode->buffer, "");
	text = ibus_text_new_from_static_string ("");
	ibus_engine_update_preedit_text ((IBusEngine *)rawcode, text, 0, FALSE);

	if(rawcode->table){
	        ibus_lookup_table_clear (rawcode->table);
	        ibus_engine_hide_lookup_table((IBusEngine *)rawcode);
	}

}
