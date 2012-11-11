struct VS_Input
{
    float3 Position : POSITION0;
};

struct VS_Output
{
    float4 Position : SV_POSITION;
};

VS_Output main(VS_Input input)
{
    VS_Output output;
    output.Position = float4(input.Position,1.0f);
    return output;
}