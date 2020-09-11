/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2020, Linaro
 *
 * NV_vertex_shader.glsl - Vertex shader code for NV family
 */

attribute vec4 vertexIn;
attribute vec2 textureIn;
varying vec2 textureOut;

void main(void)
{
	gl_Position = vertexIn;
	textureOut = textureIn;
}
