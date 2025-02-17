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
#include <yocto/yocto_color.h>
#include <yocto/yocto_image.h>
#include <yocto/yocto_math.h>
#include <yocto/yocto_scene.h>
#include <yocto/yocto_sceneio.h>
#if YOCTO_OPENGL == 1
#include <yocto_gui/yocto_glview.h>
#endif
using namespace yocto;

// convert params
struct convert_params {
  string image    = "image.png";
  string output   = "out.png";
  float  exposure = 0;
  bool   filmic   = false;
  int    width    = 0;
  int    height   = 0;
};

// Cli
void add_options(const cli_command& cli, convert_params& params) {
  add_argument(cli, "image", params.image, "Input image.");
  add_option(cli, "output", params.output, "Output image.");
  add_option(
      cli, "exposure", params.exposure, "Tonemap exposure.", {-100, +100});
  add_option(cli, "filmic", params.filmic, "Tonemap filmic.");
  add_option(cli, "width", params.width, "Resize width.", {1, int_max});
  add_option(cli, "height", params.height, "Resize height.", {1, int_max});
}

// convert images
void run_convert(const convert_params& params) {
  // load
  auto image = load_image(params.image);

  // resize if needed
  if (params.width != 0 || params.height != 0) {
    image = resize_image(image, params.width, params.height);
  }

  // tonemap if needed
  if (image.linear && is_ldr_filename(params.output)) {
    image = tonemap_image(image, params.exposure, params.filmic);
  }

  // save
  save_image(params.output, image);
}

// view params
struct view_params {
  vector<string> images = {"image.png"};
  string         output = "out.png";
};

// Cli
void add_options(const cli_command& cli, view_params& params) {
  add_argument(cli, "images", params.images, "Input images.");
  add_option(cli, "output", params.output, "Output image.");
}

#ifndef YOCTO_OPENGL

// view images
void run_view(const view_params& params) {
  throw io_error::not_supported_error("Opengl not compiled");
}

#else

// view images
void run_view(const view_params& params) {
  // load
  auto images = vector<image_data>(params.images.size());
  for (auto idx = 0; idx < (int)params.images.size(); idx++) {
    images[idx] = load_image(params.images[idx]);
  }

  // run viewer
  view_images("yimage", params.images, images);
}

#endif

// grade params
struct grade_params : colorgrade_params {
  string image  = "image.png";
  string output = "out.png";
};

// Cli
void add_options(const cli_command& cli, grade_params& params) {
  add_argument(cli, "image", params.image, "Input image.");
  add_option(cli, "output", params.output, "Output image.");
}

#ifndef YOCTO_OPENGL

// grade images
void run_grade(const grade_params& params) {
  throw io_error::not_supported_error("Opengl not compiled");
}

#else

// grade images
void run_grade(const grade_params& params) {
  // load image
  auto image = load_image(params.image);

  // run viewer
  colorgrade_image("yimage", params.image, image);
}

#endif

// resize params
struct diff_params {
  string image1    = "image1.png";
  string image2    = "image2.png";
  string output    = "";
  bool   signal    = false;
  float  threshold = 0;
};

// Cli
void add_options(const cli_command& cli, diff_params& params) {
  add_argument(cli, "image1", params.image1, "Input image 1.");
  add_argument(cli, "image2", params.image2, "Input image 2.");
  add_option(cli, "output", params.output, "Output image.");
  add_option(cli, "signal", params.signal, "Error on diff.");
  add_option(cli, "threshold", params.threshold, "Diff threshold.");
}

// resize images
void run_diff(const diff_params& params) {
  // load
  auto image1 = load_image(params.image1);
  auto image2 = load_image(params.image2);

  // check sizes
  if (image1.width != image2.width || image1.height != image2.height) {
    throw io_error::mismatch_error(
        params.image1, params.image2, "image different sizes");
  }

  // check types
  if (image1.linear != image2.linear) {
    throw io_error::mismatch_error(
        params.image1, params.image2, "image different types");
  }

  // compute diff
  auto diff = image_difference(image1, image2, true);

  // save
  if (params.output != "") save_image(params.output, diff);

  // check diff
  if (params.signal) {
    for (auto& c : diff.pixels) {
      if (max(xyz(c)) > params.threshold) {
        throw io_error::mismatch_error(
            params.image1, params.image2, "image content differs");
      }
    }
  }
}

// setalpha params
struct setalpha_params {
  string image      = "image.png";
  string alpha      = "alpha.png";
  string output     = "out.png";
  bool   from_color = false;
  bool   from_black = false;
  bool   to_color   = false;
};

// Cli
void add_options(const cli_command& cli, setalpha_params& params) {
  add_argument(cli, "image", params.image, "Input image.");
  add_argument(cli, "alpha", params.alpha, "Alpha image.");
  add_option(cli, "output", params.output, "Output image.");
  add_option(cli, "from-color", params.from_color, "Alpha from color.");
  add_option(cli, "from-black", params.from_black, "Alpha from black.");
  add_option(cli, "to-color", params.to_color, "Color from alpha.");
}

// setalpha images
void run_setalpha(const setalpha_params& params) {
  // load
  auto image = load_image(params.image);
  auto alpha = load_image(params.alpha);

  // check sizes
  if (image.width != alpha.width || image.height != alpha.height) {
    throw io_error::mismatch_error(
        params.image, params.alpha, "image different size");
  }

  // check types
  if (image.linear != alpha.linear) {
    throw io_error::mismatch_error(
        params.image, params.alpha, "image different types");
  }

  // edit alpha
  auto out = make_image(image.width, image.height, image.linear);
  for (auto idx = (size_t)0; idx < image.pixels.size(); idx++) {
    auto calpha = alpha.pixels[idx];
    auto alpha_ = params.from_color   ? mean(xyz(calpha))
                  : params.from_black ? (mean(xyz(calpha)) > 0.01 ? 1.0f : 0.0f)
                                      : calpha.w;
    if (params.to_color) {
      out.pixels[idx] = {alpha_, alpha_, alpha_, alpha_};
    } else {
      auto color      = image.pixels[idx];
      color.w         = alpha_;
      out.pixels[idx] = color;
    }
  }

  // save
  save_image(params.output, out);
}

struct app_params {
  string          command  = "convert";
  convert_params  convert  = {};
  view_params     view     = {};
  grade_params    grade    = {};
  diff_params     diff     = {};
  setalpha_params setalpha = {};
};

// Cli
void add_options(const cli_command& cli, app_params& params) {
  set_command_var(cli, params.command);
  add_command(cli, "convert", params.convert, "Convert images.");
  add_command(cli, "view", params.view, "View images.");
  add_command(cli, "grade", params.grade, "Grade images.");
  add_command(cli, "diff", params.diff, "Diff two images.");
  add_command(cli, "setalpha", params.setalpha, "Set alpha in images.");
}

// Run
void run(const vector<string>& args) {
  // command line parameters
  auto params = app_params{};
  auto cli    = make_cli("yimage", params, "Process and view images.");
  parse_cli(cli, args);

  // dispatch commands
  if (params.command == "convert") {
    return run_convert(params.convert);
  } else if (params.command == "view") {
    return run_view(params.view);
  } else if (params.command == "grade") {
    return run_grade(params.grade);
  } else if (params.command == "diff") {
    return run_diff(params.diff);
  } else if (params.command == "setalpha") {
    return run_setalpha(params.setalpha);
  } else {
    throw io_error::command_error("yimage", params.command);
  }
}

// Main
int main(int argc, const char* argv[]) {
  handle_errors(run, make_cli_args(argc, argv));
}
