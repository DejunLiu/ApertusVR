
/* pngwrite.c - general routines to write a PNG file
 *
 * Last changed in libpng 1.5.7 [December 15, 2011]
 * Copyright (c) 1998-2011 Glenn Randers-Pehrson
 * (Version 0.96 Copyright (c) 1996, 1997 Andreas Dilger)
 * (Version 0.88 Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.)
 *
 * This code is released under the libpng license.
 * For conditions of distribution and use, see the disclaimer
 * and license in png.h
 */

#include "pngpriv.h"

#ifdef PNG_WRITE_SUPPORTED

/* Writes all the PNG information.  This is the suggested way to use the
 * library.  If you have a new chunk to add, make a function to write it,
 * and put it in the correct location here.  If you want the chunk written
 * after the image data, put it in png_write_end().  I strongly encourage
 * you to supply a PNG_INFO_ flag, and check info_ptr->valid before writing
 * the chunk, as that will keep the code from breaking if you want to just
 * write a plain PNG file.  If you have long comments, I suggest writing
 * them in png_write_end(), and compressing them.
 */
void PNGAPI
png_write_info_before_PLTE(png_structp png_ptr, png_infop info_ptr)
{
   png_debug(1, "in png_write_info_before_PLTE");

   if (png_ptr == NULL || info_ptr == NULL)
      return;

   if (!(png_ptr->mode & PNG_WROTE_INFO_BEFORE_PLTE))
   {
   /* Write PNG signature */
   png_write_sig(png_ptr);

#ifdef PNG_MNG_FEATURES_SUPPORTED
   if ((png_ptr->mode&PNG_HAVE_PNG_SIGNATURE) && \
       (png_ptr->mng_features_permitted))
   {
      png_warning(png_ptr, "MNG features are not allowed in a PNG datastream");
      png_ptr->mng_features_permitted = 0;
   }
#endif

   /* Write IHDR information. */
   png_write_IHDR(png_ptr, info_ptr->width, info_ptr->height,
       info_ptr->bit_depth, info_ptr->color_type, info_ptr->compression_type,
       info_ptr->filter_type,
#ifdef PNG_WRITE_INTERLACING_SUPPORTED
       info_ptr->interlace_type);
#else
       0);
#endif
   /* The rest of these check to see if the valid field has the appropriate
    * flag set, and if it does, writes the chunk.
    */
#ifdef PNG_WRITE_gAMA_SUPPORTED
   if (info_ptr->valid & PNG_INFO_gAMA)
      png_write_gAMA_fixed(png_ptr, info_ptr->gamma);
#endif
#ifdef PNG_WRITE_sRGB_SUPPORTED
   if (info_ptr->valid & PNG_INFO_sRGB)
      png_write_sRGB(png_ptr, (int)info_ptr->srgb_intent);
#endif

#ifdef PNG_WRITE_iCCP_SUPPORTED
   if (info_ptr->valid & PNG_INFO_iCCP)
      png_write_iCCP(png_ptr, info_ptr->iccp_name, PNG_COMPRESSION_TYPE_BASE,
          (png_charp)info_ptr->iccp_profile, (int)info_ptr->iccp_proflen);
#endif
#ifdef PNG_WRITE_sBIT_SUPPORTED
   if (info_ptr->valid & PNG_INFO_sBIT)
      png_write_sBIT(png_ptr, &(info_ptr->sig_bit), info_ptr->color_type);
#endif
#ifdef PNG_WRITE_cHRM_SUPPORTED
   if (info_ptr->valid & PNG_INFO_cHRM)
      png_write_cHRM_fixed(png_ptr,
          info_ptr->x_white, info_ptr->y_white,
          info_ptr->x_red, info_ptr->y_red,
          info_ptr->x_green, info_ptr->y_green,
          info_ptr->x_blue, info_ptr->y_blue);
#endif

#ifdef PNG_WRITE_UNKNOWN_CHUNKS_SUPPORTED
   if (info_ptr->unknown_chunks_num)
   {
      png_unknown_chunk *up;

      png_debug(5, "writing extra chunks");

      for (up = info_ptr->unknown_chunks;
           up < info_ptr->unknown_chunks + info_ptr->unknown_chunks_num;
           up++)
      {
         int keep = png_handle_as_unknown(png_ptr, up->name);

         if (keep != PNG_HANDLE_CHUNK_NEVER &&
             up->location &&
             !(up->location & PNG_HAVE_PLTE) &&
             !(up->location & PNG_HAVE_IDAT) &&
             !(up->location & PNG_AFTER_IDAT) &&
             ((up->name[3] & 0x20) || keep == PNG_HANDLE_CHUNK_ALWAYS ||
             (png_ptr->flags & PNG_FLAG_KEEP_UNSAFE_CHUNKS)))
         {
            if (up->size == 0)
               png_warning(png_ptr, "Writing zero-length unknown chunk");

            png_write_chunk(png_ptr, up->name, up->data, up->size);
         }
      }
   }
#endif
      png_ptr->mode |= PNG_WROTE_INFO_BEFORE_PLTE;
   }
}

