#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragColor;
layout(location = 0) out vec4 outColor;
layout(location = 1) in vec2 fragTexCoord;
layout(binding = 0) uniform usampler1D texSampler;

uint convert_to_psmct32(ivec2 coord)
{
    /* Get offset from texture */
    int dest_base = coord.y * 640 + coord.x;

    /* Get page offset */
    int page_x = coord.x / 64;
    int page_y = coord.y / 32;
    int page = page_y * 10 + page_x;

    /* Block layout */
    const int block_layout[4][8] = int[4][8](
        int[8](0,  1,  4,  5, 16, 17, 20, 21),
        int[8](2,  3,  6,  7, 18, 19, 22, 23),
        int[8](8,  9, 12, 13, 24, 25, 28, 29),
        int[8](10, 11, 14, 15, 26, 27, 30, 31));

    int block_x = (coord.x / 8) % 8;
    int block_y = (coord.y / 8) % 4;
    int block = block_layout[block_y][block_x];

    /* Pixel layout */
    const int pixels[2][8] = int[2][8](int[8](0, 1, 4, 5,  8,  9, 12, 13),
                                       int[8](2, 3, 6, 7, 10, 11, 14, 15));

    int column = (coord.y / 2) % 4;
    int pixel = pixels[coord.y & 1][coord.x % 8];
    int offset = column * 16 + pixel;

    int final_offset = page * 2048 + block * 64 + offset;
    return final_offset;
}

void main()
{
    vec2 screen_coords = gl_FragCoord.xy * (vec2(640, 368) / vec2(800, 600));

    // Account for VRAM swizzling
    uint swizzled_coord = convert_to_psmct32(ivec2(gl_FragCoord));

    uint pixel = texelFetch(texSampler, int(swizzled_coord), 0).r;
    uvec3 rgb = uvec3(pixel & 0xff, (pixel >> 8) & 0xff, (pixel >> 16) & 0xff);
    vec3 color = vec3(rgb) / vec3(255.0f);
    color += fragColor;

    outColor = vec4(color, 1.0);
}
