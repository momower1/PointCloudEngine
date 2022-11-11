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

bool IsInsideQuad(float2 position, float2 quadTopLeft, float2 quadTopRight, float2 quadBottomRight, float2 quadBottomLeft)
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

void GetQuadQuadOverlappingPolygon(in float2 quadClockwise[4], in float2 quadClockwiseOther[4], out uint polygonVertexCount, out float2 polygonVertices[8])
{
	// Quad vertices must be given in clockwise/counterclockwise order, otherwise diagonal edges will be constructed
	float2 quadEdges[] =
	{
		quadClockwise[1] - quadClockwise[0],
		quadClockwise[2] - quadClockwise[1],
		quadClockwise[3] - quadClockwise[2],
		quadClockwise[0] - quadClockwise[3]
	};

	float2 quadEdgesOther[] =
	{
		quadClockwiseOther[1] - quadClockwiseOther[0],
		quadClockwiseOther[2] - quadClockwiseOther[1],
		quadClockwiseOther[3] - quadClockwiseOther[2],
		quadClockwiseOther[0] - quadClockwiseOther[3]
	};

	// The overlapping area of the two quads is a polygon with at most 8 vertices
	polygonVertexCount = 0;

	// Each polygon vertex is either at an intersection of two edges or it is a quad vertex that is contained inside the other quad
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			// Perform line segment intersection for all edges of the first quad against all edges of the other quad
			float2 intersection;

			if (GetLineSegmentIntersection(quadClockwise[i], quadEdges[i], quadClockwiseOther[j], quadEdgesOther[j], intersection))
			{
				polygonVertices[polygonVertexCount++] = intersection;
			}
		}

		// Also cross check if any vertex of one quad is inside the other quad and add it to the polygon
		if (IsInsideQuad(quadClockwise[i], quadClockwiseOther[0], quadClockwiseOther[1], quadClockwiseOther[2], quadClockwiseOther[3]))
		{
			polygonVertices[polygonVertexCount++] = quadClockwise[i];
		}

		if (IsInsideQuad(quadClockwiseOther[i], quadClockwise[0], quadClockwise[1], quadClockwise[2], quadClockwise[3]))
		{
			polygonVertices[polygonVertexCount++] = quadClockwiseOther[i];
		}
	}
}

void SortConvexPolygonVerticesClockwise(in uint polygonVertexCount, inout float2 polygonVertices[8])
{
	// For convex polygons this center must lie within the polygon
	float2 polygonCenter = float2(0, 0);

	for (int i = 0; i < polygonVertexCount; i++)
	{
		polygonCenter += polygonVertices[i];
	}

	polygonCenter /= polygonVertexCount;

	// Now using the center, order the vertices in counter clockwise order relative to some edge from the center to a vertex
	float2 polygonVerticesOrdered[8] = polygonVertices;
	float polygonVertexAngles[8] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

	// Just use the edge to the first vertex for sorting
	float2 edgeForAngleSorting = polygonVertices[0] - polygonCenter;
	polygonVerticesOrdered[0] = polygonVertices[0];
	polygonVertexAngles[0] = 0.0f;

	// Calculate all counter clockwise angles for each vertex (can skip initial vertex since this angle must be zero)
	for (int i = 1; i < polygonVertexCount; i++)
	{
		polygonVertexAngles[i] = GetCounterclockwiseAngle(edgeForAngleSorting, polygonVertices[i] - polygonCenter);
	}

	// Now perform simple sorting by the angle
	for (int j = 1; j < polygonVertexCount; j++)
	{
		int indexWithSmallestAngle = 1;

		for (int i = 2; i < polygonVertexCount; i++)
		{
			if (polygonVertexAngles[i] < polygonVertexAngles[indexWithSmallestAngle])
			{
				indexWithSmallestAngle = i;
			}
		}

		polygonVerticesOrdered[j] = polygonVertices[indexWithSmallestAngle];

		// No longer consider this vertex in the next iterations by setting its angle to two PI
		polygonVertexAngles[indexWithSmallestAngle] = 2 * 3.141592654f;
	}

	polygonVertices = polygonVerticesOrdered;
}