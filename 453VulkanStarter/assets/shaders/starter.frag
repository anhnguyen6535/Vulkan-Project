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

void main() {
    
    // intersect against sphere of radius 1 centered at the origin

    vec3 dir = normalize(d);

    float prod = 2.0 * dot(p,dir);
    float normp = length(p);
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
            vec3 ipoint = p + t*(dir);
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
            // normalize coordinates for texture sampling. 
            // Top-left of texture is (0,0) in Vulkan, so we can stick to spherical coordinates
             color = texture(textures[ int(mod(pc.time,MAX_TEXTURES)) ], 
		vec2(1.0+0.5*theta/PI, phi/PI ));
        }
    }
}
