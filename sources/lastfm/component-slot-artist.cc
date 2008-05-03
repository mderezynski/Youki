#include "component-slot-artist.hh"

MPX::ComponentSlotArtist::ComponentSlotArtist ()
: ObjectBase("component-slot-artist")
, property_artist_ (*this, "artist", "")
{
}

MPX::ComponentSlotArtist::~ComponentSlotArtist ()
{
}

