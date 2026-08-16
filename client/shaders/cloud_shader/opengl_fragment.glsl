uniform lowp vec4 fogColor;
uniform float fogDistance;
uniform float fogShadingParameter;

VARYING_ highp vec3 eyeVec;

VARYING_ lowp vec4 varColor;

void main(void)
{
	vec4 col = varColor;

	float d = (length(eyeVec) - fogDistance * fogShadingParameter) / (fogDistance * (1.0 - fogShadingParameter));
	col.rgb = mix(fogColor.rgb, col.rgb, clamp(1.0 - d, 0.0, 1.0));

	gl_FragColor = col;
}
