#define TYPE_DBUS_OBJ_TRACKLIST (DBusTrackList::get_type ())
#define DBUS_OBJ_TRACKLIST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_DBUS_OBJ_TRACKLIST, DBusTrackList))

  struct DBusTrackListClass
  {
    GObjectClass parent;
  };

  struct Player::DBusTrackList
  {
    GObject parent;
    Player * player;

    enum
    {
      TRACKLIST_SIGNAL_CAPS,
      TRACKLIST_SIGNAL_STATE,
      N_SIGNALS,
    };

    static guint signals[N_SIGNALS];

    static gpointer parent_class;

    static GType
    get_type ();

    static DBusTrackList *
    create (Player &, DBusGConnection*);

    static void
    class_init (gpointer klass,
                gpointer class_data);

    static GObject *
    constructor (GType                   type,
                 guint                   n_construct_properties,
                 GObjectConstructParam * construct_properties);

    static gboolean
    add_track (DBusTrackList* self,
               GHashTable** metadata,
               GError** error);
  };

  gpointer Player::DBusTrackList::parent_class       = 0;
  guint    Player::DBusTrackList::signals[N_SIGNALS] = { 0 };

// HACK: Hackery to rename functions in glue
#define tracklist_add_track add_track 

#include "dbus-obj-TRACKLIST-glue.h"

	void
	Player::DBusTrackList::class_init (gpointer klass, gpointer class_data)
	{
	  parent_class = g_type_class_peek_parent (klass);

	  GObjectClass *gobject_class = reinterpret_cast<GObjectClass*>(klass);
	  gobject_class->constructor  = &DBusTrackList::constructor;
	}

	GObject *
	Player::DBusTrackList::constructor (GType                   type,
							guint                   n_construct_properties,
							GObjectConstructParam*  construct_properties)
	{
	  GObject *object = G_OBJECT_CLASS (parent_class)->constructor (type, n_construct_properties, construct_properties);

	  return object;
	}

	Player::DBusTrackList *
	Player::DBusTrackList::create (Player & player, DBusGConnection * session_bus)
	{
		dbus_g_object_type_install_info (TYPE_DBUS_OBJ_TRACKLIST, &dbus_glib_player_object_info);

		DBusTrackList * self = DBUS_OBJ_TRACKLIST (g_object_new (TYPE_DBUS_OBJ_TRACKLIST, NULL));
		self->player = &player;

	  if(session_bus)
	  {
		dbus_g_connection_register_g_object (session_bus, "/TrackList", G_OBJECT(self));
		g_message("%s: /TrackList Object registered on session DBus", G_STRLOC);
	  }

	  return self;
	}

	GType
	Player::DBusTrackList::get_type ()
	{
	  static GType type = 0;

	  if (G_UNLIKELY (type == 0))
		{
		  static GTypeInfo const type_info =
			{
			  sizeof (DBusTrackListClass),
			  NULL,
			  NULL,
			  &class_init,
			  NULL,
			  NULL,
			  sizeof (DBusTrackList),
			  0,
			  NULL
			};

		  type = g_type_register_static (G_TYPE_OBJECT, "TrackList", &type_info, GTypeFlags (0));
		}

	  return type;
	}
