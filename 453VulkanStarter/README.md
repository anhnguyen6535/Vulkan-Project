# 453HW4Starter

This repository contains starter code that you can use for homework assignments in CPSC 453 (Introduction to Computer Graphics) at the University of Calgary.

It is based on based on [VulkanLaunchpad](https://github.com/cg-tuwien/VulkanLaunchpad) from TU Wien, a framework targeted at those learning Vulkan. It abstracts some of the hard and overly verbose parts of the Vulkan C API.

## Setup

1. Clone this repository using the ```https``` address and switch to the `HW4` branch. 
    ```
    git clone <https address>
    ```
2. Optionally, in your project directory, create a new folder called build and change into it:
    ```
    mkdir build
    cd build
    ```
3. Use cmake to generate the necessary files for compiling the project. Please review the [Setup Instructions](https://github.com/cg-tuwien/VulkanLaunchpad#setup-instructions) for `VulkanLaunchpadStarter`` to setup your IDE and build environment. For this step, you can either use cmake-gui or run the cmake command directly in the terminal:
    ```
    cmake ..
    ```
4. For lab computers to setup vulkan sdk env use:
    ```
    source /home/share/gfx/vulkanSDK/1.3.261.1/setup-env.sh
    ```
5. You can now compile and run your project. In most cases, this is as simple as:
    ```
    make -j
    HW4/HW4
    ```

The main executable takes only one optional argument on the command line: the path to the assets it needs.
  This only needs to be invoked if the program is somehow unable to find the assets directory, which is unlikely to occur 
  during normal development.

## Troubleshooting

- Make sure that Vulkan SDK is installed correctly. You can verify this by running the vkCube application. (Installed automatically when you install Vulkan SDK)

- For Linux, you need to install build-essentials.

- For windows, ensure that the c++ desktop development kit is installed inside visual studio. You can use developer command prompt in windows.

- For macOS, you must have the latest version of OS and Xcode installed.

## Available Examples and Skeletons

This branch consists of the following:
- `/HW4`:

Starter code for HW4 which is a functioning example that renders a full screen quad and uses it to generate rays for 
  ray tracing in the fragment shader. A fragment shader is provided that intersects rays with a unit sphere located at the origin. 
  An arcball camera is integrated so that the user can move the camera around the ray-traced sphere using the mouse. 
  Texture setup code is also provided and all the textures you need are properly set up, with the fragment shader demonstrating
  how to access all of them.

Shader hot-reloaing is supported via `F5`. Increment and decrement the current time by using the left and right arrow keys.
  The demo fragment shader uses the time variable to cycle between all the loaded textures; you will want to change this in your
  own implementation.

The starter code has been set up so that most students will be able to fulfill all the basic requirements by modifying a single
  file, the fragment shader. You may alter the starter if you wish, but bear in mind that the sole input of mechanical orreries 
  is a crank that can be turned backwards and forwards; all orbital mechanics were handled internally, with no external assistance. 
  Adding transformations beyond the view transform should not be necessary, and could cost you marks. Any variable you add should
  only deal with cosmetic details, such as toggling shading on or off.
  
