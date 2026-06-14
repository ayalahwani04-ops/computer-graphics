# Assignment: Virtual Cameras and Projections

## Overview

In Assignment 2, you successfully loaded a 3D model, applied mathematical transformations, and orthographically flattened it to the screen. In this assignment, we will implement a proper virtual camera system. You will explore the View matrix, replace your basic orthographic projection with a true Perspective projection, and calculate geometric normals to prepare our models for lighting.

### Part 1: Coordinate Frames and Bounding Boxes

##### Background: Visualizing Space

When manipulating 3D objects, it is incredibly easy to lose track of where the object actually is versus where its local center is. When you translate an object in the "world" frame, its local axes move with it. When you rotate it in the "local" frame, its axes spin. To debug complex transformations, graphics programmers draw helper geometry (like bounding boxes and coordinate axes) to visualize these invisible mathematical spaces.

##### Task

Implement two visual debugging features in your renderer, and add UI checkboxes to toggle them on and off:

1. **Coordinate Axes:** Draw short, colored lines (e.g., Red for X, Green for Y, Blue for Z) originating from the center of the model to represent its Local axes, and a fixed set of axes at `(0,0,0)` to represent the World axes.

2. **Bounding Box:** Calculate the 8 corners of the object's 3D bounding box. Draw the wireframe of this box.
   *Test your implementation:* Transform your model. If you transform in the model frame, the model's axes should remain fixed relative to the model. If you transform in the world frame, the model's axes should transform alongside it!

### Part 2: The Virtual Camera (View Matrix)

##### Background: The Camera Illusion

In computer graphics, a "camera" doesn't actually exist. To create the illusion of a camera moving forward into a scene, we actually move the entire 3D universe backward. This inverse transformation is called the **View Matrix**.

If a camera is positioned at $(C_x, C_y, C_z)$ and rotated by some angle, the View matrix applies the exact *opposite* translation and rotation to every vertex in the scene, effectively bringing the entire world into the camera's local coordinate space.

##### Task

Create a `Camera` object or struct. Give it position and rotation properties. Add UI sliders to control the camera's position and rotation in the world.
Construct the View matrix from these parameters (remembering to invert the transformation!) and multiply your model's vertices by this View matrix *after* the Model matrix but *before* the Projection matrix ($P \cdot V \cdot M \cdot v$). Verify that moving the camera left shifts the object to the right on your screen.

### Part 3: Perspective Projection

##### Background: The View Frustum and Perspective Divide

Orthographic projection (dropping the Z coordinate) makes architectural drafting easy, but it lacks depth—objects far away look the same size as objects close up.

A **Perspective Projection** maps a 3D truncated pyramid (the *frustum*) into a standardized 3D cube (Normalized Device Coordinates). It achieves the illusion of depth through the **Perspective Divide**: dividing the $X$ and $Y$ coordinates by the vertex's distance from the camera ($Z$ or $W$ in homogeneous coordinates). The further away a vertex is, the more its $X$ and $Y$ values are squashed toward the center of the screen.

##### Task

Use GLM (or derive the math yourself) to construct a Perspective Projection matrix. You will need to define a Field of View (FOV), an aspect ratio (based on your window size), and Near/Far clipping planes. Replace your orthographic projection with this new matrix. Add a UI button to toggle between Orthographic and Perspective modes. Load a mesh, move the camera away from it, and ensure the difference between the two projections is clearly visible.

### Part 4: Calculating Normals

##### Background: Which way is up?

To eventually calculate how light hits our object, we need to know which direction every polygon is facing. This direction is represented by a 3D unit vector called a **Normal**.

* A **Face Normal** is a single vector pointing perpendicular to the surface of a triangle.

* A **Vertex Normal** is a vector assigned to a vertex, usually calculated by averaging the face normals of all triangles sharing that vertex. This allows for smooth shading across jagged geometry.

##### Task

Write an algorithm to compute both the Face Normals and Vertex Normals for your loaded mesh. Use the cross product of the triangle's edges to find the face normal.
To verify your math is correct, implement a "Draw Normals" debug toggle in your UI. When enabled, use your `draw_line` function to draw short line segments pointing outward from the center of each face (for face normals) and from each vertex (for vertex normals). Make sure they transform correctly when you rotate the model!

### Part 5: Pair Programming Extensions

*Students working in pairs are required to complete the following extensions.*

##### 1. The "LookAt" Transformation

* **Background:** Manually adjusting camera rotations with sliders to look at an object is difficult. The `LookAt` function mathematically constructs a View matrix based on three vectors: the camera's *Position*, a *Target* point to look at, and an *Up* vector defining the camera's roll.

* **Task:** Implement a `LookAt` camera mode. Add UI input fields for a Target Coordinate $(X, Y, Z)$. Calculate the View matrix so that the camera always points perfectly at the target, regardless of where the camera is positioned.

##### 2. The Dolly Zoom (Vertigo Effect)

* **Background:** Made famous by Alfred Hitchcock, a Dolly Zoom occurs when a camera physically moves away from a subject while simultaneously zooming in (changing the Field of View) to keep the subject the exact same size on screen. This distorts the background perspective dramatically.

* **Task:** Add a single "Dolly Zoom" slider to your UI. As you drag the slider, mathematically link the camera's Z-position and the Perspective Projection's FOV so the active model remains visually stationary while the perspective distortion shifts wildly.
