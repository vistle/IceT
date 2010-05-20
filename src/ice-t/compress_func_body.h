/* -*- c -*- *******************************************************/
/*
 * Copyright (C) 2003 Sandia Corporation
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the U.S. Government.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that this Notice and any statement
 * of authorship are reproduced on all copies.
 */

/* This is not a traditional header file, but rather a "macro" file that
 * defines the body of a compression function.  In general, there are many
 * flavors of the compression functionality which differ only slightly.
 * Rather than maintain lots of different code bases or try to debug big
 * macros, we just include this file with various parameters.
 *
 * The following macros must be defined:
 *      COMPRESSED_BUFFER - the buffer that will hold the compressed image.
 *      COLOR_FORMAT - color format IceTEnum for input and output
 *      DEPTH_FORMAT - depth format IceTEnum for input and output
 *      PIXEL_COUNT - the number of pixels in the original image (or a
 *              variable holding it.
 *      ACTIVE() - provides a true value if the current pixel is active.
 *      WRITE_PIXEL(pointer) - writes the current pixel to the pointer and
 *              increments the pointer.
 *      INCREMENT_PIXEL() - Increments to the next pixel.
 *
 * The following macros are optional:
 *      PADDING - If defined, enables inactive pixels to be placed
 *              around the file.  If defined, then SPACE_BOTTOM, SPACE_TOP,
 *              SPACE_LEFT, SPACE_RIGHT, FULL_WIDTH, and FULL_HEIGHT must
 *              all also be defined.
 *
 * All of the above macros are undefined at the end of this file.
 */

#ifndef COMPRESSED_BUFFER
#error Need COMPRESSED_BUFFER macro.  Is this included in image.c?
#endif
#ifndef COLOR_FORMAT
#error Need COLOR_FORMAT macro.  Is this included in image.c?
#endif
#ifndef DEPTH_FORMAT
#error Need DEPTH_FORMAT macro.  Is this included in image.c ?
#endif
#ifndef PIXEL_COUNT
#error Need PIXEL_COUNT macro.  Is this included in image.c?
#endif
#ifndef ICET_IMAGE_DATA
#error Need ICET_IMAGE_DATA macro.  Is this included in image.c?
#endif
#ifndef INACTIVE_RUN_LENGTH
#error Need INACTIVE_RUN_LENGTH macro.  Is this included in image.c?
#endif
#ifndef ACTIVE_RUN_LENGTH
#error Need ACTIVE_RUN_LENGTH macro.  Is this included in image.c?
#endif
#ifndef RUN_LENGTH_SIZE
#error Need RUN_LENGTH_SIZE macro.  Is this included in image.c?
#endif

#ifdef COMPRESSED_SIZE
#error No longer using COMPRESSED_SIZE.  Size stored directly in image.  Change code.  Delete this after everything is clear.
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4127)
#endif

