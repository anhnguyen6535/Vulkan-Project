#version 450

#define PI 3.1415926535897932384626433832795
#define MAX_TEXTURES 4		// save some space in the push constants by hard-wiring this

layout(location = 0) out vec4 color;

// interpolated position and direction of ray in world space
layout(location = 0) in vec3 p;
layout(location = 1) in vec3 d;

//push constants block
layout( push_constant ) uniform constants
{
	mat4 invView; // camera-to-world
	vec4 proj; // (near, far, aspect, fov)
	float time;

} pc;

layout(binding = 0) uniform sampler2D textures[ MAX_TEXTURES ];

// Material properties
vec3 bg_color = vec3(0.00,0.00,0.05);
float offsetDistances[MAX_TEXTURES-1] = {0.0, 1.0, 2.0};
float scaleFactors[MAX_TEXTURES] = {0.5, 0.7, 0.8, 1.0};

void checkIntersections(int index){
    vec3 p2 = p + vec3(offsetDistances[index-1], 0, 0);
    vec3 dir = normalize(d);
    float prod = 2.0 * dot(p2,dir);
    float normp = length(p2);
    float discriminant = prod*prod -4.0*(-1.0 + normp*normp);
    color = vec4(bg_color, 1.0);

    if( discriminant >= 0.0) {
    // determine intersection point
    float t1 = 0.5 * (-prod - sqrt(discriminant));
    float t2 = 0.5 * (-prod + sqrt(discriminant));
    float tmin, tmax;
    float t;
    if(t1 < t2) {
        tmin = t1;
        tmax = t2;
    } else {
        tmin = t2;
        tmax = t1;
    }

    if(tmax > 0.0) {
        t = (tmin > 0) ? tmin : tmax;
        vec3 ipoint = p2 + t*(dir);

        vec3 normal = normalize(ipoint);

       
            
        // determine texture coordinates in spherical coordinates
            
        // First rotate about x through 90 degrees so that y is up.
        normal.z = -normal.z;
        normal = normal.xzy;

        float phi = acos(normal.z);
        float theta;
        if(abs(normal.x) < 0.001) {
            theta = sign(normal.y)*PI*0.5; 
        } else {
            theta = atan(normal.y, normal.x); 
        }

        // Offset the entire texture by adding the xOffset and yOffset
        // add vec2 to textureCoordinate for rotate
        vec2 textureCoordinates = vec2(1.0 + 0.5 * theta / PI, phi / PI);
        // normalize coordinates for texture sampling. 
        // Top-left of texture is (0,0) in Vulkan, so we can stick to spherical coordinates
            color = texture(textures[index], 
	                textureCoordinates);
    }}
    else{
        color = texture(textures[0], 
		vec2(0.5,0.5));
    }
}

void main() {
    
    // intersect against sphere of radius 1 centered at the origin
    checkIntersections(1);
    checkIntersections(2);
    checkIntersections(3);
}
