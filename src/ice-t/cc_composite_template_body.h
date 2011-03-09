/* -*- c -*- *******************************************************/
/*
 * Copyright (C) 2011 Sandia Corporation
 * Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 * the U.S. Government retains certain rights in this software.
 *
 * This source code is released under the New BSD License.
 */

/* This is not a traditional header file, but rather a "macro" file that defines
 * a template for a decompression function.  (If this were C++, we would
 * actually use tempaltes.)  In general, there are many flavors of the
 * decompression functionality which differ only slightly.  Rather than maintain
 * lots of different code bases or try to debug big macros, we just include this
 * file with various parameters.
 *
 * The following macros must be defined:
 *      CCC_FRONT_COMPRESSED_IMAGE - compressed image to blend in front.
 *      CCC_BACK_COMPRESSED_IMAGE - compressed image to blend in back.
 *      CCC_DEST_COMPRESSED_IMAGE - the resulting compressed image buffer.
 *      CCC_COMPOSITE(front_pointer, back_pointer, dest_pointer) - given
 *              pointers to actual data in the three buffers, perform the
 *              actual compositing operation and increment the pointers.
 *      CCC_COPY(src_pointer, dest_pointer) - copy a pixel from the src
 *              pointer the the dest pointer and increment both pointers.
 *
 * All of the above macros are undefined at the end of this file.
 */

#ifndef ICET_IMAGE_DATA
#error Need ICET_IMAGE_DATA macro.  Is this included in image.c?
#endif
#ifndef INACTIVE_RUN_LENGTH
#error Need INACTIVE_RUN_LENGTH macro.  Is this included in image.c?
#endif
#ifndef ACTIVE_RUN_LENGTH
#error Need ACTIVE_RUN_LENGTH macro.  Is this included in image.c?
#endif

#define CCC_MIN(x, y) ((x) < (y) ? (x) : (y))

