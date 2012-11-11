struct GSIn {
	float3 PositionIndex : POSITIONINDEX;
	float4 GlyphColor : GLYPHCOLOR;
};
	
GSIn VS(GSIn Input) {
	return Input;
}
	