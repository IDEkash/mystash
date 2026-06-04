#define rendered texture0
#define bloom texture1
#define prevFrame texture2

#ifdef GL_ES
// Dithering requires sufficient floating-point precision
#ifndef GL_FRAGMENT_PRECISION_HIGH
#undef ENABLE_DITHERING
#endif
#endif

struct ExposureParams {
	float compensationFactor;
};

uniform sampler2D rendered;
uniform sampler2D bloom;
uniform sampler2D prevFrame;

uniform vec2 texelSize0;

uniform ExposureParams exposureParams;
uniform lowp float bloomIntensity;
uniform lowp float saturation;
uniform float animationTimer;

CENTROID_ VARYING_ mediump vec2 varTexCoord;

#ifdef ENABLE_AUTO_EXPOSURE
VARYING_ float exposure; // linear exposure factor, see vertex shader
#endif

// VHS DEFINES
#define LENS_DISTORTION
#define ASPECT_RATIO
#define INTERLACING
#define COLOUR_DEPTH  256.0
#define LUMA_NOISE    0.025
#define CHROMA_NOISE  0.1
#define LUMA_LOD      2.0
#define CHROMA_LOD    4.0
#define INTERLACE_FPS 24.0
#define SHARPNESS     1.0

#ifdef GL_EXT_shader_texture_lod
#extension GL_EXT_shader_texture_lod : enable
#define TEXTURE_2D_LOD(s, u, l) texture2DLodEXT(s, u, l)
#else
#define TEXTURE_2D_LOD(s, u, l) texture2D(s, u, l)
#endif

#ifdef ENABLE_BLOOM
vec4 applyBloom(vec4 color, vec2 uv)
{
	vec3 light = texture2D(bloom, uv).rgb;
#ifdef ENABLE_BLOOM_DEBUG
	if (uv.x > 0.5 && uv.y < 0.5)
		return vec4(light, color.a);
	if (uv.x < 0.5)
		return color;
#endif
	color.rgb = mix(color.rgb, light, bloomIntensity);
	return color;
}
#endif

#if ENABLE_TONE_MAPPING
highp vec3 uncharted2Tonemap(highp vec3 x)
{
	return ((x * (0.22 * x + 0.03) + 0.002) / (x * (0.22 * x + 0.3) + 0.06)) - 0.03333;
}

vec4 applyToneMapping(vec4 color)
{
	color = vec4(pow(color.rgb, vec3(2.2)), color.a);
	const float gamma = 1.6;
	const float exposureBias = 5.5;
	color.rgb = uncharted2Tonemap(exposureBias * color.rgb);
	vec3 whiteScale = vec3(1.036015346);
	color.rgb *= whiteScale;
	return vec4(pow(color.rgb, vec3(1.0 / gamma)), color.a);
}
#endif

vec3 applySaturation(vec3 color, float factor)
{
	float brightness = dot(color, vec3(0.2125, 0.7154, 0.0721));
	return mix(vec3(brightness), color, factor);
}

#ifdef ENABLE_DITHERING
vec3 screen_space_dither(highp vec2 frag_coord) {
	highp vec3 dither = vec3(dot(vec2(171.0, 231.0), frag_coord));
	dither.rgb = fract(dither.rgb / vec3(103.0, 71.0, 97.0));
	return (dither.rgb - 0.5) / 255.0;
}
#endif

