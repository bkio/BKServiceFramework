// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WVector
#define Pragma_Once_WVector

#include "WEngine.h"
#include "WVector2D.h"
#include "WString.h"
#include "WMath.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ClangTidyInspection"

// Generic axis enum (mirrored for property use in Object.h)
namespace EAxis
{
    enum Type
    {
        None,
        X,
        Y,
        Z,
    };
}

// Extended axis enum for more specialized usage
namespace EAxisList
{
    enum Type
    {
        None		= 0,
        X			= 1,
        Y			= 2,
        Z			= 4,

        Screen		= 8,
        XY			= X | Y,
        XZ			= X | Z,
        YZ			= Y | Z,
        XYZ			= X | Y | Z,
        All			= XYZ | Screen,

        //alias over Axis YZ since it isn't used when the z-rotation widget is being used
                ZRotation	= YZ,

        // alias over Screen since it isn't used when the 2d translate rotate widget is being used
                Rotate2D	= Screen,
    };
}

/**
 * A vector in 3-D space composed of components (X, Y, Z) with floating point precision.
 */
struct FVector
{
public:

    /** Vector's X component. */
    float X;

    /** Vector's Y component. */
    float Y;

    /** Vector's Z component. */
    float Z;

public:

    /** A zero vector (0,0,0) */
    static const FVector ZeroVector;

    /** One vector (1,1,1) */
    static const FVector OneVector;

    /** World up vector (0,0,1) */
    static const FVector UpVector;

    /** Unreal forward vector (1,0,0) */
    static const FVector ForwardVector;

    /** Unreal right vector (0,1,0) */
    static const FVector RightVector;

public:

    void DiagnosticCheckNaN() const {}
    void DiagnosticCheckNaN(const UTFCHAR* Message) const {}

    /** Default constructor (no initialization). */
    FVector();

    /**
     * Constructor initializing all components to a single float value.
     *
     * @param InF Value to set all components to.
     */
    explicit FVector(float InF);

    /**
     * Constructor using initial values for each component.
     *
     * @param InX X Coordinate.
     * @param InY Y Coordinate.
     * @param InZ Z Coordinate.
     */
    FVector(float InX, float InY, float InZ);

    /**
     * Constructs a vector from an FVector2D and Z value.
     *
     * @param V Vector to copy from.
     * @param InZ Z Coordinate.
     */
    explicit FVector(FVector2D V, float InZ);

    /**
     * Calculate cross product between this and another vector.
     *
     * @param V The other vector.
     * @return The cross product.
     */
    FVector operator^(const FVector& V) const;

    /**
     * Calculate the cross product of two vectors.
     *
     * @param A The first vector.
     * @param B The second vector.
     * @return The cross product.
     */
    static FVector CrossProduct(const FVector& A, const FVector& B);

    /**
     * Calculate the dot product between this and another vector.
     *
     * @param V The other vector.
     * @return The dot product.
     */
    float operator|(const FVector& V) const;

    /**
     * Calculate the dot product of two vectors.
     *
     * @param A The first vector.
     * @param B The second vector.
     * @return The dot product.
     */
    static float DotProduct(const FVector& A, const FVector& B);

    /**
     * Gets the result of component-wise addition of this and another vector.
     *
     * @param V The vector to add to this.
     * @return The result of vector addition.
     */
    FVector operator+(const FVector& V) const;

    /**
     * Gets the result of component-wise subtraction of this by another vector.
     *
     * @param V The vector to subtract from this.
     * @return The result of vector subtraction.
     */
    FVector operator-(const FVector& V) const;

    /**
     * Gets the result of subtracting from each component of the vector.
     *
     * @param Bias How much to subtract from each component.
     * @return The result of subtraction.
     */
    FVector operator-(float Bias) const;

    /**
     * Gets the result of adding to each component of the vector.
     *
     * @param Bias How much to add to each component.
     * @return The result of addition.
     */
    FVector operator+(float Bias) const;

    /**
     * Gets the result of scaling the vector (multiplying each component by a value).
     *
     * @param Scale What to multiply each component by.
     * @return The result of multiplication.
     */
    FVector operator*(float Scale) const;

    /**
     * Gets the result of dividing each component of the vector by a value.
     *
     * @param Scale What to divide each component by.
     * @return The result of division.
     */
    FVector operator/(float Scale) const;

    /**
     * Gets the result of component-wise multiplication of this vector by another.
     *
     * @param V The vector to multiply with.
     * @return The result of multiplication.
     */
    FVector operator*(const FVector& V) const;

    /**
     * Gets the result of component-wise division of this vector by another.
     *
     * @param V The vector to divide by.
     * @return The result of division.
     */
    FVector operator/(const FVector& V) const;

    // Binary comparison operators.

    /**
     * Check against another vector for equality.
     *
     * @param V The vector to check against.
     * @return true if the vectors are equal, false otherwise.
     */
    bool operator==(const FVector& V) const;

    /**
     * Check against another vector for inequality.
     *
     * @param V The vector to check against.
     * @return true if the vectors are not equal, false otherwise.
     */
    bool operator!=(const FVector& V) const;

    /**
     * Check against another vector for equality, within specified error limits.
     *
     * @param V The vector to check against.
     * @param Tolerance Error tolerance.
     * @return true if the vectors are equal within tolerance limits, false otherwise.
     */
    bool Equals(const FVector& V, float Tolerance=KINDA_SMALL_NUMBER) const;

