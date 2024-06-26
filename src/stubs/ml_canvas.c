/**************************************************************************/
/*                                                                        */
/*    Copyright 2022 OCamlPro                                             */
/*                                                                        */
/*  All rights reserved. This file is distributed under the terms of the  */
/*  GNU Lesser General Public License version 2.1, with the special       */
/*  exception on linking described in the file LICENSE.                   */
/*                                                                        */
/**************************************************************************/

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#define CAML_NAME_SPACE
#include "caml/version.h"
#include "caml/memory.h"
#include "caml/alloc.h"
#include "caml/bigarray.h"
#include "caml/fail.h"
#include "caml/callback.h"

#include "../implem/config.h"
#include "../implem/tuples.h"
#include "../implem/color.h"
#include "../implem/gradient.h"
#include "../implem/pattern.h"
#include "../implem/transform.h"
#include "../implem/path.h"
#include "../implem/path2d.h"
#include "../implem/pixmap.h"
#include "../implem/impexp.h"
#include "../implem/event.h"
#include "../implem/canvas.h"
#include "../implem/backend.h"

#include "ml_tags.h"
#include "ml_convert.h"

#define BYTECODE_STUB_6(name) \
CAMLprim value name(value *a, int n) \
{ return name##_n(a[0],a[1],a[2],a[3],a[4],a[5]); }

#define BYTECODE_STUB_7(name) \
CAMLprim value name(value *a, int n) \
{ return name##_n(a[0],a[1],a[2],a[3],a[4],a[5],a[6]); }

#define BYTECODE_STUB_8(name) \
CAMLprim value name(value *a, int n) \
{ return name##_n(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7]); }

#define BYTECODE_STUB_9(name) \
CAMLprim value name(value *a, int n) \
{ return name##_n(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8]); }

#define BYTECODE_STUB_10(name) \
CAMLprim value name(value *a, int n) \
{ return name##_n(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8],a[9]); }

#define BYTECODE_STUB_11(name) \
CAMLprim value name(value *a, int n) \
{ return name##_n(a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],a[8],a[9],a[10]); }



static bool _ml_canvas_initialized = false;

static void
_ml_canvas_ensure_initialized(
  void)
{
  if (_ml_canvas_initialized == false) {
    caml_raise_constant(*caml_named_value("Not_initialized"));
  }
}

static bool
_ml_canvas_valid_canvas_size(
  int32_t width,
  int32_t height)
{
  return 0 < width && width <= 32767 && 0 < height && height <= 32767;
}




/* Image Data (aka Pixmaps) */

CAMLprim value
ml_canvas_image_data_create_from_png(
  value mlFilename,
  value mlOnLoad)
{
  CAMLparam2(mlFilename, mlOnLoad);
  _ml_canvas_ensure_initialized();
  const char *filename = String_val(mlFilename);
  pixmap_t pixmap = { 0 };
  bool res = impexp_import_png(&pixmap, 0, 0, filename);
  if ((res == false) || (pixmap_valid(pixmap) == false)) {
    caml_raise_with_string(*caml_named_value("Read_png_failed"), filename);
  }
  caml_callback(mlOnLoad, Val_pixmap(&pixmap));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_image_data_get_size(
  value mlPixmap)
{
  CAMLparam1(mlPixmap);
  CAMLlocal1(mlResult);
  pixmap_t pixmap = Pixmap_val(mlPixmap);
  mlResult = caml_alloc_tuple(2);
  Store_field(mlResult, 0, Val_int31_clip(pixmap.width));
  Store_field(mlResult, 1, Val_int31_clip(pixmap.height));
  CAMLreturn(mlResult);
}

CAMLprim value
ml_canvas_image_data_fill(
  value mlPixmap,
  value mlColor)
{
  CAMLparam2(mlPixmap, mlColor);
  pixmap_t pixmap = Pixmap_val(mlPixmap);
  color_t_ color = color_of_int(Int32_val(mlColor));
  for (int32_t i = 0; i < pixmap.height * pixmap.width; ++i) {
    pixmap.data[i] = color;
  }
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_image_data_sub(
  value mlPixmap,
  value mlPos,
  value mlSize)
{
  CAMLparam3(mlPixmap, mlPos, mlSize);
  int32_t width = Int31_val_clip(Field(mlSize, 0));
  int32_t height = Int31_val_clip(Field(mlSize, 1));
  if (!_ml_canvas_valid_canvas_size(width, height)) {
    caml_invalid_argument("ImageData.sub: invalid dimensions");
  }
  int32_t sx = Int31_val_clip(Field(mlPos, 0));
  int32_t sy = Int31_val_clip(Field(mlPos, 1));
  pixmap_t src_pixmap = Pixmap_val(mlPixmap);
  pixmap_t dst_pixmap = pixmap(width, height, NULL);
  if (pixmap_valid(dst_pixmap) == false) {
    caml_failwith("ImageData.sub: unable to extract sub image data");
  }
  pixmap_blit(&dst_pixmap, 0, 0, &src_pixmap, sx, sy, width, height);
  CAMLreturn(Val_pixmap(&dst_pixmap));
}

CAMLprim value
ml_canvas_image_data_blit(
  value mlDstPixmap,
  value mlDPos,
  value mlSrcPixmap,
  value mlSPos,
  value mlSize)
{
  CAMLparam5(mlDstPixmap, mlDPos, mlSrcPixmap, mlSPos, mlSize);
  int32_t width = Int31_val_clip(Field(mlSize, 0));
  int32_t height = Int31_val_clip(Field(mlSize, 1));
  if (!_ml_canvas_valid_canvas_size(width, height)) {
    caml_invalid_argument("ImageData.blit: invalid dimensions");
  }
  pixmap_t dst_pixmap = Pixmap_val(mlDstPixmap);
  pixmap_t src_pixmap = Pixmap_val(mlSrcPixmap);
  pixmap_blit(&dst_pixmap,
              Int31_val_clip(Field(mlDPos, 0)),
              Int31_val_clip(Field(mlDPos, 1)),
              &src_pixmap,
              Int31_val_clip(Field(mlSPos, 0)),
              Int31_val_clip(Field(mlSPos, 1)),
              width,
              height);
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_image_data_get_pixel(
  value mlPixmap,
  value mlPos)
{
  CAMLparam2(mlPixmap, mlPos);
  pixmap_t pixmap = Pixmap_val(mlPixmap);
  CAMLreturn(caml_copy_int32(color_to_int(
             pixmap_at(pixmap, Int31_val_clip(Field(mlPos, 1)),
                               Int31_val_clip(Field(mlPos, 0))))));
}

CAMLprim value
ml_canvas_image_data_put_pixel(
  value mlPixmap,
  value mlPos,
  value mlColor)
{
  CAMLparam3(mlPixmap, mlPos, mlColor);
  pixmap_t pixmap = Pixmap_val(mlPixmap);
  pixmap_at(pixmap, Int31_val_clip(Field(mlPos, 1)),
                    Int31_val_clip(Field(mlPos, 0))) =
    color_of_int(Int32_val(mlColor));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_image_data_import_png(
  value mlPixmap,
  value mlDPos,
  value mlFilename,
  value mlOnLoad)
{
  CAMLparam4(mlPixmap, mlDPos, mlFilename, mlOnLoad);
  _ml_canvas_ensure_initialized();
  const char * filename = String_val(mlFilename);
  pixmap_t pixmap = Pixmap_val(mlPixmap);
  bool res = impexp_import_png(&pixmap,
                               Int31_val_clip(Field(mlDPos, 0)),
                               Int31_val_clip(Field(mlDPos, 1)),
                               filename);
  if ((res == false) || (pixmap_valid(pixmap) == false)) {
    caml_raise_with_string(*caml_named_value("Read_png_failed"), filename);
  }
  caml_callback(mlOnLoad, mlPixmap);
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_image_data_export_png(
  value mlPixmap,
  value mlFilename)
{
  CAMLparam2(mlPixmap, mlFilename);
  _ml_canvas_ensure_initialized();
  const char *filename = String_val(mlFilename);
  pixmap_t pixmap = Pixmap_val(mlPixmap);
  bool res = impexp_export_png(&pixmap, filename);
  if (res == false) {
    caml_raise_with_string(*caml_named_value("Write_png_failed"), filename);
  }
  CAMLreturn(Val_unit);
}



/* Path */

static void
_ml_canvas_path_destroy_callback(
  path2d_t *path2d)
{
  CAMLparam0();
  value *mlWeakPointer_ptr = (value *)path2d_get_data(path2d);
  if (mlWeakPointer_ptr != NULL) {
    path2d_set_data(path2d, NULL);
    caml_remove_generational_global_root(mlWeakPointer_ptr);
    free(mlWeakPointer_ptr);
  }
  CAMLreturn0;
}

CAMLprim value
ml_canvas_path_create(
  void)
{
  CAMLparam0();
  CAMLlocal1(mlPath);
  path2d_t *path2d = path2d_create();
  if (path2d == NULL) {
    caml_failwith("Path.create: unable to create the specified path2d");
  }
  mlPath = Val_path2d(path2d);
  path2d_release(path2d); /* Because Val_path2d retains it */
  CAMLreturn(mlPath);
}

CAMLprim value
ml_canvas_path_close(
  value mlPath2d)
{
  CAMLparam1(mlPath2d);
  path2d_close(Path2d_val(mlPath2d));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_path_move_to(
  value mlPath2d,
  value mlDest)
{
  CAMLparam2(mlPath2d, mlDest);
  path2d_move_to(Path2d_val(mlPath2d),
                 Double_val(Field(mlDest, 0)),
                 Double_val(Field(mlDest, 1)),
                 NULL);
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_path_line_to(
  value mlPath2d,
  value mlDest)
{
  CAMLparam2(mlPath2d, mlDest);
  path2d_line_to(Path2d_val(mlPath2d),
                 Double_val(Field(mlDest, 0)),
                 Double_val(Field(mlDest, 1)),
                 NULL);
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_path_arc_n(
  value mlPath2d,
  value mlP,
  value mlRadius,
  value mlTheta1,
  value mlTheta2,
  value mlCcw)
{
  CAMLparam5(mlPath2d, mlP, mlRadius, mlTheta1, mlTheta2);
  CAMLxparam1(mlCcw);
  path2d_arc(Path2d_val(mlPath2d),
             Double_val(Field(mlP, 0)),
             Double_val(Field(mlP, 1)),
             Double_val(mlRadius),
             Double_val(mlTheta1),
             Double_val(mlTheta2),
             Bool_val(mlCcw),
             NULL);
  CAMLreturn(Val_unit);
}

BYTECODE_STUB_6(ml_canvas_path_arc)

CAMLprim value
ml_canvas_path_arc_to(
  value mlPath2d,
  value mlP1,
  value mlP2,
  value mlRadius)
{
  CAMLparam4(mlPath2d, mlP1, mlP2, mlRadius);
  path2d_arc_to(Path2d_val(mlPath2d),
                Double_val(Field(mlP1, 0)),
                Double_val(Field(mlP1, 1)),
                Double_val(Field(mlP2, 0)),
                Double_val(Field(mlP2, 1)),
                Double_val(mlRadius),
                NULL);
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_path_bezier_curve_to(
  value mlPath2d,
  value mlCp1,
  value mlCp2,
  value mlP)
{
  CAMLparam4(mlPath2d, mlCp1, mlCp2, mlP);
  path2d_bezier_curve_to(Path2d_val(mlPath2d),
                         Double_val(Field(mlCp1, 0)),
                         Double_val(Field(mlCp1, 1)),
                         Double_val(Field(mlCp2, 0)),
                         Double_val(Field(mlCp2, 1)),
                         Double_val(Field(mlP, 0)),
                         Double_val(Field(mlP, 1)),
                         NULL);
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_path_quadratic_curve_to(
  value mlPath2d,
  value mlCp,
  value mlP)
{
  CAMLparam3(mlPath2d, mlCp, mlP);
  path2d_quadratic_curve_to(Path2d_val(mlPath2d),
                            Double_val(Field(mlCp, 0)),
                            Double_val(Field(mlCp, 1)),
                            Double_val(Field(mlP, 0)),
                            Double_val(Field(mlP, 1)),
                            NULL);
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_path_rect(
  value mlPath2d,
  value mlP,
  value mlSize)
{
  CAMLparam3(mlPath2d, mlP, mlSize);
  path2d_rect(Path2d_val(mlPath2d),
              Double_val(Field(mlP, 0)),
              Double_val(Field(mlP, 1)),
              Double_val(Field(mlSize, 0)),
              Double_val(Field(mlSize, 1)),
              NULL);
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_path_ellipse_n(
  value mlPath2d,
  value mlP,
  value mlRadius,
  value mlRotation,
  value mlTheta1,
  value mlTheta2,
  value mlCcw)
{
  CAMLparam5(mlPath2d, mlP, mlRadius, mlRotation, mlTheta1);
  CAMLxparam2(mlTheta2, mlCcw);
  path2d_ellipse(Path2d_val(mlPath2d),
                 Double_val(Field(mlP, 0)),
                 Double_val(Field(mlP, 1)),
                 Double_val(Field(mlRadius, 0)),
                 Double_val(Field(mlRadius, 1)),
                 Double_val(mlRotation),
                 Double_val(mlTheta1),
                 Double_val(mlTheta2),
                 Bool_val(mlCcw),
                 NULL);
  CAMLreturn(Val_unit);
}

BYTECODE_STUB_7(ml_canvas_path_ellipse)

CAMLprim value
ml_canvas_path_add(
  value mlDstPath2d,
  value mlSrcPath2d)
{
  CAMLparam2(mlDstPath2d, mlSrcPath2d);
  path2d_add(Path2d_val(mlDstPath2d),
             Path2d_val(mlSrcPath2d),
             NULL);
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_path_add_transformed(
  value mlDstPath2d,
  value mlSrcPath2d,
  value mlTransform)
{
  CAMLparam3(mlDstPath2d, mlSrcPath2d, mlTransform);
  transform_t transform = Transform_val(mlTransform);
  path2d_add(Path2d_val(mlDstPath2d),
             Path2d_val(mlSrcPath2d),
             &transform);
  CAMLreturn(Val_unit);
}



/* Gradients */

static void
_ml_canvas_gradient_destroy_callback(
  gradient_t *gradient)
{
  CAMLparam0();
  value *mlWeakPointer_ptr = (value *)gradient_get_data(gradient);
  if (mlWeakPointer_ptr != NULL) {
    gradient_set_data(gradient, NULL);
    caml_remove_generational_global_root(mlWeakPointer_ptr);
    free(mlWeakPointer_ptr);
  }
  CAMLreturn0;
}

CAMLprim value
ml_canvas_gradient_create_linear(
  value mlPos1,
  value mlPos2)
{
  CAMLparam2(mlPos1, mlPos2);
  CAMLlocal1(mlGradient);
  gradient_t *gradient =
    gradient_create_linear(Double_val(Field(mlPos1, 0)),
                           Double_val(Field(mlPos1, 1)),
                           Double_val(Field(mlPos2, 0)),
                           Double_val(Field(mlPos2, 1)));
  if (gradient == NULL) {
    caml_failwith("Gradient.createLinear: unable to create the specified linear gradient");
  }
  mlGradient = Val_gradient(gradient);
  gradient_release(gradient); /* Because Val_gradient retains it */
  CAMLreturn(mlGradient);
}

CAMLprim value
ml_canvas_gradient_create_radial(
  value mlCenter1,
  value mlRadius1,
  value mlCenter2,
  value mlRadius2)
{
  CAMLparam4(mlCenter1, mlRadius1, mlCenter2, mlRadius2);
  CAMLlocal1(mlGradient);
  gradient_t *gradient =
    gradient_create_radial(Double_val(Field(mlCenter1, 0)),
                           Double_val(Field(mlCenter1, 1)),
                           Double_val(mlRadius1),
                           Double_val(Field(mlCenter2, 0)),
                           Double_val(Field(mlCenter2, 1)),
                           Double_val(mlRadius2));
  if (gradient == NULL) {
    caml_failwith("Gradient.createRadial: unable to create the specified radial gradient");
  }
  mlGradient = Val_gradient(gradient);
  gradient_release(gradient); /* Because Val_gradient retains it */
  CAMLreturn(mlGradient);
}

CAMLprim value
ml_canvas_gradient_create_conic(
  value mlCenter,
  value mlAngle)
{
  CAMLparam2(mlCenter, mlAngle);
  CAMLlocal1(mlGradient);
  gradient_t *gradient =
    gradient_create_conic(Double_val(Field(mlCenter, 0)),
                          Double_val(Field(mlCenter, 1)),
                          Double_val(mlAngle));
  if (gradient == NULL) {
    caml_failwith("Gradient.createConic: unable to create the specified conic gradient");
  }
  mlGradient = Val_gradient(gradient);
  gradient_release(gradient); /* Because Val_gradient retains it */
  CAMLreturn(mlGradient);
}

CAMLprim value
ml_canvas_gradient_add_color_stop(
  value mlGradient,
  value mlColor,
  value mlStop)
{
  CAMLparam3(mlGradient, mlColor, mlStop);
  gradient_add_color_stop(Gradient_val(mlGradient),
                          color_of_int(Int32_val(mlColor)),
                          Double_val(mlStop));
  CAMLreturn(Val_unit);
}



/* Patterns */

static void
_ml_canvas_pattern_destroy_callback(
  pattern_t *pattern)
{
  CAMLparam0();
  value *mlWeakPointer_ptr = (value *)pattern_get_data(pattern);
  if (mlWeakPointer_ptr != NULL) {
    pattern_set_data(pattern, NULL);
    caml_remove_generational_global_root(mlWeakPointer_ptr);
    free(mlWeakPointer_ptr);
  }
  CAMLreturn0;
}

CAMLprim value
ml_canvas_pattern_create(
  value mlPixmap,
  value mlRepeat)
{
  CAMLparam2(mlPixmap, mlRepeat);
  CAMLlocal1(mlPattern);
  pixmap_t pixmap = Pixmap_val(mlPixmap);
  pattern_t *pattern = pattern_create(&pixmap, Repeat_val(mlRepeat));
  if (pattern == NULL) {
    caml_failwith("Pattern.create: unable to create the specified pattern");
  }
  mlPattern = Val_pattern(pattern);
  pattern_release(pattern); /* Because Val_pattern retains it */
  CAMLreturn(mlPattern);
}



/* Canvas */

/* Comparison and hashing */

static int32_t
_ml_canvas_get_id_raw(
  value mlCanvas)
{
  canvas_t *canvas = *((canvas_t **)Data_custom_val(mlCanvas));
  if (canvas == NULL) {
    return 0; // should probably be a critical error...
  } else {
    return canvas_get_id(canvas);
  }
}

int
ml_canvas_compare_raw(
  value mlCanvas1,
  value mlCanvas2)
{
  int32_t id1 = _ml_canvas_get_id_raw(mlCanvas1);
  int32_t id2 = _ml_canvas_get_id_raw(mlCanvas2);
  if (id1 < id2) {
    return -1;
  }
  else if (id1 > id2) {
    return 1;
  }
  else {
    return 0;
  }
}

CAMLprim value
ml_canvas_compare(
  value mlCanvas1,
  value mlCanvas2)
{
  CAMLparam2(mlCanvas1, mlCanvas2);
  CAMLreturn(Val_long(ml_canvas_compare_raw(mlCanvas1, mlCanvas2)));
}

CAMLprim value
ml_canvas_hash(
  value mlCanvas)
{
  CAMLparam1(mlCanvas);
  static const value *mlHash = NULL;
  if (mlHash == NULL) {
    mlHash = caml_named_value("Hashtbl.hash");
  }
  CAMLreturn(caml_callback(*mlHash, Val_long(_ml_canvas_get_id_raw(mlCanvas))));
}



/* Creation */

static void
_ml_canvas_canvas_destroy_callback(
  canvas_t *canvas)
{
  CAMLparam0();
  value *mlWeakPointer_ptr = (value *)canvas_get_data(canvas);
  if (mlWeakPointer_ptr != NULL) {
    canvas_set_data(canvas, NULL);
    caml_remove_generational_global_root(mlWeakPointer_ptr);
    free(mlWeakPointer_ptr);
  }
  CAMLreturn0;
}

CAMLprim value
ml_canvas_create_onscreen_n(
  value mlAutocommit, /* bool, optional, default = true */
  value mlDecorated,  /* bool, optional, default = true */
  value mlResizeable, /* bool, optional, default = true */
  value mlMinimize,   /* bool, optional, default = true */
  value mlMaximize,   /* bool, optional, default = true */
  value mlClose,      /* bool, optional, default = true */
  value mlTitle,      /* string, optional, default = "" */
  value mlTarget,     /* string, optional */
  value mlPos,        /* (int * int), optional */
  value mlSize,       /* (int * int), non-optional */
  value mlUnit)
{
  CAMLparam5(mlAutocommit, mlDecorated, mlResizeable, mlMinimize, mlMaximize);
  CAMLxparam5(mlClose, mlTitle, mlTarget, mlPos, mlSize);
  CAMLxparam1(mlUnit);
  CAMLlocal1(mlCanvas);
  _ml_canvas_ensure_initialized();
  int32_t width = Int31_val_clip(Field(mlSize, 0));
  int32_t height = Int31_val_clip(Field(mlSize, 1));
  if (!_ml_canvas_valid_canvas_size(width, height)) {
    caml_invalid_argument("Canvas.createOnscreen: invalid dimensions");
  }
  int32_t x = 0, y = 0;
  if (Is_some(mlPos)) {
    x = Int31_val_clip(Field(Some_val(mlPos), 0));
    y = Int31_val_clip(Field(Some_val(mlPos), 1));
  }
  canvas_t *canvas =
    canvas_create_onscreen(Optional_bool_val(mlAutocommit, true),
                           Optional_bool_val(mlDecorated, true),
                           Optional_bool_val(mlResizeable, true),
                           Optional_bool_val(mlMinimize, true),
                           Optional_bool_val(mlMaximize, true),
                           Optional_bool_val(mlClose, true),
                           Optional_string_val(mlTitle, NULL),
                           x, y, width, height);
  if (canvas == NULL) {
    caml_failwith("Canvas.createOnscreen: unable to create the specified framed canvas");
  }
  mlCanvas = Val_canvas(canvas);
  canvas_release(canvas); /* Because Val_canvas retains it */
  CAMLreturn(mlCanvas);
}

BYTECODE_STUB_11(ml_canvas_create_onscreen)

CAMLprim value
ml_canvas_create_offscreen(
  value mlSize)
{
  CAMLparam1(mlSize);
  CAMLlocal1(mlCanvas);
  _ml_canvas_ensure_initialized();
  int32_t width = Int31_val_clip(Field(mlSize, 0));
  int32_t height = Int31_val_clip(Field(mlSize, 1));
  if (!_ml_canvas_valid_canvas_size(width, height)) {
    caml_invalid_argument("Canvas.createOffscreen: invalid dimensions");
  }
  canvas_t *canvas = canvas_create_offscreen(width, height);
  if (canvas == NULL) {
    caml_failwith("Canvas.createOffscreen: unable to create the specified offscreen canvas");
  }
  mlCanvas = Val_canvas(canvas);
  canvas_release(canvas); /* Because Val_canvas retains it */
  CAMLreturn(mlCanvas);
}

CAMLprim value
ml_canvas_create_offscreen_from_image_data(
  value mlPixmap)
{
  CAMLparam1(mlPixmap);
  CAMLlocal1(mlCanvas);
  _ml_canvas_ensure_initialized();
  pixmap_t pixmap = Pixmap_val(mlPixmap);
  /* We need to duplicate the pixmap, as canvas_create_offscreen_from_pixmap
     steals the data pointer */
  pixmap = pixmap_copy(pixmap);
  if (pixmap_valid(pixmap) == false) {
    caml_failwith("Canvas.createOffscreen: unable to create a canvas from the given image data");
  }
  canvas_t *canvas = canvas_create_offscreen_from_pixmap(&pixmap);
  if (canvas == NULL) {
    caml_failwith("Canvas.createOffscreen: unable to create a canvas from the given image data");
  }
  /* We delete the pixmap copy we did earlier; it is safe to do so,
     as pixmap deletion checks if the data pointer is NULL (which
     is the case if canvas creation succeeds) */
  pixmap_destroy(pixmap);
  mlCanvas = Val_canvas(canvas);
  canvas_release(canvas); /* Because Val_canvas retains it */
  CAMLreturn(mlCanvas);
}

CAMLprim value
ml_canvas_create_offscreen_from_png(
  value mlFilename,
  value mlOnLoad)
{
  CAMLparam2(mlFilename, mlOnLoad);
  CAMLlocal1(mlCanvas);
  _ml_canvas_ensure_initialized();
  const char *filename = String_val(mlFilename);
  canvas_t *canvas = canvas_create_offscreen_from_png(filename);
  if (canvas == NULL) {
    caml_raise_with_string(*caml_named_value("Read_png_failed"), filename);
  }
  mlCanvas = Val_canvas(canvas);
  canvas_release(canvas); /* Because Val_canvas retains it */
  caml_callback(mlOnLoad, mlCanvas);
  CAMLreturn(Val_unit);
}



/* Visibility */

CAMLprim value
ml_canvas_show(
  value mlCanvas)
{
  CAMLparam1(mlCanvas);
  canvas_show(Canvas_val(mlCanvas));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_hide(
  value mlCanvas)
{
  CAMLparam1(mlCanvas);
  canvas_hide(Canvas_val(mlCanvas));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_close(
  value mlCanvas)
{
  CAMLparam1(mlCanvas);
  canvas_close(Canvas_val(mlCanvas));
  CAMLreturn(Val_unit);
}



/* Rendering */
CAMLprim value
ml_canvas_commit(
  value mlCanvas)
{
  CAMLparam1(mlCanvas);
  canvas_commit(Canvas_val(mlCanvas));
  CAMLreturn(Val_unit);
}



/* Configuration */

CAMLprim value
ml_canvas_get_id(
  value mlCanvas)
{
  CAMLparam1(mlCanvas);
  CAMLreturn(Val_long(_ml_canvas_get_id_raw(mlCanvas)));
}

CAMLprim value
ml_canvas_get_size(
  value mlCanvas)
{
  CAMLparam1(mlCanvas);
  CAMLlocal1(mlResult);
  pair_t(int32_t) result = canvas_get_size(Canvas_val(mlCanvas));
  mlResult = caml_alloc_tuple(2);
  Store_field(mlResult, 0, Val_int31_clip(fst(result)));
  Store_field(mlResult, 1, Val_int31_clip(snd(result)));
  CAMLreturn(mlResult);
}

CAMLprim value
ml_canvas_set_size(
  value mlCanvas,
  value mlSize)
{
  CAMLparam2(mlCanvas, mlSize);
  int32_t width = Int31_val_clip(Field(mlSize, 0));
  int32_t height = Int31_val_clip(Field(mlSize, 1));
  if (!_ml_canvas_valid_canvas_size(width, height)) {
    caml_invalid_argument("Canvas.setSize: invalid dimensions");
  }
  canvas_set_size(Canvas_val(mlCanvas),
                  width, height);
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_get_position(
  value mlCanvas)
{
  CAMLparam1(mlCanvas);
  CAMLlocal1(mlResult);
  pair_t(int32_t) result = canvas_get_position(Canvas_val(mlCanvas));
  mlResult = caml_alloc_tuple(2);
  Store_field(mlResult, 0, Val_int31_clip(fst(result)));
  Store_field(mlResult, 1, Val_int31_clip(snd(result)));
  CAMLreturn(mlResult);
}

CAMLprim value
ml_canvas_set_position(
  value mlCanvas,
  value mlPos)
{
  CAMLparam2(mlCanvas, mlPos);
  canvas_set_position(Canvas_val(mlCanvas),
                      Int31_val_clip(Field(mlPos, 0)),
                      Int31_val_clip(Field(mlPos, 1)));
  CAMLreturn(Val_unit);
}



/* Transform */

CAMLprim value
ml_canvas_set_transform(
  value mlCanvas,
  value mlTransform)
{
  CAMLparam2(mlCanvas, mlTransform);
  transform_t transform = Transform_val(mlTransform);
  canvas_set_transform(Canvas_val(mlCanvas),
                       &transform);
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_transform(
  value mlCanvas,
  value mlTransform)
{
  CAMLparam2(mlCanvas, mlTransform);
  transform_t transform = Transform_val(mlTransform);
  canvas_transform(Canvas_val(mlCanvas),
                   &transform);
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_translate(
  value mlCanvas,
  value mlVec)
{
  CAMLparam2(mlCanvas, mlVec);
  canvas_translate(Canvas_val(mlCanvas),
                   Double_val(Field(mlVec, 0)),
                   Double_val(Field(mlVec, 1)));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_scale(
  value mlCanvas,
  value mlVec)
{
  CAMLparam2(mlCanvas, mlVec);
  canvas_scale(Canvas_val(mlCanvas),
               Double_val(Field(mlVec, 0)),
               Double_val(Field(mlVec, 1)));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_shear(
  value mlCanvas,
  value mlVec)
{
  CAMLparam2(mlCanvas, mlVec);
  canvas_shear(Canvas_val(mlCanvas),
               Double_val(Field(mlVec, 0)),
               Double_val(Field(mlVec, 1)));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_rotate(
  value mlCanvas,
  value mlTheta)
{
  CAMLparam2(mlCanvas, mlTheta);
  canvas_rotate(Canvas_val(mlCanvas),
                Double_val(mlTheta));
  CAMLreturn(Val_unit);
}



/* State */

CAMLprim value
ml_canvas_save(
  value mlCanvas)
{
  CAMLparam1(mlCanvas);
  canvas_save(Canvas_val(mlCanvas));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_restore(
  value mlCanvas)
{
  CAMLparam1(mlCanvas);
  canvas_restore(Canvas_val(mlCanvas));
  CAMLreturn(Val_unit);
}



/* Style / config */

CAMLprim value
ml_canvas_get_line_width(
  value mlCanvas)
{
  CAMLparam1(mlCanvas);
  CAMLreturn(caml_copy_double(canvas_get_line_width(Canvas_val(mlCanvas))));
}

CAMLprim value
ml_canvas_set_line_width(
  value mlCanvas,
  value mlLineWidth)
{
  CAMLparam2(mlCanvas, mlLineWidth);
  canvas_set_line_width(Canvas_val(mlCanvas),
                        Double_val(mlLineWidth));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_get_line_join(
  value mlCanvas)
{
  CAMLparam1(mlCanvas);
  CAMLreturn(Val_join_type(canvas_get_join_type(Canvas_val(mlCanvas))));
}

CAMLprim value
ml_canvas_set_line_join(
  value mlCanvas,
  value mlLineJoin)
{
  CAMLparam2(mlCanvas, mlLineJoin);
  canvas_set_join_type(Canvas_val(mlCanvas),
                       Join_type_val(mlLineJoin));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_get_line_cap(
  value mlCanvas)
{
  CAMLparam1(mlCanvas);
  CAMLreturn(Val_cap_type(canvas_get_cap_type(Canvas_val(mlCanvas))));
}

CAMLprim value
ml_canvas_set_line_cap(
  value mlCanvas,
  value mlLineCap)
{
  CAMLparam2(mlCanvas, mlLineCap);
  canvas_set_cap_type(Canvas_val(mlCanvas),
                      Cap_type_val(mlLineCap));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_get_miter_limit(
  value mlCanvas)
{
  CAMLparam1(mlCanvas);
  CAMLreturn(caml_copy_double(canvas_get_miter_limit(Canvas_val(mlCanvas))));
}

CAMLprim value
ml_canvas_set_miter_limit(
  value mlCanvas,
  value mlMiterLimit)
{
  CAMLparam2(mlCanvas, mlMiterLimit);
  canvas_set_miter_limit(Canvas_val(mlCanvas),
                         Double_val(mlMiterLimit));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_get_line_dash_offset(
  value mlCanvas)
{
  CAMLparam1(mlCanvas);
  CAMLreturn(caml_copy_double(
    canvas_get_line_dash_offset(Canvas_val(mlCanvas))));
}

CAMLprim value
ml_canvas_set_line_dash_offset(
  value mlCanvas,
  value mlLineOffset)
{
  CAMLparam2(mlCanvas, mlLineOffset);
  canvas_set_line_dash_offset(Canvas_val(mlCanvas),
                              Double_val(mlLineOffset));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_get_line_dash(
  value mlCanvas)
{
  CAMLparam1(mlCanvas);
  canvas_t *canvas = Canvas_val(mlCanvas);
  CAMLreturn(Val_double_array(canvas_get_line_dash(canvas),
                              canvas_get_line_dash_length(canvas)));
}

CAMLprim value
ml_canvas_set_line_dash(
  value mlCanvas,
  value mlDashPattern)
{
  CAMLparam1(mlCanvas);
  double *dash_pattern = Double_array_val(mlDashPattern);
  canvas_set_line_dash(Canvas_val(mlCanvas),
                       dash_pattern,
                       Wosize_val(mlDashPattern));
  if (dash_pattern != NULL) {
    free(dash_pattern);
  }
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_get_stroke_color(
  value mlCanvas)
{
  CAMLparam1(mlCanvas);
  CAMLreturn(caml_copy_int32(
    color_to_int(canvas_get_stroke_color(Canvas_val(mlCanvas)))));
}

CAMLprim value
ml_canvas_set_stroke_color(
  value mlCanvas,
  value mlColor)
{
  CAMLparam2(mlCanvas, mlColor);
  canvas_set_stroke_color(Canvas_val(mlCanvas),
                          color_of_int(Int32_val(mlColor)));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_set_stroke_gradient(
  value mlCanvas,
  value mlGradient)
{
  CAMLparam2(mlCanvas, mlGradient);
  canvas_set_stroke_gradient(Canvas_val(mlCanvas),
                             Gradient_val(mlGradient));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_set_stroke_pattern(
  value mlCanvas,
  value mlPattern)
{
  CAMLparam2(mlCanvas, mlPattern);
  canvas_set_stroke_pattern(Canvas_val(mlCanvas),
                            Pattern_val(mlPattern));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_get_stroke_style(
  value mlCanvas)
{
  CAMLparam1(mlCanvas);
  CAMLreturn(Val_style(canvas_get_stroke_style(Canvas_val(mlCanvas))));
}

CAMLprim value
ml_canvas_set_stroke_style(
  value mlCanvas,
  value mlStyle)
{
  CAMLparam2(mlCanvas, mlStyle);
  switch (Tag_val(mlStyle)) {
    case TAG_COLOR:
      canvas_set_stroke_color(Canvas_val(mlCanvas),
                              color_of_int(Int32_val(Field(mlStyle, 0))));
      break;
    case TAG_GRADIENT:
      canvas_set_stroke_gradient(Canvas_val(mlCanvas),
                                 Gradient_val(Field(mlStyle, 0)));
      break;
    case TAG_PATTERN:
      canvas_set_stroke_pattern(Canvas_val(mlCanvas),
                                Pattern_val(Field(mlStyle, 0)));
    default:
      assert(!"Unknown draw style");
      break;
  }
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_get_fill_color(
  value mlCanvas)
{
  CAMLparam1(mlCanvas);
  CAMLreturn(caml_copy_int32(
    color_to_int(canvas_get_fill_color(Canvas_val(mlCanvas)))));
}

CAMLprim value
ml_canvas_set_fill_color(
  value mlCanvas,
  value mlColor)
{
  CAMLparam2(mlCanvas, mlColor);
  canvas_set_fill_color(Canvas_val(mlCanvas),
                        color_of_int(Int32_val(mlColor)));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_set_fill_gradient(
  value mlCanvas,
  value mlGradient)
{
  CAMLparam2(mlCanvas, mlGradient);
  canvas_set_fill_gradient(Canvas_val(mlCanvas),
                           Gradient_val(mlGradient));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_set_fill_pattern(
  value mlCanvas,
  value mlPattern)
{
  CAMLparam2(mlCanvas, mlPattern);
  canvas_set_fill_pattern(Canvas_val(mlCanvas),
                          Pattern_val(mlPattern));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_get_fill_style(
  value mlCanvas)
{
  CAMLparam1(mlCanvas);
  CAMLreturn(Val_style(canvas_get_fill_style(Canvas_val(mlCanvas))));
}

CAMLprim value
ml_canvas_set_fill_style(
  value mlCanvas,
  value mlStyle)
{
  CAMLparam2(mlCanvas, mlStyle);
  switch (Tag_val(mlStyle)) {
    case TAG_COLOR:
      canvas_set_fill_color(Canvas_val(mlCanvas),
                            color_of_int(Int32_val(Field(mlStyle, 0))));
      break;
    case TAG_GRADIENT:
      canvas_set_fill_gradient(Canvas_val(mlCanvas),
                               Gradient_val(Field(mlStyle, 0)));
      break;
    case TAG_PATTERN:
      canvas_set_fill_pattern(Canvas_val(mlCanvas),
                              Pattern_val(Field(mlStyle, 0)));
    default:
      assert(!"Unknown style");
      break;
  }
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_get_global_alpha(
  value mlCanvas)
{
  CAMLparam1(mlCanvas);
  CAMLreturn(caml_copy_double(canvas_get_global_alpha(Canvas_val(mlCanvas))));
}

CAMLprim value
ml_canvas_set_global_alpha(
  value mlCanvas,
  value mlGlobalAlpha)
{
  CAMLparam2(mlCanvas, mlGlobalAlpha);
  canvas_set_global_alpha(Canvas_val(mlCanvas),
                          Double_val(mlGlobalAlpha));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_get_global_composite_operation(
  value mlCanvas)
{
  CAMLparam1(mlCanvas);
  CAMLreturn(Val_compop(canvas_get_comp_operation(Canvas_val(mlCanvas))));
}

CAMLprim value
ml_canvas_set_global_composite_operation(
  value mlCanvas,
  value mlCompositeOperation)
{
  CAMLparam2(mlCanvas, mlCompositeOperation);
  canvas_set_comp_operation(Canvas_val(mlCanvas),
                            Compop_val(mlCompositeOperation));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_get_shadow_color(
  value mlCanvas)
{
  CAMLparam1(mlCanvas);
  CAMLreturn(caml_copy_int32(
             color_to_int(canvas_get_shadow_color(Canvas_val(mlCanvas)))));
}

CAMLprim value
ml_canvas_set_shadow_color(
  value mlCanvas,
  value mlColor)
{
  CAMLparam2(mlCanvas, mlColor);
  canvas_set_shadow_color(Canvas_val(mlCanvas),
                          color_of_int(Int32_val(mlColor)));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_get_shadow_blur(
  value mlCanvas)
{
  CAMLparam1(mlCanvas);
  CAMLreturn(caml_copy_double(canvas_get_shadow_blur(Canvas_val(mlCanvas))));
}

CAMLprim value
ml_canvas_set_shadow_blur(
  value mlCanvas,
  value mlShadowBlur)
{
  CAMLparam2(mlCanvas, mlShadowBlur);
  canvas_set_shadow_blur(Canvas_val(mlCanvas),
                         Double_val(mlShadowBlur));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_get_shadow_offset(
  value mlCanvas)
{
  CAMLparam1(mlCanvas);
  CAMLlocal1(mlResult);
  pair_t(double) result = canvas_get_shadow_offset(Canvas_val(mlCanvas));
  mlResult = caml_alloc_tuple(2);
  Store_field(mlResult, 0, caml_copy_double(fst(result)));
  Store_field(mlResult, 1, caml_copy_double(snd(result)));
  CAMLreturn(mlResult);
}

CAMLprim value
ml_canvas_set_shadow_offset(
  value mlCanvas,
  value mlShadowOffset)
{
  CAMLparam2(mlCanvas, mlShadowOffset);
  canvas_set_shadow_offset(Canvas_val(mlCanvas),
                           Double_val(Field(mlShadowOffset, 0)),
                           Double_val(Field(mlShadowOffset, 1)));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_set_font(
  value mlCanvas,
  value mlFamily,
  value mlSize,
  value mlSlant,
  value mlWeight)
{
  CAMLparam5(mlCanvas, mlFamily, mlSize, mlSlant, mlWeight);
  canvas_set_font(Canvas_val(mlCanvas),
                  String_val(mlFamily),
                  Double_val(mlSize),
                  Slant_val(mlSlant),
                  Int31_val_clip(mlWeight));
  CAMLreturn(Val_unit);
}



/* Paths */

CAMLprim value
ml_canvas_clear_path(
  value mlCanvas)
{
  CAMLparam1(mlCanvas);
  canvas_clear_path(Canvas_val(mlCanvas));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_close_path(
  value mlCanvas)
{
  CAMLparam1(mlCanvas);
  canvas_close_path(Canvas_val(mlCanvas));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_move_to(
  value mlCanvas,
  value mlP)
{
  CAMLparam2(mlCanvas, mlP);
  canvas_move_to(Canvas_val(mlCanvas),
                 Double_val(Field(mlP, 0)),
                 Double_val(Field(mlP, 1)));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_line_to(
  value mlCanvas,
  value mlP)
{
  CAMLparam2(mlCanvas, mlP);
  canvas_line_to(Canvas_val(mlCanvas),
                 Double_val(Field(mlP, 0)),
                 Double_val(Field(mlP, 1)));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_arc_n(
  value mlCanvas,
  value mlP,
  value mlRadius,
  value mlTheta1,
  value mlTheta2,
  value mlCcw)
{
  CAMLparam5(mlCanvas, mlP, mlRadius, mlTheta1, mlTheta2);
  CAMLxparam1(mlCcw);
  canvas_arc(Canvas_val(mlCanvas),
             Double_val(Field(mlP, 0)),
             Double_val(Field(mlP, 1)),
             Double_val(mlRadius),
             Double_val(mlTheta1),
             Double_val(mlTheta2),
             Bool_val(mlCcw));
  CAMLreturn(Val_unit);
}

BYTECODE_STUB_6(ml_canvas_arc)

CAMLprim value
ml_canvas_arc_to(
  value mlCanvas,
  value mlP1,
  value mlP2,
  value mlRadius)
{
  CAMLparam4(mlCanvas, mlP1, mlP2, mlRadius);
  canvas_arc_to(Canvas_val(mlCanvas),
                Double_val(Field(mlP1, 0)),
                Double_val(Field(mlP1, 1)),
                Double_val(Field(mlP2, 0)),
                Double_val(Field(mlP2, 1)),
                Double_val(mlRadius));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_quadratic_curve_to(
  value mlCanvas,
  value mlCP,
  value mlP)
{
  CAMLparam3(mlCanvas, mlCP, mlP);
  canvas_quadratic_curve_to(Canvas_val(mlCanvas),
                            Double_val(Field(mlCP, 0)),
                            Double_val(Field(mlCP, 1)),
                            Double_val(Field(mlP, 0)),
                            Double_val(Field(mlP, 1)));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_bezier_curve_to(
  value mlCanvas,
  value mlCP1,
  value mlCP2,
  value mlP)
{
  CAMLparam4(mlCanvas, mlCP1, mlCP2, mlP);
  canvas_bezier_curve_to(Canvas_val(mlCanvas),
                         Double_val(Field(mlCP1, 0)),
                         Double_val(Field(mlCP1, 1)),
                         Double_val(Field(mlCP2, 0)),
                         Double_val(Field(mlCP2, 1)),
                         Double_val(Field(mlP, 0)),
                         Double_val(Field(mlP, 1)));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_rect(
  value mlCanvas,
  value mlP,
  value mlSize)
{
  CAMLparam3(mlCanvas, mlP, mlSize);
  canvas_rect(Canvas_val(mlCanvas),
              Double_val(Field(mlP, 0)),
              Double_val(Field(mlP, 1)),
              Double_val(Field(mlSize, 0)),
              Double_val(Field(mlSize, 1)));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_ellipse_n(
  value mlCanvas,
  value mlP,
  value mlRadius,
  value mlRotation,
  value mlTheta1,
  value mlTheta2,
  value mlCcw)
{
  CAMLparam5(mlCanvas, mlP, mlRadius, mlRotation, mlTheta1);
  CAMLxparam2(mlTheta2, mlCcw);
  canvas_ellipse(Canvas_val(mlCanvas),
                 Double_val(Field(mlP, 0)),
                 Double_val(Field(mlP, 1)),
                 Double_val(Field(mlRadius, 0)),
                 Double_val(Field(mlRadius, 1)),
                 Double_val(mlRotation),
                 Double_val(mlTheta1),
                 Double_val(mlTheta2),
                 Bool_val(mlCcw));
  CAMLreturn(Val_unit);
}

BYTECODE_STUB_7(ml_canvas_ellipse)



/* Path stroking/filling */

CAMLprim value
ml_canvas_fill(
  value mlCanvas,
  value mlNonZero)
{
  CAMLparam2(mlCanvas, mlNonZero);
  canvas_fill(Canvas_val(mlCanvas),
              Bool_val(mlNonZero));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_fill_path(
  value mlCanvas,
  value mlPath2d,
  value mlNonZero)
{
  CAMLparam3(mlCanvas, mlPath2d, mlNonZero);
  canvas_fill_path(Canvas_val(mlCanvas),
                   Path2d_val(mlPath2d),
                   Bool_val(mlNonZero));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_stroke(
  value mlCanvas)
{
  CAMLparam1(mlCanvas);
  canvas_stroke(Canvas_val(mlCanvas));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_stroke_path(
  value mlCanvas,
  value mlPath2d)
{
  CAMLparam2(mlCanvas, mlPath2d);
  canvas_stroke_path(Canvas_val(mlCanvas),
                     Path2d_val(mlPath2d));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_clip(
  value mlCanvas,
  value mlNonZero)
{
  CAMLparam2(mlCanvas, mlNonZero);
  canvas_clip(Canvas_val(mlCanvas),
              Bool_val(mlNonZero));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_clip_path(
  value mlCanvas,
  value mlPath2d,
  value mlNonZero)
{
  CAMLparam3(mlCanvas, mlPath2d, mlNonZero);
  canvas_clip_path(Canvas_val(mlCanvas),
                   Path2d_val(mlPath2d),
                   Bool_val(mlNonZero));
  CAMLreturn(Val_unit);
}


/* Immediate drawing */

CAMLprim value
ml_canvas_fill_rect(
  value mlCanvas,
  value mlP,
  value mlSize)
{
  CAMLparam3(mlCanvas, mlP, mlSize);
  canvas_fill_rect(Canvas_val(mlCanvas),
                   Double_val(Field(mlP, 0)),
                   Double_val(Field(mlP, 1)),
                   Double_val(Field(mlSize, 0)),
                   Double_val(Field(mlSize, 1)));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_stroke_rect(
  value mlCanvas,
  value mlP,
  value mlSize)
{
  CAMLparam3(mlCanvas, mlP, mlSize);
  canvas_stroke_rect(Canvas_val(mlCanvas),
                     Double_val(Field(mlP, 0)),
                     Double_val(Field(mlP, 1)),
                     Double_val(Field(mlSize, 0)),
                     Double_val(Field(mlSize, 1)));
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_fill_text(
  value mlCanvas,
  value mlText,
  value mlP)
{
  CAMLparam3(mlCanvas, mlText, mlP);
  canvas_fill_text(Canvas_val(mlCanvas),
                   String_val(mlText),
                   Double_val(Field(mlP, 0)),
                   Double_val(Field(mlP, 1)),
                   0.0);
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_stroke_text(
  value mlCanvas,
  value mlText,
  value mlP)
{
  CAMLparam3(mlCanvas, mlText, mlP);
  canvas_stroke_text(Canvas_val(mlCanvas),
                     String_val(mlText),
                     Double_val(Field(mlP, 0)),
                     Double_val(Field(mlP, 1)),
                     0.0);
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_blit(
  value mlDstCanvas,
  value mlDPos,
  value mlSrcCanvas,
  value mlSPos,
  value mlSize)
{
  CAMLparam5(mlDstCanvas, mlDPos, mlSrcCanvas, mlSPos, mlSize);
  int32_t width = Int31_val_clip(Field(mlSize, 0));
  int32_t height = Int31_val_clip(Field(mlSize, 1));
  if (!_ml_canvas_valid_canvas_size(width, height)) {
    caml_invalid_argument("Canvas.blit: invalid dimensions");
  }
  canvas_blit(Canvas_val(mlDstCanvas),
              Int31_val_clip(Field(mlDPos, 0)),
              Int31_val_clip(Field(mlDPos, 1)),
              Canvas_val(mlSrcCanvas),
              Int31_val_clip(Field(mlSPos, 0)),
              Int31_val_clip(Field(mlSPos, 1)),
              width,
              height);
  CAMLreturn(Val_unit);
}



/* Direct pixel access */

CAMLprim value
ml_canvas_get_pixel(
  value mlCanvas,
  value mlPos)
{
  CAMLparam2(mlCanvas, mlPos);
  CAMLreturn(caml_copy_int32(color_to_int(
             canvas_get_pixel(Canvas_val(mlCanvas),
             Int31_val_clip(Field(mlPos, 0)),
             Int31_val_clip(Field(mlPos, 1))))));
}

CAMLprim value
ml_canvas_put_pixel(
  value mlCanvas,
  value mlPos,
  value mlColor)
{
  CAMLparam3(mlCanvas, mlPos, mlColor);
  canvas_put_pixel(Canvas_val(mlCanvas),
                   Int31_val_clip(Field(mlPos, 0)),
                   Int31_val_clip(Field(mlPos, 1)),
                   color_of_int(Int32_val(mlColor)));
  CAMLreturn(Val_unit);
}



CAMLprim value
ml_canvas_get_image_data(
  value mlCanvas,
  value mlPos,
  value mlSize)
{
  CAMLparam3(mlCanvas, mlPos, mlSize);
  int32_t width = Int31_val_clip(Field(mlSize, 0));
  int32_t height = Int31_val_clip(Field(mlSize, 1));
  if (!_ml_canvas_valid_canvas_size(width, height)) {
    caml_invalid_argument("Canvas.getImageData: invalid dimensions");
  }
  pixmap_t pixmap =
    canvas_get_pixmap(Canvas_val(mlCanvas),
                      Int31_val_clip(Field(mlPos, 0)),
                      Int31_val_clip(Field(mlPos, 1)),
                      width,
                      height);
  if (pixmap_valid(pixmap) == false) {
    caml_failwith("Canvas.getImageData: unable to retrieve image data");
  }
  CAMLreturn(Val_pixmap(&pixmap));
}

CAMLprim value
ml_canvas_put_image_data(
  value mlCanvas,
  value mlDPos,
  value mlPixmap,
  value mlSPos,
  value mlSize)
{
  CAMLparam5(mlCanvas, mlDPos, mlPixmap, mlSPos, mlSize);
  int32_t width = Int31_val_clip(Field(mlSize, 0));
  int32_t height = Int31_val_clip(Field(mlSize, 1));
  if (!_ml_canvas_valid_canvas_size(width, height)) {
    caml_invalid_argument("Canvas.putImageData: invalid dimensions");
  }
  pixmap_t pixmap = Pixmap_val(mlPixmap);
  canvas_put_pixmap(Canvas_val(mlCanvas),
                    Int31_val_clip(Field(mlDPos, 0)),
                    Int31_val_clip(Field(mlDPos, 1)),
                    &pixmap,
                    Int31_val_clip(Field(mlSPos, 0)),
                    Int31_val_clip(Field(mlSPos, 1)),
                    width,
                    height);
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_import_png(
  value mlCanvas,
  value mlDPos,
  value mlFilename,
  value mlOnLoad)
{
  CAMLparam4(mlCanvas, mlDPos, mlFilename, mlOnLoad);
  const char *filename = String_val(mlFilename);
  bool res = canvas_import_png(Canvas_val(mlCanvas),
                               Int31_val_clip(Field(mlDPos, 0)),
                               Int31_val_clip(Field(mlDPos, 1)),
                               filename);
  if (res == false) {
    caml_raise_with_string(*caml_named_value("Read_png_failed"), filename);
  }
  caml_callback(mlOnLoad, mlCanvas);
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_export_png(
  value mlCanvas,
  value mlFilename)
{
  CAMLparam2(mlCanvas, mlFilename);
  const char *filename = String_val(mlFilename);
  bool res = canvas_export_png(Canvas_val(mlCanvas), filename);
  if (res == false) {
    caml_raise_with_string(*caml_named_value("Write_png_failed"), filename);
  }
  CAMLreturn(Val_unit);
}



/* Event */

CAMLprim value
ml_canvas_int_of_key(
  value mlKeyCode)
{
  CAMLparam1(mlKeyCode);
  CAMLreturn(Val_long((intnat)Key_code_val(mlKeyCode)));
}

CAMLprim value
ml_canvas_key_of_int(
  value mlInt)
{
  CAMLparam1(mlInt);
  intnat i = Long_val(mlInt);
  if ((i < 0) || (i > 255)) {
    caml_invalid_argument("Event.key_of_int: i must be in the 0-255 range");
  };
  CAMLreturn(Val_key_code((key_code_t)i));
}



/* Backend */

CAMLprim value
ml_canvas_init()
{
  CAMLparam0();

  if (_ml_canvas_initialized == true) {
    CAMLreturn(Val_unit);
  }

  switch (get_os_type()) {
    case OS_WIN32: _ml_canvas_initialized = backend_init(IMPL_GDI); break;
    case OS_OSX: _ml_canvas_initialized = backend_init(IMPL_QUARTZ); break;
    case OS_UNIX:
      _ml_canvas_initialized = backend_init(IMPL_WAYLAND);
      if (_ml_canvas_initialized == false) {
        _ml_canvas_initialized = backend_init(IMPL_X11);
      }
      break;
    default: assert(!"Invalid OS type"); break;
  }

  if (_ml_canvas_initialized == false) {
    caml_failwith("Backend.init: backend initialization failed");
  }

  canvas_set_destroy_callback(_ml_canvas_canvas_destroy_callback);
  gradient_set_destroy_callback(_ml_canvas_gradient_destroy_callback);
  pattern_set_destroy_callback(_ml_canvas_pattern_destroy_callback);
  path2d_set_destroy_callback(_ml_canvas_path_destroy_callback);

  CAMLreturn(Val_unit);
}

static value _ml_canvas_mlException = Val_unit;
static value _ml_canvas_mlProcessEvent = Val_unit;

static bool
_ml_canvas_process_event(
  event_t *event,
  event_listener_t *next_listener)
{
  CAMLparam0();
  CAMLlocal1(mlResult);

  if (_ml_canvas_mlProcessEvent == Val_unit) {
    CAMLreturnT(bool, false);
  }

  mlResult = caml_callback_exn(_ml_canvas_mlProcessEvent, Val_event(event));

  /* If an exception was raised, save for later */
  if (Is_exception_result(mlResult)) {
    mlResult = Extract_exception(mlResult);
    /* Only remember the first exception (shouldn't have more anyways) */
    if (_ml_canvas_mlException == Val_unit) {
      caml_modify_generational_global_root(&_ml_canvas_mlException, mlResult);
    }
    backend_stop();
    CAMLreturnT(bool, false);
  }

  CAMLreturnT(bool, true);
}

CAMLprim value
ml_canvas_run(
  value mlProcessEvent,
  value mlContinuation)
{
  CAMLparam2(mlProcessEvent, mlContinuation);
  CAMLlocal1(mlResult);

  _ml_canvas_ensure_initialized();

  /* If already running, ignore */
  if (_ml_canvas_mlProcessEvent != Val_unit) {
    CAMLreturn(Val_unit);
  }

  _ml_canvas_mlProcessEvent = mlProcessEvent;
  caml_register_generational_global_root(&_ml_canvas_mlProcessEvent);

  _ml_canvas_mlException = Val_unit;
  caml_register_generational_global_root(&_ml_canvas_mlException);

  event_listener_t event_listener = {
    .process_event = _ml_canvas_process_event,
    .next_listener = NULL
  };
  backend_run(&event_listener);

  mlResult = _ml_canvas_mlException;
  caml_remove_generational_global_root(&_ml_canvas_mlException);
  _ml_canvas_mlException = Val_unit;

  caml_remove_generational_global_root(&_ml_canvas_mlProcessEvent);
  _ml_canvas_mlProcessEvent = Val_unit;

  /* If an exception was raised, re-raise */
  if (mlResult != Val_unit) {
    caml_raise_constant(mlResult);
  }

  mlResult = caml_callback_exn(mlContinuation, Val_unit);
  if (Is_exception_result(mlResult)) {
    mlResult = Extract_exception(mlResult);
    caml_raise_constant(mlResult);
  }

  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_stop(
  void)
{
  CAMLparam0();
  _ml_canvas_ensure_initialized();
  backend_stop();
  CAMLreturn(Val_unit);
}

CAMLprim value
ml_canvas_get_canvas(
  value mlId)
{
  CAMLparam1(mlId);
  _ml_canvas_ensure_initialized();
  canvas_t *result = backend_get_canvas(Long_val(mlId));
  if (result == NULL) {
    caml_raise_not_found();
  }
  CAMLreturn(Val_canvas(result));
}

CAMLprim value
ml_canvas_get_current_timestamp(
  void)
{
  CAMLparam0();
  _ml_canvas_ensure_initialized();
  CAMLreturn(caml_copy_int64(backend_get_time()));
}
