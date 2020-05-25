/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2020, Linaro
 *
 * shader.h - YUV format converter shader source code
 */
#ifndef __SHADER_H__
#define __SHADER_H__

/* Vertex shader for NV formats */
char NV_Vertex_shader[] = "attribute vec4 vertexIn;\n"
                        "attribute vec2 textureIn;\n"
                        "varying vec2 textureOut;\n"
                        "void main(void)\n"
                        "{\n"
                        "gl_Position = vertexIn;\n"
                        "textureOut = textureIn;\n"
                        "}\n";

/* Fragment shader for NV12, NV16 and NV24 */
char NV_2_planes_UV[] = "#ifdef GL_ES\n"
                "precision highp float;\n"
                "#endif\n"
                "varying vec2 textureOut;\n"
                "uniform sampler2D tex_y;\n"
                "uniform sampler2D tex_u;\n"
                "void main(void)\n"
                "{\n"
                "vec3 yuv;\n"
                "vec3 rgb;\n"
                "mat3 convert_mat = mat3(vec3(1.1640625,  1.1640625, 1.1640625),\n"
                "                        vec3(0.0,   -0.390625, 2.015625),\n"
                "                        vec3(1.5975625, -0.8125, 0.0));\n"
                "yuv.x = texture2D(tex_y, textureOut).r - 0.0625;\n"
                "yuv.y = texture2D(tex_u, textureOut).r - 0.5;\n"
                "yuv.z = texture2D(tex_u, textureOut).g - 0.5;\n"
                "rgb = convert_mat * yuv;\n"
                "gl_FragColor = vec4(rgb, 1.0);\n"
                "}\n";

/* Fragment shader for NV21, NV61 and NV42 */
char NV_2_planes_VU[] = "#ifdef GL_ES\n"
                "precision highp float;\n"
                "#endif\n"
                "varying vec2 textureOut;\n"
                "uniform sampler2D tex_y;\n"
                "uniform sampler2D tex_u;\n"
                "void main(void)\n"
                "{\n"
                "vec3 yuv;\n"
                "vec3 rgb;\n"
                "mat3 convert_mat = mat3(vec3(1.1640625,  1.1640625, 1.1640625),\n"
                "                        vec3(0.0,   -0.390625, 2.015625),\n"
                "                        vec3(1.5975625, -0.8125, 0.0));\n"
                "yuv.x = texture2D(tex_y, textureOut).r - 0.0625;\n"
                "yuv.y = texture2D(tex_u, textureOut).g - 0.5;\n"
                "yuv.z = texture2D(tex_u, textureOut).r - 0.5;\n"
                "rgb = convert_mat * yuv;\n"
                "gl_FragColor = vec4(rgb, 1.0);\n"
                "}\n";

/* Fragment shader for YUV420 */
char NV_3_planes_UV[] = "#ifdef GL_ES\n"
                "precision highp float;\n"
                "#endif\n"
                "varying vec2 textureOut;\n"
                "uniform sampler2D tex_y;\n"
                "uniform sampler2D tex_u;\n"
                "uniform sampler2D tex_v;\n"
                "void main(void)\n"
                "{\n"
                "vec3 yuv;\n"
                "vec3 rgb;\n"
                "mat3 convert_mat = mat3(vec3(1.1640625,  1.1640625, 1.1640625),\n"
                "                        vec3(0.0,   -0.390625, 2.015625),\n"
                "                        vec3(1.5975625, -0.8125, 0.0));\n"
                "yuv.x = texture2D(tex_y, textureOut).r - 0.0625;\n"
                "yuv.y = texture2D(tex_u, textureOut).r - 0.5;\n"
                "yuv.z = texture2D(tex_v, textureOut).g - 0.5;\n"
                "rgb = convert_mat * yuv;\n"
                "gl_FragColor = vec4(rgb, 1);\n"
                "}\n";

/* Fragment shader for YVU420 */
char NV_3_planes_VU[] = "precision highp float;\n"
                "#endif\n"
                "varying vec2 textureOut;\n"
                "uniform sampler2D tex_y;\n"
                "uniform sampler2D tex_u;\n"
                "uniform sampler2D tex_v;\n"
                "void main(void)\n"
                "{\n"
                "vec3 yuv;\n"
                "vec3 rgb;\n"
                "mat3 convert_mat = mat3(vec3(1.1640625,  1.1640625, 1.1640625),\n"
                "                        vec3(0.0,   -0.390625, 2.015625),\n"
                "                        vec3(1.5975625, -0.8125, 0.0));\n"
                "yuv.x = texture2D(tex_y, textureOut).r - 0.0625;\n"
                "yuv.y = texture2D(tex_v, textureOut).g - 0.5;\n"
                "yuv.z = texture2D(tex_u, textureOut).r - 0.5;\n"
                "rgb = convert_mat * yuv;\n"
                "gl_FragColor = vec4(rgb, 1);\n"
                "}\n";
#endif // __SHADER_H__
