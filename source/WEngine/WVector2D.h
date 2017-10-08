// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WVector2D
#define Pragma_Once_WVector2D

#include "WEngine.h"
#include "WMath.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ClangTidyInspection"

/**
 * A vector in 2-D space composed of components (X, Y) with floating point precision.
 */
struct FVector2D
{
    /** Vector's X component. */
    float X = 0.0f;

    /** Vector's Y component. */
    float Y = 0.0f;

public:

    /** Global 2D zero vector constant (0,0) */
    static const FVector2D ZeroVector;

    /** Global 2D unit vector constant (1,1) */
    static const FVector2D UnitVector;

public:

    /** Default constructor (no initialization). */
    FVector2D()
    {
        X = 0.0f;
        Y = 0.0f;
    }

    /**
     * Constructor using initial values for each component.
     *
     * @param InX X coordinate.
     * @param InY Y coordinate.
     */
    FVector2D(float InX, float InY);

public:

    /**
     * Gets the result of adding two vectors together.
     *
     * @param V The other vector to add to this.
     * @return The result of adding the vectors together.
     */
    FVector2D operator+(const FVector2D& V) const;

    /**
     * Gets the result of subtracting a vector from this one.
     *
     * @param V The other vector to subtract from this.
     * @return The result of the subtraction.
     */
    FVector2D operator-(const FVector2D& V) const;

    /**
     * Gets the result of scaling the vector (multiplying each component by a value).
     *
     * @param Scale How much to scale the vector by.
     * @return The result of scaling this vector.
     */
    FVector2D operator*(float Scale) const;

    /**
     * Gets the result of dividing each component of the vector by a value.
     *
     * @param Scale How much to divide the vector by.
     * @return The result of division on this vector.
     */
    FVector2D operator/(float Scale) const;

    /**
     * Gets the result of this vector + float A.
     *
     * @param A Float to add to each component.
     * @return The result of this vector + float A.
     */
    FVector2D operator+(float A) const;

    /**
     * Gets the result of subtracting from each component of the vector.
     *
     * @param A Float to subtract from each component
     * @return The result of this vector - float A.
     */
    FVector2D operator-(float A) const;

    /**
     * Gets the result of component-wise multiplication of this vector by another.
     *
     * @param V The other vector to multiply this by.
     * @return The result of the multiplication.
     */
    FVector2D operator*(const FVector2D& V) const;

    /**
     * Gets the result of component-wise division of this vector by another.
     *
     * @param V The other vector to divide this by.
     * @return The result of the division.
     */
    FVector2D operator/(const FVector2D& V) const;

    /**
     * Calculates dot product of this vector and another.
     *
     * @param V The other vector.
     * @return The dot product.
     */
    float operator|(const FVector2D& V) const;

    /**
     * Calculates cross product of this vector and another.
     *
     * @param V The other vector.
     * @return The cross product.
     */
    float operator^(const FVector2D& V) const;

public:

    /**
     * Compares this vector against another for equality.
     *
     * @param V The vector to compare against.
     * @return true if the two vectors are equal, otherwise false.
     */
    bool operator==(const FVector2D& V) const;

    /**
     * Compares this vector against another for inequality.
     *
     * @param V The vector to compare against.
     * @return true if the two vectors are not equal, otherwise false.
     */
    bool operator!=(const FVector2D& V) const;

    /**
     * Checks whether both components of this vector are less than another.
     *
     * @param Other The vector to compare against.
     * @return true if this is the smaller vector, otherwise false.
     */
    bool operator<(const FVector2D& Other) const;

    /**
     * Checks whether both components of this vector are greater than another.
     *
     * @param Other The vector to compare against.
     * @return true if this is the larger vector, otherwise false.
     */
    bool operator>(const FVector2D& Other) const;

    /**
     * Checks whether both components of this vector are less than or equal to another.
     *
     * @param Other The vector to compare against.
     * @return true if this vector is less than or equal to the other vector, otherwise false.
     */
    bool operator<=(const FVector2D& Other) const;

    /**
     * Checks whether both components of this vector are greater than or equal to another.
     *
     * @param Other The vector to compare against.
     * @return true if this vector is greater than or equal to the other vector, otherwise false.
     */
    bool operator>=(const FVector2D& Other) const;

    /**
     * Gets a negated copy of the vector.
     *
     * @return A negated copy of the vector.
     */
    FVector2D operator-() const;

    /**
     * Adds another vector to this.
     *
     * @param V The other vector to add.
     * @return Copy of the vector after addition.
     */
    FVector2D operator+=(const FVector2D& V);