    /**
     * Checks whether all components of this vector are the same, within a tolerance.
     *
     * @param Tolerance Error tolerance.
     * @return true if the vectors are equal within tolerance limits, false otherwise.
     */
    bool AllComponentsEqual(float Tolerance=KINDA_SMALL_NUMBER) const;

    /**
     * Get a negated copy of the vector.
     *
     * @return A negated copy of the vector.
     */
    FVector operator-() const;

    /**
     * Adds another vector to this.
     * Uses component-wise addition.
     *
     * @param V Vector to add to this.
     * @return Copy of the vector after addition.
     */
    FVector operator+=(const FVector& V);

    /**
     * Subtracts another vector from this.
     * Uses component-wise subtraction.
     *
     * @param V Vector to subtract from this.
     * @return Copy of the vector after subtraction.
     */
    FVector operator-=(const FVector& V);

    /**
     * Scales the vector.
     *
     * @param Scale Amount to scale this vector by.
     * @return Copy of the vector after scaling.
     */
    FVector operator*=(float Scale);

    /**
     * Divides the vector by a number.
     *
     * @param V What to divide this vector by.
     * @return Copy of the vector after division.
     */
    FVector operator/=(float V);

    /**
     * Multiplies the vector with another vector, using component-wise multiplication.
     *
     * @param V What to multiply this vector with.
     * @return Copy of the vector after multiplication.
     */
    FVector operator*=(const FVector& V);

    /**
     * Divides the vector by another vector, using component-wise division.
     *
     * @param V What to divide vector by.
     * @return Copy of the vector after division.
     */
    FVector operator/=(const FVector& V);

    /**
     * Gets specific component of the vector.
     *
     * @param Index the index of vector component
     * @return reference to component.
     */
    float& operator[](int32 Index);

    /**
     * Gets specific component of the vector.
     *
     * @param Index the index of vector component
     * @return Copy of the component.
     */
    float operator[](int32 Index)const;

    /**
    * Gets a specific component of the vector.
    *
    * @param Index The index of the component required.
    *
    * @return Reference to the specified component.
    */
    float& Component(int32 Index);

    /**
    * Gets a specific component of the vector.
    *
    * @param Index The index of the component required.
    * @return Copy of the specified component.
    */
    float Component(int32 Index) const;


    /** Get a specific component of the vector, given a specific axis by enum */
    float GetComponentForAxis(EAxis::Type Axis) const;

    /** Set a specified componet of the vector, given a specific axis by enum */
    void SetComponentForAxis(EAxis::Type Axis, float Component);

public:

    // Simple functions.

    /**
     * Set the values of the vector directly.
     *
     * @param InX New X coordinate.
     * @param InY New Y coordinate.
     * @param InZ New Z coordinate.
     */
    void Set(float InX, float InY, float InZ);

    /**
     * Get the maximum value of the vector's components.
     *
     * @return The maximum value of the vector's components.
     */
    float GetMax() const;

    /**
     * Get the maximum absolute value of the vector's components.
     *
     * @return The maximum absolute value of the vector's components.
     */
    float GetAbsMax() const;

    /**
     * Get the minimum value of the vector's components.
     *
     * @return The minimum value of the vector's components.
     */
    float GetMin() const;

    /**
     * Get the minimum absolute value of the vector's components.
     *
     * @return The minimum absolute value of the vector's components.
     */
    float GetAbsMin() const;

    /** Gets the component-wise min of two vectors. */
    FVector ComponentMin(const FVector& Other) const;

    /** Gets the component-wise max of two vectors. */
    FVector ComponentMax(const FVector& Other) const;

    /**
     * Get a copy of this vector with absolute value of each component.
     *
     * @return A copy of this vector with absolute value of each component.
     */
    FVector GetAbs() const;

    /**
     * Get the length (magnitude) of this vector.
     *
     * @return The length of this vector.
     */
    float Size() const;

    /**
     * Get the squared length of this vector.
     *
     * @return The squared length of this vector.
     */
    float SizeSquared() const;

    /**
     * Get the length of the 2D components of this vector.
     *
     * @return The 2D length of this vector.
     */
    float Size2D() const ;

    /**
     * Get the squared length of the 2D components of this vector.
     *
     * @return The squared 2D length of this vector.
     */
    float SizeSquared2D() const ;

    /**
     * Checks whether vector is near to zero within a specified tolerance.
     *
     * @param Tolerance Error tolerance.
     * @return true if the vector is near to zero, false otherwise.
     */
    bool IsNearlyZero(float Tolerance=KINDA_SMALL_NUMBER) const;

    /**
     * Checks whether all components of the vector are exactly zero.
     *
     * @return true if the vector is exactly zero, false otherwise.
     */
    bool IsZero() const;

    /**
     * Normalize this vector in-place if it is larger than a given tolerance. Leaves it unchanged if not.
     *
     * @param Tolerance Minimum squared length of vector for normalization.
     * @return true if the vector was normalized correctly, false otherwise.
     */
    bool Normalize(float Tolerance=SMALL_NUMBER);

    /**
     * Checks whether vector is normalized.
     *
     * @return true if Normalized, false otherwise.
     */
    bool IsNormalized() const;

    /**
     * Util to convert this vector into a unit direction vector and its original length.
     *
     * @param OutDir Reference passed in to store unit direction vector.
     * @param OutLength Reference passed in to store length of the vector.
     */
    void ToDirectionAndLength(FVector &OutDir, float &OutLength) const;

    /**
     * Get a copy of the vector as sign only.
     * Each component is set to +1 or -1, with the sign of zero treated as +1.
     *
     * @param A copy of the vector with each component set to +1 or -1
     */
    FVector GetSignVector() const;