void main(void)
{
	vec2 uv = varTexCoord.st;

#ifdef LENS_DISTORTION
	uv = (uv * 2.0 - 1.0);
	uv *= 1.0 + dot(uv, uv) * 0.1;
	uv = (uv * 0.85) * 0.5 + 0.5;
#endif

#ifdef ASPECT_RATIO
	if (abs(uv.x * 2.0 - 1.0) >= 0.75 || uv.y < 0.0 || uv.y > 1.0 || uv.x < 0.0 || uv.x > 1.0) {
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}
#else
	if (uv.y < 0.0 || uv.y > 1.0 || uv.x < 0.0 || uv.x > 1.0) {
		gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}
#endif

#ifdef ENABLE_SSAA
	vec4 color = vec4(0.);
	for (float dx = 1.; dx < SSAA_SCALE; dx += 2.)
	for (float dy = 1.; dy < SSAA_SCALE; dy += 2.)
		color += texture2D(rendered, uv + texelSize0 * vec2(dx, dy)).rgba;
	color /= SSAA_SCALE * SSAA_SCALE / 4.;
#else
	vec4 color = texture2D(rendered, uv).rgba;
#endif

	// translate to linear colorspace (approximate)
	color.rgb = pow(color.rgb, vec3(2.2));

	{
		color.rgb *= exposureParams.compensationFactor;
#ifdef ENABLE_AUTO_EXPOSURE
		color.rgb *= exposure;
#endif
	}

#ifdef ENABLE_BLOOM
	color = applyBloom(color, uv);
#endif

	color.rgb = clamp(color.rgb, vec3(0.), vec3(1.));

	// return to sRGB colorspace (approximate)
	color.rgb = pow(color.rgb, vec3(1.0 / 2.2));

	{
#if ENABLE_TONE_MAPPING
		color = applyToneMapping(color);
#endif
		color.rgb = applySaturation(color.rgb, saturation);
	}

#ifdef ENABLE_DITHERING
	color.rgb += screen_space_dither(gl_FragCoord.xy);
#endif

	// VHS EFFECTS
	// 1. COLOR DEPTH REDUCTION
	color.rgb = floor(color.rgb * COLOUR_DEPTH) / COLOUR_DEPTH;

	// 2. CHROMA BLEED
	vec4 lumaSample = TEXTURE_2D_LOD(rendered, uv, LUMA_LOD);
	vec4 chromaSample = TEXTURE_2D_LOD(rendered, uv, CHROMA_LOD);

	float Y_blur = 0.299 * lumaSample.r + 0.587 * lumaSample.g + 0.114 * lumaSample.b;
	float U_blur = 0.493 * (chromaSample.b - (0.299 * chromaSample.r + 0.587 * chromaSample.g + 0.114 * chromaSample.b));
	float V_blur = 0.877 * (chromaSample.r - (0.299 * chromaSample.r + 0.587 * chromaSample.g + 0.114 * chromaSample.b));

	// 3. FILM GRAIN
	float noise = fract(sin(dot(gl_FragCoord.xy + fract(animationTimer * 5.0), vec2(12.9898, 78.233))) * 43758.5453);
	Y_blur += (noise - 0.5) * LUMA_NOISE;
	U_blur += (noise - 0.5) * CHROMA_NOISE;
	V_blur += (noise - 0.5) * CHROMA_NOISE;

	vec3 vhsRGB;
	vhsRGB.r = Y_blur + V_blur / 0.877;
	vhsRGB.g = Y_blur - 0.39393 * U_blur - 0.58081 * V_blur;
	vhsRGB.b = Y_blur + U_blur / 0.493;

	// mix with processed color for brightness but vhs color for smear
	color.rgb = mix(color.rgb, vhsRGB, 0.8);

	// 7. SHARPENING
	vec3 blurred = vec3(0.0);
	float sum = 0.0;
	// Unrolled 5x5 for ES 2.0
	float weights[25];
	weights[0]=1.0; weights[1]=4.0; weights[2]=7.0; weights[3]=4.0; weights[4]=1.0;
	weights[5]=4.0; weights[6]=16.0;weights[7]=26.0;weights[8]=16.0;weights[9]=4.0;
	weights[10]=7.0;weights[11]=26.0;weights[12]=41.0;weights[13]=26.0;weights[14]=7.0;
	weights[15]=4.0;weights[16]=16.0;weights[17]=26.0;weights[18]=16.0;weights[19]=4.0;
	weights[20]=1.0;weights[21]=4.0; weights[22]=7.0; weights[23]=4.0; weights[24]=1.0;

	for(int i = -2; i <= 2; i++) {
		for(int j = -2; j <= 2; j++) {
			float w = weights[(i+2)*5 + (j+2)];
			blurred += texture2D(rendered, uv + vec2(float(i), float(j)) * texelSize0).rgb * w;
			sum += w;
		}
	}
	blurred /= sum;
	color.rgb = mix(color.rgb, blurred, -SHARPNESS);

#ifdef INTERLACING
	float interlace_mask = fract(gl_FragCoord.y * 0.25) > 0.5 ? 1.0 : 0.0;
	if (fract(animationTimer * INTERLACE_FPS) > 0.5) {
		interlace_mask = 1.0 - interlace_mask;
	}
	vec4 prevCol = texture2D(prevFrame, uv);
	color.rgb = mix(color.rgb, prevCol.rgb, interlace_mask * 0.5);
#endif

	gl_FragColor = vec4(color.rgb, 1.0);
}
