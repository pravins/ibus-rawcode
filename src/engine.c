/* vim:set et sts=4: */

#include <ibus.h>
#include <hangul.h>
#include <string.h>
#include "engine.h"
#include <glib.h>

typedef struct _IBusRawcodeEngine IBusRawcodeEngine;
typedef struct _IBusRawcodeEngineClass IBusRawcodeEngineClass;


struct _IBusRawcodeEngine {
	IBusEngine parent;

    /* members */
//    RawcodeInputContext *context;
    GString *buffer;
    gboolean rawcode_mode;

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
static void ibus_rawcode_engine_toggle_rawcode_mode
                                            (IBusRawcodeEngine       *rawcode);
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
static void ibus_rawcode_engine_process_preedit_text(IBusRawcodeEngine       *rawcode);
static gunichar get_unicode_value (const GString *preedit);
static int ascii_to_hex (int ascii);
static int hex_to_ascii (int hex);

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
//    rawcode->context = hangul_ic_new ("2");
    rawcode->rawcode_mode = TRUE;
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

    rawcode->prop_list = ibus_prop_list_new ();
    ibus_prop_list_append (rawcode->prop_list,  rawcode->rawcode_mode_prop);

    rawcode->table = ibus_lookup_table_new (9, 0, TRUE, TRUE);
}

static GObject*
ibus_rawcode_engine_constructor (GType                   type,
                                guint                   n_construct_params,
                                GObjectConstructParam  *construct_params)
{
    IBusRawcodeEngine *hangul;

    hangul = (IBusRawcodeEngine *) G_OBJECT_CLASS (parent_class)->constructor (type,
                                                       n_construct_params,
                                                       construct_params);

    return (GObject *)hangul;
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

//    if (hangul->context) {
//        hangul_ic_delete (hangul->context);
//        hangul->context = NULL;
//    }

	IBUS_OBJECT_CLASS (parent_class)->destroy ((IBusObject *)rawcode);
}

static void
ibus_rawcode_engine_update_preedit_text (IBusRawcodeEngine *hangul)
{
    IBusText *text;
  const gchar *str;
str= hangul->buffer->str;

  g_debug("%s string-> %d lenght", hangul->buffer->str, hangul->buffer->len );
//		if(hangul->buffer->str!=NULL)
		if (str != NULL) {
			        text = ibus_text_new_from_string (str);
			        ibus_text_append_attribute (text, IBUS_ATTR_TYPE_FOREGROUND, 0x00ffffff, 0, -1);
			        ibus_text_append_attribute (text, IBUS_ATTR_TYPE_BACKGROUND, 0x00000000, 0, -1);
			        ibus_engine_update_preedit_text ((IBusEngine *)hangul,
						                                         text,
						                                         ibus_text_get_length (text),
				                                         TRUE);
		                      g_object_unref (text);
		}
	      else {
		        text = ibus_text_new_from_static_string ("");
		        ibus_engine_update_preedit_text ((IBusEngine *)hangul, text, 0, FALSE);
		        g_object_unref (text);
		    } 
// 	g_object_unref (text);
}

static gboolean
ibus_rawcode_engine_process_key_event (IBusEngine     *engine,
                                      guint           keyval,
                                      guint           modifiers)
{
    IBusRawcodeEngine *rawcode = (IBusRawcodeEngine *) engine;
  const gchar *keyval_name;
//    gboolean retval;
//    const gunichar *str;

    if (modifiers & IBUS_RELEASE_MASK)
        return FALSE;

    if (modifiers & (IBUS_CONTROL_MASK | IBUS_MOD1_MASK))
        return FALSE;

    if ((keyval == IBUS_BackSpace) && (rawcode->buffer->len!=0)) {
	g_string_truncate(rawcode->buffer, (rawcode->buffer->len)-1);
	ibus_rawcode_engine_update_preedit_text (rawcode);
//        retval = hangul_ic_backspace (hangul->context);
        return TRUE;
    }

    if (keyval == IBUS_Escape) {
	ibus_rawcode_engine_reset ( engine);
        return TRUE;
    } 

    if ((keyval >= IBUS_0 && keyval <= IBUS_9) || (keyval >= IBUS_A && keyval <= IBUS_F) || (keyval >= IBUS_a && keyval <= IBUS_f)) {
        keyval_name = ibus_keyval_name(keyval);
        g_string_append (rawcode->buffer, keyval_name);
        g_debug("keyval%x", keyval);
        ibus_rawcode_engine_update_preedit_text (rawcode);
       ibus_rawcode_engine_process_preedit_text(rawcode);
/*	if(hangul->buffer->len==4){
//			       gunichar c = get_unicode_value (hangul->buffer);
//			        g_debug("unicode%x", c);
//			       IBusText  *text = ibus_text_new_from_unichar(c);
			        ibus_engine_commit_text ((IBusEngine *)hangul, text);
			        g_string_assign (hangul->buffer, "");
			        text = ibus_text_new_from_static_string ("");
			       ibus_engine_update_preedit_text ((IBusEngine *)hangul, text, 0, FALSE);
		      	       g_object_unref (text);
		       	      }
	else

        return TRUE;
    } */
   return TRUE;
    }

    ibus_rawcode_engine_flush (rawcode);
    return FALSE;
}

static void
ibus_rawcode_engine_flush (IBusRawcodeEngine *rawcode)
{
  //  const gunichar *str;
    IBusText *text;

//    str = hangul_ic_flush (hangul->context);

//    if (str == NULL || str[0] == 0)
//        return;

//    text = ibus_text_new_from_ucs4 (str);
	text = ibus_text_new_from_static_string ("");
	ibus_engine_update_preedit_text ((IBusEngine *)rawcode, text, 0, FALSE);

//    ibus_engine_hide_preedit_text ((IBusEngine *) hangul);
//    ibus_engine_commit_text ((IBusEngine *) hangul, text);

    g_object_unref (text);
}

