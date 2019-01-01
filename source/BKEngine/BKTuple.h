// Copyright Burak Kara, All rights reserved.

#ifndef Pragma_Once_BKTuple
#define Pragma_Once_BKTuple

template <typename I1, typename I2>
class BKTuple_Two
{

private:
    BKTuple_Two() = default;

public:
    I1 Item1;
    I2 Item2;

    explicit BKTuple_Two(I1 _Item1, I2 _Item2)
    {
        Item1 = _Item1;
        Item2 = _Item2;
    }
};

template <typename I1, typename I2, typename I3>
class BKTuple_Three
{

private:
    BKTuple_Three() = default;

public:
    I1 Item1;
    I2 Item2;
    I3 Item3;

    explicit BKTuple_Three(I1 _Item1, I2 _Item2, I3 _Item3)
    {
        Item1 = _Item1;
        Item2 = _Item2;
        Item3 = _Item3;
    }
};

template <typename I1, typename I2, typename I3, typename I4>
class BKTuple_Four
{

private:
    BKTuple_Four() = default;

public:
    I1 Item1;
    I2 Item2;
    I3 Item3;
    I4 Item4;

    explicit BKTuple_Four(I1 _Item1, I2 _Item2, I3 _Item3, I4 _Item4)
    {
        Item1 = _Item1;
        Item2 = _Item2;
        Item3 = _Item3;
        Item4 = _Item4;
    }
};

template <typename I1, typename I2, typename I3, typename I4, typename I5>
class BKTuple_Five
{

private:
    BKTuple_Five() = default;

public:
    I1 Item1;
    I2 Item2;
    I3 Item3;
    I4 Item4;
    I5 Item5;

    explicit BKTuple_Five(I1 _Item1, I2 _Item2, I3 _Item3, I4 _Item4, I5 _Item5)
    {
        Item1 = _Item1;
        Item2 = _Item2;
        Item3 = _Item3;
        Item4 = _Item4;
        Item5 = _Item5;
    }
};

template <typename I1, typename I2, typename I3, typename I4, typename I5, typename I6>
class BKTuple_Six
{

private:
    BKTuple_Six() = default;

public:
    I1 Item1;
    I2 Item2;
    I3 Item3;
    I4 Item4;
    I5 Item5;
    I6 Item6;

    explicit BKTuple_Six(I1 _Item1, I2 _Item2, I3 _Item3, I4 _Item4, I5 _Item5, I6 _Item6)
    {
        Item1 = _Item1;
        Item2 = _Item2;
        Item3 = _Item3;
        Item4 = _Item4;
        Item5 = _Item5;
        Item6 = _Item6;
    }
};

#endif //Pragma_Once_BKTuple