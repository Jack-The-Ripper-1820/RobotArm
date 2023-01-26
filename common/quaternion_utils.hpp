#ifndef QUATERNION_UTILS_H
#define QUATERNION_UTILS_H

using namespace glm;

glm::quat RotationBetweenVectors(vec3 start, vec3 dest);

glm::quat LookAt(vec3 direction, vec3 desiredUp);

glm::quat RotateTowards(quat q1, quat q2, float maxAngle);


#endif // QUATERNION_UTILS_H