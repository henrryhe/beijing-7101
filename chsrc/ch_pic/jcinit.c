/*
 * jcinit.c
 *
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file contains initialization logic for the JPEG compressor.
 * This routine is in charge of selecting the modules to be executed and
 * making an initialization call to each one.
 *
 * Logically, this code belongs in jcmaster.c.  It's split out because
 * linking this routine implies linking the entire compression library.
 * For a transcoding-only application, we want to be able to use jcmaster.c
 * without linking in the whole library.
 */

#define JPEG__INTERNALS
#include "jinclude.h"
#include "jpeglib.h"


/*
 * Master selection of compression modules.
 * This is done once at the start of processing an image.  We determine
 * which modules will be used and give them appropriate initialization calls.
 */

GLOBAL(void)
jinit__compress__master (j__compress__ptr cinfo)
{
  /* Initialize master control (includes parameter checking/processing) */
  jinit__c__master__control(cinfo, FALSE /* full compression */);

  /* Preprocessing */
  if (! cinfo->raw__data__in) {
    jinit__color__converter(cinfo);
    jinit__downsampler(cinfo);
    jinit__c__prep__controller(cinfo, FALSE /* never need full buffer here */);
  }
  /* Forward DCT */
  jinit__forward__dct(cinfo);
  /* Entropy encoding: either Huffman or arithmetic coding. */
  if (cinfo->arith__code) {
    ERREXIT(cinfo, JERR__ARITH__NOTIMPL);
  } else {
    if (cinfo->progressive__mode) {
#ifdef C__PROGRESSIVE__SUPPORTED
      jinit__phuff__encoder(cinfo);
#else
      ERREXIT(cinfo, JERR__NOT__COMPILED);
#endif
    } else
      jinit__huff__encoder(cinfo);
  }

  /* Need a full-image coefficient buffer in any multi-pass mode. */
  jinit__c__coef__controller(cinfo,
		(CH_BOOL) (cinfo->num__scans > 1 || cinfo->optimize__coding));
  jinit__c__main__controller(cinfo, FALSE /* never need full buffer here */);

  jinit__marker__writer(cinfo);

  /* We can now tell the memory manager to allocate virtual arrays. */
  (*cinfo->mem->realize__virt__arrays) ((j__common__ptr) cinfo);

  /* Write the datastream header (SOI) immediately.
   * Frame and scan headers are postponed till later.
   * This lets application insert special markers after the SOI.
   */
  (*cinfo->marker->write__file__header) (cinfo);
}
