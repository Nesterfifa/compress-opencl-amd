typedef struct frame_data {
  uchar frame_content_size_flag;
  bool single_segment_flag;
  bool unused_bit;
  bool reserved_bit;
  bool content_checksum_flag;
  uchar dictionary_id_flag;

  uchar fcs_field_size;
  uchar did_field_size;
  ulong window_size;
  ulong dictionary_id;
  ulong frame_content_size;
} frame_data;

typedef struct block_data {
  uchar literals_block_type;
  uchar size_format;
  uint regenerated_size;
  uint compressed_size;
} block_data;

void decode_magic_number(__global uchar *input, int *pos, __global int *err) {
  uint res = 0;
  for (int i = 0; i < 4; i++) {
    uint byte = input[*pos++];
    res |= byte << (i * 8);
  }

  if (res != 0xFD2FB528) {
    err = 1;
  }
}

void decode_frame_header(__global uchar *input, int *global_position,
                         frame_data *data, __global int *err) {
  uchar byte = input[*global_position++];
  data->frame_content_size_flag = byte >> 6;
  data->single_segment_flag = (byte >> 5) & 1;
  data->unused_bit = (byte >> 4) & 1;
  data->reserved_bit = (byte >> 3) & 1;
  data->content_checksum_flag = (byte >> 2) & 1;
  data->dictionary_id_flag = byte & 3;

  data->fcs_field_size = 1 << data->frame_content_size_flag;
  if (data->fcs_field_size == 1) {
    data->fcs_field_size = data->single_segment_flag;
  }

  if (unused_bit || reserved_bit) {
    err = 2;
    return;
  }

  data->did_field_size =
      data->dictionary_id_flag == 3 ? 4 : data->dictionary_id_flag;

  if (!data->single_segment_flag) {
    byte = input[*global_position++];
    ulong exponent = byte >> 3;
    ulong mantissa = byte & 7;
    ulong window_log = 10 + exponent;
    ulong window_base = 1 << window_log;
    ulong window_add = (window_base / 8) * mantissa;
    data->window_size = window_base + window_add;
  }

  for (int i = 0; i < data->did_field_size; i++) {
    byte = input[*global_position++];
    data->dictionary_id |= byte << (i * 8);
  }

  for (int i = 0; i < data->fcs_field_size; i++) {
    byte = input[*global_position++];
    data->frame_content_size |= byte << (i * 8);
  } 

  if (data->fcs_field_size == 2) {
    data->frame_content_size += 256;
  }
}

void decode_block(__global uchar *input, __global uchar *out,
                  __global size_t *out_size, int *global_position,
                  __global int *err) {
  block_data *data;

  uint byte0 = input[*global_position++];
  uint byte1, byte2, byte3, byte4;
  data->literals_block_type = byte0 & 3;
  if (data->literals_block_type < 2) {
    data->size_format = (byte0 >> 2) & 3;
    switch (data->size_format) {
    case 0:
      data->regenerated_size = byte0 >> 3;
      break;
    case 1:
      data->regenerated_size = byte0 >> 3;
      break;
    case 2:
      byte1 = input[*global_position++];
      data->regenerated_size = (byte0 >> 4) | (byte1 << 4);
      break;
    case 3:
      byte1 = input[*global_position++];
      byte2 = input[*global_position++];
      data->regenerated_size = (byte0 >> 4) | (byte1 << 4) | (byte2 << 12);
      break;
    }
  } else {
    data->size_format = (byte0 >> 2) & 3;
    switch (data->size_format) {
    case 0:
      byte1 = input[*global_position++];
      byte2 = input[*global_position++];
      data->regenerated_size = (byte0 >> 4) | ((byte1 & 63) << 4);
      data->compressed_size = (byte1 >> 6) | (byte2 << 2);
      break;
    case 1:
      byte1 = input[*global_position++];
      byte2 = input[*global_position++];
      data->regenerated_size = (byte0 >> 4) | ((byte1 & 63) << 4);
      data->compressed_size = (byte1 >> 6) | (byte2 << 2);
      break;
    case 2:
      byte1 = input[*global_position++];
      byte2 = input[*global_position++];
      byte3 = input[*global_position++];
      data->regenerated_size =
          (byte0 >> 4) | (byte1 << 4) | ((byte2 & 3) << 12);
      data->compressed_size = (byte2 >> 2) | (byte3 << 6);
      break;
    case 3:
      byte1 = input[*global_position++];
      byte2 = input[*global_position++];
      byte3 = input[*global_position++];
      byte4 = input[*global_position++];
      data->regenerated_size =
          (byte0 >> 4) | (byte1 << 4) || ((byte2 & 63) << 12);
      data->compressed_size = (byte2 >> 6) | (byte3 << 2) | (byte4 << 10);
      break;
    }
  }
}

void decode_frame(__global uchar *input, __global uchar *out,
                  __global size_t *out_size, int *global_position,
                  __global int *err) {
  decode_magic_number(input, pos, err);
  frame_data *data;
  decode_frame_header(input, data, global_position, err);

  while (true) {
    uint block_header = 0;
    for (int i = 0; i < 3; i++) {
      uint byte = input[*global_position++];
      block_header |= byte << (8 * i);
    }
    bool last = block_header & 1;
    uchar type = (block_header >> 1) & 3;
    uint size = (block_header >> 3);

    switch (type) {
    case 0:
      for (int i = 0; i < size; i++) {
        out[*out_size++] = input[*global_position++];
      }
      break;
    case 1:
      uchar byte = input[*global_position++];
      for (int i = 0; i < size; i++) {
        out[*out_size++] = byte;
      }
      break;
    case 2:

      break;
    case 3:
      *err = 3;
      return;
    default:
      *err = 4;
      return;
    }

    if (last) {
      break;
    }
  }
}

__kernel void decompress_zstd(__global uchar *input, int input_size,
                              __global uchar *out, __global int err) {
  const int index = get_global_id(0);
  if (index == 0) {
    __global size_t global_position = 0;
    __global size_t out_size = 0;
    while (global_position < input_size - 4) {
      decode_frame(input, out, &out_size, &global_position, &err)
    }
  }
  barrier(CLK_GLOBAL_MEM_FENCE);
}