    /**
     * Projects 2D components of vector based on Z.
     *
     * @return Projected version of vector based on Z.
     */
    FVector Projection() const;

    /**
     * Calculates normalized version of vector without checking for zero length.
     *
     * @return Normalized version of vector.
     * @see GetSafeNormal()
     */
    FVector GetUnsafeNormal() const;

    /**
     * Gets a copy of this vector snapped to a grid.
     *
     * @param GridSz Grid dimension.
     * @return A copy of this vector snapped to a grid.
     * @see FMath::GridSnap()
     */
    FVector GridSnap(const float& GridSz) const;

    /**
     * Get a copy of this vector, clamped inside of a cube.
     *
     * @param Radius Half size of the cube.
     * @return A copy of this vector, bound by cube.
     */
    FVector BoundToCube(float Radius) const;

    /** Create a copy of this vector, with its magnitude clamped between Min and Max. */
    FVector GetClampedToSize(float Min, float Max) const;

    FVector ClampSize(float Min, float Max) const;

    /** Create a copy of this vector, with the 2D magnitude clamped between Min and Max. Z is unchanged. */
    FVector GetClampedToSize2D(float Min, float Max) const;

    FVector ClampSize2D(float Min, float Max) const;

    FVector GetClampedToMaxSize(float MaxSize) const;

    FVector ClampMaxSize(float MaxSize) const;

    /** Create a copy of this vector, with the maximum 2D magnitude clamped to MaxSize. Z is unchanged. */
    FVector GetClampedToMaxSize2D(float MaxSize) const;

    FVector ClampMaxSize2D(float MaxSize) const;

    /**
     * Add a vector to this and clamp the result in a cube.
     *
     * @param V Vector to add.
     * @param Radius Half size of the cube.
     */
    void AddBounded(const FVector& V, float Radius=MAX_int16);

    /**
     * Gets the reciprocal of this vector, avoiding division by zero.
     * Zero components are set to BIG_NUMBER.
     *
     * @return Reciprocal of this vector.
     */
    FVector Reciprocal() const;

    /**
     * Check whether X, Y and Z are nearly equal.
     *
     * @param Tolerance Specified Tolerance.
     * @return true if X == Y == Z within the specified tolerance.
     */
    bool IsUniform(float Tolerance=KINDA_SMALL_NUMBER) const;

    /**
     * Mirror a vector about a normal vector.
     *
     * @param MirrorNormal Normal vector to mirror about.
     * @return Mirrored vector.
     */
    FVector MirrorByVector(const FVector& MirrorNormal) const;

    /**
     * Rotates around Axis (assumes Axis.Size() == 1).
     *
     * @param Angle Angle to rotate (in degrees).
     * @param Axis Axis to rotate around.
     * @return Rotated Vector.
     */
    FVector RotateAngleAxis(float AngleDeg, const FVector& Axis) const;

    /**
     * Gets a normalized copy of the vector, checking it is safe to do so based on the length.
     * Returns zero vector if vector length is too small to safely normalize.
     *
     * @param Tolerance Minimum squared vector length.
     * @return A normalized copy if safe, (0,0,0) otherwise.
     */
    FVector GetSafeNormal(float Tolerance=SMALL_NUMBER) const;

    /**
     * Gets a normalized copy of the 2D components of the vector, checking it is safe to do so. Z is set to zero.
     * Returns zero vector if vector length is too small to normalize.
     *
     * @param Tolerance Minimum squared vector length.
     * @return Normalized copy if safe, otherwise returns zero vector.
     */
    FVector GetSafeNormal2D(float Tolerance=SMALL_NUMBER) const;

    /**
     * Returns the cosine of the angle between this vector and another projected onto the XY plane (no Z).
     *
     * @param B the other vector to find the 2D cosine of the angle with.
     * @return The cosine.
     */
    float CosineAngle2D(FVector B) const;

    /**
     * Gets a copy of this vector projected onto the input vector.
     *
     * @param A	Vector to project onto, does not assume it is normalized.
     * @return Projected vector.
     */
    FVector ProjectOnTo(const FVector& A) const ;

    /**
     * Gets a copy of this vector projected onto the input vector, which is assumed to be unit length.
     *
     * @param  Normal Vector to project onto (assumed to be unit length).
     * @return Projected vector.
     */
    FVector ProjectOnToNormal(const FVector& Normal) const;

    /**
     * Check if the vector is of unit length, with specified tolerance.
     *
     * @param LengthSquaredTolerance Tolerance against squared length.
     * @return true if the vector is a unit vector within the specified tolerance.
     */
    bool IsUnit(float LengthSquaredTolerance = KINDA_SMALL_NUMBER) const;

    /**
     * Get a textual representation of this vector.
     *
     * @return A string describing the vector.
     */
    FString ToString() const;

    /**
     * Convert a direction vector into a 'heading' angle.
     *
     * @return 'Heading' angle between +/-PI. 0 is pointing down +X.
     */
    float HeadingAngle() const;

    /**
     * Compare two points and see if they're the same, using a threshold.
     *
     * @param P First vector.
     * @param Q Second vector.
     * @return Whether points are the same within a threshold. Uses fast distance approximation (linear per-component distance).
     */
    static bool PointsAreSame(const FVector &P, const FVector &Q);

    /**
     * Compare two points and see if they're within specified distance.
     *
     * @param Point1 First vector.
     * @param Point2 Second vector.
     * @param Dist Specified distance.
     * @return Whether two points are within the specified distance. Uses fast distance approximation (linear per-component distance).
     */
    static bool PointsAreNear(const FVector &Point1, const FVector &Point2, float Dist);