    /**
     * Subtracts another vector from this.
     *
     * @param V The other vector to subtract.
     * @return Copy of the vector after subtraction.
     */
    FVector2D operator-=(const FVector2D& V);

    /**
     * Scales this vector.
     *
     * @param Scale The scale to multiply vector by.
     * @return Copy of the vector after scaling.
     */
    FVector2D operator*=(float Scale);

    /**
     * Divides this vector.
     *
     * @param V What to divide vector by.
     * @return Copy of the vector after division.
     */
    FVector2D operator/=(float V);

    /**
     * Multiplies this vector with another vector, using component-wise multiplication.
     *
     * @param V The vector to multiply with.
     * @return Copy of the vector after multiplication.
     */
    FVector2D operator*=(const FVector2D& V);

    /**
     * Divides this vector by another vector, using component-wise division.
     *
     * @param V The vector to divide by.
     * @return Copy of the vector after division.
     */
    FVector2D operator/=(const FVector2D& V);

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
     * @return copy of component value.
     */
    float operator[](int32 Index) const;

    /**
    * Gets a specific component of the vector.
    *
    * @param Index The index of the component required.
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

public:

    /**
     * Calculates the dot product of two vectors.
     *
     * @param A The first vector.
     * @param B The second vector.
     * @return The dot product.
     */
    static float DotProduct(const FVector2D& A, const FVector2D& B);

    /**
     * Squared distance between two 2D points.
     *
     * @param V1 The first point.
     * @param V2 The second point.
     * @return The squared distance between two 2D points.
     */
    static float DistSquared(const FVector2D& V1, const FVector2D& V2);

    /**
     * Distance between two 2D points.
     *
     * @param V1 The first point.
     * @param V2 The second point.
     * @return The distance between two 2D points.
     */
    static float Distance(const FVector2D& V1, const FVector2D& V2);

    /**
     * Calculate the cross product of two vectors.
     *
     * @param A The first vector.
     * @param B The second vector.
     * @return The cross product.
     */
    static float CrossProduct(const FVector2D& A, const FVector2D& B);

    /**
     * Checks for equality with error-tolerant comparison.
     *
     * @param V The vector to compare.
     * @param Tolerance Error tolerance.
     * @return true if the vectors are equal within specified tolerance, otherwise false.
     */
    bool Equals(const FVector2D& V, float Tolerance=KINDA_SMALL_NUMBER) const;

    /**
     * Set the values of the vector directly.
     *
     * @param InX New X coordinate.
     * @param InY New Y coordinate.
     */
    void Set(float InX, float InY);

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
     * Gets a normalized copy of the vector, checking it is safe to do so based on the length.
     * Returns zero vector if vector length is too small to safely normalize.
     *
     * @param Tolerance Minimum squared length of vector for normalization.
     * @return A normalized copy of the vector if safe, (0,0) otherwise.
     */
    FVector2D GetSafeNormal(float Tolerance=SMALL_NUMBER) const;

    /**
     * Normalize this vector in-place if it is large enough, set it to (0,0) otherwise.
     *
     * @param Tolerance Minimum squared length of vector for normalization.
     * @see GetSafeNormal()
     */
    void Normalize(float Tolerance=SMALL_NUMBER);

    /**
     * Checks whether vector is near to zero within a specified tolerance.
     *
     * @param Tolerance Error tolerance.
     * @return true if vector is in tolerance to zero, otherwise false.
     */
    bool IsNearlyZero(float Tolerance=KINDA_SMALL_NUMBER) const;

    /**
     * Util to convert this vector into a unit direction vector and its original length.
     *
     * @param OutDir Reference passed in to store unit direction vector.
     * @param OutLength Reference passed in to store length of the vector.
     */
    void ToDirectionAndLength(FVector2D &OutDir, float &OutLength) const;

    /**
     * Checks whether all components of the vector are exactly zero.
     *
     * @return true if vector is exactly zero, otherwise false.
     */
    bool IsZero() const;

    /**
     * Creates a copy of this vector with both axes clamped to the given range.
     * @return New vector with clamped axes.
     */
    FVector2D ClampAxes(float MinAxisVal, float MaxAxisVal) const;

    /**
    * Get a copy of the vector as sign only.
    * Each component is set to +1 or -1, with the sign of zero treated as +1.
    *
    * @param A copy of the vector with each component set to +1 or -1
    */
    FVector2D GetSignVector() const;

