#ifndef MPX_SOURCE_PODCASTS_TYPES_HH
#define MPX_SOURCE_PODCASTS_TYPES_HH

#include <string>
#include <ctime>
#include <glib.h>

namespace MPX
{
    struct Channel
    {
        std::string         title;
        std::string         link;
        std::string         description;
        std::string         language;
        time_t              last_build_date; //parsed
        std::string         copyright;
        std::string         docs;
        time_t              ttl;

        std::string         image_title;
        std::string         image_url;
        std::string         image_link;
    };

    struct Item
    {
        std::string         title;
        std::string         description;
        std::string         link;
        std::string         guid;
        bool                guid_is_permanent;
        time_t              pub_date; // parsed
        std::string         category;

        std::string         enclosure_url;
        gint64              enclosure_length;
        std::string         enclosure_type;
        
    };
}

#endif
