

//out vec4 colorOut;
 
//void main()
//{
//    colorOut = vec4(1.0, 0.0, 0.0, 1.0);
//}

// fPassThrough.glsl
// Pass through fragment shader.

void main()
{
    gl_FragColor = gl_Color;
}

//in vec4 gl_FragCoord;
//in bool gl_FrontFacing;
//in vec2 gl_PointCoord;

/*
#version 120
in vec2 LensLeftCenter;
in vec2 LensRightCenter;
in vec2 ScreenCenter;
in vec2 Scale;
//in vec2 ScaleIn;
in vec4 HmdWarpParam;
uniform sampler2D colorTex;

vec2 HmdWarp(vec2 in01)
{
	vec2 LensCenter;
	if (in01.x >= 0.5)
		LensCenter = LensRightCenter;
	else
		LensCenter = LensLeftCenter;

	vec2 theta = (in01 - LensCenter); // * ScaleIn;

	float rSq = theta.x * theta.x + theta.y * theta.y;
	vec2 rvector =   theta * (HmdWarpParam.x + HmdWarpParam.y * rSq + 
							  HmdWarpParam.z * rSq * rSq +
							  HmdWarpParam.w * rSq * rSq * rSq);

	return LensCenter + Scale * rvector;
}

void main()
{
	vec2 tc = HmdWarp(gl_PointCoord);
	vec2 clamped = clamp(tc, ScreenCenter-vec2(0.25,0.5),
					  ScreenCenter+vec2(0.25,0.5)) - tc;
	//if (clamped.x != 0 || clamped.y != 0 )
	//	gl_FragColor = vec4(0.0, 0.0, 1.0, 1.0);
	//else
	//	gl_FragColor = texture2D(colorTex, tc);

	gl_FragColor = texture2D(colorTex, gl_PointCoord);
}
*/