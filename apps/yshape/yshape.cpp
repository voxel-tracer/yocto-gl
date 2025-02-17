//
// LICENSE:
//
// Copyright (c) 2016 -- 2021 Fabio Pellacini
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//

#include <yocto/yocto_cli.h>
#include <yocto/yocto_geometry.h>
#include <yocto/yocto_image.h>
#include <yocto/yocto_math.h>
#include <yocto/yocto_scene.h>
#include <yocto/yocto_sceneio.h>
#include <yocto/yocto_shape.h>
#if YOCTO_OPENGL == 1
#include <yocto_gui/yocto_glview.h>
#endif
using namespace yocto;

// convert params
struct convert_params {
  string shape        = "shape.ply";
  string output       = "out.ply";
  bool   info         = false;
  bool   smooth       = false;
  bool   facet        = false;
  bool   aspositions  = false;
  bool   astriangles  = false;
  vec3f  translate    = {0, 0, 0};
  vec3f  rotate       = {0, 0, 0};
  vec3f  scale        = {1, 1, 1};
  int    subdivisions = 0;
  bool   catmullclark = false;
  bool   toedges      = false;
  bool   tovertices   = false;
};

void add_options(const cli_command& cli, convert_params& params) {
  add_argument(cli, "shape", params.shape, "Input shape.");
  add_option(cli, "output", params.output, "Output shape.");
  add_option(cli, "smooth", params.smooth, "Smooth normals.");
  add_option(cli, "facet", params.facet, "Facet normals.");
  add_option(
      cli, "aspositions", params.aspositions, "Remove all but positions.");
  add_option(cli, "astriangles", params.astriangles, "Convert to triangles.");
  add_option(cli, "translate", params.translate, "Translate shape.");
  add_option(cli, "scale", params.scale, "Scale shape.");
  add_option(cli, "rotate", params.rotate, "Rotate shape.");
  add_option(cli, "subdivisions", params.subdivisions, "Apply subdivision.");
  add_option(
      cli, "catmullclark", params.catmullclark, "Catmull-Clark subdivision.");
  add_option(cli, "toedges", params.toedges, "Convert shape to edges.");
  add_option(
      cli, "tovertices", params.tovertices, "Convert shape to vertices.");
}

// convert images
void run_convert(const convert_params& params) {
  // load mesh
  auto shape = load_shape(params.shape, true);

  // remove data
  if (params.aspositions) {
    shape.normals   = {};
    shape.texcoords = {};
    shape.colors    = {};
    shape.radius    = {};
  }

  // convert data
  if (params.astriangles) {
    if (!shape.quads.empty()) {
      shape.triangles = quads_to_triangles(shape.quads);
      shape.quads     = {};
    }
  }

  // print stats
  if (params.info) {
    print_info("shape stats ------------");
    auto stats = shape_stats(shape);
    for (auto& stat : stats) print_info(stat);
  }

  // subdivision
  if (params.subdivisions > 0) {
    shape = subdivide_shape(shape, params.subdivisions, params.catmullclark);
  }

  // transform
  if (params.translate != vec3f{0, 0, 0} || params.rotate != vec3f{0, 0, 0} ||
      params.scale != vec3f{1, 1, 1}) {
    auto translation = translation_frame(params.translate);
    auto scaling     = scaling_frame(params.scale);
    auto rotation    = rotation_frame({1, 0, 0}, radians(params.rotate.x)) *
                    rotation_frame({0, 0, 1}, radians(params.rotate.z)) *
                    rotation_frame({0, 1, 0}, radians(params.rotate.y));
    auto xform = translation * scaling * rotation;
    for (auto& p : shape.positions) p = transform_point(xform, p);
    auto nonuniform_scaling = min(params.scale) != max(params.scale);
    for (auto& n : shape.normals)
      n = transform_normal(xform, n, nonuniform_scaling);
  }

  // convert to edges
  if (params.toedges) {
    // check faces
    if (shape.triangles.empty() && shape.quads.empty())
      throw io_error::shape_error(params.shape);

    // convert to edges
    auto edges = !shape.triangles.empty() ? get_edges(shape.triangles)
                                          : get_edges(shape.quads);
    shape      = lines_to_cylinders(edges, shape.positions, 4, 0.001f);
  }

  // convert to vertices
  if (params.tovertices) {
    // convert to spheres
    shape = points_to_spheres(shape.positions);
  }

  // compute normals
  if (params.smooth) {
    if (!shape.points.empty()) {
      shape.normals = vector<vec3f>{shape.positions.size(), {0, 0, 1}};
    } else if (!shape.lines.empty()) {
      shape.normals = lines_tangents(shape.lines, shape.positions);
    } else if (!shape.triangles.empty()) {
      shape.normals = triangles_normals(shape.triangles, shape.positions);
    } else if (!shape.quads.empty()) {
      shape.normals = quads_normals(shape.quads, shape.positions);
    }
  }

  // remove normals
  if (params.facet) {
    shape.normals = {};
  }

  if (params.info) {
    print_info("shape stats ------------");
    auto stats = shape_stats(shape);
    for (auto& stat : stats) print_info(stat);
  }

  // save mesh
  save_shape(params.output, shape, true);
}

