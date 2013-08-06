uniform float time;

void main()
{
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;

    vec4 eyePosition = gl_ModelViewMatrix * gl_Vertex;
    vec4 eyeLightPos = gl_LightSource[0].position;

    vec3 eyeNormalVec = normalize(gl_NormalMatrix * gl_Normal);
    vec3 eyeLightVec = normalize(eyeLightPos.xyz - eyePosition.xyz);
    vec3 eyeViewVec = -normalize(eyePosition.xyz);
    vec3 eyeHalfVec = normalize(eyeLightVec + eyeViewVec);

    float Kd = max(dot(eyeLightVec, eyeNormalVec), 0.0);
    float Ks = max(pow(dot(eyeNormalVec, eyeHalfVec),
    	gl_FrontMaterial.shininess), 0.0);
    float Ka = 1.0;

    gl_FrontColor = Kd * gl_FrontLightProduct[0].diffuse +
                Ks * gl_FrontLightProduct[0].specular +
            Ka * gl_FrontLightModelProduct.sceneColor;
}