void PNGAPI
png_write_info(png_structp png_ptr, png_infop info_ptr)
{
#if defined(PNG_WRITE_TEXT_SUPPORTED) || defined(PNG_WRITE_sPLT_SUPPORTED)
   int i;
#endif

   png_debug(1, "in png_write_info");

   if (png_ptr == NULL || info_ptr == NULL)
      return;

   png_write_info_before_PLTE(png_ptr, info_ptr);

   if (info_ptr->valid & PNG_INFO_PLTE)
      png_write_PLTE(png_ptr, info_ptr->palette,
          (png_uint_32)info_ptr->num_palette);

   else if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      png_error(png_ptr, "Valid palette required for paletted images");

#ifdef PNG_WRITE_tRNS_SUPPORTED
   if (info_ptr->valid & PNG_INFO_tRNS)
   {
#ifdef PNG_WRITE_INVERT_ALPHA_SUPPORTED
      /* Invert the alpha channel (in tRNS) */
      if ((png_ptr->transformations & PNG_INVERT_ALPHA) &&
          info_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      {
         int j;
         for (j = 0; j<(int)info_ptr->num_trans; j++)
            info_ptr->trans_alpha[j] =
               (png_byte)(255 - info_ptr->trans_alpha[j]);
      }
#endif
      png_write_tRNS(png_ptr, info_ptr->trans_alpha, &(info_ptr->trans_color),
          info_ptr->num_trans, info_ptr->color_type);
   }
#endif
#ifdef PNG_WRITE_bKGD_SUPPORTED
   if (info_ptr->valid & PNG_INFO_bKGD)
      png_write_bKGD(png_ptr, &(info_ptr->background), info_ptr->color_type);
#endif

#ifdef PNG_WRITE_hIST_SUPPORTED
   if (info_ptr->valid & PNG_INFO_hIST)
      png_write_hIST(png_ptr, info_ptr->hist, info_ptr->num_palette);
#endif

#ifdef PNG_WRITE_oFFs_SUPPORTED
   if (info_ptr->valid & PNG_INFO_oFFs)
      png_write_oFFs(png_ptr, info_ptr->x_offset, info_ptr->y_offset,
          info_ptr->offset_unit_type);
#endif

#ifdef PNG_WRITE_pCAL_SUPPORTED
   if (info_ptr->valid & PNG_INFO_pCAL)
      png_write_pCAL(png_ptr, info_ptr->pcal_purpose, info_ptr->pcal_X0,
          info_ptr->pcal_X1, info_ptr->pcal_type, info_ptr->pcal_nparams,
          info_ptr->pcal_units, info_ptr->pcal_params);
#endif

#ifdef PNG_WRITE_sCAL_SUPPORTED
   if (info_ptr->valid & PNG_INFO_sCAL)
      png_write_sCAL_s(png_ptr, (int)info_ptr->scal_unit,
          info_ptr->scal_s_width, info_ptr->scal_s_height);
#endif /* sCAL */

#ifdef PNG_WRITE_pHYs_SUPPORTED
   if (info_ptr->valid & PNG_INFO_pHYs)
      png_write_pHYs(png_ptr, info_ptr->x_pixels_per_unit,
          info_ptr->y_pixels_per_unit, info_ptr->phys_unit_type);
#endif /* pHYs */

#ifdef PNG_WRITE_tIME_SUPPORTED
   if (info_ptr->valid & PNG_INFO_tIME)
   {
      png_write_tIME(png_ptr, &(info_ptr->mod_time));
      png_ptr->mode |= PNG_WROTE_tIME;
   }
#endif /* tIME */

#ifdef PNG_WRITE_sPLT_SUPPORTED
   if (info_ptr->valid & PNG_INFO_sPLT)
      for (i = 0; i < (int)info_ptr->splt_palettes_num; i++)
         png_write_sPLT(png_ptr, info_ptr->splt_palettes + i);
#endif /* sPLT */

#ifdef PNG_WRITE_TEXT_SUPPORTED
   /* Check to see if we need to write text chunks */
   for (i = 0; i < info_ptr->num_text; i++)
   {
      png_debug2(2, "Writing header text chunk %d, type %d", i,
          info_ptr->text[i].compression);
      /* An internationalized chunk? */
      if (info_ptr->text[i].compression > 0)
      {
#ifdef PNG_WRITE_iTXt_SUPPORTED
         /* Write international chunk */
         png_write_iTXt(png_ptr,
             info_ptr->text[i].compression,
             info_ptr->text[i].key,
             info_ptr->text[i].lang,
             info_ptr->text[i].lang_key,
             info_ptr->text[i].text);
#else
          png_warning(png_ptr, "Unable to write international text");
#endif
          /* Mark this chunk as written */
          info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_NONE_WR;
      }

      /* If we want a compressed text chunk */
      else if (info_ptr->text[i].compression == PNG_TEXT_COMPRESSION_zTXt)
      {
#ifdef PNG_WRITE_zTXt_SUPPORTED
         /* Write compressed chunk */
         png_write_zTXt(png_ptr, info_ptr->text[i].key,
             info_ptr->text[i].text, 0,
             info_ptr->text[i].compression);
#else
         png_warning(png_ptr, "Unable to write compressed text");
#endif
         /* Mark this chunk as written */
         info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_zTXt_WR;
      }

      else if (info_ptr->text[i].compression == PNG_TEXT_COMPRESSION_NONE)
      {
#ifdef PNG_WRITE_tEXt_SUPPORTED
         /* Write uncompressed chunk */
         png_write_tEXt(png_ptr, info_ptr->text[i].key,
             info_ptr->text[i].text,
             0);
         /* Mark this chunk as written */
         info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_NONE_WR;
#else
         /* Can't get here */
         png_warning(png_ptr, "Unable to write uncompressed text");
#endif
      }
   }
#endif /* tEXt */

#ifdef PNG_WRITE_UNKNOWN_CHUNKS_SUPPORTED
   if (info_ptr->unknown_chunks_num)
   {
      png_unknown_chunk *up;

      png_debug(5, "writing extra chunks");

      for (up = info_ptr->unknown_chunks;
           up < info_ptr->unknown_chunks + info_ptr->unknown_chunks_num;
           up++)
      {
         int keep = png_handle_as_unknown(png_ptr, up->name);
         if (keep != PNG_HANDLE_CHUNK_NEVER &&
             up->location &&
             (up->location & PNG_HAVE_PLTE) &&
             !(up->location & PNG_HAVE_IDAT) &&
             !(up->location & PNG_AFTER_IDAT) &&
             ((up->name[3] & 0x20) || keep == PNG_HANDLE_CHUNK_ALWAYS ||
             (png_ptr->flags & PNG_FLAG_KEEP_UNSAFE_CHUNKS)))
         {
            png_write_chunk(png_ptr, up->name, up->data, up->size);
         }
      }
   }
#endif
}

/* Writes the end of the PNG file.  If you don't want to write comments or
 * time information, you can pass NULL for info.  If you already wrote these
 * in png_write_info(), do not write them again here.  If you have long
 * comments, I suggest writing them here, and compressing them.
 */
void PNGAPI
png_write_end(png_structp png_ptr, png_infop info_ptr)
{
   png_debug(1, "in png_write_end");

   if (png_ptr == NULL)
      return;

   if (!(png_ptr->mode & PNG_HAVE_IDAT))
      png_error(png_ptr, "No IDATs written into file");

   /* See if user wants us to write information chunks */
   if (info_ptr != NULL)
   {
#ifdef PNG_WRITE_TEXT_SUPPORTED
      int i; /* local index variable */
#endif
#ifdef PNG_WRITE_tIME_SUPPORTED
      /* Check to see if user has supplied a time chunk */
      if ((info_ptr->valid & PNG_INFO_tIME) &&
          !(png_ptr->mode & PNG_WROTE_tIME))
         png_write_tIME(png_ptr, &(info_ptr->mod_time));

#endif
#ifdef PNG_WRITE_TEXT_SUPPORTED
      /* Loop through comment chunks */
      for (i = 0; i < info_ptr->num_text; i++)
      {
         png_debug2(2, "Writing trailer text chunk %d, type %d", i,
            info_ptr->text[i].compression);
         /* An internationalized chunk? */
         if (info_ptr->text[i].compression > 0)
         {
#ifdef PNG_WRITE_iTXt_SUPPORTED
            /* Write international chunk */
            png_write_iTXt(png_ptr,
                info_ptr->text[i].compression,
                info_ptr->text[i].key,
                info_ptr->text[i].lang,
                info_ptr->text[i].lang_key,
                info_ptr->text[i].text);
#else
            png_warning(png_ptr, "Unable to write international text");
#endif
            /* Mark this chunk as written */
            info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_NONE_WR;
         }

         else if (info_ptr->text[i].compression >= PNG_TEXT_COMPRESSION_zTXt)
         {
#ifdef PNG_WRITE_zTXt_SUPPORTED
            /* Write compressed chunk */
            png_write_zTXt(png_ptr, info_ptr->text[i].key,
                info_ptr->text[i].text, 0,
                info_ptr->text[i].compression);
#else
            png_warning(png_ptr, "Unable to write compressed text");
#endif
            /* Mark this chunk as written */
            info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_zTXt_WR;
         }

         else if (info_ptr->text[i].compression == PNG_TEXT_COMPRESSION_NONE)
         {
#ifdef PNG_WRITE_tEXt_SUPPORTED
            /* Write uncompressed chunk */
            png_write_tEXt(png_ptr, info_ptr->text[i].key,
                info_ptr->text[i].text, 0);
#else
            png_warning(png_ptr, "Unable to write uncompressed text");
#endif

            /* Mark this chunk as written */
            info_ptr->text[i].compression = PNG_TEXT_COMPRESSION_NONE_WR;
         }
      }
#endif
#ifdef PNG_WRITE_UNKNOWN_CHUNKS_SUPPORTED
   if (info_ptr->unknown_chunks_num)
   {
      png_unknown_chunk *up;

      png_debug(5, "writing extra chunks");

      for (up = info_ptr->unknown_chunks;
           up < info_ptr->unknown_chunks + info_ptr->unknown_chunks_num;
           up++)
      {
         int keep = png_handle_as_unknown(png_ptr, up->name);
         if (keep != PNG_HANDLE_CHUNK_NEVER &&
             up->location &&
             (up->location & PNG_AFTER_IDAT) &&
             ((up->name[3] & 0x20) || keep == PNG_HANDLE_CHUNK_ALWAYS ||
             (png_ptr->flags & PNG_FLAG_KEEP_UNSAFE_CHUNKS)))
         {
            png_write_chunk(png_ptr, up->name, up->data, up->size);
         }
      }
   }
#endif
   }

   png_ptr->mode |= PNG_AFTER_IDAT;

   /* Write end of PNG file */
   png_write_IEND(png_ptr);
   /* This flush, added in libpng-1.0.8, removed from libpng-1.0.9beta03,
    * and restored again in libpng-1.2.30, may cause some applications that
    * do not set png_ptr->output_flush_fn to crash.  If your application
    * experiences a problem, please try building libpng with
    * PNG_WRITE_FLUSH_AFTER_IEND_SUPPORTED defined, and report the event to
    * png-mng-implement at lists.sf.net .
    */
#ifdef PNG_WRITE_FLUSH_SUPPORTED
#  ifdef PNG_WRITE_FLUSH_AFTER_IEND_SUPPORTED
   png_flush(png_ptr);
#  endif
#endif
}

#ifdef PNG_CONVERT_tIME_SUPPORTED
/* "tm" structure is not supported on WindowsCE */
void PNGAPI
png_convert_from_struct_tm(png_timep ptime, PNG_CONST struct tm FAR * ttime)
{
   png_debug(1, "in png_convert_from_struct_tm");

   ptime->year = (png_uint_16)(1900 + ttime->tm_year);
   ptime->month = (png_byte)(ttime->tm_mon + 1);
   ptime->day = (png_byte)ttime->tm_mday;
   ptime->hour = (png_byte)ttime->tm_hour;
   ptime->minute = (png_byte)ttime->tm_min;
   ptime->second = (png_byte)ttime->tm_sec;
}

void PNGAPI
png_convert_from_time_t(png_timep ptime, time_t ttime)
{
   struct tm *tbuf;

   png_debug(1, "in png_convert_from_time_t");

   tbuf = gmtime(&ttime);
   png_convert_from_struct_tm(ptime, tbuf);
}
#endif

/* Initialize png_ptr structure, and allocate any memory needed */
PNG_FUNCTION(png_structp,PNGAPI
png_create_write_struct,(png_const_charp user_png_ver, png_voidp error_ptr,
    png_error_ptr error_fn, png_error_ptr warn_fn),PNG_ALLOCATED)
{
#ifdef PNG_USER_MEM_SUPPORTED
   return (png_create_write_struct_2(user_png_ver, error_ptr, error_fn,
       warn_fn, NULL, NULL, NULL));
}

/* Alternate initialize png_ptr structure, and allocate any memory needed */
static void png_reset_filter_heuristics(png_structp png_ptr); /* forward decl */

PNG_FUNCTION(png_structp,PNGAPI
png_create_write_struct_2,(png_const_charp user_png_ver, png_voidp error_ptr,
    png_error_ptr error_fn, png_error_ptr warn_fn, png_voidp mem_ptr,
    png_malloc_ptr malloc_fn, png_free_ptr free_fn),PNG_ALLOCATED)
{
#endif /* PNG_USER_MEM_SUPPORTED */
   volatile int png_cleanup_needed = 0;
#ifdef PNG_SETJMP_SUPPORTED
   volatile
#endif
   png_structp png_ptr;
#ifdef PNG_SETJMP_SUPPORTED
#ifdef USE_FAR_KEYWORD
   jmp_buf tmp_jmpbuf;
#endif
#endif

   png_debug(1, "in png_create_write_struct");

#ifdef PNG_USER_MEM_SUPPORTED
   png_ptr = (png_structp)png_create_struct_2(PNG_STRUCT_PNG,
       (png_malloc_ptr)malloc_fn, (png_voidp)mem_ptr);
#else
   png_ptr = (png_structp)png_create_struct(PNG_STRUCT_PNG);
#endif /* PNG_USER_MEM_SUPPORTED */
   if (png_ptr == NULL)
      return (NULL);

   /* Added at libpng-1.2.6 */
#ifdef PNG_SET_USER_LIMITS_SUPPORTED
   png_ptr->user_width_max = PNG_USER_WIDTH_MAX;
   png_ptr->user_height_max = PNG_USER_HEIGHT_MAX;
#endif

#ifdef PNG_SETJMP_SUPPORTED
/* Applications that neglect to set up their own setjmp() and then
 * encounter a png_error() will longjmp here.  Since the jmpbuf is
 * then meaningless we abort instead of returning.
 */
#ifdef USE_FAR_KEYWORD
   if (setjmp(tmp_jmpbuf))
#else
   if (setjmp(png_jmpbuf(png_ptr))) /* sets longjmp to match setjmp */
#endif
#ifdef USE_FAR_KEYWORD
   png_memcpy(png_jmpbuf(png_ptr), tmp_jmpbuf, png_sizeof(jmp_buf));
#endif
      PNG_ABORT();
#endif

#ifdef PNG_USER_MEM_SUPPORTED
   png_set_mem_fn(png_ptr, mem_ptr, malloc_fn, free_fn);
#endif /* PNG_USER_MEM_SUPPORTED */
   png_set_error_fn(png_ptr, error_ptr, error_fn, warn_fn);

   if (!png_user_version_check(png_ptr, user_png_ver))
      png_cleanup_needed = 1;

   /* Initialize zbuf - compression buffer */
   png_ptr->zbuf_size = PNG_ZBUF_SIZE;

   if (!png_cleanup_needed)
   {
      png_ptr->zbuf = (png_bytep)png_malloc_warn(png_ptr,
          png_ptr->zbuf_size);
      if (png_ptr->zbuf == NULL)
         png_cleanup_needed = 1;
   }

   if (png_cleanup_needed)
   {
       /* Clean up PNG structure and deallocate any memory. */
       png_free(png_ptr, png_ptr->zbuf);
       png_ptr->zbuf = NULL;
#ifdef PNG_USER_MEM_SUPPORTED
       png_destroy_struct_2((png_voidp)png_ptr,
           (png_free_ptr)free_fn, (png_voidp)mem_ptr);
#else
       png_destroy_struct((png_voidp)png_ptr);
#endif
       return (NULL);
   }

   png_set_write_fn(png_ptr, NULL, NULL, NULL);

#ifdef PNG_WRITE_WEIGHTED_FILTER_SUPPORTED
   png_reset_filter_heuristics(png_ptr);
#endif

   return (png_ptr);
}


/* Write a few rows of image data.  If the image is interlaced,
 * either you will have to write the 7 sub images, or, if you
 * have called png_set_interlace_handling(), you will have to
 * "write" the image seven times.
 */
void PNGAPI
png_write_rows(png_structp png_ptr, png_bytepp row,
    png_uint_32 num_rows)
{
   png_uint_32 i; /* row counter */
   png_bytepp rp; /* row pointer */

   png_debug(1, "in png_write_rows");

   if (png_ptr == NULL)
      return;

   /* Loop through the rows */
   for (i = 0, rp = row; i < num_rows; i++, rp++)
   {
      png_write_row(png_ptr, *rp);
   }
}

/* Write the image.  You only need to call this function once, even
 * if you are writing an interlaced image.
 */
void PNGAPI
png_write_image(png_structp png_ptr, png_bytepp image)
{
   png_uint_32 i; /* row index */
   int pass, num_pass; /* pass variables */
   png_bytepp rp; /* points to current row */

   if (png_ptr == NULL)
      return;

   png_debug(1, "in png_write_image");

#ifdef PNG_WRITE_INTERLACING_SUPPORTED
   /* Initialize interlace handling.  If image is not interlaced,
    * this will set pass to 1
    */
   num_pass = png_set_interlace_handling(png_ptr);
#else
   num_pass = 1;
#endif
   /* Loop through passes */
   for (pass = 0; pass < num_pass; pass++)
   {
      /* Loop through image */
      for (i = 0, rp = image; i < png_ptr->height; i++, rp++)
      {
         png_write_row(png_ptr, *rp);
      }
   }
}

/* Called by user to write a row of image data */
void PNGAPI
png_write_row(png_structp png_ptr, png_const_bytep row)
{
   /* 1.5.6: moved from png_struct to be a local structure: */
   png_row_info row_info;

   if (png_ptr == NULL)
      return;

   png_debug2(1, "in png_write_row (row %u, pass %d)",
      png_ptr->row_number, png_ptr->pass);

   /* Initialize transformations and other stuff if first time */
   if (png_ptr->row_number == 0 && png_ptr->pass == 0)
   {
      /* Make sure we wrote the header info */
      if (!(png_ptr->mode & PNG_WROTE_INFO_BEFORE_PLTE))
         png_error(png_ptr,
             "png_write_info was never called before png_write_row");

      /* Check for transforms that have been set but were defined out */
#if !defined(PNG_WRITE_INVERT_SUPPORTED) && defined(PNG_READ_INVERT_SUPPORTED)
      if (png_ptr->transformations & PNG_INVERT_MONO)
         png_warning(png_ptr, "PNG_WRITE_INVERT_SUPPORTED is not defined");
#endif

#if !defined(PNG_WRITE_FILLER_SUPPORTED) && defined(PNG_READ_FILLER_SUPPORTED)
      if (png_ptr->transformations & PNG_FILLER)
         png_warning(png_ptr, "PNG_WRITE_FILLER_SUPPORTED is not defined");
#endif
#if !defined(PNG_WRITE_PACKSWAP_SUPPORTED) && \
    defined(PNG_READ_PACKSWAP_SUPPORTED)
      if (png_ptr->transformations & PNG_PACKSWAP)
         png_warning(png_ptr,
             "PNG_WRITE_PACKSWAP_SUPPORTED is not defined");
#endif

#if !defined(PNG_WRITE_PACK_SUPPORTED) && defined(PNG_READ_PACK_SUPPORTED)
      if (png_ptr->transformations & PNG_PACK)
         png_warning(png_ptr, "PNG_WRITE_PACK_SUPPORTED is not defined");
#endif

#if !defined(PNG_WRITE_SHIFT_SUPPORTED) && defined(PNG_READ_SHIFT_SUPPORTED)
      if (png_ptr->transformations & PNG_SHIFT)
         png_warning(png_ptr, "PNG_WRITE_SHIFT_SUPPORTED is not defined");
#endif

#if !defined(PNG_WRITE_BGR_SUPPORTED) && defined(PNG_READ_BGR_SUPPORTED)
      if (png_ptr->transformations & PNG_BGR)
         png_warning(png_ptr, "PNG_WRITE_BGR_SUPPORTED is not defined");
#endif

#if !defined(PNG_WRITE_SWAP_SUPPORTED) && defined(PNG_READ_SWAP_SUPPORTED)
      if (png_ptr->transformations & PNG_SWAP_BYTES)
         png_warning(png_ptr, "PNG_WRITE_SWAP_SUPPORTED is not defined");
#endif

      png_write_start_row(png_ptr);
   }

#ifdef PNG_WRITE_INTERLACING_SUPPORTED
   /* If interlaced and not interested in row, return */
   if (png_ptr->interlaced && (png_ptr->transformations & PNG_INTERLACE))
   {
      switch (png_ptr->pass)
      {
         case 0:
            if (png_ptr->row_number & 0x07)
            {
               png_write_finish_row(png_ptr);
               return;
            }
            break;

         case 1:
            if ((png_ptr->row_number & 0x07) || png_ptr->width < 5)
            {
               png_write_finish_row(png_ptr);
               return;
            }
            break;

         case 2:
            if ((png_ptr->row_number & 0x07) != 4)
            {
               png_write_finish_row(png_ptr);
               return;
            }
            break;

         case 3:
            if ((png_ptr->row_number & 0x03) || png_ptr->width < 3)
            {
               png_write_finish_row(png_ptr);
               return;
            }
            break;

         case 4:
            if ((png_ptr->row_number & 0x03) != 2)
            {
               png_write_finish_row(png_ptr);
               return;
            }
            break;

         case 5:
            if ((png_ptr->row_number & 0x01) || png_ptr->width < 2)
            {
               png_write_finish_row(png_ptr);
               return;
            }
            break;

         case 6:
            if (!(png_ptr->row_number & 0x01))
            {
               png_write_finish_row(png_ptr);
               return;
            }
            break;

         default: /* error: ignore it */
            break;
      }
   }
#endif

   /* Set up row info for transformations */
   row_info.color_type = png_ptr->color_type;
   row_info.width = png_ptr->usr_width;
   row_info.channels = png_ptr->usr_channels;
   row_info.bit_depth = png_ptr->usr_bit_depth;
   row_info.pixel_depth = (png_byte)(row_info.bit_depth * row_info.channels);
   row_info.rowbytes = PNG_ROWBYTES(row_info.pixel_depth, row_info.width);

   png_debug1(3, "row_info->color_type = %d", row_info.color_type);
   png_debug1(3, "row_info->width = %u", row_info.width);
   png_debug1(3, "row_info->channels = %d", row_info.channels);
   png_debug1(3, "row_info->bit_depth = %d", row_info.bit_depth);
   png_debug1(3, "row_info->pixel_depth = %d", row_info.pixel_depth);
   png_debug1(3, "row_info->rowbytes = %lu", (unsigned long)row_info.rowbytes);

   /* Copy user's row into buffer, leaving room for filter byte. */
   png_memcpy(png_ptr->row_buf + 1, row, row_info.rowbytes);

#ifdef PNG_WRITE_INTERLACING_SUPPORTED
   /* Handle interlacing */
   if (png_ptr->interlaced && png_ptr->pass < 6 &&
       (png_ptr->transformations & PNG_INTERLACE))
   {
      png_do_write_interlace(&row_info, png_ptr->row_buf + 1, png_ptr->pass);
      /* This should always get caught above, but still ... */
      if (!(row_info.width))
      {
         png_write_finish_row(png_ptr);
         return;
      }
   }
#endif

#ifdef PNG_WRITE_TRANSFORMS_SUPPORTED
   /* Handle other transformations */
   if (png_ptr->transformations)
      png_do_write_transformations(png_ptr, &row_info);
#endif

   /* At this point the row_info pixel depth must match the 'transformed' depth,
    * which is also the output depth.
    */
   if (row_info.pixel_depth != png_ptr->pixel_depth ||
      row_info.pixel_depth != png_ptr->transformed_pixel_depth)
      png_error(png_ptr, "internal write transform logic error");

#ifdef PNG_MNG_FEATURES_SUPPORTED
   /* Write filter_method 64 (intrapixel differencing) only if
    * 1. Libpng was compiled with PNG_MNG_FEATURES_SUPPORTED and
    * 2. Libpng did not write a PNG signature (this filter_method is only
    *    used in PNG datastreams that are embedded in MNG datastreams) and
    * 3. The application called png_permit_mng_features with a mask that
    *    included PNG_FLAG_MNG_FILTER_64 and
    * 4. The filter_method is 64 and
    * 5. The color_type is RGB or RGBA
    */
   if ((png_ptr->mng_features_permitted & PNG_FLAG_MNG_FILTER_64) &&
       (png_ptr->filter_type == PNG_INTRAPIXEL_DIFFERENCING))
   {
      /* Intrapixel differencing */
      png_do_write_intrapixel(&row_info, png_ptr->row_buf + 1);
   }
#endif

   /* Find a filter if necessary, filter the row and write it out. */
   png_write_find_filter(png_ptr, &row_info);

   if (png_ptr->write_row_fn != NULL)
      (*(png_ptr->write_row_fn))(png_ptr, png_ptr->row_number, png_ptr->pass);
}

#ifdef PNG_WRITE_FLUSH_SUPPORTED
/* Set the automatic flush interval or 0 to turn flushing off */
void PNGAPI
png_set_flush(png_structp png_ptr, int nrows)
{
   png_debug(1, "in png_set_flush");

   if (png_ptr == NULL)
      return;

   png_ptr->flush_dist = (nrows < 0 ? 0 : nrows);
}

/* Flush the current output buffers now */
void PNGAPI
png_write_flush(png_structp png_ptr)
{
   int wrote_IDAT;

   png_debug(1, "in png_write_flush");

   if (png_ptr == NULL)
      return;

   /* We have already written out all of the data */
   if (png_ptr->row_number >= png_ptr->num_rows)
      return;

   do
   {
      int ret;

      /* Compress the data */
      ret = deflate(&png_ptr->zstream, Z_SYNC_FLUSH);
      wrote_IDAT = 0;

      /* Check for compression errors */
      if (ret != Z_OK)
      {
         if (png_ptr->zstream.msg != NULL)
            png_error(png_ptr, png_ptr->zstream.msg);

         else
            png_error(png_ptr, "zlib error");
      }

      if (!(png_ptr->zstream.avail_out))
      {
         /* Write the IDAT and reset the zlib output buffer */
         png_write_IDAT(png_ptr, png_ptr->zbuf, png_ptr->zbuf_size);
         wrote_IDAT = 1;
      }
   } while (wrote_IDAT == 1);

   /* If there is any data left to be output, write it into a new IDAT */
   if (png_ptr->zbuf_size != png_ptr->zstream.avail_out)
   {
      /* Write the IDAT and reset the zlib output buffer */
      png_write_IDAT(png_ptr, png_ptr->zbuf,
          png_ptr->zbuf_size - png_ptr->zstream.avail_out);
   }
   png_ptr->flush_rows = 0;
   png_flush(png_ptr);
}
#endif /* PNG_WRITE_FLUSH_SUPPORTED */

/* Free all memory used by the write */
void PNGAPI
png_destroy_write_struct(png_structpp png_ptr_ptr, png_infopp info_ptr_ptr)
{
   png_structp png_ptr = NULL;
   png_infop info_ptr = NULL;
#ifdef PNG_USER_MEM_SUPPORTED
   png_free_ptr free_fn = NULL;
   png_voidp mem_ptr = NULL;
#endif

   png_debug(1, "in png_destroy_write_struct");

   if (png_ptr_ptr != NULL)
      png_ptr = *png_ptr_ptr;

#ifdef PNG_USER_MEM_SUPPORTED
   if (png_ptr != NULL)
   {
      free_fn = png_ptr->free_fn;
      mem_ptr = png_ptr->mem_ptr;
   }
#endif

   if (info_ptr_ptr != NULL)
      info_ptr = *info_ptr_ptr;

   if (info_ptr != NULL)
   {
      if (png_ptr != NULL)
      {
         png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);

#ifdef PNG_HANDLE_AS_UNKNOWN_SUPPORTED
         if (png_ptr->num_chunk_list)
         {
            png_free(png_ptr, png_ptr->chunk_list);
            png_ptr->num_chunk_list = 0;
         }
#endif
      }

#ifdef PNG_USER_MEM_SUPPORTED
      png_destroy_struct_2((png_voidp)info_ptr, (png_free_ptr)free_fn,
          (png_voidp)mem_ptr);
#else
      png_destroy_struct((png_voidp)info_ptr);
#endif
      *info_ptr_ptr = NULL;
   }

   if (png_ptr != NULL)
   {
      png_write_destroy(png_ptr);
#ifdef PNG_USER_MEM_SUPPORTED
      png_destroy_struct_2((png_voidp)png_ptr, (png_free_ptr)free_fn,
          (png_voidp)mem_ptr);
#else
      png_destroy_struct((png_voidp)png_ptr);
#endif
      *png_ptr_ptr = NULL;
   }
}