    /**
    * Get a copy of this vector with absolute value of each component.
    *
    * @return A copy of this vector with absolute value of each component.
    */
    FVector2D GetAbs() const;

    /**
     * Get a textual representation of the vector.
     *
     * @return Text describing the vector.
     */
    FString ToString() const;

    static float GetRangePct(FVector2D const& Range, float Value);

    static float GetRangeValue(FVector2D const& Range, float Pct);

/** For the given Value clamped to the [Input:Range] inclusive, returns the corresponding percentage in [Output:Range] Inclusive. */
    static float GetMappedRangeValueClamped(const FVector2D& InputRange, const FVector2D& OutputRange, float Value);

/** Transform the given Value relative to the input range to the Output Range. */
    static float GetMappedRangeValueUnclamped(const FVector2D& InputRange, const FVector2D& OutputRange, float Value);
};

/* FVector2D inline functions
 *****************************************************************************/

const FVector2D FVector2D::ZeroVector(0.0f, 0.0f);
const FVector2D FVector2D::UnitVector(1.0f, 1.0f);

FVector2D operator*(float Scale, const FVector2D& V)
{
    return V.operator*(Scale);
}

FVector2D::FVector2D(float InX,float InY)
        :	X(InX), Y(InY)
{ }


FVector2D FVector2D::operator+(const FVector2D& V) const
{
    return {X + V.X, Y + V.Y};
}


FVector2D FVector2D::operator-(const FVector2D& V) const
{
    return {X - V.X, Y - V.Y};
}


FVector2D FVector2D::operator*(float Scale) const
{
    return {X * Scale, Y * Scale};
}


FVector2D FVector2D::operator/(float Scale) const
{
    const float RScale = 1.f/Scale;
    return {X * RScale, Y * RScale};
}


FVector2D FVector2D::operator+(float A) const
{
    return {X + A, Y + A};
}


FVector2D FVector2D::operator-(float A) const
{
    return {X - A, Y - A};
}


FVector2D FVector2D::operator*(const FVector2D& V) const
{
    return {X * V.X, Y * V.Y};
}


FVector2D FVector2D::operator/(const FVector2D& V) const
{
    return {X / V.X, Y / V.Y};
}


float FVector2D::operator|(const FVector2D& V) const
{
    return X*V.X + Y*V.Y;
}


float FVector2D::operator^(const FVector2D& V) const
{
    return X*V.Y - Y*V.X;
}


float FVector2D::DotProduct(const FVector2D& A, const FVector2D& B)
{
    return A | B;
}


float FVector2D::DistSquared(const FVector2D &V1, const FVector2D &V2)
{
    return FMath::Square(V2.X-V1.X) + FMath::Square(V2.Y-V1.Y);
}


float FVector2D::Distance(const FVector2D& V1, const FVector2D& V2)
{
    return FMath::Sqrt(FVector2D::DistSquared(V1, V2));
}


float FVector2D::CrossProduct(const FVector2D& A, const FVector2D& B)
{
    return A ^ B;
}


bool FVector2D::operator==(const FVector2D& V) const
{
    return X==V.X && Y==V.Y;
}


bool FVector2D::operator!=(const FVector2D& V) const
{
    return X!=V.X || Y!=V.Y;
}


bool FVector2D::operator<(const FVector2D& Other) const
{
    return X < Other.X && Y < Other.Y;
}


bool FVector2D::operator>(const FVector2D& Other) const
{
    return X > Other.X && Y > Other.Y;
}


bool FVector2D::operator<=(const FVector2D& Other) const
{
    return X <= Other.X && Y <= Other.Y;
}


bool FVector2D::operator>=(const FVector2D& Other) const
{
    return X >= Other.X && Y >= Other.Y;
}


bool FVector2D::Equals(const FVector2D& V, float Tolerance) const
{
    return FMath::Abs(X-V.X) <= Tolerance && FMath::Abs(Y-V.Y) <= Tolerance;
}


FVector2D FVector2D::operator-() const
{
    return {-X, -Y};
}


FVector2D FVector2D::operator+=(const FVector2D& V)
{
    X += V.X; Y += V.Y;
    return *this;
}


FVector2D FVector2D::operator-=(const FVector2D& V)
{
    X -= V.X; Y -= V.Y;
    return *this;
}


FVector2D FVector2D::operator*=(float Scale)
{
    X *= Scale; Y *= Scale;
    return *this;
}


FVector2D FVector2D::operator/=(float V)
{
    const float RV = 1.f/V;
    X *= RV; Y *= RV;
    return *this;
}