    /**
     * Calculate the signed distance (in the direction of the normal) between a point and a plane.
     *
     * @param Point The Point we are checking.
     * @param PlaneBase The Base Point in the plane.
     * @param PlaneNormal The Normal of the plane (assumed to be unit length).
     * @return Signed distance between point and plane.
     */
    static float PointPlaneDist(const FVector &Point, const FVector &PlaneBase, const FVector &PlaneNormal);

    /**
     * Calculate the projection of a vector on the plane defined by PlaneNormal.
     *
     * @param  V The vector to project onto the plane.
     * @param  PlaneNormal Normal of the plane (assumed to be unit length).
     * @return Projection of V onto plane.
     */
    static FVector VectorPlaneProject(const FVector& V, const FVector& PlaneNormal);

    /**
     * Euclidean distance between two points.
     *
     * @param V1 The first point.
     * @param V2 The second point.
     * @return The distance between two points.
     */
    static float Dist(const FVector &V1, const FVector &V2);
    static float Distance(const FVector &V1, const FVector &V2) { return Dist(V1, V2); }

    /**
    * Euclidean distance between two points in the XY plane (ignoring Z).
    *
    * @param V1 The first point.
    * @param V2 The second point.
    * @return The distance between two points in the XY plane.
    */
    static float DistXY(const FVector &V1, const FVector &V2);
    static float Dist2D(const FVector &V1, const FVector &V2) { return DistXY(V1, V2); }

    /**
     * Squared distance between two points.
     *
     * @param V1 The first point.
     * @param V2 The second point.
     * @return The squared distance between two points.
     */
    static float DistSquared(const FVector &V1, const FVector &V2);

    /**
     * Squared distance between two points in the XY plane only.
     *
     * @param V1 The first point.
     * @param V2 The second point.
     * @return The squared distance between two points in the XY plane
     */
    static float DistSquaredXY(const FVector &V1, const FVector &V2);
    static float DistSquared2D(const FVector &V1, const FVector &V2) { return DistSquaredXY(V1, V2); }

    /**
     * Compute pushout of a box from a plane.
     *
     * @param Normal The plane normal.
     * @param Size The size of the box.
     * @return Pushout required.
     */
    static float BoxPushOut(const FVector& Normal, const FVector& Size);

    /**
     * See if two normal vectors are nearly parallel, meaning the angle between them is close to 0 degrees.
     *
     * @param  Normal1 First normalized vector.
     * @param  Normal1 Second normalized vector.
     * @param  ParallelCosineThreshold Normals are parallel if absolute value of dot product (cosine of angle between them) is greater than or equal to this. For example: cos(1.0 degrees).
     * @return true if vectors are nearly parallel, false otherwise.
     */
    static bool Parallel(const FVector& Normal1, const FVector& Normal2, float ParallelCosineThreshold = THRESH_NORMALS_ARE_PARALLEL);

    /**
     * See if two normal vectors are coincident (nearly parallel and point in the same direction).
     *
     * @param  Normal1 First normalized vector.
     * @param  Normal2 Second normalized vector.
     * @param  ParallelCosineThreshold Normals are coincident if dot product (cosine of angle between them) is greater than or equal to this. For example: cos(1.0 degrees).
     * @return true if vectors are coincident (nearly parallel and point in the same direction), false otherwise.
     */
    static bool Coincident(const FVector& Normal1, const FVector& Normal2, float ParallelCosineThreshold = THRESH_NORMALS_ARE_PARALLEL);

    /**
     * See if two normal vectors are nearly orthogonal (perpendicular), meaning the angle between them is close to 90 degrees.
     *
     * @param  Normal1 First normalized vector.
     * @param  Normal2 Second normalized vector.
     * @param  OrthogonalCosineThreshold Normals are orthogonal if absolute value of dot product (cosine of angle between them) is less than or equal to this. For example: cos(89.0 degrees).
     * @return true if vectors are orthogonal (perpendicular), false otherwise.
     */
    static bool Orthogonal(const FVector& Normal1, const FVector& Normal2, float OrthogonalCosineThreshold = THRESH_NORMALS_ARE_ORTHOGONAL);

    /**
     * See if two planes are coplanar. They are coplanar if the normals are nearly parallel and the planes include the same set of points.
     *
     * @param Base1 The base point in the first plane.
     * @param Normal1 The normal of the first plane.
     * @param Base2 The base point in the second plane.
     * @param Normal2 The normal of the second plane.
     * @param ParallelCosineThreshold Normals are parallel if absolute value of dot product is greater than or equal to this.
     * @return true if the planes are coplanar, false otherwise.
     */
    static bool Coplanar(const FVector& Base1, const FVector& Normal1, const FVector& Base2, const FVector& Normal2, float ParallelCosineThreshold = THRESH_NORMALS_ARE_PARALLEL);

    /**
     * Triple product of three vectors: X dot (Y cross Z).
     *
     * @param X The first vector.
     * @param Y The second vector.
     * @param Z The third vector.
     * @return The triple product: X dot (Y cross Z).
     */
    static float Triple(const FVector& X, const FVector& Y, const FVector& Z);

    /**
     * Converts a vector containing radian values to a vector containing degree values.
     *
     * @param RadVector	Vector containing radian values
     * @return Vector  containing degree values
     */
    static FVector RadiansToDegrees(const FVector& RadVector);

    /**
     * Converts a vector containing degree values to a vector containing radian values.
     *
     * @param DegVector	Vector containing degree values
     * @return Vector containing radian values
     */
    static FVector DegreesToRadians(const FVector& DegVector);
};