/* Free any memory used in png_ptr struct (old method) */
void /* PRIVATE */
png_write_destroy(png_structp png_ptr)
{
#ifdef PNG_SETJMP_SUPPORTED
   jmp_buf tmp_jmp; /* Save jump buffer */
#endif
   png_error_ptr error_fn;
#ifdef PNG_WARNINGS_SUPPORTED
   png_error_ptr warning_fn;
#endif
   png_voidp error_ptr;
#ifdef PNG_USER_MEM_SUPPORTED
   png_free_ptr free_fn;
#endif

   png_debug(1, "in png_write_destroy");

   /* Free any memory zlib uses */
   if (png_ptr->zlib_state != PNG_ZLIB_UNINITIALIZED)
      deflateEnd(&png_ptr->zstream);

   /* Free our memory.  png_free checks NULL for us. */
   png_free(png_ptr, png_ptr->zbuf);
   png_free(png_ptr, png_ptr->row_buf);
#ifdef PNG_WRITE_FILTER_SUPPORTED
   png_free(png_ptr, png_ptr->prev_row);
   png_free(png_ptr, png_ptr->sub_row);
   png_free(png_ptr, png_ptr->up_row);
   png_free(png_ptr, png_ptr->avg_row);
   png_free(png_ptr, png_ptr->paeth_row);
#endif

#ifdef PNG_WRITE_WEIGHTED_FILTER_SUPPORTED
   /* Use this to save a little code space, it doesn't free the filter_costs */
   png_reset_filter_heuristics(png_ptr);
   png_free(png_ptr, png_ptr->filter_costs);
   png_free(png_ptr, png_ptr->inv_filter_costs);
#endif

#ifdef PNG_SETJMP_SUPPORTED
   /* Reset structure */
   png_memcpy(tmp_jmp, png_ptr->longjmp_buffer, png_sizeof(jmp_buf));
#endif

   error_fn = png_ptr->error_fn;
#ifdef PNG_WARNINGS_SUPPORTED
   warning_fn = png_ptr->warning_fn;
#endif
   error_ptr = png_ptr->error_ptr;
#ifdef PNG_USER_MEM_SUPPORTED
   free_fn = png_ptr->free_fn;
#endif

   png_memset(png_ptr, 0, png_sizeof(png_struct));

   png_ptr->error_fn = error_fn;
#ifdef PNG_WARNINGS_SUPPORTED
   png_ptr->warning_fn = warning_fn;
#endif
   png_ptr->error_ptr = error_ptr;
#ifdef PNG_USER_MEM_SUPPORTED
   png_ptr->free_fn = free_fn;
#endif

#ifdef PNG_SETJMP_SUPPORTED
   png_memcpy(png_ptr->longjmp_buffer, tmp_jmp, png_sizeof(jmp_buf));
#endif
}

