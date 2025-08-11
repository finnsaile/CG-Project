#pragma once
/**************
	In Sample Assignment 03 you have to create the world, view and projection matrices to support a simple truck simulation application.
	The simulator, will allow the user to experience driving both in first person (from the inside of the truck), and in third person
	(looking the truck from outside). For reason that will be explained in a future lesson, an application needs both
	the world-view-projection matrix and the world matrix alone to correctly compute the colors of points of the objects.
	The simulation then requires two types of procedures: one to compute the view-projection matrices, and another to create the world matrix.
	Inside the code that supports the assignment, the application will then make the product of the two to create the required world-view-projection matrix.
	Compute the world view and projection matrices, as described below

	Since the application supports both a first and a third person view, it requires the constructions of the view matrix
	with either the look-in-direction, and the look-at models. Moreover, since all objects have been already modeled with
	the right size, scale transformation in the world matrix is not required (it can be considered always equal to 1.0).

	WARNING!

	THIS IS A SAMPLE ASSIGNMENT, YOU DO NOT NEED TO SUBMIT ANYTHING FOR THIS ASSIGNMENT!

	Since it is a C program, you can use for loops and functions if you think they can be helpful in your solution.
	However, please include all your code in this file, since it will be put in an automatic correction process
	for the final evaluation. Please also be cautious when using standard libraries and symbols, since they
	might not be available in all the development environments (especially, they might not be available
	in the final evaluation environment, preventing your code from compiling).
	This WARNING will be valid far ALL THE ASSIGNMENTs, but it will not be repeated in the following texts,
	so please remember these advices carefully!
 
***************/
inline glm::mat4 MakeViewProjectionLookInDirection(glm::vec3 Pos, float Yaw, float Pitch, float Roll, float FOVy, float Ar, float nearPlane, float farPlane) {
	// Make a View-Projection matrix, where the view matrix uses the look-in-direction model,
	// and the projection matrix supports perspective. Parameters are the following:
	// Projection:
	//	- Perspective with:
	//	- Fov-y defined in formal parameter >FOVy<
	//  - Aspect ratio defined in formal parameter >Ar<
	//  - Near Plane distance defined in formal parameter >nearPlane<
	//  - Far Plane distance defined in formal parameter >farPlane<
	// View:
	//	- Use the Look-In-Direction model with:
	//	- Camera Positon defined in formal parameter >Pos<
	//	- Looking direction defined in formal parameter >Yaw<
	//	- Looking elevation defined in formal parameter >Pitch<
	//	- Looking rool defined in formal parameter >Roll<
	glm::mat4 M = glm::mat4(1.0f);
	glm::mat4 Mprj = glm::perspective(FOVy, Ar, nearPlane, farPlane);
	Mprj[1][1] *= -1;

	glm::mat4 Mv =
		glm::rotate(M, -Roll, glm::vec3(0,0,1)) *
		glm::rotate(M, -Pitch, glm::vec3(1,0,0)) *
		glm::rotate(M, -Yaw, glm::vec3(0,1,0)) *
		glm::translate(M, -Pos);

	M = Mprj * Mv;

	return M;
}

inline glm::mat4 MakeViewProjectionLookAt(glm::vec3 Pos, glm::vec3 Target, glm::vec3 Up, float Roll, float FOVy, float Ar, float nearPlane, float farPlane) {
	// Make a View-Projection matrix, where the view matrix uses the look-at model,
	// and the projection matrix supports perspective. Parameters are the following:
	// Projection:
	//	- Perspective with:
	//	- Fov-y defined in formal parameter >FOVy<
	//  - Aspect ratio defined in formal parameter >Ar<
	//  - Near Plane distance defined in formal parameter >nearPlane<
	//  - Far Plane distance defined in formal parameter >farPlane<
	// View:
	//	- Use the Look-At model with:
	//	- Camera Positon defined in formal parameter >Pos<
	//	- Camera Target defined in formal parameter >Target<
	//	- Up vector defined in formal parameter >Up<
	//	- Looking rool defined in formal parameter >Roll<
	glm::mat4 M = glm::mat4(1.0f);
	glm::mat4 Mprj = glm::perspective(FOVy, Ar, nearPlane, farPlane);
	Mprj[1][1] *= -1;

	glm::vec3 vz = glm::normalize(Pos - Target);
	glm::vec3 vx = glm::normalize(glm::cross(Up, vz));
	glm::vec3 vy = glm::cross(vz, vx);
	glm::mat4 Mc = glm::mat4(glm::vec4(vx, 0.f),
		glm::vec4(vy, 0.f),
		glm::vec4(vz, 0.f),
		glm::vec4(Pos, 1.f));

	glm::mat4 Mv = glm::inverse(Mc);
	// Alternative glm function
    //	Mv = glm::lookAt(Pos, Target, Up);
	M = Mprj * Mv;
	return M;
}

inline glm::mat4 MakeWorld(glm::vec3 Pos, float Yaw, float Pitch, float Roll) {
	// Make a World matrix, where rotation is specified using Euler’s angles,
	// with the zxy convention (z for roll, x for pitch and y for yaw).
	// Scaling is considered always equal to 1, and thus it is not passed to the procedure.
	// Parameters are the following:
	//	- Object Positon defined in formal parameter >Pos<
	//	- Euler angle rotation yaw defined in formal parameter >Yaw<
	//	- Euler angle rotation pitch defined in formal parameter >Pitch<
	//	- Euler angle rotation roll defined in formal parameter >Roll<
	//  - Scaling constant and equal to 1 (and not passed to the procedure)
	glm::mat4 M = glm::mat4(1.0f);

	glm::mat4 Mw =
		glm::translate(M, Pos) *
        glm::rotate(M, Yaw, glm::vec3(0, 1, 0)) *
        glm::rotate(M, Pitch, glm::vec3(1, 0, 0)) *
        glm::rotate(M, Roll, glm::vec3(0,0,1));

	return Mw;
}