/* FVector inline functions
 *****************************************************************************/

/**
 * Multiplies a vector by a scaling factor.
 *
 * @param Scale Scaling factor.
 * @param V Vector to scale.
 * @return Result of multiplication.
 */
FVector operator*(float Scale, const FVector& V)
{
    return V.operator*(Scale);
}


/**
 * Util to calculate distance from a point to a bounding box
 *
 * @param Mins 3D Point defining the lower values of the axis of the bound box
 * @param Max 3D Point defining the lower values of the axis of the bound box
 * @param Point 3D position of interest
 * @return the distance from the Point to the bounding box.
 */
float ComputeSquaredDistanceFromBoxToPoint(const FVector& Mins, const FVector& Maxs, const FVector& Point)
{
    // Accumulates the distance as we iterate axis
    float DistSquared = 0.f;

    // Check each axis for min/max and add the distance accordingly
    // NOTE: Loop manually unrolled for > 2x speed up
    if (Point.X < Mins.X)
    {
        DistSquared += FMath::Square(Point.X - Mins.X);
    }
    else if (Point.X > Maxs.X)
    {
        DistSquared += FMath::Square(Point.X - Maxs.X);
    }

    if (Point.Y < Mins.Y)
    {
        DistSquared += FMath::Square(Point.Y - Mins.Y);
    }
    else if (Point.Y > Maxs.Y)
    {
        DistSquared += FMath::Square(Point.Y - Maxs.Y);
    }

    if (Point.Z < Mins.Z)
    {
        DistSquared += FMath::Square(Point.Z - Mins.Z);
    }
    else if (Point.Z > Maxs.Z)
    {
        DistSquared += FMath::Square(Point.Z - Maxs.Z);
    }

    return DistSquared;
}


FVector::FVector(const FVector2D V, float InZ)
        : X(V.X), Y(V.Y), Z(InZ)
        {
                DiagnosticCheckNaN();
        }


inline FVector FVector::RotateAngleAxis(const float AngleDeg, const FVector& Axis) const
{
    float S, C;
    FMath::SinCos(&S, &C, FMath::DegreesToRadians(AngleDeg));

    const float XX	= Axis.X * Axis.X;
    const float YY	= Axis.Y * Axis.Y;
    const float ZZ	= Axis.Z * Axis.Z;

    const float XY	= Axis.X * Axis.Y;
    const float YZ	= Axis.Y * Axis.Z;
    const float ZX	= Axis.Z * Axis.X;

    const float XS	= Axis.X * S;
    const float YS	= Axis.Y * S;
    const float ZS	= Axis.Z * S;

    const float OMC	= 1.f - C;

    return {
            (OMC * XX + C) * X + (OMC * XY - ZS) * Y + (OMC * ZX + YS) * Z,
            (OMC * XY + ZS) * X + (OMC * YY + C) * Y + (OMC * YZ - XS) * Z,
            (OMC * ZX - YS) * X + (OMC * YZ + XS) * Y + (OMC * ZZ + C) * Z
    };
}

inline bool FVector::PointsAreSame(const FVector &P, const FVector &Q)
{
    float Temp;
    Temp=P.X-Q.X;
    if((Temp > -THRESH_POINTS_ARE_SAME) && (Temp < THRESH_POINTS_ARE_SAME))
    {
        Temp=P.Y-Q.Y;
        if((Temp > -THRESH_POINTS_ARE_SAME) && (Temp < THRESH_POINTS_ARE_SAME))
        {
            Temp=P.Z-Q.Z;
            if((Temp > -THRESH_POINTS_ARE_SAME) && (Temp < THRESH_POINTS_ARE_SAME))
            {
                return true;
            }
        }
    }
    return false;
}

inline bool FVector::PointsAreNear(const FVector &Point1, const FVector &Point2, float Dist)
{
    float Temp;
    Temp=(Point1.X - Point2.X); if (FMath::Abs(Temp)>=Dist) return false;
    Temp=(Point1.Y - Point2.Y); if (FMath::Abs(Temp)>=Dist) return false;
    Temp=(Point1.Z - Point2.Z);
    return FMath::Abs(Temp) < Dist;
}

inline float FVector::PointPlaneDist
        (
                const FVector &Point,
                const FVector &PlaneBase,
                const FVector &PlaneNormal
        )
{
    return (Point - PlaneBase) | PlaneNormal;
}

inline FVector FVector::VectorPlaneProject(const FVector& V, const FVector& PlaneNormal)
{
    return V - V.ProjectOnToNormal(PlaneNormal);
}

inline bool FVector::Parallel(const FVector& Normal1, const FVector& Normal2, float ParallelCosineThreshold)
{
    const float NormalDot = Normal1 | Normal2;
    return FMath::Abs(NormalDot) >= ParallelCosineThreshold;
}

inline bool FVector::Coincident(const FVector& Normal1, const FVector& Normal2, float ParallelCosineThreshold)
{
    const float NormalDot = Normal1 | Normal2;
    return NormalDot >= ParallelCosineThreshold;
}

inline bool FVector::Orthogonal(const FVector& Normal1, const FVector& Normal2, float OrthogonalCosineThreshold)
{
    const float NormalDot = Normal1 | Normal2;
    return FMath::Abs(NormalDot) <= OrthogonalCosineThreshold;
}

