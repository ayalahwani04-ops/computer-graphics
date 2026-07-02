# HW5 Report: Lighting, Materials, and Shading

## Part 1: Light Sources and Material Properties
I created a `PointLight` struct with position and ambient/diffuse/specular color components, and a `Material` struct with matching properties plus a shininess value. I added UI sliders to control the light's X, Y, Z position in real time. The material is set to an orange color (1.0, 0.5, 0.3) with white specular highlights. For now only the ambient component is implemented — the object looks flat but responds to material color changes.

![Part 1 - Ambient Setup](./assets/part1_ambient.png)

