# Yocto/SceneIO: Scene serialization

Yocto/SceneIO is a collection of functions to load and save images and
scene elements, together with path manipulation utilities.
Yocto/SceneIO is implemented in `yocto_sceneio.h` and `yocto_sceneio.cpp`,
and depends on `json.hpp` for Json serialization, and `stb_image.h`,
`stb_image_write.h` and `tinyexr.h` for the image serialization.

## Image serialization

Images are loaded with `load_image(filename, img, error)` and saved with
`save_image(filename, img, error)`. Both loading and saving take a filename,
an image buffer and return whether or not the image was loaded successfully.
In the case of an error, the IO functions set the `error` string with a
message suitable for displaying to a user.
Yocto/SceneIO supports loading and saving to JPG, PNG, TGA, BMP, HDR, EXR.

When loading images, the color space is detected by the filename and stored
in the returned image. Since color space information is not
present in many images and since it is customary in 3D graphics to misuse
color encoding, e.g. normal maps stored in sRGB, during loading 8bit file
formats are assume to be encoded sRGB, while float formats are assumed to
be encoded in linear RGB.
Use `is_hdr_filename(filename)` to determine whether a supported format
contains linear float data.
When saving images, pixel values are converted to the color space supported
by the chosen file format.

```cpp
auto error = string{};
auto image = image_data{};              // define an image
if(!load_image(filename, image, error)) // load image
  print_error(error);                   // check and print error
if(!save_image(filename, image, error)) // save image
  print_error(error);                   // check and print error
```

## Scene serialization

Scenes are loaded with `load_scene(filename, scene, error, progress)` and
saved with `save_scene(filename, scene, error, progress)`.
Both loading and saving take a filename, a scene reference and return
whether or not the scene was loaded successfully.
In the case of an error, the IO functions set the `error` string with a
message suitable for displaying to a user.
Yocto/SceneIO supports loading and saving to a custom Json format,
Obj, glTF, Pbrt, and all shape file formats.

```cpp
auto scene = scene_data{};               // scene
auto error = string{};                   // error buffer
if(!load_scene(filename, scene, error))  // load scene
  print_error(error);
if(!save_scene(filename, scene, error))  // save scene
  print_error(error);
```

## Shape serialization

Shapes are loaded with `load_shape(filename, shape, error)` and
saved with `save_shape(filename, shape, error)`.
Face-varying shapes with `load_fvshape(filename, shape, error)`
and saved with `save_fvshape(filename, shape, error)`.
Both loading and saving take a filename, a scene reference and return
whether or not the scene was loaded successfully.
In the case of an error, the IO functions set the `error` string with a
message suitable for displaying to a user.
Yocto/SceneIO supports loading and saving to Ply, Obj, Stl.

For indexed meshes, the type of elements is determined during loading,
while face-varying coerce all face to quads. By default, texture coordinates
are flipped vertically to match the convention of OpenGL texturing;
this can be disabled by setting the `flipv` flag.

```cpp
auto shape = shape_data{};                   // shape
auto error = string{};                       // error buffer
if(!load_shape(filename, shape, error))      // load shape
  print_error(error);
if(!save_shape(filename, shape, error))      // save shape
  print_error(error);

auto fvshape = fvshape_data{};               // face-varying shape
auto error = string{};                       // error buffer
if(!load_fvshape(filename, fvshape, error))  // load shape
  print_error(error);
if(!save_fvshape(filename, fvshape, error))  // save shape
  print_error(error);
```

## Texture serialization

Textures are loaded with `load_texture(filename, texture, error)` and saved with
`save_texture(filename, texture, error)`. Both loading and saving take a filename,
a texture and return whether or not the image was loaded successfully.
In the case of an error, the IO functions set the `error` string with a
message suitable for displaying to a user.
Yocto/SceneIO supports loading and saving to JPG, PNG, TGA, BMP, HDR, EXR.

When loading textures, the color space and bit depth is detected by the filename.
Texture are loaded as float for float file formats, and as bytes for 8bit file
formats. Since color space information is not present in many images,
we assume that byte textures are encoded in sRGB, and float texture in linear RGB.
Use `is_hdr_filename(filename)` to determine whether a supported format
contains linear float data.
When saving images, pixel values are converted to the color space supported
by the chosen file format.

```cpp
auto error = string{};
auto texture = texture_data{};              // define a texture
if(!load_texture(filename, texture, error)) // load texture
  print_error(error);                       // check and print error
if(!save_texture(filename, texture, error)) // save texture
  print_error(error);                       // check and print error
```