static void
ibus_rawcode_engine_toggle_hangul_mode (IBusRawcodeEngine *rawcode)
{
    IBusText *text;
    rawcode->rawcode_mode = ! rawcode->rawcode_mode;

    ibus_rawcode_engine_flush (rawcode);

    if (rawcode->rawcode_mode) {
        text = ibus_text_new_from_static_string ("한");
    }
    else {
        text = ibus_text_new_from_static_string ("A");
    }

    ibus_property_set_label (rawcode->rawcode_mode_prop, text);
    ibus_engine_update_property ((IBusEngine *)rawcode, rawcode->rawcode_mode_prop);
    g_object_unref (text);
}

static void
ibus_rawcode_engine_focus_in (IBusEngine *engine)
{
    IBusRawcodeEngine *hangul = (IBusRawcodeEngine *) engine;

    ibus_engine_register_properties (engine, hangul->prop_list);

    parent_class->focus_in (engine);
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
    IBusText *text;
  const gchar *str;
    gunichar c;
  int i;
const  GString *lookuptablebuffer;
lookuptablebuffer = g_string_new("");
   gunichar trail;
    str= rawcode->buffer->str;

        if (rawcode->buffer->len > 0) {
            if (rawcode->buffer->str[0] == '0')
                MAXLEN = 4;
            else if (rawcode->buffer->str[0] == '1')
                MAXLEN = 6;
            else
                MAXLEN = 5;
        }

	if(rawcode->buffer->len==3){
					g_debug("in lenght = 3");
					ibus_lookup_table_set_page_size (rawcode->table, 16);
					g_string_append(lookuptablebuffer, rawcode->buffer->str);
//				               str= rawcode->buffer->str;
		    for (i=0; i<16; ++i) {
					trail =(gchar) hex_to_ascii (i);
					g_debug("trail = %c", trail);
					g_string_append_c (lookuptablebuffer, trail);		
					g_debug("buffer %s", lookuptablebuffer->str);
					g_debug("lookuptable = %s", lookuptablebuffer->str);
					c = get_unicode_value (lookuptablebuffer);
	      			        g_debug("unicode vale in 3= %x", c);
					       text = ibus_text_new_from_unichar(c);			
	      			         g_debug("Text= %c", text);
					ibus_lookup_table_append_candidate (rawcode->table, text);

					ibus_engine_update_lookup_table ((IBusEngine *)rawcode, rawcode->table, TRUE);
//					g_string_append (rawcode->buffer, "");
//					g_string_append (rawcode->buffer, str);	
				               g_string_truncate(lookuptablebuffer, lookuptablebuffer->len-1);
		      	                               g_object_unref (text);

	}
	}  

	if(rawcode->buffer->len==MAXLEN){
			       gunichar c = get_unicode_value (rawcode->buffer);
			        g_debug("unicode value = %x", c);
			       IBusText  *text = ibus_text_new_from_unichar(c);
			        ibus_engine_commit_text ((IBusEngine *)rawcode, text);
			        g_string_assign (rawcode->buffer, "");
			        text = ibus_text_new_from_static_string ("");
			       ibus_engine_update_preedit_text ((IBusEngine *)rawcode, text, 0, FALSE);
		      	       g_object_unref (text);
		       	      }


/*  g_debug("%s string-> %d lenght", hangul->buffer->str, hangul->buffer->len );
//		if(hangul->buffer->str!=NULL)
		if (str != NULL) {
			        text = ibus_text_new_from_string (str);
			        ibus_text_append_attribute (text, IBUS_ATTR_TYPE_FOREGROUND, 0x00ffffff, 0, -1);
			        ibus_text_append_attribute (text, IBUS_ATTR_TYPE_BACKGROUND, 0x00000000, 0, -1);
			        ibus_engine_update_preedit_text ((IBusEngine *)hangul,
						                                         text,
						                                         ibus_text_get_length (text),
				                                         TRUE);
		                      g_object_unref (text);
		}
	      else {
		        text = ibus_text_new_from_static_string ("");
		        ibus_engine_update_preedit_text ((IBusEngine *)hangul, text, 0, FALSE);
		        g_object_unref (text);
		    } 
// 	g_object_unref (text); */
}

static gunichar get_unicode_value (const GString *preedit)
{
    gunichar code = 0;
//  g_debug("in get unicode vale\n");
  g_debug("preedit str in get unicodde value %s\n", preedit->str);
    int i;
    for (i=0; i<preedit->len; ++i) {
	g_debug("asciitohex%d",ascii_to_hex ((int) preedit->str[i]));
        code = (code << 4) | (ascii_to_hex ((int) preedit->str[i]) & 0x0f);
    }
	g_debug("code %x",code );
    return code;
}

static int ascii_to_hex (int ascii)
{
    if (ascii >= '0' && ascii <= '9')
        return ascii - '0';
    else if (ascii >= 'a' && ascii <= 'f')
        return ascii - 'a' + 10;
    else if (ascii >= 'A' && ascii <= 'F')
        return ascii - 'A' + 10;
    return 0;
}

static void
ibus_rawcode_engine_update_lookup_table (IBusRawcodeEngine *rawcode)
{
   ibus_lookup_table_clear (rawcode->table);
 

}

static int hex_to_ascii (int hex)
{
    hex %= 16;

    if (hex >= 0 && hex <= 9)
        return hex + '0';

    return hex - 10 + 'a';
}