// fvconvert params
struct fvconvert_params {
  string shape        = "shape.obj";
  string output       = "out.obj";
  bool   info         = false;
  bool   smooth       = false;
  bool   facet        = false;
  bool   aspositions  = false;
  vec3f  translate    = {0, 0, 0};
  vec3f  rotate       = {0, 0, 0};
  vec3f  scale        = {1, 1, 1};
  int    subdivisions = 0;
  bool   catmullclark = false;
};

void add_options(const cli_command& cli, fvconvert_params& params) {
  add_argument(cli, "shape", params.shape, "Input shape.");
  add_option(cli, "output", params.output, "Output shape.");
  add_option(cli, "smooth", params.smooth, "Smooth normals.");
  add_option(cli, "facet", params.facet, "Facet normals.");
  add_option(
      cli, "aspositions", params.aspositions, "Remove all but positions.");
  add_option(cli, "translate", params.translate, "Translate shape.");
  add_option(cli, "scale", params.scale, "Scale shape.");
  add_option(cli, "rotate", params.rotate, "Rotate shape.");
  add_option(cli, "subdivisions", params.subdivisions, "Apply subdivision.");
  add_option(
      cli, "catmullclark", params.catmullclark, "Catmull-Clark subdivision.");
}

// convert images
void run_fvconvert(const fvconvert_params& params) {
  // load mesh
  auto shape = load_fvshape(params.shape);

  // remove data
  if (params.aspositions) {
    shape.normals       = {};
    shape.texcoords     = {};
    shape.quadsnorm     = {};
    shape.quadstexcoord = {};
  }

  // print info
  if (params.info) {
    print_info("shape stats ------------");
    auto stats = fvshape_stats(shape);
    for (auto& stat : stats) print_info(stat);
  }

  // subdivision
  if (params.subdivisions > 0) {
    shape = subdivide_fvshape(shape, params.subdivisions, params.catmullclark);
  }

  // transform
  if (params.translate != vec3f{0, 0, 0} || params.rotate != vec3f{0, 0, 0} ||
      params.scale != vec3f{1, 1, 1}) {
    auto translation = translation_frame(params.translate);
    auto scaling     = scaling_frame(params.scale);
    auto rotation    = rotation_frame({1, 0, 0}, radians(params.rotate.x)) *
                    rotation_frame({0, 0, 1}, radians(params.rotate.z)) *
                    rotation_frame({0, 1, 0}, radians(params.rotate.y));
    auto xform = translation * scaling * rotation;
    for (auto& p : shape.positions) p = transform_point(xform, p);
    auto nonuniform_scaling = min(params.scale) != max(params.scale);
    for (auto& n : shape.normals)
      n = transform_normal(xform, n, nonuniform_scaling);
  }

  // compute normals
  if (params.smooth) {
    if (!shape.quadspos.empty()) {
      shape.normals = quads_normals(shape.quadspos, shape.positions);
      if (!shape.quadspos.empty()) shape.quadsnorm = shape.quadspos;
    }
  }

  // remove normals
  if (params.facet) {
    shape.normals   = {};
    shape.quadsnorm = {};
  }

  if (params.info) {
    print_info("shape stats ------------");
    auto stats = fvshape_stats(shape);
    for (auto& stat : stats) print_info(stat);
  }

  // save mesh
  save_fvshape(params.output, shape, true);
}

