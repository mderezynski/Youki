// (c) 2007 M. Derezynski

#include <boost/algorithm/string.hpp>

#include <glibmm.h>
#include <gmodule.h>

#include "audio.hh"

#include "util-file.hh"
#include "util-string.hh"

#include "metadatareader-taglib.hh"

using namespace Glib;
using boost::algorithm::is_any_of;

namespace
{
  inline bool
  is_module (std::string const& basename)
  {
    return MPX::Util::str_has_suffix_nocase
      (basename.c_str (), G_MODULE_SUFFIX);
  } 
}

namespace MPX
{
    bool
    MetadataReaderTagLib::load_taglib_plugin (std::string const& path)
    {
      enum
      {
        LIB_BASENAME1,
        LIB_BASENAME2,
        LIB_PLUGNAME,
        LIB_SUFFIX
      };

      const std::string type = "taglib_plugin";

      std::string basename (path_get_basename (path));
      std::string pathname (path_get_dirname  (path));

      if (!is_module (basename))
        return false;

      StrV subs; 
      split (subs, basename, is_any_of ("_."));
      std::string name  = type + std::string("_") + subs[LIB_PLUGNAME];
      std::string mpath = Module::build_path (build_filename(PLUGIN_DIR, "taglib"), name);

      Module module (mpath, ModuleFlags (0)); 
      if (!module)
      {
        g_message("Taglib plugin load FAILURE '%s': %s", mpath.c_str (), module.get_last_error().c_str());
        return false;
      }

      void * _plugin_version;
      if (module.get_symbol ("_plugin_version", _plugin_version))
      {
        int version = *((int*)(_plugin_version));
        if (version != PLUGIN_VERSION)
        {
          g_message("Taglib plugin is of old version %d, not loading ('%s')", version, mpath.c_str ());
          return false;
        }
      }

      module.make_resident();
      g_message("Taglib plugin load SUCCESS '%s'", mpath.c_str ());

      void * _plugin_has_accessors = 0; //dummy
      if (module.get_symbol ("_plugin_has_accessors", _plugin_has_accessors))
      {
        TaglibPluginPtr plugin = TaglibPluginPtr (new TaglibPlugin());

        if (!g_module_symbol (module.gobj(), "_get", (gpointer*)(&plugin->get)))
          plugin->get = NULL;

        if (!g_module_symbol (module.gobj(), "_set", (gpointer*)(&plugin->set)))
          plugin->set = NULL;
        else
          g_message("     >> Plugin '%s' can write metadata", subs[LIB_PLUGNAME].c_str());

        if (g_module_symbol (module.gobj(), "_mimetypes", (gpointer*)(&plugin->mimetypes)))
        {
          const char ** mimetypes (plugin->mimetypes());
          while (*mimetypes)
          {
            g_message("     >> Plugin registers for %s", *mimetypes); 
            m_taglib_plugins.insert (std::make_pair (std::string (*mimetypes), plugin));

            ++mimetypes;
          }
        }
        else
        {
          m_taglib_plugins_keeper.push_back (plugin);
        }
      }
      return false;
    }

    MetadataReaderTagLib::MetadataReaderTagLib ()
    {
      Util::dir_for_each_entry (build_filename(PLUGIN_DIR, "taglib"), sigc::mem_fun(*this, &MPX::MetadataReaderTagLib::load_taglib_plugin));  
    }

    MetadataReaderTagLib::~MetadataReaderTagLib ()
    {}

    bool
    MetadataReaderTagLib::get (std::string const& uri, Track & track)
    {
      std::string type;
      try{
          std::string filename = filename_from_uri (uri);
          MPX::Audio::typefind(filename, type);
          TaglibPluginsMap::const_iterator i = m_taglib_plugins.find (type);
          if (i != m_taglib_plugins.end() && i->second->get && i->second->get (filename, track))
          {
            track[ATTRIBUTE_LOCATION] = uri;
            return true;
          }
        }
      catch (Glib::ConvertError & cxe)
        {
        }
      return false; 
    }

    bool
    MetadataReaderTagLib::set (std::string const& uri, Track & track)
    {}
}
