#version 450

#define PI 3.1415926535897932384626433832795
#define MAX_TEXTURES 4		// save some space in the push constants by hard-wiring this
#define MAX_SPHERES 3       // Sun, Moon, and Earth

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

// Material properties (temp remove)
// vec3 bg_color = vec3(0.00,0.00,0.05);

struct Sphere
{
    vec3 position;
    float scale;
    int textureIndex;
    float orbitPeriod;
    float orbitRadius;
    float axialSpeed;
};

vec3 getNorm(vec3 sphereCenter, float sphereRadius, vec3 point){
    vec3 norm = normalize(point - sphereCenter);

    norm /= sphereRadius;
    return norm;
}

void main() {
    
    float amStrength = 0.05f;
    float difStrength = 0.5f;
    float specRate = 0.001f;
    vec3 specIntensity = vec3(specRate, specRate, specRate);
    float spec_pow = pow(2, 0.01);
//    float spec_pow = pow(2, 2);   // on point
    vec3 lightCol = vec3(1.0f, 0.8f, 0.6f);  // Sun-like light color


    vec2 scaledCoords = (vec2(p.x, p.y)) * 50;
    color = texture(textures[0], scaledCoords);

    Sphere spheres[MAX_SPHERES];
    
    // Sun
    spheres[0] = Sphere(vec3(0.0, 0.0, 0.0), 0.5, 1, 0, 0, 270);
    // Earth
    spheres[1] = Sphere(vec3(2.5, 0.0, 0.0), 0.2, 2, 365, 2.5, 10);
    // Moon
    spheres[2] = Sphere(vec3(3.0, 0.0, 0.0), 0.1, 3, 27, 0.5, 270);

    // Calculate the position of Earth and Moon in its circular orbit 
    for(int i = 1; i < MAX_SPHERES; i++){
        float orbitSpeed = 2.0 * PI / spheres[i].orbitPeriod; 
        float orbitAngle = pc.time * orbitSpeed;
        vec3 orbitPosition = vec3(sin(orbitAngle), 0.0, cos(orbitAngle)) * spheres[i].orbitRadius;

        spheres[i].position = spheres[i-1].position + orbitPosition;;
    }

    // Sort spheres based on distance from the camera
    for (int i = 0; i < MAX_SPHERES - 1; ++i) {
        for (int j = i + 1; j < MAX_SPHERES; ++j) {
            float distanceI = length(p - spheres[i].position);
            float distanceJ = length(p - spheres[j].position);
            if (distanceI < distanceJ) {
                Sphere temp = spheres[i];
                spheres[i] = spheres[j];
                spheres[j] = temp;
            }
        }
    }

    // Intersect against spheres
    for (int i = 0; i < MAX_SPHERES; ++i)
    {
        vec3 dir = normalize(d);
        vec3 p2 = (p - spheres[i].position);

        float a = dot(dir, dir);
        float prod = 2.0 * dot(p2, dir);
//        float normp = length(p2);
        float c = dot(p2, p2) - spheres[i].scale * spheres[i].scale;

        float discriminant = prod * prod - 4.0 * a * c;
//        float discriminant = prod * prod - 4.0 * (-1.0 + normp * normp);

        if (discriminant >= 0.0)
        {
            // Determine intersection point
            float t1 = 0.5 * (-prod - sqrt(discriminant)) / a;
            float t2 = 0.5 * (-prod + sqrt(discriminant)) / a;

            float t;
            float tmin = min(t1, t2);
            float tmax = max(t1, t2);

            if (tmax > 0.0)
            {
                t = (tmin > 0) ? tmin : tmax;
                vec3 ipoint = p + t * (dir);
                vec3 normal = normalize(ipoint - spheres[i].position) ;

                // Determine texture coordinates in spherical coordinates

                // First rotate about x through 90 degrees so that y is up.
                normal.z = -normal.z;
                normal = normal.xzy;

                float phi = acos(normal.z);
                float theta;

                if (abs(normal.x) < 0.001)
                {
                    theta = sign(normal.y) * PI * 0.5;
                }
                else
                {
                    theta = atan(normal.y, normal.x);
                }

                // find axial angle 
                float angle = -pc.time / spheres[i].axialSpeed;

                // Adjust texture coordinates based on sphere's position and scale
                vec2 sphereCoords = vec2(angle + 0.5 * theta / PI, phi / PI);

                color = texture(textures[spheres[i].textureIndex], sphereCoords);

                //***** LIGHT ******//

                if(spheres[i].textureIndex != 1){
                    vec3 lightPosition = vec3(0.0f, 0.0f, 0.0f);
                    vec3 lightDir = normalize(lightPosition - spheres[i].position);

                    // spec //
                    vec3 viewDir = normalize(spheres[i].position - p);      // view direction
                    vec3 worldDir = getNorm(spheres[i].position, spheres[i].scale, ipoint);   // normal

                    vec3 perfectReflect = reflect(lightDir, worldDir);   // perfectly reflection direction

                    float angleIn = dot(worldDir, lightDir);
                    float angleOut = dot(viewDir, perfectReflect);

                    if (angleIn < 0) {
                        angleIn = 0.0f;
                        angleOut = 0.0f;
                    }
                    if (angleOut < 0) {
                        angleOut = 0.0f;
                    }
                    vec3 spec = pow(angleOut, spec_pow) * specIntensity;

                    // diffuse //
                    float difDot = max(0.0, dot(normal, lightDir));
                    vec3 diffuse = angleIn * color.xyz * difStrength;

                    // ambient //
                    vec3 ambient = amStrength * color.xyz;

                    color = vec4((diffuse + ambient + spec) * lightCol, 1 );
                }

            }
        }
    }
}