inline bool FVector::Coplanar(const FVector &Base1, const FVector &Normal1, const FVector &Base2, const FVector &Normal2, float ParallelCosineThreshold)
{
    if      (!FVector::Parallel(Normal1,Normal2,ParallelCosineThreshold)) return false;
    return FVector::PointPlaneDist (Base2, Base1, Normal1) <= THRESH_POINT_ON_PLANE;
}

inline float FVector::Triple(const FVector& X, const FVector& Y, const FVector& Z)
{
    return
            (	(X.X * (Y.Y * Z.Z - Y.Z * Z.Y))
                 +	(X.Y * (Y.Z * Z.X - Y.X * Z.Z))
                 +	(X.Z * (Y.X * Z.Y - Y.Y * Z.X)));
}

inline FVector FVector::RadiansToDegrees(const FVector& RadVector)
{
    return RadVector * (180.f / PI);
}

inline FVector FVector::DegreesToRadians(const FVector& DegVector)
{
    return DegVector * (PI / 180.f);
}

FVector::FVector()
{
    X = 0.0f;
    Y = 0.0f;
    Z = 0.0f;
}

FVector::FVector(float InF)
        : X(InF), Y(InF), Z(InF)
{
    DiagnosticCheckNaN();
}

FVector::FVector(float InX, float InY, float InZ)
        : X(InX), Y(InY), Z(InZ)
{
    DiagnosticCheckNaN();
}

FVector FVector::operator^(const FVector& V) const
{
    return {
            Y * V.Z - Z * V.Y,
                    Z * V.X - X * V.Z,
                    X * V.Y - Y * V.X
    };
}

FVector FVector::CrossProduct(const FVector& A, const FVector& B)
{
    return A ^ B;
}

float FVector::operator|(const FVector& V) const
{
    return X*V.X + Y*V.Y + Z*V.Z;
}

float FVector::DotProduct(const FVector& A, const FVector& B)
{
    return A | B;
}

FVector FVector::operator+(const FVector& V) const
{
    return {X + V.X, Y + V.Y, Z + V.Z};
}

FVector FVector::operator-(const FVector& V) const
{
    return {X - V.X, Y - V.Y, Z - V.Z};
}

FVector FVector::operator-(float Bias) const
{
    return {X - Bias, Y - Bias, Z - Bias};
}

FVector FVector::operator+(float Bias) const
{
    return {X + Bias, Y + Bias, Z + Bias};
}

FVector FVector::operator*(float Scale) const
{
    return {X * Scale, Y * Scale, Z * Scale};
}

FVector FVector::operator/(float Scale) const
{
    const float RScale = 1.f/Scale;
    return {X * RScale, Y * RScale, Z * RScale};
}

FVector FVector::operator*(const FVector& V) const
{
    return {X * V.X, Y * V.Y, Z * V.Z};
}

FVector FVector::operator/(const FVector& V) const
{
    return {X / V.X, Y / V.Y, Z / V.Z};
}

bool FVector::operator==(const FVector& V) const
{
    return X==V.X && Y==V.Y && Z==V.Z;
}

bool FVector::operator!=(const FVector& V) const
{
    return X!=V.X || Y!=V.Y || Z!=V.Z;
}

bool FVector::Equals(const FVector& V, float Tolerance) const
{
    return FMath::Abs(X-V.X) <= Tolerance && FMath::Abs(Y-V.Y) <= Tolerance && FMath::Abs(Z-V.Z) <= Tolerance;
}

bool FVector::AllComponentsEqual(float Tolerance) const
{
    return FMath::Abs(X - Y) <= Tolerance && FMath::Abs(X - Z) <= Tolerance && FMath::Abs(Y - Z) <= Tolerance;
}


FVector FVector::operator-() const
{
    return {-X, -Y, -Z};
}


FVector FVector::operator+=(const FVector& V)
{
    X += V.X; Y += V.Y; Z += V.Z;
    DiagnosticCheckNaN();
    return *this;
}

FVector FVector::operator-=(const FVector& V)
{
    X -= V.X; Y -= V.Y; Z -= V.Z;
    DiagnosticCheckNaN();
    return *this;
}

FVector FVector::operator*=(float Scale)
{
    X *= Scale; Y *= Scale; Z *= Scale;
    DiagnosticCheckNaN();
    return *this;
}

FVector FVector::operator/=(float V)
{
    const float RV = 1.f/V;
    X *= RV; Y *= RV; Z *= RV;
    DiagnosticCheckNaN();
    return *this;
}

FVector FVector::operator*=(const FVector& V)
{
    X *= V.X; Y *= V.Y; Z *= V.Z;
    DiagnosticCheckNaN();
    return *this;
}

FVector FVector::operator/=(const FVector& V)
{
    X /= V.X; Y /= V.Y; Z /= V.Z;
    DiagnosticCheckNaN();
    return *this;
}

float& FVector::operator[](int32 Index)
{
    if(Index == 0)
    {
        return X;
    }
    if(Index == 1)
    {
        return Y;
    }
    return Z;

}

float FVector::operator[](int32 Index)const
{
    if(Index == 0)
    {
        return X;
    }
    if(Index == 1)
    {
        return Y;
    }
    return Z;
}

void FVector::Set(float InX, float InY, float InZ)
{
    X = InX;
    Y = InY;
    Z = InZ;
    DiagnosticCheckNaN();
}

float FVector::GetMax() const
{
    return FMath::Max(FMath::Max(X,Y),Z);
}

float FVector::GetAbsMax() const
{
    return FMath::Max(FMath::Max(FMath::Abs(X),FMath::Abs(Y)),FMath::Abs(Z));
}

float FVector::GetMin() const
{
    return FMath::Min(FMath::Min(X,Y),Z);
}

