/* -*- c -*- *******************************************************/
/*
 * Copyright (C) 2003 Sandia Corporation
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the U.S. Government.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that this Notice and any statement
 * of authorship are reproduced on all copies.
 */

#include <IceT.h>

#include <IceTDevImage.h>
#include <IceTDevState.h>
#include <IceTDevDiagnostics.h>
#include "common.h"

static IceTImage sequentialCompose(void);

IceTStrategy ICET_STRATEGY_SEQUENTIAL
    = { "Sequential", ICET_TRUE, sequentialCompose };

#define SEQUENTIAL_IMAGE_BUFFER                 ICET_STRATEGY_BUFFER_0
#define SEQUENTIAL_IN_SPARSE_IMAGE_BUFFER       ICET_STRATEGY_BUFFER_1
#define SEQUENTIAL_OUT_SPARSE_IMAGE_BUFFER      ICET_STRATEGY_BUFFER_2
#define SEQUENTIAL_FINAL_IMAGE_BUFFER           ICET_STRATEGY_BUFFER_3
#define SEQUENTIAL_COMPOSE_GROUP_BUFFER         ICET_STRATEGY_BUFFER_4

static IceTImage sequentialCompose(void)
{
    IceTInt num_tiles;
    IceTInt max_width, max_height;
    IceTInt rank;
    IceTInt num_proc;
    IceTInt *display_nodes;
    IceTBoolean ordered_composite;
    IceTImage myImage;
    IceTImage image;
    IceTVoid *inSparseImageBuffer;
    IceTSparseImage outSparseImage;
    IceTSizeType sparseImageSize;
    IceTInt *compose_group;
    int i;

    icetGetIntegerv(ICET_NUM_TILES, &num_tiles);
    icetGetIntegerv(ICET_TILE_MAX_WIDTH, &max_width);
    icetGetIntegerv(ICET_TILE_MAX_HEIGHT, &max_height);
    icetGetIntegerv(ICET_RANK, &rank);
    icetGetIntegerv(ICET_NUM_PROCESSES, &num_proc);
    display_nodes = icetUnsafeStateGetInteger(ICET_DISPLAY_NODES);
    ordered_composite = icetIsEnabled(ICET_ORDERED_COMPOSITE);

    sparseImageSize = icetSparseImageBufferSize(max_width, max_height);

    image               = icetGetStateBufferImage(SEQUENTIAL_IMAGE_BUFFER,
                                                  max_width, max_height);
    inSparseImageBuffer = icetGetStateBuffer(SEQUENTIAL_IN_SPARSE_IMAGE_BUFFER,
                                             sparseImageSize);
    outSparseImage      = icetGetStateBufferSparseImage(
                                             SEQUENTIAL_OUT_SPARSE_IMAGE_BUFFER,
                                             max_width, max_height);
    compose_group       = icetGetStateBuffer(SEQUENTIAL_COMPOSE_GROUP_BUFFER,
                                             sizeof(IceTInt)*num_proc);

    myImage = icetImageNull();

    if (ordered_composite) {
	icetGetIntegerv(ICET_COMPOSITE_ORDER, compose_group);
    } else {
	for (i = 0; i < num_proc; i++) {
	    compose_group[i] = i;
	}
    }

  /* Render and compose every tile. */
    for (i = 0; i < num_tiles; i++) {
        IceTImage tileImage;
	int d_node = display_nodes[i];
	int image_dest;

      /* Make the image go to the display node. */
	if (ordered_composite) {
	    for (image_dest = 0; compose_group[image_dest] != d_node;
		 image_dest++);
	} else {
	  /* Technically, the above computation will work, but this is
	     faster. */
	    image_dest = d_node;
	}

      /* If this processor is display node, make sure image goes to
         myColorBuffer. */
	if (d_node == rank) {
            tileImage = icetGetStateBufferImage(SEQUENTIAL_FINAL_IMAGE_BUFFER,
                                                max_width, max_height);
	} else {
	    tileImage = image;
          /* A previous iteration may have changed the image buffer to remove
             the depth.  This command restores that so that both buffers are
             read in for the next pass. */
            icetImageAdjustForInput(tileImage);
	}

	icetGetTileImage(i, tileImage);
	icetBswapCompose(compose_group, num_proc, image_dest,
			 tileImage, inSparseImageBuffer, outSparseImage);

        if (d_node == rank) {
            myImage = tileImage;
        }
    }

    return myImage;
}
