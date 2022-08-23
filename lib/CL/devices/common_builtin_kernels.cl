
__kernel void
org_khronos_openvx_scale_image_nn_u8 (
    global const unsigned char *__restrict input,
    global unsigned char *__restrict output, float width_scale,
    float height_scale, int input_width, int input_height)
{
  int x = get_global_id (0);
  int y = get_global_id (1);
  int z = get_global_id (2);
  int num_planes = get_global_size (2);

  float x_src = ((float)x + 0.5f) * width_scale - 0.5f;
  float y_src = ((float)y + 0.5f) * height_scale - 0.5f;
  float x_min = floor (x_src);
  float y_min = floor (y_src);

  int x1 = (int)x_min;
  int y1 = (int)y_min;

  if (x_src - x_min >= 0.5f)
    x1++;
  if (y_src - y_min >= 0.5f)
    y1++;

  unsigned char result = input[num_planes * (x1 * input_width + y1) + z];

  output[num_planes * (x * get_global_size (0) + y) + z] = result;
}

__kernel void
org_khronos_openvx_scale_image_bl_u8 (
    global const unsigned char *__restrict input,
    global unsigned char *__restrict output, float width_scale,
    float height_scale, int input_width, int input_height)
{
  size_t planes = get_global_size (2);
  size_t x = get_global_id (0);
  size_t y = get_global_id (1);
  size_t plane = get_global_id (2);

  int input_size = input_width * input_height;
  int output_width = get_global_size(0);
  int output_height = get_global_size(1);
  int output_size = output_width * output_height;

  float x_src = ((float)x + 0.5f) * width_scale - 0.5f;
  float y_src = ((float)y + 0.5f) * height_scale - 0.5f;

  int x_min = floor (x_src);
  int y_min = floor (y_src);

  float s = x_src - x_min;
  float t = y_src - y_min;

  float result
      = (1 - s) * (1 - t)
            * input[plane*input_width*input_height + y_min * input_width + x_min]
        + s * (1 - t)
              * input[plane*input_width*input_height + (y_min + 1) * input_width + x_min]
        + (1 - s) * t
              * input[plane*input_width*input_height + y_min * input_width + x_min + 1]
        + s * t
              * input[plane*input_width*input_height + (y_min + 1) * input_width + x_min + 1];

  output[plane*output_width*output_height + y*output_width + x]
      = (unsigned char)result;
}

__kernel void
pocl_add_i8 (global const signed char *__restrict a,
             global const signed char *__restrict b,
             global signed char *__restrict output)
{
  size_t x = get_global_id (0);

  output[x] = a[x] + b[x];
}

__kernel void
org_khronos_openvx_tensor_convert_depth_wrap_u8_f32 (
            global const unsigned char *__restrict input,
            global float *__restrict output,
            float norm, float offset)
{
  int x = get_global_id(0);
  int y = get_global_id(1);
  int z = get_global_id(2);
  int width = get_global_size(0);
  int height = get_global_size(1);
  int idx = z*width*height + y*width + x;
  output[idx] = (input[idx] - offset) / norm;
}