// view params
struct view_params {
  string shape  = "shape.ply";
  string output = "out.ply";
  bool   addsky = false;
};

void add_options(const cli_command& cli, view_params& params) {
  add_argument(cli, "shape", params.shape, "Input shape.");
  add_option(cli, "output", params.output, "Output shape.");
  add_option(cli, "addsky", params.addsky, "Add sky.");
}

#ifndef YOCTO_OPENGL

// view shapes
void run_view(const view_params& params) {
  throw io_error::not_implemented_error("Opengl not compiled");
}

#else

// view shapes
void run_view(const view_params& params) {
  // load shape
  auto shape = load_shape(params.shape, true);

  // make scene
  auto scene = make_shape_scene(shape, params.addsky);

  // run view
  view_scene("yshape", params.shape, scene);
}

#endif

struct heightfield_params {
  string image     = "heightfield.png"s;
  string output    = "out.ply"s;
  bool   smooth    = false;
  float  height    = 1.0f;
  bool   info      = false;
  vec3f  translate = {0, 0, 0};
  vec3f  rotate    = {0, 0, 0};
  vec3f  scale     = {1, 1, 1};
};

void add_options(const cli_command& cli, heightfield_params& params) {
  add_argument(cli, "image", params.image, "Input image.");
  add_option(cli, "output", params.output, "Output shape.");
  add_option(cli, "smooth", params.smooth, "Smoooth normals.");
  add_option(cli, "height", params.height, "Shape height.");
  add_option(cli, "info", params.info, "Print info.");
  add_option(cli, "translate", params.translate, "Translate shape.");
  add_option(cli, "scale", params.scale, "Scale shape.");
  add_option(cli, "rotate", params.rotate, "Rotate shape.");
}

void run_heightfield(const heightfield_params& params) {
  // load image
  auto image = load_image(params.image);

  // adjust height
  if (params.height != 1) {
    for (auto& pixel : image.pixels) pixel *= params.height;
  }

  // create heightfield
  auto shape = make_heightfield({image.width, image.height}, image.pixels);
  if (!params.smooth) shape.normals.clear();

  // print info
  if (params.info) {
    print_info("shape stats ------------");
    auto stats = shape_stats(shape);
    for (auto& stat : stats) print_info(stat);
  }

  // transform
  if (params.translate != vec3f{0, 0, 0} || params.rotate != vec3f{0, 0, 0} ||
      params.scale != vec3f{1, 1, 1}) {
    auto translation = translation_frame(params.translate);
    auto scaling     = scaling_frame(params.scale);
    auto rotation    = rotation_frame({1, 0, 0}, radians(params.rotate.x)) *
                    rotation_frame({0, 0, 1}, radians(params.rotate.z)) *
                    rotation_frame({0, 1, 0}, radians(params.rotate.y));
    auto xform = translation * scaling * rotation;
    for (auto& p : shape.positions) p = transform_point(xform, p);
    auto nonuniform_scaling = min(params.scale) != max(params.scale);
    for (auto& n : shape.normals)
      n = transform_normal(xform, n, nonuniform_scaling);
  }

  // save mesh
  save_shape(params.output, shape, true);
}

struct hair_params {
  string shape   = "shape.ply"s;
  string output  = "out.ply"s;
  int    hairs   = 65536;
  int    steps   = 8;
  float  length  = 0.02f;
  float  noise   = 0.001f;
  float  gravity = 0.0005f;
  float  radius  = 0.0001f;
};

void add_options(const cli_command& cli, hair_params& params) {
  add_argument(cli, "shape", params.shape, "Input shape.");
  add_option(cli, "output", params.output, "Output shape.");
  add_option(cli, "hairs", params.hairs, "Number of hairs.");
  add_option(cli, "steps", params.steps, "Hair steps.");
  add_option(cli, "length", params.length, "Hair length.");
  add_option(cli, "noise", params.noise, "Noise weight.");
  add_option(cli, "gravity", params.gravity, "Gravity scale.");
  add_option(cli, "radius", params.radius, "Hair radius.");
}