/* Allow the application to select one or more row filters to use. */
void PNGAPI
png_set_filter(png_structp png_ptr, int method, int filters)
{
   png_debug(1, "in png_set_filter");

   if (png_ptr == NULL)
      return;

#ifdef PNG_MNG_FEATURES_SUPPORTED
   if ((png_ptr->mng_features_permitted & PNG_FLAG_MNG_FILTER_64) &&
       (method == PNG_INTRAPIXEL_DIFFERENCING))
      method = PNG_FILTER_TYPE_BASE;

#endif
   if (method == PNG_FILTER_TYPE_BASE)
   {
      switch (filters & (PNG_ALL_FILTERS | 0x07))
      {
#ifdef PNG_WRITE_FILTER_SUPPORTED
         case 5:
         case 6:
         case 7: png_warning(png_ptr, "Unknown row filter for method 0");
#endif /* PNG_WRITE_FILTER_SUPPORTED */
         case PNG_FILTER_VALUE_NONE:
            png_ptr->do_filter = PNG_FILTER_NONE; break;

#ifdef PNG_WRITE_FILTER_SUPPORTED
         case PNG_FILTER_VALUE_SUB:
            png_ptr->do_filter = PNG_FILTER_SUB; break;

         case PNG_FILTER_VALUE_UP:
            png_ptr->do_filter = PNG_FILTER_UP; break;

         case PNG_FILTER_VALUE_AVG:
            png_ptr->do_filter = PNG_FILTER_AVG; break;

         case PNG_FILTER_VALUE_PAETH:
            png_ptr->do_filter = PNG_FILTER_PAETH; break;

         default:
            png_ptr->do_filter = (png_byte)filters; break;
#else
         default:
            png_warning(png_ptr, "Unknown row filter for method 0");
#endif /* PNG_WRITE_FILTER_SUPPORTED */
      }

      /* If we have allocated the row_buf, this means we have already started
       * with the image and we should have allocated all of the filter buffers
       * that have been selected.  If prev_row isn't already allocated, then
       * it is too late to start using the filters that need it, since we
       * will be missing the data in the previous row.  If an application
       * wants to start and stop using particular filters during compression,
       * it should start out with all of the filters, and then add and
       * remove them after the start of compression.
       */
      if (png_ptr->row_buf != NULL)
      {
#ifdef PNG_WRITE_FILTER_SUPPORTED
         if ((png_ptr->do_filter & PNG_FILTER_SUB) && png_ptr->sub_row == NULL)
         {
            png_ptr->sub_row = (png_bytep)png_malloc(png_ptr,
                (png_ptr->rowbytes + 1));
            png_ptr->sub_row[0] = PNG_FILTER_VALUE_SUB;
         }

         if ((png_ptr->do_filter & PNG_FILTER_UP) && png_ptr->up_row == NULL)
         {
            if (png_ptr->prev_row == NULL)
            {
               png_warning(png_ptr, "Can't add Up filter after starting");
               png_ptr->do_filter = (png_byte)(png_ptr->do_filter &
                   ~PNG_FILTER_UP);
            }

            else
            {
               png_ptr->up_row = (png_bytep)png_malloc(png_ptr,
                   (png_ptr->rowbytes + 1));
               png_ptr->up_row[0] = PNG_FILTER_VALUE_UP;
            }
         }

         if ((png_ptr->do_filter & PNG_FILTER_AVG) && png_ptr->avg_row == NULL)
         {
            if (png_ptr->prev_row == NULL)
            {
               png_warning(png_ptr, "Can't add Average filter after starting");
               png_ptr->do_filter = (png_byte)(png_ptr->do_filter &
                   ~PNG_FILTER_AVG);
            }

            else
            {
               png_ptr->avg_row = (png_bytep)png_malloc(png_ptr,
                   (png_ptr->rowbytes + 1));
               png_ptr->avg_row[0] = PNG_FILTER_VALUE_AVG;
            }
         }

         if ((png_ptr->do_filter & PNG_FILTER_PAETH) &&
             png_ptr->paeth_row == NULL)
         {
            if (png_ptr->prev_row == NULL)
            {
               png_warning(png_ptr, "Can't add Paeth filter after starting");
               png_ptr->do_filter &= (png_byte)(~PNG_FILTER_PAETH);
            }

            else
            {
               png_ptr->paeth_row = (png_bytep)png_malloc(png_ptr,
                   (png_ptr->rowbytes + 1));
               png_ptr->paeth_row[0] = PNG_FILTER_VALUE_PAETH;
            }
         }

         if (png_ptr->do_filter == PNG_NO_FILTERS)
#endif /* PNG_WRITE_FILTER_SUPPORTED */
            png_ptr->do_filter = PNG_FILTER_NONE;
      }
   }
   else
      png_error(png_ptr, "Unknown custom filter method");
}