FVector2D FVector2D::operator*=(const FVector2D& V)
{
    X *= V.X; Y *= V.Y;
    return *this;
}


FVector2D FVector2D::operator/=(const FVector2D& V)
{
    X /= V.X; Y /= V.Y;
    return *this;
}


float& FVector2D::operator[](int32 Index)
{
    return ((Index == 0) ? X : Y);
}


float FVector2D::operator[](int32 Index) const
{
    return ((Index == 0) ? X : Y);
}


void FVector2D::Set(float InX, float InY)
{
    X = InX;
    Y = InY;
}


float FVector2D::GetMax() const
{
    return FMath::Max(X,Y);
}


float FVector2D::GetAbsMax() const
{
    return FMath::Max(FMath::Abs(X),FMath::Abs(Y));
}


float FVector2D::GetMin() const
{
    return FMath::Min(X,Y);
}


float FVector2D::Size() const
{
    return FMath::Sqrt(X*X + Y*Y);
}


float FVector2D::SizeSquared() const
{
    return X*X + Y*Y;
}

FVector2D FVector2D::GetSafeNormal(float Tolerance) const
{
    const float SquareSum = X*X + Y*Y;
    if(SquareSum > Tolerance)
    {
        const float Scale = FMath::InvSqrt(SquareSum);
        return FVector2D(X*Scale, Y*Scale);
    }
    return FVector2D(0.f, 0.f);
}


void FVector2D::Normalize(float Tolerance)
{
    const float SquareSum = X*X + Y*Y;
    if(SquareSum > Tolerance)
    {
        const float Scale = FMath::InvSqrt(SquareSum);
        X *= Scale;
        Y *= Scale;
        return;
    }
    X = 0.0f;
    Y = 0.0f;
}


void FVector2D::ToDirectionAndLength(FVector2D &OutDir, float &OutLength) const
{
    OutLength = Size();
    if (OutLength > SMALL_NUMBER)
    {
        float OneOverLength = 1.0f / OutLength;
        OutDir = FVector2D(X*OneOverLength, Y*OneOverLength);
    }
    else
    {
        OutDir = FVector2D::ZeroVector;
    }
}


bool FVector2D::IsNearlyZero(float Tolerance) const
{
    return	FMath::Abs(X)<=Tolerance
              &&	FMath::Abs(Y)<=Tolerance;
}


bool FVector2D::IsZero() const
{
    return X==0.f && Y==0.f;
}


float& FVector2D::Component(int32 Index)
{
    return (&X)[Index];
}


float FVector2D::Component(int32 Index) const
{
    return (&X)[Index];
}

FVector2D FVector2D::ClampAxes(float MinAxisVal, float MaxAxisVal) const
{
    return {FMath::Clamp(X, MinAxisVal, MaxAxisVal), FMath::Clamp(Y, MinAxisVal, MaxAxisVal)};
}


FVector2D FVector2D::GetSignVector() const
{
    return {
            FMath::FloatSelect(X, 1.f, -1.f),
                    FMath::FloatSelect(Y, 1.f, -1.f)
    };
}

FVector2D FVector2D::GetAbs() const
{
    return {FMath::Abs(X), FMath::Abs(Y)};
}


FString FVector2D::ToString() const
{
    return FString::Printf(L"X=%3.3f Y=%3.3f", X, Y);
}

float FVector2D::GetRangePct(FVector2D const& Range, float Value)
{
    return (Range.X != Range.Y) ? (Value - Range.X) / (Range.Y - Range.X) : Range.X;
}

float FVector2D::GetRangeValue(FVector2D const& Range, float Pct)
{
    return FMath::Lerp<float>(Range.X, Range.Y, Pct);
}

/** For the given Value clamped to the [Input:Range] inclusive, returns the corresponding percentage in [Output:Range] Inclusive. */
float FVector2D::GetMappedRangeValueClamped(const FVector2D& InputRange, const FVector2D& OutputRange, const float Value)
{
    const auto ClampedPct = FMath::Clamp<float>(GetRangePct(InputRange, Value), 0.f, 1.f);
    return GetRangeValue(OutputRange, ClampedPct);
}

/** Transform the given Value relative to the input range to the Output Range. */
float FVector2D::GetMappedRangeValueUnclamped(const FVector2D& InputRange, const FVector2D& OutputRange, const float Value)
{
    return GetRangeValue(OutputRange, GetRangePct(InputRange, Value));
}

#endif //Pragma_Once_WVector2D
#pragma clang diagnostic pop