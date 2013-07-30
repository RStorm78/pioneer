#ifdef TEXTURE0
uniform sampler2D texture0; //diffuse
uniform sampler2D texture1; //specular
uniform sampler2D texture2; //glow
uniform sampler2D texture3; //pattern
uniform sampler2D texture4; //color
varying vec2 texCoord0;
#endif
#ifdef VERTEXCOLOR
varying vec4 vertexColor;
#endif
#if (NUM_LIGHTS > 0)
varying vec3 eyePos;
varying vec3 normal;
#endif

uniform Scene scene;
uniform Material material;

#if (NUM_LIGHTS > 0)
//ambient, diffuse, specular
//would be a good idead to make specular optional
void ads0(in vec3 pos, in vec3 n, inout vec4 light, inout vec4 specular)
{
	vec3 s = normalize(vec3(gl_LightSource[0].position)); //directional light
	vec3 v = normalize(vec3(-pos));
	vec3 h = normalize(v + s);
	light += gl_LightSource[0].diffuse * material.diffuse * max(dot(s, n), 0.0);
#ifdef MAP_SPECULAR
	specular += texture2D(texture1, texCoord0) * material.specular * gl_LightSource[0].diffuse * pow(max(dot(h, n), 0.0), material.shininess);
#else
	specular += material.specular * gl_LightSource[0].diffuse * pow(max(dot(h, n), 0.0), material.shininess);
#endif
	specular.a = 0.0;
	light.a = 1.0;
}
#endif // NUM_LIGHTS > 0
#if (NUM_LIGHTS > 1)
void ads1(in vec3 pos, in vec3 n, inout vec4 light, inout vec4 specular)
{
	vec3 s = normalize(vec3(gl_LightSource[1].position)); //directional light
	vec3 v = normalize(vec3(-pos));
	vec3 h = normalize(v + s);
	light += gl_LightSource[1].diffuse * material.diffuse * max(dot(s, n), 0.0);
#ifdef MAP_SPECULAR
	specular += texture2D(texture1, texCoord0) * material.specular * gl_LightSource[1].diffuse * pow(max(dot(h, n), 0.0), material.shininess);
#else
	specular += material.specular * gl_LightSource[1].diffuse * pow(max(dot(h, n), 0.0), material.shininess);
#endif
	specular.a = 0.0;
	light.a = 1.0;
}
#endif // NUM_LIGHTS > 1
#if (NUM_LIGHTS > 2)
void ads2(in vec3 pos, in vec3 n, inout vec4 light, inout vec4 specular)
{
	vec3 s = normalize(vec3(gl_LightSource[2].position)); //directional light
	vec3 v = normalize(vec3(-pos));
	vec3 h = normalize(v + s);
	light += gl_LightSource[2].diffuse * material.diffuse * max(dot(s, n), 0.0);
#ifdef MAP_SPECULAR
	specular += texture2D(texture1, texCoord0) * material.specular * gl_LightSource[2].diffuse * pow(max(dot(h, n), 0.0), material.shininess);
#else
	specular += material.specular * gl_LightSource[2].diffuse * pow(max(dot(h, n), 0.0), material.shininess);
#endif
	specular.a = 0.0;
	light.a = 1.0;
}
#endif // NUM_LIGHTS > 2
#if (NUM_LIGHTS > 3)
void ads3(in vec3 pos, in vec3 n, inout vec4 light, inout vec4 specular)
{
	vec3 s = normalize(vec3(gl_LightSource[3].position)); //directional light
	vec3 v = normalize(vec3(-pos));
	vec3 h = normalize(v + s);
	light += gl_LightSource[3].diffuse * material.diffuse * max(dot(s, n), 0.0);
#ifdef MAP_SPECULAR
	specular += texture2D(texture1, texCoord0) * material.specular * gl_LightSource[3].diffuse * pow(max(dot(h, n), 0.0), material.shininess);
#else
	specular += material.specular * gl_LightSource[3].diffuse * pow(max(dot(h, n), 0.0), material.shininess);
#endif
	specular.a = 0.0;
	light.a = 1.0;
}
#endif // NUM_LIGHTS > 3

void main(void)
{
#ifdef VERTEXCOLOR
	vec4 color = vertexColor;
#else
	vec4 color = material.diffuse;
#endif
#ifdef TEXTURE0
	color *= texture2D(texture0, texCoord0);
#endif
//patterns - simple lookup
#ifdef MAP_COLOR
	float pat = texture2D(texture3, texCoord0).r;
	vec4 mapColor = texture2D(texture4, vec2(pat, 0.0));
	color *= mapColor;
#endif

#ifdef ALPHA_TEST
	if (color.a < 0.5)
		discard;
#endif

//directional lighting
#if (NUM_LIGHTS > 0)
	vec4 light = scene.ambient +
//ambient and emissive only make sense with lighting
#ifdef MAP_EMISSIVE
		texture2D(texture2, texCoord0); //glow map
#else
		material.emission; //just emissive parameter
#endif
	vec4 specular = vec4(0.0);
//&&for (int i=0; i<NUM_LIGHTS; ++i) {
		ads0(eyePos, normal, light, specular);
#if (NUM_LIGHTS > 1)		
		ads1(eyePos, normal, light, specular);
#endif // NUM_LIGHTS > 1
#if (NUM_LIGHTS > 2)		
		ads2(eyePos, normal, light, specular);
#endif // NUM_LIGHTS > 2
#if (NUM_LIGHTS > 3)		
		ads3(eyePos, normal, light, specular);
#endif // NUM_LIGHTS > 3
//&&}
#endif //NUM_LIGHTS

#if (NUM_LIGHTS > 0)
	gl_FragColor = color * light + specular;
#else
	gl_FragColor = color;
#endif
	SetFragDepth();
}