float FVector::GetAbsMin() const
{
    return FMath::Min(FMath::Min(FMath::Abs(X),FMath::Abs(Y)),FMath::Abs(Z));
}

FVector FVector::ComponentMin(const FVector& Other) const
{
    return {FMath::Min(X, Other.X), FMath::Min(Y, Other.Y), FMath::Min(Z, Other.Z)};
}

FVector FVector::ComponentMax(const FVector& Other) const
{
    return {FMath::Max(X, Other.X), FMath::Max(Y, Other.Y), FMath::Max(Z, Other.Z)};
}

FVector FVector::GetAbs() const
{
    return {FMath::Abs(X), FMath::Abs(Y), FMath::Abs(Z)};
}

float FVector::Size() const
{
    return FMath::Sqrt(X*X + Y*Y + Z*Z);
}

float FVector::SizeSquared() const
{
    return X*X + Y*Y + Z*Z;
}

float FVector::Size2D() const
{
    return FMath::Sqrt(X*X + Y*Y);
}

float FVector::SizeSquared2D() const
{
    return X*X + Y*Y;
}

bool FVector::IsNearlyZero(float Tolerance) const
{
    return
            FMath::Abs(X)<=Tolerance
            &&	FMath::Abs(Y)<=Tolerance
            &&	FMath::Abs(Z)<=Tolerance;
}

bool FVector::IsZero() const
{
    return X==0.f && Y==0.f && Z==0.f;
}

bool FVector::Normalize(float Tolerance)
{
    const float SquareSum = X*X + Y*Y + Z*Z;
    if(SquareSum > Tolerance)
    {
        const float Scale = FMath::InvSqrt(SquareSum);
        X *= Scale; Y *= Scale; Z *= Scale;
        return true;
    }
    return false;
}

bool FVector::IsNormalized() const
{
    return (FMath::Abs(1.f - SizeSquared()) < THRESH_VECTOR_NORMALIZED);
}

void FVector::ToDirectionAndLength(FVector &OutDir, float &OutLength) const
{
    OutLength = Size();
    if (OutLength > SMALL_NUMBER)
    {
        float OneOverLength = 1.0f/OutLength;
        OutDir = FVector(X*OneOverLength, Y*OneOverLength,
                         Z*OneOverLength);
    }
    else
    {
        OutDir = FVector::ZeroVector;
    }
}

FVector FVector::GetSignVector() const
{
    return {
            FMath::FloatSelect(X, 1.f, -1.f),
                    FMath::FloatSelect(Y, 1.f, -1.f),
                    FMath::FloatSelect(Z, 1.f, -1.f)
    };
}

FVector FVector::Projection() const
{
    const float RZ = 1.f/Z;
    return {X*RZ, Y*RZ, 1};
}

FVector FVector::GetUnsafeNormal() const
{
    const float Scale = FMath::InvSqrt(X*X+Y*Y+Z*Z);
    return {X*Scale, Y*Scale, Z*Scale};
}

FVector FVector::GridSnap(const float& GridSz) const
{
    return {FMath::GridSnap(X, GridSz),FMath::GridSnap(Y, GridSz),FMath::GridSnap(Z, GridSz)};
}

FVector FVector::BoundToCube(float Radius) const
{
    return {
            FMath::Clamp(X,-Radius,Radius),
                    FMath::Clamp(Y,-Radius,Radius),
                    FMath::Clamp(Z,-Radius,Radius)
    };
}

FVector FVector::GetClampedToSize(float Min, float Max) const
{
    float VecSize = Size();
    const FVector VecDir = (VecSize > SMALL_NUMBER) ? (*this/VecSize) : FVector::ZeroVector;

    VecSize = FMath::Clamp(VecSize, Min, Max);

    return VecSize * VecDir;
}

FVector FVector::ClampSize(float Min, float Max) const
{
    return GetClampedToSize(Min, Max);
}

FVector FVector::GetClampedToSize2D(float Min, float Max) const
{
    float VecSize2D = Size2D();
    const FVector VecDir = (VecSize2D > SMALL_NUMBER) ? (*this/VecSize2D) : FVector::ZeroVector;

    VecSize2D = FMath::Clamp(VecSize2D, Min, Max);

    return {VecSize2D * VecDir.X, VecSize2D * VecDir.Y, Z};
}

FVector FVector::ClampSize2D(float Min, float Max) const
{
    return GetClampedToSize2D(Min, Max);
}


FVector FVector::GetClampedToMaxSize(float MaxSize) const
{
    if (MaxSize < KINDA_SMALL_NUMBER)
    {
        return FVector::ZeroVector;
    }

    const float VSq = SizeSquared();
    if (VSq > FMath::Square(MaxSize))
    {
        const float Scale = MaxSize * FMath::InvSqrt(VSq);
        return {X*Scale, Y*Scale, Z*Scale};
    }
    return *this;

}

FVector FVector::ClampMaxSize(float MaxSize) const
{
    return GetClampedToMaxSize(MaxSize);
}

FVector FVector::GetClampedToMaxSize2D(float MaxSize) const
{
    if (MaxSize < KINDA_SMALL_NUMBER)
    {
        return {0.f, 0.f, Z};
    }

    const float VSq2D = SizeSquared2D();
    if (VSq2D > FMath::Square(MaxSize))
    {
        const float Scale = MaxSize * FMath::InvSqrt(VSq2D);
        return {X*Scale, Y*Scale, Z};
    }
    return *this;
}

