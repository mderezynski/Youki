/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#ifndef __MPX_SPECTRUM_H__
#define __MPX_SPECTRUM_H__


#include <gst/gst.h>
#include <gst/base/gstadapter.h>
#include <gst/base/gstbasetransform.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define MPX_TYPE_SPECTRUM            (bmp_spectrum_get_type())
#define MPX_SPECTRUM(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj),MPX_TYPE_SPECTRUM,MPXSpectrum))
#define MPX_IS_SPECTRUM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj),MPX_TYPE_SPECTRUM))
#define MPX_SPECTRUM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), MPX_TYPE_SPECTRUM,MPXSpectrumClass))
#define MPX_IS_SPECTRUM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), MPX_TYPE_SPECTRUM))

typedef struct _MPXSpectrum MPXSpectrum;
typedef struct _MPXSpectrumClass MPXSpectrumClass;

struct _MPXSpectrum {
  GstBaseTransform element;

  GstPad *sinkpad,*srcpad;
  GstAdapter *adapter;
  GstSegment segment;

  /* properties */
  gboolean message;             /* whether or not to post messages */
  guint64 interval;             /* how many seconds between emits */
  guint bands;                  /* number of spectrum bands */
  gint threshold;               /* energy level treshold */

  gint num_frames;              /* frame count (1 sample per channel)
                                 * since last emit */
                                 
  gint rate;                    /* caps variables */
  gint channels;

  /* <private> */
  gint base, len;
  gint16 *re, *im, *loud;
  guchar *spect;
};

struct _MPXSpectrumClass {
  GstBaseTransformClass parent_class;
};

GType bmp_spectrum_get_type (void);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __MPX_SPECTRUM_H__ */