{
    IceTVoid *_dest;
    IceTSizeType _pixels = PIXEL_COUNT;
    IceTSizeType _p;
    IceTSizeType _count;
#ifdef DEBUG
    IceTSizeType _totalcount = 0;
#endif
    IceTDouble _timer;
    IceTDouble *_compress_time;
    IceTSizeType _compressed_size;

    _compress_time = icetUnsafeStateGetDouble(ICET_COMPRESS_TIME);
    _timer = icetWallTime();

    icetSparseImageInitialize(COMPRESSED_BUFFER, COLOR_FORMAT,
                              DEPTH_FORMAT, _pixels);
    _dest = ICET_IMAGE_DATA(COMPRESSED_BUFFER);

#ifndef PADDING
    _count = 0;
#else /* PADDING */
    _count = SPACE_BOTTOM*FULL_WIDTH;

    if ((SPACE_LEFT != 0) || (SPACE_RIGHT != 0)) {
        int _line, _lastline;
        for (_line = SPACE_BOTTOM, _lastline = FULL_HEIGHT-SPACE_TOP;
             _line < _lastline; _line++) {
            int _x = SPACE_LEFT;
            int _lastx = FULL_WIDTH-SPACE_RIGHT;
            _count += SPACE_LEFT;
            while (ICET_TRUE) {
                IceTUInt *_runlengths;
                while ((_x < _lastx) && (!ACTIVE())) {
                    _x++;
                    _count++;
                    INCREMENT_PIXEL();
                }
                if (_x >= _lastx) break;
                _runlengths = _dest++;
                while (_count > 0xFFFF) {
                    INACTIVE_RUN_LENGTH(_runlengths) = 0xFFFF;
                    ACTIVE_RUN_LENGTH(_runlengths) = 0;
#ifdef DEBUG
                    _totalcount += 0xFFFF;
#endif
                    _count -= 0xFFFF;
                    _runlengths = _dest++;
                }
                INACTIVE_RUN_LENGTH(_runlengths) = (IceTUShort)_count;
#ifdef DEBUG
                _totalcount += _count;
#endif
                _count = 0;
                while ((_x < _lastx) && ACTIVE() && (_count < 0xFFFF)) {
                    WRITE_PIXEL(_dest);
                    INCREMENT_PIXEL();
                    _count++;
                    _x++;
                }
                ACTIVE_RUN_LENGTH(_runlengths) = (IceTUShort)_count;
#ifdef DEBUG
                _totalcount += _count;
#endif
                _count = 0;
                if (_x >= _lastx) break;
            }
            _count += SPACE_RIGHT;
        }
    } else { /* SPACE_LEFT == SPACE_RIGHT == 0 */
        _pixels = (FULL_HEIGHT-SPACE_BOTTOM-SPACE_TOP)*FULL_WIDTH;
#endif /* PADDING */

        _p = 0;
        while (_p < _pixels) {
            IceTUInt *_runlengths = _dest++;
          /* Count background pixels. */
            while ((_p < _pixels) && (!ACTIVE())) {
                _p++;
                _count++;
                INCREMENT_PIXEL();
            }
            while (_count > 0xFFFF) {
                INACTIVE_RUN_LENGTH(_runlengths) = 0xFFFF;
                ACTIVE_RUN_LENGTH(_runlengths) = 0;
#ifdef DEBUG
                _totalcount += 0xFFFF;
#endif
                _count -= 0xFFFF;
                _runlengths = _dest++;
            }
            INACTIVE_RUN_LENGTH(_runlengths) = (IceTUShort)_count;
#ifdef DEBUG
            _totalcount += _count;
#endif

          /* Count and store active pixels. */
            _count = 0;
            while ((_p < _pixels) && ACTIVE() && (_count < 0xFFFF)) {
                WRITE_PIXEL(_dest);
                INCREMENT_PIXEL();
                _count++;
                _p++;
            }
            ACTIVE_RUN_LENGTH(_runlengths) = (IceTUShort)_count;
#ifdef DEBUG
            _totalcount += _count;
#endif

            _count = 0;
        }
#ifdef PADDING
    }

    _count += SPACE_TOP*FULL_WIDTH;
    if (_count > 0) {
        while (_count > 0xFFFF) {
            INACTIVE_RUN_LENGTH(_dest) = 0xFFFF;
            ACTIVE_RUN_LENGTH(_dest) = 0;
            _dest++;
#ifdef DEBUG
            _totalcount += 0xFFFF;
#endif /*DEBUG*/
            _count -= 0xFFFF;
        }
        INACTIVE_RUN_LENGTH(_dest) = (IceTUShort)_count;
        ACTIVE_RUN_LENGTH(_dest) = 0;
        _dest++;
#ifdef DEBUG
        _totalcount += _count;
#endif /*DEBUG*/
    }
#endif /*PADDING*/

#ifdef DEBUG
    if (_totalcount != (IceTUInt)PIXEL_COUNT) {
        char msg[256];
        sprintf(msg, "Total run lengths don't equal pixel count: %d != %d",
                (int)_totalcount, (int)(PIXEL_COUNT));
        icetRaiseError(msg, ICET_SANITY_CHECK_FAIL);
    }
#endif

    *_compress_time += icetWallTime() - _timer;

    _compressed_size
        = (IceTSizeType)(  (IceTPointerArithmetic)_dest
                         - (IceTPointerArithmetic)COMPRESSED_BUFFER);
    ICET_IMAGE_HEADER(COMPRESSED_BUFFER)[ICET_IMAGE_ACTUAL_BUFFER_SIZE_INDEX]
        = _compressed_size;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#undef COMPRESSED_BUFFER
#undef COLOR_FORMAT
#undef DEPTH_FORMAT
#undef PIXEL_COUNT
#undef ACTIVE
#undef WRITE_PIXEL
#undef INCREMENT_PIXEL
#undef COMPRESSED_SIZE

#ifdef PADDING
#undef PADDING
#undef SPACE_BOTTOM
#undef SPACE_TOP
#undef SPACE_LEFT
#undef SPACE_RIGHT
#undef FULL_WIDTH
#undef FULL_HEIGHT
#endif
