# RobotArm
vcpkg installation - https://github.com/microsoft/vcpkg/blob/master/README.md

cmake installation - https://cmake.org/install/

Demo - Demo.webm file

## Project Description - Robot Arm Simulation

### Task 1: Grid

Draw an integer grid on the Y=0-plane for the rectangle (-5,0,-5) to (+5,0,+5).  
Draw the positive X axis in red, the Y axis in green and the Z axis in blue. Only draw the positive portion of each axis, of length 5.

### Task 2: Camera Rotations

-   Use  **Perspective**  projection
-   Place the camera so you can see the whole scene: use glm::LookAt to generate the View matrix.
-   Key  C  selects the camera.
-   Keys  **←**  and  **→**  move the camera along the blue circle parallel to the equator.
-   Keys  **↑**  and  **↓**  rotate the camera along the red circle orthogonal to the equator.
-   Choose the "up" direction so the camera always points to the origin.
    
   ### Task 3: Draw the robot arm

   The robot arm consists of the following parts:
    
   -   Base: truncated tetrahedron placed on the x-z-plane
    -   Top: icosahedron placed on top of and slightly penetrating the Base.
    -   Arm1: rectangular box emmanating from Top and hinged at the center of Top.
    -   Joint: dodecahedron of appropriate radius at the other end of Arm1.
    -   Arm2: cylinder connected to the center of Joint.
    -   Pen: truncated octahedron connected to the other end of Arm2.
    -   Button: a small box on Pen.
    

    
   ### Task 4: Keyboard interaction
      
   Write the code to move the robot arm, rotate the top, rotate the arms and the pen, and twist the pen using the keyboard, as explained below:
   
   -   **Pen**: Select the pen using key  p. The pen should  _rotate_  when the arrow keys are pressed.  **←**,  **→**,  **↑**  and  **↓**  are longitude (J4) and latitude (J5) rotations. (Note that one end is always connected to  _arm2_).  shift+**←**  and  shift+**→**  should twist the pen around its axis (J6) (including buttons).
   -   **Base**: Select the base using key  b. The whole model should  _slide_  on the XZ plane according to the arrow keys.
   -   **Top** : Select the top using key  t. The top, arms and pen should  _rotate_  about the Y direction when pressing the left or right arrow keys (J1).
   -   **Arm1**: Select arm1 using key  1. The arm (and the other connected arm and pen) should  _rotate_  up and down when using the arrow keys (one end is fixed at the center of the  _top_  green cylinder) (J2).
   -   **Arm2**: Select  _Arm2_  using key  2. The arm (and pen) should  _rotate_  up and down when using the arrow keys (one end is fixed at the center of the  _joint_) (J3).
   
   Indicate the selected part by drawing it in a brighter color.
   
   ### Task 5: Light up the scene   
-   Add two lights to the scene.  
   For each light, supply position, diffuse color, ambient color and specular color.  
   Position the lights near the camera so that one light comes from the left and another one from the right.  
   You are free to choose any light colors and positions as long as the scene looks good.
-   Set diffuse and ambient material of the objects to the color of the object. Set specular as a multiple (eg one tenth) of the diffuse color.
   
   ### Task 6: Teleporting
     
-   When  s  is pressed have a Platonic solid exit the tip of the stylus, with tangent equal to the stylus axis and derivative in length equal to the stylus length
-   The solid follows an arc according to Newton's law under gravity until it hits the grid. (Hint: use the BB-form of degree 2)
-   Animate the projectile and, on impact, move the robot arm to the impact location.
