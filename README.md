# ChiselEngine
Chisel Engine is an OpenXR Game Engine that is being developed for my Steam game - Rock Life: The Rock Simulator 

## Instructions 
Since this is project is working of a Solution (.sln) file, I recommend to use Visual Studio 2022

Ensure you have the C++ Development Package Installed (You will be shown these as option in the Visual Studio Installer)

Using Visual Studio, you will need to Install the following NuGet packages: 

**OpenXR.Loader** \
**OpenXR.Headers (Should download along with OpenXR.Headers)** \
**Assimp** \
**Assimp.redist (Should download along with Assimp)**

Additionally, you will need to use your own **OBJ** file(s) in the Resources folder.\
For example: `Resources/suzanne.obj`

Before running, make sure you have an instance of OpenXR running along with a connected VR Headset or MR Device.

## Preview and Features

- OpenXR and OpenGL implemented
- Desktop Window to Display what is shown on the VR Headset via OpenXR
- Controller detection and input support

<img width="573" alt="Screenshot 2024-12-16 233921" src="https://github.com/user-attachments/assets/de5c6c8d-4527-48e8-b493-6c9620fb4ea3" />






