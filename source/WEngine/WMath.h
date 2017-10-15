// Copyright Pagansoft.com, All rights reserved.

#ifndef Pragma_Once_WMath
#define Pragma_Once_WMath

#include <cmath>
#include "WEngine.h"

#pragma clang diagnostic push
#pragma ide diagnostic ignored "ClangTidyInspection"

class FMath
{

public:
    static int32 TruncToInt(float F)
    {
        return (int32)std::trunc(F);
    }

    static float TruncToFloat( float F )
    {
        return std::trunc(F);
    }

    static int32 RoundToInt( float F )
    {
        return (int32)std::round(F);
    }

    static float RoundToFloat(float F)
    {
        return std::round(F);
    }

    static int32 FloorToInt(float F)
    {
        return (int32)std::floor(F);
    }

    static float FloorToFloat(float F)
    {
        return std::floor(F);
    }

    static int32 CeilToInt(float F)
    {
        return (int32)std::ceil(F);
    }

    static float CeilToFloat(float F)
    {
        return std::ceil(F);
    }

    template< class T >
    static T Min( const T A, const T B )
    {
        return (A<=B) ? A : B;
    }

    template< class T >
    static T Max( const T A, const T B )
    {
        return (A>=B) ? A : B;
    }

    /** Performs a linear interpolation between two values, Alpha ranges from 0-1 */
    template< class T, class U >
    static T Lerp( const T& A, const T& B, const U& Alpha )
    {
        return (T)(A + Alpha * (B-A));
    }

    template< class T >
    static T Abs( const T A )
    {
        return (A>=(T)0) ? A : -A;
    }

    /**
     *	Checks if two floating point numbers are nearly equal.
     *	@param A				First number to compare
     *	@param B				Second number to compare
     *	@param ErrorTolerance	Maximum allowed difference for considering them as 'nearly equal'
     *	@return					true if A and B are nearly equal
     */
    static bool IsNearlyEqual(double A, double B, double ErrorTolerance = SMALL_NUMBER)
    {
        return Abs<double>( A - B ) <= ErrorTolerance;
    }

    /**
     *	Checks if a floating point number is nearly zero.
     *	@param Value			Number to compare
     *	@param ErrorTolerance	Maximum allowed difference for considering Value as 'nearly zero'
     *	@return					true if Value is nearly zero
     */
    static bool IsNearlyZero(float Value, float ErrorTolerance = SMALL_NUMBER)
    {
        return Abs<float>( Value ) <= ErrorTolerance;
    }

    /**
     *	Checks if a floating point number is nearly zero.
     *	@param Value			Number to compare
     *	@param ErrorTolerance	Maximum allowed difference for considering Value as 'nearly zero'
     *	@return					true if Value is nearly zero
     */
    static bool IsNearlyZero(double Value, double ErrorTolerance = SMALL_NUMBER)
    {
        return Abs<double>( Value ) <= ErrorTolerance;
    }

    /**
     *	Checks whether a number is a power of two.
     *	@param Value	Number to check
     *	@return			true if Value is a power of two
     */
    template <typename T>
    static bool IsPowerOfTwo( T Value )
    {
        return ((Value & (Value - 1)) == (T)0);
    }


    // Math Operations

    static float Sin( float Value ) { return sinf(Value); }
    static float Asin( float Value ) { return asinf( (Value<-1.f) ? -1.f : ((Value<1.f) ? Value : 1.f) ); }
    static float Sinh(float Value) { return sinhf(Value); }
    static float Cos( float Value ) { return cosf(Value); }
    static float Acos( float Value ) { return acosf( (Value<-1.f) ? -1.f : ((Value<1.f) ? Value : 1.f) ); }
    static float Tan( float Value ) { return tanf(Value); }
    static float Atan( float Value ) { return atanf(Value); }
    static float Sqrt( float Value ) { return sqrtf(Value); }
    static float Pow( float A, float B ) { return powf(A,B); }

    /** Returns highest of 3 values */
    template< class T >
    static T Max3( const T A, const T B, const T C )
    {
        return Max ( Max( A, B ), C );
    }

    /** Returns lowest of 3 values */
    template< class T >
    static T Min3( const T A, const T B, const T C )
    {
        return Min ( Min( A, B ), C );
    }

    /** Multiples value by itself */
    template< class T >
    static T Square( const T A )
    {
        return A*A;
    }

    /** Clamps X to be between Min and Max, inclusive */
    template< class T >
    static T Clamp( const T X, const T Min, const T Max )
    {
        return X<Min ? Min : X<Max ? X : Max;
    }

    static float FloatSelect( float Comparand, float ValueGEZero, float ValueLTZero )
    {
        return Comparand >= 0.f ? ValueGEZero : ValueLTZero;
    }

    /** Computes a fully accurate inverse square root */
    static float InvSqrt( float F )
    {
        return 1.0f / sqrtf( F );
    }

    static void SinCos( float* ScalarSin, float* ScalarCos, float  Value )
    {
        // Map Value to y in [-pi,pi], x = 2*pi*quotient + remainder.
        float quotient = (INV_PI*0.5f)*Value;
        if (Value >= 0.0f)
        {
            quotient = (float)((int)(quotient + 0.5f));
        }
        else
        {
            quotient = (float)((int)(quotient - 0.5f));
        }
        float y = Value - (2.0f*PI)*quotient;

        // Map y to [-pi/2,pi/2] with sin(y) = sin(Value).
        float sign;
        if (y > HALF_PI)
        {
            y = PI - y;
            sign = -1.0f;
        }
        else if (y < -HALF_PI)
        {
            y = -PI - y;
            sign = -1.0f;
        }
        else
        {
            sign = +1.0f;
        }

        float y2 = y * y;

        // 11-degree minimax approximation
        *ScalarSin = ( ( ( ( (-2.3889859e-08f * y2 + 2.7525562e-06f) * y2 - 0.00019840874f ) * y2 + 0.0083333310f ) * y2 - 0.16666667f ) * y2 + 1.0f ) * y;

        // 10-degree minimax approximation
        float p = ( ( ( ( -2.6051615e-07f * y2 + 2.4760495e-05f ) * y2 - 0.0013888378f ) * y2 + 0.041666638f ) * y2 - 0.5f ) * y2 + 1.0f;
        *ScalarCos = sign*p;
    }

    template<class T>
    static auto DegreesToRadians(T const& DegVal) -> decltype(DegVal * (PI / 180.f))
    {
        return DegVal * (PI / 180.f);
    }

    /** Snaps a value to the nearest grid multiple */
    static float GridSnap( float Location, float Grid )
    {
        if( Grid==0.f )	return Location;
        return FloorToFloat(static_cast<float>((Location + 0.5 * Grid) / Grid)) * Grid;
    }

    static float Fmod(float X, float Y)
    {
        if (fabsf(Y) <= 1.e-8f)
        {
            return 0.f;
        }
        const float Quotient = TruncToFloat(X / Y);
        float IntPortion = Y * Quotient;

        // Rounding and imprecision could cause IntPortion to exceed X and cause the result to be outside the expected range.
        // For example Fmod(55.8, 9.3) would result in a very small negative value!
        if (fabsf(IntPortion) > fabsf(X))
        {
            IntPortion = X;
        }

        const float Result = X - IntPortion;
        return Result;
    }
};

#endif //Pragma_Once_WMath
