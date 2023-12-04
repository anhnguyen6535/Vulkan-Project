### How to build
I just follow the setup by default:

1. Clone this repository using the https address and switch to the HW4 branch.
git clone <https address>


2. In your project directory:
mkdir build
cd build

3.
cmake ..

4.
source /home/share/gfx/vulkanSDK/1.3.261.1/setup-env.sh


### How to build
in the build folder:
make -j
HW4/HW4


### How to test
Use right arrow to rotate and orbit forward
Use left arrow to rotate and orbit backward

### Requirement 1: Spheres and Texture Coordinates
- Created a sphere struct contains: necessary information for each sphere
- Set background to starry texture
- Sphere.position of each sphere corresponds to:
	+ Sun: 0, 0, 0   
	+ Earth: 2.5, 0, 0
	+ Moon: 3, 0, 0
- Sphere.scale is its radius

### Requirement 2: Transformations and Control
- Change the center of earth and moon in circular such that:
	+ center of circular for earth is sun
	+ center of circular for moon is earth
- Make self rotation by adding angle to theta 
- Sort sphere by distance and render the furthest first

### Requirement 3: 
- Light source from sun, only add shading if spheres.textureIndex != 0 which are sun and moon
