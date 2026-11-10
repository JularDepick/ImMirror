#include "Headers/header.hlsl"

float4x4 g_Proj;
float4 g_SunColor;

struct VS_INPUT
{
    float3 pos        : POSITION;
    float2 uv        : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 pos : POSITION;
	float4 diffuse : COLOR0;
	float2 uv  : TEXCOORD0;	
};

VS_OUTPUT VSMain(VS_INPUT input)
{
	VS_OUTPUT output;

	output.pos = mul(float4(input.pos,1.0), g_Proj);
	output.pos=output.pos.xyzz;
	output.diffuse = g_SunColor;
	output.uv = input.uv;
	
	return output;
}

TEXTURE2D(g_SunTex);    SAMPLER2D(s_SunSampler);


float4 PSMain(VS_OUTPUT input):SV_Target
{
	float4 color= SAMPLE_TEXTURE2D(g_SunTex,s_SunSampler,input.uv);
	color.rgb=color.rgb* input.diffuse.rgb;
	return color;
}
