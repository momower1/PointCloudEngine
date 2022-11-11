float2 GetPerpendicularVector(float2 v)
{
	return float2(v.y, -v.x);
}

float2 FlipVectorIntoDirection(float2 v, float2 direction)
{
	return v * sign(dot(v, direction));
}

float GetSignedAngle(float2 u, float2 v)
{
	// Returns either clockwise or counter-clockwise signed angle in [-PI, PI] depending on coordinate system
	return atan2(u.x * v.y - u.y * v.x, u.x * v.x + u.y * v.y);
}

float GetCounterclockwiseAngle(float2 u, float2 v)
{
	// Returns counterclockwise angle in [0, 2 * PI]
	float angle = GetSignedAngle(u, v);
	return (angle < 0) ? (2 * 3.141592654f + angle) : angle;
}

float GetTriangleArea(float2 t1, float2 t2, float2 t3)
{
	// Heron's formula
	float a = length(t2 - t1);
	float b = length(t3 - t1);
	float c = length(t3 - t2);
	float s = (a + b + c) / 2;

	return sqrt(s * (s - a) * (s - b) * (s - c));
}

float2 GetLineLineIntersection(float2 o1, float2 d1, float2 o2, float2 d2)
{
	// Assume that the lines are not parallel (otherwise division by zero)
	// Both lines have origin and direction, calculate distance along second line to the intersection point
	float lambda2 = (d1.x * o2.y - d1.x * o1.y - d1.y * o2.x + d1.y * o1.x) / (d1.y * d2.x - d1.x * d2.y);

	return o2 + lambda2 * d2;
}

bool GetLineSegmentIntersection(float2 o1, float2 d1, float2 o2, float2 d2, out float2 intersection)
{
	// Assume that the lines are not parallel (otherwise division by zero)
	// Both lines have origin and direction, calculate distance along second line to the intersection point
	// Need to explicitly calculate both lambdas, otherwise there is a configuration where one of them is incorrect
	float lambda1 = (d2.x * o1.y - d2.x * o2.y - d2.y * o1.x + d2.y * o2.x) / (d2.y * d1.x - d2.x * d1.y);
	float lambda2 = (d1.x * o2.y - d1.x * o1.y - d1.y * o2.x + d1.y * o1.x) / (d1.y * d2.x - d1.x * d2.y);
	intersection = o2 + lambda2 * d2;

	return (lambda1 >= 0) && (lambda1 <= 1) && (lambda2 >= 0) && (lambda2 <= 1);
}

bool IsInsideQuad(float2 position, float2 quadTopLeft, float2 quadBottomRight, float2 quadLeftNormal, float2 quadTopNormal, float2 quadRightNormal, float2 quadBottomNormal)
{
	bool isInsideTopEdge = dot(position - quadTopLeft, quadTopNormal) > 0;
	bool isInsideLeftEdge = dot(position - quadTopLeft, quadLeftNormal) > 0;
	bool isInsideRightEdge = dot(position - quadBottomRight, quadRightNormal) > 0;
	bool isInsideBottomEdge = dot(position - quadBottomRight, quadBottomNormal) > 0;

	return isInsideTopEdge && isInsideLeftEdge && isInsideRightEdge && isInsideBottomEdge;
}

bool IsInsideQuad(float2 position, float2 quadTopLeft, float2 quadTopRight, float2 quadBottomLeft, float2 quadBottomRight)
{
	// Construct quad edges
	float2 eLeft = quadTopLeft - quadBottomLeft;
	float2 eRight = quadTopRight - quadBottomRight;
	float2 eTop = quadTopRight - quadTopLeft;
	float2 eBottom = quadBottomRight - quadBottomLeft;

	// Get normal vector perpendicular to each edge
	float2 nLeft = GetPerpendicularVector(eLeft);
	float2 nRight = GetPerpendicularVector(eRight);
	float2 nTop = GetPerpendicularVector(eTop);
	float2 nBottom = GetPerpendicularVector(eBottom);

	// Make sure that the edge normals point towards the quad center
	float2 center = (quadTopLeft + quadTopRight + quadBottomLeft + quadBottomRight) / 4.0f;
	nLeft = FlipVectorIntoDirection(nLeft, center - quadTopLeft);
	nRight = FlipVectorIntoDirection(nRight, center - quadTopRight);
	nTop = FlipVectorIntoDirection(nTop, center - quadTopRight);
	nBottom = FlipVectorIntoDirection(nBottom, center - quadBottomRight);

	// Check on which side of each quad edge the point lies
	bool isInsideLeftEdge = dot(position - quadTopLeft, nLeft) > 0;
	bool isInsideRightEdge = dot(position - quadBottomRight, nRight) > 0;
	bool isInsideTopEdge = dot(position - quadTopLeft, nTop) > 0;
	bool isInsideBottomEdge = dot(position - quadBottomRight, nBottom) > 0;

	// The point can only be inside the quad if it is inside all edges
	return isInsideLeftEdge && isInsideRightEdge && isInsideTopEdge && isInsideBottomEdge;
}

bool IsInsideTriangle(float2 position, float2 t1, float2 t2, float2 t3)
{
	// Construct triangle edges
	float2 e12 = t2 - t1;
	float2 e13 = t3 - t1;
	float2 e23 = t3 - t2;

	// Get normal vector perpendicular to each edge
	float2 n12 = GetPerpendicularVector(e12);
	float2 n13 = GetPerpendicularVector(e13);
	float2 n23 = GetPerpendicularVector(e23);

	// Make sure that the edge normals point towards the triangle center
	float2 center = (t1 + t2 + t3) / 3.0f;
	n12 = FlipVectorIntoDirection(n12, center - t1);
	n13 = FlipVectorIntoDirection(n13, center - t1);
	n23 = FlipVectorIntoDirection(n23, center - t2);

	// Check on which side of each triangle edge the point lies
	bool isInsideEdge12 = dot(position - t1, n12) > 0;
	bool isInsideEdge13 = dot(position - t1, n13) > 0;
	bool isInsideEdge23 = dot(position - t3, n23) > 0;

	// The point can only be inside the triangle if it is inside all edges
	return isInsideEdge12 && isInsideEdge13 && isInsideEdge23;
}