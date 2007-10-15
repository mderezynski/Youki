#ifndef _HAL_CC_HH_
#define _HAL_CC_HH_

#ifdef HAVE_BMP

#  include "context.hh"
#  include "device.hh"
#  include "drive.hh"
#  include "storage.hh"
#  include "types.hh"
#  include "volume.hh"

#else

#  include <hal++/context.hh>
#  include <hal++/device.hh>
#  include <hal++/drive.hh>
#  include <hal++/storage.hh>
#  include <hal++/types.hh>
#  include <hal++/volume.hh>

#endif //HAVE_BMP

#endif //!_HAL_CC_HH_
