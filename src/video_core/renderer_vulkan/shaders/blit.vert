// Copyright 2019 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

/*
 * Build instructions:
 * $ glslangValidator -V $THIS_FILE -o output.spv
 * $ spirv-opt -O --strip-debug output.spv -o optimized.spv
 * $ xxd -i optimized.spv
 *
 * Then copy that bytecode to the C++ file
 */

#version 460 core

layout (location = 0) in vec2 vert_position;
layout (location = 1) in vec2 vert_tex_coord;

layout (location = 0) out vec2 frag_tex_coord;

layout (set = 0, binding = 0) uniform MatrixBlock {
    mat4 modelview_matrix;
};

void main() {
    gl_Position = modelview_matrix * vec4(vert_position, 0.0, 1.0);
    frag_tex_coord = vert_tex_coord;
}
