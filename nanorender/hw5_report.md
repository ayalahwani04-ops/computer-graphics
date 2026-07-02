# HW5 Report: Lighting, Materials, and Shading

## Part 1: Light Sources and Material Properties
I created a `PointLight` struct with position and ambient/diffuse/specular color components, and a `Material` struct with matching properties plus a shininess value. I added UI sliders to control the light's X, Y, Z position in real time. The material is set to an orange color (1.0, 0.5, 0.3) with white specular highlights. For now only the ambient component is implemented — the object looks flat but responds to material color changes.

![Part 1 - Ambient Setup](./assets/part1_ambient.png)

## Part 2: Flat Shading (Diffuse Lighting)
I implemented flat shading using Lambert's Cosine Law. For each triangle, I calculate the dot product between the face normal and the light direction vector. A higher dot product (face pointing toward light) gives a brighter color. I add this diffuse component to the ambient component.

The result shows the cube with realistic shading — faces pointing toward the light are bright orange, while faces pointing away are dark. Moving the light position sliders changes the shading in real time.

![Part 2 - Flat Shading](./assets/part2_flat_shading.png)