{
    /* Use IceTByte for byte-based pointer arithmetic. */
    const IceTByte *_front;
    const IceTByte *_back;
    IceTByte *_dest;
    IceTVoid *_dest_runlengths;
    IceTSizeType _num_pixels;
    IceTSizeType _pixel;
    IceTSizeType _front_num_inactive;
    IceTSizeType _front_num_active;
    IceTSizeType _back_num_inactive;
    IceTSizeType _back_num_active;
    IceTSizeType _dest_num_active;

    _num_pixels = icetSparseImageGetNumPixels(CCC_FRONT_COMPRESSED_IMAGE);
    if (_num_pixels != icetSparseImageGetNumPixels(CCC_BACK_COMPRESSED_IMAGE)) {
        icetRaiseError("Input buffers do not agree for compressed-compressed"
                       " composite.",
                       ICET_SANITY_CHECK_FAIL);
    }
    icetSparseImageSetDimensions(
                           CCC_DEST_COMPRESSED_IMAGE,
                           icetSparseImageGetWidth(CCC_FRONT_COMPRESSED_IMAGE),
                           icetSparseImageGetHeight(CCC_BACK_COMPRESSED_IMAGE));

    _front = ICET_IMAGE_DATA(CCC_FRONT_COMPRESSED_IMAGE);
    _back = ICET_IMAGE_DATA(CCC_BACK_COMPRESSED_IMAGE);
    _dest = ICET_IMAGE_DATA(CCC_DEST_COMPRESSED_IMAGE);
    _dest_runlengths = NULL;

    _pixel = 0;
    _front_num_inactive = _front_num_active = 0;
    _back_num_inactive = _back_num_active = 0;
    _dest_num_active = 0;
    while (_pixel < _num_pixels) {
        /* When num_active is 0, we have exhausted all active pixels and the
           buffer pointer must be pointing to run lengths. */
        while(   (_front_num_active == 0)
              && ((_front_num_inactive + _pixel) < _num_pixels) ) {
            _front_num_inactive += INACTIVE_RUN_LENGTH(_front);
            _front_num_active = ACTIVE_RUN_LENGTH(_front);
            _front += RUN_LENGTH_SIZE;
        }
        while(   (_back_num_active == 0)
              && ((_back_num_inactive + _pixel) < _num_pixels) ) {
            _back_num_inactive += INACTIVE_RUN_LENGTH(_back);
            _back_num_active = ACTIVE_RUN_LENGTH(_back);
            _back += RUN_LENGTH_SIZE;
        }

        {
            IceTSizeType _dest_num_inactive
                = CCC_MIN(_front_num_inactive, _back_num_inactive);
            if (_dest_num_inactive > 0) {
                /* Record active pixel count.  (Special case on first iteration
                 * where there is no runlength and no place to put it.) */
                if (_dest_runlengths != NULL) {
                    ACTIVE_RUN_LENGTH(_dest_runlengths)
                        = (IceTUShort)(_dest_num_active);
                    _dest_num_active = 0;
                }
                _dest_runlengths = _dest;
                _dest += RUN_LENGTH_SIZE;
                /* Handle inactive pixel region. */
                _pixel += _dest_num_inactive;
                _front_num_inactive -= _dest_num_inactive;
                _back_num_inactive -= _dest_num_inactive;
                while (0xFFFF < _dest_num_inactive) {
                    INACTIVE_RUN_LENGTH(_dest_runlengths) = 0xFFFF;
                    ACTIVE_RUN_LENGTH(_dest_runlengths) = 0;
                    _dest_runlengths = _dest;
                    _dest += RUN_LENGTH_SIZE;
                    _dest_num_inactive -= 0xFFFF;
                }
                INACTIVE_RUN_LENGTH(_dest_runlengths)
                    = (IceTUShort)(_dest_num_inactive);
            } else {
                /* Handle special case where first pixel is active. */
                if (_dest_runlengths == NULL) {
                    _dest_runlengths = _dest;
                    _dest += RUN_LENGTH_SIZE;
                    INACTIVE_RUN_LENGTH(_dest_runlengths) = 0;
                }
            }
        }

#define CCC_INCREMENT_DEST_NUM_ACTIVE()                                 \
    _dest_num_active++;                                                 \
    _pixel++;                                                           \
    if (0xFFFF < _dest_num_active) {                                    \
        ACTIVE_RUN_LENGTH(_dest_runlengths) = 0xFFFF;                   \
        _dest_num_active -= 0xFFFF;                                     \
        _dest_runlengths = _dest;                                       \
        _dest += RUN_LENGTH_SIZE;                                       \
        INACTIVE_RUN_LENGTH(_dest_runlengths) = 0;                      \
    }

        while ((0 < _front_num_inactive) && (0 < _back_num_active)) {
            CCC_INCREMENT_DEST_NUM_ACTIVE();
            CCC_COPY(_back, _dest);
            _front_num_inactive--;
            _back_num_active--;
        }

        while ((0 < _back_num_inactive) && (0 < _front_num_active)) {
            CCC_INCREMENT_DEST_NUM_ACTIVE();
            CCC_COPY(_front, _dest);
            _back_num_inactive--;
            _front_num_active--;
        }

        if ((_front_num_inactive == 0) && (_back_num_inactive == 0)) {
            while ((0 < _front_num_active) && (0 < _back_num_active)) {
                CCC_INCREMENT_DEST_NUM_ACTIVE();
                CCC_COMPOSITE(_front, _back, _dest);
                _front_num_active--;
                _back_num_active--;
            }
        }
    }

    if (_dest_runlengths != NULL) {
        ACTIVE_RUN_LENGTH(_dest_runlengths) = (IceTUShort)(_dest_num_active);
    }

    if (_pixel != _num_pixels) {
        icetRaiseError("Corrupt compressed image.", ICET_INVALID_VALUE);
    }

    {
        /* Compute the actual number of bytes used to store the image. */
        IceTPointerArithmetic _buffer_begin
            =(IceTPointerArithmetic)ICET_IMAGE_HEADER(CCC_DEST_COMPRESSED_IMAGE);
        IceTPointerArithmetic _buffer_end
            =(IceTPointerArithmetic)_dest;
        IceTPointerArithmetic _compressed_size = _buffer_end - _buffer_begin;
        ICET_IMAGE_HEADER(CCC_DEST_COMPRESSED_IMAGE)
            [ICET_IMAGE_ACTUAL_BUFFER_SIZE_INDEX]
            = (IceTInt)_compressed_size;
    }
}

#undef CCC_FRONT_COMPRESSED_IMAGE
#undef CCC_BACK_COMPRESSED_IMAGE
#undef CCC_DEST_COMPRESSED_IMAGE
#undef CCC_COMPOSITE
#undef CCC_COPY