void run_hair(const hair_params& params) {
  // load mesh
  auto shape = load_shape(params.shape);

  // generate hair
  auto hair = make_hair2(shape, {params.steps, params.hairs},
      {params.length, params.length}, {params.radius, params.radius},
      params.noise, params.gravity);

  // save mesh
  save_shape(params.output, hair, true);
}

struct sample_params {
  string shape   = "shape.ply"s;
  string output  = "out.ply"s;
  int    samples = 4096;
};

void add_options(const cli_command& cli, sample_params& params) {
  add_argument(cli, "shape", params.shape, "Input shape.");
  add_option(cli, "output", params.output, "Output shape.");
  add_option(cli, "samples", params.samples, "Number of samples.");
}

void run_sample(const sample_params& params) {
  // load mesh
  auto shape = load_shape(params.shape);

  // generate samples
  auto samples = sample_shape(shape, params.samples);

  // sample shape
  auto sshape = shape_data{};
  for (auto& sample : samples) {
    sshape.points.push_back((int)sshape.points.size());
    sshape.positions.push_back(eval_position(shape, sample.element, sample.uv));
    sshape.radius.push_back(0.001f);
  }

  // save mesh
  save_shape(params.output, sshape);
}

struct glview_params {
  string shape  = "shape.ply";
  bool   addsky = false;
};

// Cli
void add_options(const cli_command& cli, glview_params& params) {
  add_argument(cli, "shape", params.shape, "Input shape.");
  add_option(cli, "addsky", params.addsky, "Add sky.");
}

#ifndef YOCTO_OPENGL

// view shapes
void run_glview(const glview_params& params) {
  throw io_error::not_implemented_error("Opengl not compiled");
}

#else

void run_glview(const glview_params& params) {
  // loading shape
  auto shape = load_shape(params.shape);

  // make scene
  auto scene = make_shape_scene(shape, params.addsky);

  // run viewer
  glview_scene("yshape", params.shape, scene, {});
}

#endif

struct app_params {
  string             command     = "convert";
  convert_params     convert     = {};
  fvconvert_params   fvconvert   = {};
  view_params        view        = {};
  heightfield_params heightfield = {};
  hair_params        hair        = {};
  sample_params      sample      = {};
  glview_params      glview      = {};
};

// Cli
void add_options(const cli_command& cli, app_params& params) {
  set_command_var(cli, params.command);
  add_command(cli, "convert", params.convert, "Convert shapes.");
  add_command(
      cli, "fvconvert", params.fvconvert, "Convert face-varying shapes.");
  add_command(cli, "view", params.view, "View shapes.");
  add_command(cli, "heightfield", params.heightfield, "Create an heightfield.");
  add_command(cli, "hair", params.hair, "Grow hairs on a shape.");
  add_command(cli, "sample", params.sample, "Sample shapepoints on a shape.");
  add_command(cli, "glview", params.glview, "View shapes with OpenGL.");
}

// Run
void run(const vector<string>& args) {
  // command line parameters
  auto params = app_params{};
  auto cli    = make_cli("yshape", params, "Process and view shapes.");
  parse_cli(cli, args);

  // dispatch commands
  if (params.command == "convert") {
    return run_convert(params.convert);
  } else if (params.command == "fvconvert") {
    return run_fvconvert(params.fvconvert);
  } else if (params.command == "view") {
    return run_view(params.view);
  } else if (params.command == "heightfield") {
    return run_heightfield(params.heightfield);
  } else if (params.command == "hair") {
    return run_hair(params.hair);
  } else if (params.command == "sample") {
    return run_sample(params.sample);
  } else if (params.command == "glview") {
    return run_glview(params.glview);
  } else {
    throw io_error::command_error("yshape", params.command);
  }
}

// Main
int main(int argc, const char* argv[]) {
  handle_errors(run, make_cli_args(argc, argv));
}
