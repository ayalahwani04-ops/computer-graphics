# HW3 Report: Virtual Cameras and Projections

## Part 1: Coordinate Frames and Bounding Boxes
I added two debug checkboxes to the UI — "Show Axes" and "Show Bounding Box". When Show Axes is enabled, three colored lines are drawn from the model's center: red for X, green for Y, and blue for Z. The axes transform correctly with the model — when I apply world transformations, the axes move with the cube.

![Part 1 - Axes](./assets/part1_axes.png)


## Part 2: The Virtual Camera (View Matrix)
I created a camera with position (x, y, z) and rotation (rx, ry) properties controlled by GUI sliders. The View matrix applies the inverse of the camera's transformation to all vertices — moving the camera left shifts the world right. I multiply the matrices as: View * World * Local * vertex. When I rotate the camera, the cube appears to rotate in 3D space.

![Part 2 - Camera](./assets/part2_camera.png)