/* This allows us to influence the way in which libpng chooses the "best"
 * filter for the current scanline.  While the "minimum-sum-of-absolute-
 * differences metric is relatively fast and effective, there is some
 * question as to whether it can be improved upon by trying to keep the
 * filtered data going to zlib more consistent, hopefully resulting in
 * better compression.
 */
#ifdef PNG_WRITE_WEIGHTED_FILTER_SUPPORTED      /* GRR 970116 */
/* Convenience reset API. */
static void
png_reset_filter_heuristics(png_structp png_ptr)
{
   /* Clear out any old values in the 'weights' - this must be done because if
    * the app calls set_filter_heuristics multiple times with different
    * 'num_weights' values we would otherwise potentially have wrong sized
    * arrays.
    */
   png_ptr->num_prev_filters = 0;
   png_ptr->heuristic_method = PNG_FILTER_HEURISTIC_UNWEIGHTED;
   if (png_ptr->prev_filters != NULL)
   {
      png_bytep old = png_ptr->prev_filters;
      png_ptr->prev_filters = NULL;
      png_free(png_ptr, old);
   }
   if (png_ptr->filter_weights != NULL)
   {
      png_uint_16p old = png_ptr->filter_weights;
      png_ptr->filter_weights = NULL;
      png_free(png_ptr, old);
   }

   if (png_ptr->inv_filter_weights != NULL)
   {
      png_uint_16p old = png_ptr->inv_filter_weights;
      png_ptr->inv_filter_weights = NULL;
      png_free(png_ptr, old);
   }

   /* Leave the filter_costs - this array is fixed size. */
}

