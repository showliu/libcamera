/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2020, Linaro
 *
 * fshader.h - Fragment shader code
 */
#ifndef __FSHADER_H__
#define __FSHADER_H__

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
                "yuv.x = texture2D(tex_y, textureOut).r - 0.0625;\n"
                "yuv.y = texture2D(tex_u, textureOut).r - 0.5;\n"
                "yuv.z = texture2D(tex_u, textureOut).g - 0.5;\n"
                "rgb = mat3( 1.0,       1.0,         1.0,\n"
                "            0.0,       -0.39465,  2.03211,\n"
                "            1.13983, -0.58060,  0.0) * yuv;\n"
                "gl_FragColor = vec4(rgb, 1.0);\n"
                "}\n";

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
                "yuv.x = texture2D(tex_y, textureOut).r - 0.0625;\n"
                "yuv.y = texture2D(tex_u, textureOut).g - 0.5;\n"
                "yuv.z = texture2D(tex_u, textureOut).r - 0.5;\n"
                "rgb = mat3( 1.0,       1.0,         1.0,\n"
                "            0.0,       -0.39465,  2.03211,\n"
                "            1.13983, -0.58060,  0.0) * yuv;\n"
                "gl_FragColor = vec4(rgb, 1.0);\n"
                "}\n";

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
                "yuv.x = texture2D(tex_y, textureOut).r - 0.0625;\n"
                "yuv.y = texture2D(tex_u, textureOut).r - 0.5;\n"
                "yuv.z = texture2D(tex_v, textureOut).g - 0.5;\n"
                "rgb = mat3( 1,       1,         1,\n"
                "            0,       -0.39465,  2.03211,\n"
                "            1.13983, -0.58060,  0) * yuv;\n"
                "gl_FragColor = vec4(rgb, 1);\n"
                "}\n";

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
                "yuv.x = texture2D(tex_y, textureOut).r - 0.0625;\n"
                "yuv.y = texture2D(tex_u, textureOut).g - 0.5;\n"
                "yuv.z = texture2D(tex_v, textureOut).r - 0.5;\n"
                "rgb = mat3( 1,       1,         1,\n"
                "            0,       -0.39465,  2.03211,\n"
                "            1.13983, -0.58060,  0) * yuv;\n"
                "gl_FragColor = vec4(rgb, 1);\n"
                "}\n";
#endif // __FSHADER_H__
