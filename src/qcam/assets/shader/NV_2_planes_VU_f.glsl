/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2020, Linaro
 *
 * NV_2_planes_VU_f.glsl - Fragment shader code for NV21, NV61 and NV42 formats
 */

#ifdef GL_ES
precision highp float;
#endif

varying vec2 textureOut;
uniform sampler2D tex_y;
uniform sampler2D tex_u;

void main(void)
{
	vec3 yuv;
	vec3 rgb;
	mat3 convert_mat = mat3(
							vec3(1.1640625, 1.1640625, 1.1640625),
							vec3(0.0, -0.390625, 2.015625),
							vec3(1.5975625, -0.8125, 0.0)
						   );

	yuv.x = texture2D(tex_y, textureOut).r - 0.0625;
	yuv.y = texture2D(tex_u, textureOut).g - 0.5;
	yuv.z = texture2D(tex_u, textureOut).r - 0.5;
	
	rgb = convert_mat * yuv;
	gl_FragColor = vec4(rgb, 1.0);
}
