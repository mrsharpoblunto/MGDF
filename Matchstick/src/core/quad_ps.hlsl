struct VS_Output
{
    float4 Position : SV_POSITION;
};

cbuffer ColorBuffer : register(cb0)
{
	float4 Color;
}

float4 main(VS_Output output) : SV_TARGET
{
	return Color;
}