FVector FVector::ClampMaxSize2D(float MaxSize) const
{
    return GetClampedToMaxSize2D(MaxSize);
}


void FVector::AddBounded(const FVector& V, float Radius)
{
    *this = (*this + V).BoundToCube(Radius);
}

float& FVector::Component(int32 Index)
{
    return (&X)[Index];
}

float FVector::Component(int32 Index) const
{
    return (&X)[Index];
}

float FVector::GetComponentForAxis(EAxis::Type Axis) const
{
    switch (Axis)
    {
        case EAxis::X:
            return X;
        case EAxis::Y:
            return Y;
        case EAxis::Z:
            return Z;
        default:
            return 0.f;
    }
}

void FVector::SetComponentForAxis(EAxis::Type Axis, float Component)
{
    switch (Axis)
    {
        case EAxis::X:
            X = Component;
            break;
        case EAxis::Y:
            Y = Component;
            break;
        case EAxis::Z:
            Z = Component;
            break;
        case EAxis::None:break;
    }
}

FVector FVector::Reciprocal() const
{
    FVector RecVector;
    if (X!=0.f)
    {
        RecVector.X = 1.f/X;
    }
    else
    {
        RecVector.X = BIG_NUMBER;
    }
    if (Y!=0.f)
    {
        RecVector.Y = 1.f/Y;
    }
    else
    {
        RecVector.Y = BIG_NUMBER;
    }
    if (Z!=0.f)
    {
        RecVector.Z = 1.f/Z;
    }
    else
    {
        RecVector.Z = BIG_NUMBER;
    }

    return RecVector;
}

bool FVector::IsUniform(float Tolerance) const
{
    return AllComponentsEqual(Tolerance);
}

FVector FVector::MirrorByVector(const FVector& MirrorNormal) const
{
    return *this - MirrorNormal * (2.f * (*this | MirrorNormal));
}

FVector FVector::GetSafeNormal(float Tolerance) const
{
    const float SquareSum = X*X + Y*Y + Z*Z;

    // Not sure if it's safe to add tolerance in there. Might introduce too many errors
    if(SquareSum == 1.f)
    {
        return *this;
    }
    if(SquareSum < Tolerance)
    {
        return FVector::ZeroVector;
    }
    const float Scale = FMath::InvSqrt(SquareSum);
    return {X*Scale, Y*Scale, Z*Scale};
}

FVector FVector::GetSafeNormal2D(float Tolerance) const
{
    const float SquareSum = X*X + Y*Y;

    // Not sure if it's safe to add tolerance in there. Might introduce too many errors
    if(SquareSum == 1.f)
    {
        if(Z == 0.f)
        {
            return *this;
        }
        return FVector(X, Y, 0.f);
    }
    else if(SquareSum < Tolerance)
    {
        return FVector::ZeroVector;
    }

    const float Scale = FMath::InvSqrt(SquareSum);
    return FVector(X*Scale, Y*Scale, 0.f);
}

float FVector::CosineAngle2D(FVector B) const
{
    FVector A(*this);
    A.Z = 0.0f;
    B.Z = 0.0f;
    A.Normalize();
    B.Normalize();
    return A | B;
}

FVector FVector::ProjectOnTo(const FVector& A) const
{
    return (A * ((*this | A) / (A | A)));
}

FVector FVector::ProjectOnToNormal(const FVector& Normal) const
{
    return (Normal * (*this | Normal));
}

bool FVector::IsUnit(float LengthSquaredTolerance) const
{
    return FMath::Abs(1.0f - SizeSquared()) < LengthSquaredTolerance;
}

FString FVector::ToString() const
{
    return FString::Printf("X=%3.3f Y=%3.3f Z=%3.3f", X, Y, Z);
}

float FVector::HeadingAngle() const
{
    // Project Dir into Z plane.
    FVector PlaneDir = *this;
    PlaneDir.Z = 0.f;
    PlaneDir = PlaneDir.GetSafeNormal();

    float Angle = FMath::Acos(PlaneDir.X);

    if(PlaneDir.Y < 0.0f)
    {
        Angle *= -1.0f;
    }

    return Angle;
}



float FVector::Dist(const FVector &V1, const FVector &V2)
{
    return FMath::Sqrt(FVector::DistSquared(V1, V2));
}

float FVector::DistXY(const FVector &V1, const FVector &V2)
{
    return FMath::Sqrt(FVector::DistSquaredXY(V1, V2));
}

float FVector::DistSquared(const FVector &V1, const FVector &V2)
{
    return FMath::Square(V2.X-V1.X) + FMath::Square(V2.Y-V1.Y) + FMath::Square(V2.Z-V1.Z);
}

float FVector::DistSquaredXY(const FVector &V1, const FVector &V2)
{
    return FMath::Square(V2.X-V1.X) + FMath::Square(V2.Y-V1.Y);
}

float FVector::BoxPushOut(const FVector& Normal, const FVector& Size)
{
    return FMath::Abs(Normal.X*Size.X) + FMath::Abs(Normal.Y*Size.Y) + FMath::Abs(Normal.Z*Size.Z);
}

/** Component-wise clamp for FVector */
FVector ClampVector(const FVector& V, const FVector& Min, const FVector& Max)
{
    return {
            FMath::Clamp(V.X,Min.X,Max.X),
            FMath::Clamp(V.Y,Min.Y,Max.Y),
            FMath::Clamp(V.Z,Min.Z,Max.Z)
    };
}

template <> struct TIsPODType<FVector> { enum { Value = 1
    }; };

/* FMath inline functions
 *****************************************************************************/

#endif //Pragma_Once_WVector