#include "Headers/header.hlsl"



float   g_SkyHeight;
float   g_DayTime;
float3  g_SunDirect;
float4  g_SunColor;


struct VS_INPUT
{
    float3 pos        : POSITION;
    float3 normal        : NORMAL;
    float2 staruv        : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 position : POSITION;
	float4 diffuse : COLOR0;
	float2 uv0 : TEXCOORD0;
	float2 uv1 : TEXCOORD1;
};

VS_OUTPUT VSMain(VS_INPUT input)
{
	VS_OUTPUT output;

	output.position = mul(float4(input.pos+g_EyePos,1.0), g_WorldViewProj);
	output.position=output.position.xyww;
	float t = (dot(g_SunDirect, input.normal) + 1.1f) * 0.5;
	output.diffuse.rgb = t * t * g_SunColor.rgb;
	output.diffuse.a = g_SunColor.a;
	output.uv0 = float2(g_DayTime, 1.0 - input.pos.y/g_SkyHeight);
	output.uv1 = input.staruv;
	
	return output;
}


TEXTURE2D(g_SkyTex);    SAMPLER2D(s_SkySampler);
TEXTURE2D(g_StarTex);   SAMPLER2D(s_StarSampler);


float4  g_SkyModColor;

float4 PSMain(VS_OUTPUT input):SV_Target
{

	float4 color1 = SAMPLE_TEXTURE2D(g_SkyTex,s_SkySampler,input.uv0);
	float4 color2 = SAMPLE_TEXTURE2D(g_StarTex,s_StarSampler,input.uv1) * g_SkyModColor.a;
	float4 color;
	color.rgb = (color1.rgb + input.diffuse.rgb + input.diffuse.a*color2.rgb) * g_SkyModColor.rgb;
	color.a = 1.0;
	return color;
}