static int
png_init_filter_heuristics(png_structp png_ptr, int heuristic_method,
   int num_weights)
{
   if (png_ptr == NULL)
      return 0;

   /* Clear out the arrays */
   png_reset_filter_heuristics(png_ptr);

   /* Check arguments; the 'reset' function makes the correct settings for the
    * unweighted case, but we must handle the weight case by initializing the
    * arrays for the caller.
    */
   if (heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
   {
      int i;

      if (num_weights > 0)
      {
         png_ptr->prev_filters = (png_bytep)png_malloc(png_ptr,
             (png_uint_32)(png_sizeof(png_byte) * num_weights));

         /* To make sure that the weighting starts out fairly */
         for (i = 0; i < num_weights; i++)
         {
            png_ptr->prev_filters[i] = 255;
         }

         png_ptr->filter_weights = (png_uint_16p)png_malloc(png_ptr,
             (png_uint_32)(png_sizeof(png_uint_16) * num_weights));

         png_ptr->inv_filter_weights = (png_uint_16p)png_malloc(png_ptr,
             (png_uint_32)(png_sizeof(png_uint_16) * num_weights));

         for (i = 0; i < num_weights; i++)
         {
            png_ptr->inv_filter_weights[i] =
            png_ptr->filter_weights[i] = PNG_WEIGHT_FACTOR;
         }

         /* Safe to set this now */
         png_ptr->num_prev_filters = (png_byte)num_weights;
      }

      /* If, in the future, there are other filter methods, this would
       * need to be based on png_ptr->filter.
       */
      if (png_ptr->filter_costs == NULL)
      {
         png_ptr->filter_costs = (png_uint_16p)png_malloc(png_ptr,
             (png_uint_32)(png_sizeof(png_uint_16) * PNG_FILTER_VALUE_LAST));

         png_ptr->inv_filter_costs = (png_uint_16p)png_malloc(png_ptr,
             (png_uint_32)(png_sizeof(png_uint_16) * PNG_FILTER_VALUE_LAST));
      }

      for (i = 0; i < PNG_FILTER_VALUE_LAST; i++)
      {
         png_ptr->inv_filter_costs[i] =
         png_ptr->filter_costs[i] = PNG_COST_FACTOR;
      }

      /* All the arrays are inited, safe to set this: */
      png_ptr->heuristic_method = PNG_FILTER_HEURISTIC_WEIGHTED;

      /* Return the 'ok' code. */
      return 1;
   }
   else if (heuristic_method == PNG_FILTER_HEURISTIC_DEFAULT ||
      heuristic_method == PNG_FILTER_HEURISTIC_UNWEIGHTED)
   {
      return 1;
   }
   else
   {
      png_warning(png_ptr, "Unknown filter heuristic method");
      return 0;
   }
}

/* Provide floating and fixed point APIs */
#ifdef PNG_FLOATING_POINT_SUPPORTED
void PNGAPI
png_set_filter_heuristics(png_structp png_ptr, int heuristic_method,
    int num_weights, png_const_doublep filter_weights,
    png_const_doublep filter_costs)
{
   png_debug(1, "in png_set_filter_heuristics");

   /* The internal API allocates all the arrays and ensures that the elements of
    * those arrays are set to the default value.
    */
   if (!png_init_filter_heuristics(png_ptr, heuristic_method, num_weights))
      return;

   /* If using the weighted method copy in the weights. */
   if (heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
   {
      int i;
      for (i = 0; i < num_weights; i++)
      {
         if (filter_weights[i] <= 0.0)
         {
            png_ptr->inv_filter_weights[i] =
            png_ptr->filter_weights[i] = PNG_WEIGHT_FACTOR;
         }

         else
         {
            png_ptr->inv_filter_weights[i] =
                (png_uint_16)(PNG_WEIGHT_FACTOR*filter_weights[i]+.5);

            png_ptr->filter_weights[i] =
                (png_uint_16)(PNG_WEIGHT_FACTOR/filter_weights[i]+.5);
         }
      }

      /* Here is where we set the relative costs of the different filters.  We
       * should take the desired compression level into account when setting
       * the costs, so that Paeth, for instance, has a high relative cost at low
       * compression levels, while it has a lower relative cost at higher
       * compression settings.  The filter types are in order of increasing
       * relative cost, so it would be possible to do this with an algorithm.
       */
      for (i = 0; i < PNG_FILTER_VALUE_LAST; i++) if (filter_costs[i] >= 1.0)
      {
         png_ptr->inv_filter_costs[i] =
             (png_uint_16)(PNG_COST_FACTOR / filter_costs[i] + .5);

         png_ptr->filter_costs[i] =
             (png_uint_16)(PNG_COST_FACTOR * filter_costs[i] + .5);
      }
   }
}
#endif /* FLOATING_POINT */

#ifdef PNG_FIXED_POINT_SUPPORTED
void PNGAPI
png_set_filter_heuristics_fixed(png_structp png_ptr, int heuristic_method,
    int num_weights, png_const_fixed_point_p filter_weights,
    png_const_fixed_point_p filter_costs)
{
   png_debug(1, "in png_set_filter_heuristics_fixed");

   /* The internal API allocates all the arrays and ensures that the elements of
    * those arrays are set to the default value.
    */
   if (!png_init_filter_heuristics(png_ptr, heuristic_method, num_weights))
      return;

   /* If using the weighted method copy in the weights. */
   if (heuristic_method == PNG_FILTER_HEURISTIC_WEIGHTED)
   {
      int i;
      for (i = 0; i < num_weights; i++)
      {
         if (filter_weights[i] <= 0)
         {
            png_ptr->inv_filter_weights[i] =
            png_ptr->filter_weights[i] = PNG_WEIGHT_FACTOR;
         }

         else
         {
            png_ptr->inv_filter_weights[i] = (png_uint_16)
               ((PNG_WEIGHT_FACTOR*filter_weights[i]+PNG_FP_HALF)/PNG_FP_1);

            png_ptr->filter_weights[i] = (png_uint_16)((PNG_WEIGHT_FACTOR*
               PNG_FP_1+(filter_weights[i]/2))/filter_weights[i]);
         }
      }

      /* Here is where we set the relative costs of the different filters.  We
       * should take the desired compression level into account when setting
       * the costs, so that Paeth, for instance, has a high relative cost at low
       * compression levels, while it has a lower relative cost at higher
       * compression settings.  The filter types are in order of increasing
       * relative cost, so it would be possible to do this with an algorithm.
       */
      for (i = 0; i < PNG_FILTER_VALUE_LAST; i++)
         if (filter_costs[i] >= PNG_FP_1)
      {
         png_uint_32 tmp;

         /* Use a 32 bit unsigned temporary here because otherwise the
          * intermediate value will be a 32 bit *signed* integer (ANSI rules)
          * and this will get the wrong answer on division.
          */
         tmp = PNG_COST_FACTOR*PNG_FP_1 + (filter_costs[i]/2);
         tmp /= filter_costs[i];

         png_ptr->inv_filter_costs[i] = (png_uint_16)tmp;

         tmp = PNG_COST_FACTOR * filter_costs[i] + PNG_FP_HALF;
         tmp /= PNG_FP_1;

         png_ptr->filter_costs[i] = (png_uint_16)tmp;
      }
   }
}
#endif /* FIXED_POINT */
#endif /* PNG_WRITE_WEIGHTED_FILTER_SUPPORTED */

void PNGAPI
png_set_compression_level(png_structp png_ptr, int level)
{
   png_debug(1, "in png_set_compression_level");

   if (png_ptr == NULL)
      return;

   png_ptr->flags |= PNG_FLAG_ZLIB_CUSTOM_LEVEL;
   png_ptr->zlib_level = level;
}

void PNGAPI
png_set_compression_mem_level(png_structp png_ptr, int mem_level)
{
   png_debug(1, "in png_set_compression_mem_level");

   if (png_ptr == NULL)
      return;

   png_ptr->flags |= PNG_FLAG_ZLIB_CUSTOM_MEM_LEVEL;
   png_ptr->zlib_mem_level = mem_level;
}

void PNGAPI
png_set_compression_strategy(png_structp png_ptr, int strategy)
{
   png_debug(1, "in png_set_compression_strategy");

   if (png_ptr == NULL)
      return;

   png_ptr->flags |= PNG_FLAG_ZLIB_CUSTOM_STRATEGY;
   png_ptr->zlib_strategy = strategy;
}

/* If PNG_WRITE_OPTIMIZE_CMF_SUPPORTED is defined, libpng will use a
 * smaller value of window_bits if it can do so safely.
 */
void PNGAPI
png_set_compression_window_bits(png_structp png_ptr, int window_bits)
{
   if (png_ptr == NULL)
      return;

   if (window_bits > 15)
      png_warning(png_ptr, "Only compression windows <= 32k supported by PNG");

   else if (window_bits < 8)
      png_warning(png_ptr, "Only compression windows >= 256 supported by PNG");

#ifndef WBITS_8_OK
   /* Avoid libpng bug with 256-byte windows */
   if (window_bits == 8)
      {
        png_warning(png_ptr, "Compression window is being reset to 512");
        window_bits = 9;
      }

#endif
   png_ptr->flags |= PNG_FLAG_ZLIB_CUSTOM_WINDOW_BITS;
   png_ptr->zlib_window_bits = window_bits;
}

void PNGAPI
png_set_compression_method(png_structp png_ptr, int method)
{
   png_debug(1, "in png_set_compression_method");

   if (png_ptr == NULL)
      return;

   if (method != 8)
      png_warning(png_ptr, "Only compression method 8 is supported by PNG");

   png_ptr->flags |= PNG_FLAG_ZLIB_CUSTOM_METHOD;
   png_ptr->zlib_method = method;
}

/* The following were added to libpng-1.5.4 */
#ifdef PNG_WRITE_CUSTOMIZE_ZTXT_COMPRESSION_SUPPORTED
void PNGAPI
png_set_text_compression_level(png_structp png_ptr, int level)
{
   png_debug(1, "in png_set_text_compression_level");

   if (png_ptr == NULL)
      return;

   png_ptr->flags |= PNG_FLAG_ZTXT_CUSTOM_LEVEL;
   png_ptr->zlib_text_level = level;
}

void PNGAPI
png_set_text_compression_mem_level(png_structp png_ptr, int mem_level)
{
   png_debug(1, "in png_set_text_compression_mem_level");

   if (png_ptr == NULL)
      return;

   png_ptr->flags |= PNG_FLAG_ZTXT_CUSTOM_MEM_LEVEL;
   png_ptr->zlib_text_mem_level = mem_level;
}

void PNGAPI
png_set_text_compression_strategy(png_structp png_ptr, int strategy)
{
   png_debug(1, "in png_set_text_compression_strategy");

   if (png_ptr == NULL)
      return;

   png_ptr->flags |= PNG_FLAG_ZTXT_CUSTOM_STRATEGY;
   png_ptr->zlib_text_strategy = strategy;
}

/* If PNG_WRITE_OPTIMIZE_CMF_SUPPORTED is defined, libpng will use a
 * smaller value of window_bits if it can do so safely.
 */
void PNGAPI
png_set_text_compression_window_bits(png_structp png_ptr, int window_bits)
{
   if (png_ptr == NULL)
      return;

   if (window_bits > 15)
      png_warning(png_ptr, "Only compression windows <= 32k supported by PNG");

   else if (window_bits < 8)
      png_warning(png_ptr, "Only compression windows >= 256 supported by PNG");

#ifndef WBITS_8_OK
   /* Avoid libpng bug with 256-byte windows */
   if (window_bits == 8)
      {
        png_warning(png_ptr, "Text compression window is being reset to 512");
        window_bits = 9;
      }

#endif
   png_ptr->flags |= PNG_FLAG_ZTXT_CUSTOM_WINDOW_BITS;
   png_ptr->zlib_text_window_bits = window_bits;
}

void PNGAPI
png_set_text_compression_method(png_structp png_ptr, int method)
{
   png_debug(1, "in png_set_text_compression_method");

   if (png_ptr == NULL)
      return;

   if (method != 8)
      png_warning(png_ptr, "Only compression method 8 is supported by PNG");

   png_ptr->flags |= PNG_FLAG_ZTXT_CUSTOM_METHOD;
   png_ptr->zlib_text_method = method;
}
#endif /* PNG_WRITE_CUSTOMIZE_ZTXT_COMPRESSION_SUPPORTED */
/* end of API added to libpng-1.5.4 */

void PNGAPI
png_set_write_status_fn(png_structp png_ptr, png_write_status_ptr write_row_fn)
{
   if (png_ptr == NULL)
      return;

   png_ptr->write_row_fn = write_row_fn;
}

#ifdef PNG_WRITE_USER_TRANSFORM_SUPPORTED
void PNGAPI
png_set_write_user_transform_fn(png_structp png_ptr, png_user_transform_ptr
    write_user_transform_fn)
{
   png_debug(1, "in png_set_write_user_transform_fn");

   if (png_ptr == NULL)
      return;

   png_ptr->transformations |= PNG_USER_TRANSFORM;
   png_ptr->write_user_transform_fn = write_user_transform_fn;
}
#endif


#ifdef PNG_INFO_IMAGE_SUPPORTED
void PNGAPI
png_write_png(png_structp png_ptr, png_infop info_ptr,
    int transforms, voidp params)
{
   if (png_ptr == NULL || info_ptr == NULL)
      return;

   /* Write the file header information. */
   png_write_info(png_ptr, info_ptr);

   /* ------ these transformations don't touch the info structure ------- */

#ifdef PNG_WRITE_INVERT_SUPPORTED
   /* Invert monochrome pixels */
   if (transforms & PNG_TRANSFORM_INVERT_MONO)
      png_set_invert_mono(png_ptr);
#endif

#ifdef PNG_WRITE_SHIFT_SUPPORTED
   /* Shift the pixels up to a legal bit depth and fill in
    * as appropriate to correctly scale the image.
    */
   if ((transforms & PNG_TRANSFORM_SHIFT)
       && (info_ptr->valid & PNG_INFO_sBIT))
      png_set_shift(png_ptr, &info_ptr->sig_bit);
#endif

#ifdef PNG_WRITE_PACK_SUPPORTED
   /* Pack pixels into bytes */
   if (transforms & PNG_TRANSFORM_PACKING)
       png_set_packing(png_ptr);
#endif

#ifdef PNG_WRITE_SWAP_ALPHA_SUPPORTED
   /* Swap location of alpha bytes from ARGB to RGBA */
   if (transforms & PNG_TRANSFORM_SWAP_ALPHA)
      png_set_swap_alpha(png_ptr);
#endif

#ifdef PNG_WRITE_FILLER_SUPPORTED
   /* Pack XRGB/RGBX/ARGB/RGBA into RGB (4 channels -> 3 channels) */
   if (transforms & PNG_TRANSFORM_STRIP_FILLER_AFTER)
      png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);

   else if (transforms & PNG_TRANSFORM_STRIP_FILLER_BEFORE)
      png_set_filler(png_ptr, 0, PNG_FILLER_BEFORE);
#endif

#ifdef PNG_WRITE_BGR_SUPPORTED
   /* Flip BGR pixels to RGB */
   if (transforms & PNG_TRANSFORM_BGR)
      png_set_bgr(png_ptr);
#endif

#ifdef PNG_WRITE_SWAP_SUPPORTED
   /* Swap bytes of 16-bit files to most significant byte first */
   if (transforms & PNG_TRANSFORM_SWAP_ENDIAN)
      png_set_swap(png_ptr);
#endif

#ifdef PNG_WRITE_PACKSWAP_SUPPORTED
   /* Swap bits of 1, 2, 4 bit packed pixel formats */
   if (transforms & PNG_TRANSFORM_PACKSWAP)
      png_set_packswap(png_ptr);
#endif

#ifdef PNG_WRITE_INVERT_ALPHA_SUPPORTED
   /* Invert the alpha channel from opacity to transparency */
   if (transforms & PNG_TRANSFORM_INVERT_ALPHA)
      png_set_invert_alpha(png_ptr);
#endif

   /* ----------------------- end of transformations ------------------- */

   /* Write the bits */
   if (info_ptr->valid & PNG_INFO_IDAT)
       png_write_image(png_ptr, info_ptr->row_pointers);

   /* It is REQUIRED to call this to finish writing the rest of the file */
   png_write_end(png_ptr, info_ptr);

   PNG_UNUSED(transforms)   /* Quiet compiler warnings */
   PNG_UNUSED(params)
}
#endif
#endif /* PNG_WRITE_SUPPORTED */
