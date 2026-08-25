#ifndef LDG_FP64_MATH
#define LDG_FP64_MATH

double ldg_exp(double x)
{
    const double LN2_HI = 6.93147180369123816490e-01;
    const double LN2_LO = 1.90821492927058770002e-10;
    const double INV_LN2 = 1.44269504088896338700e+00;
    const double P1 = 1.66666666666666019037e-01;
    const double P2 = -2.77777777770155933842e-03;
    const double P3 = 6.61375632143793436117e-05;
    const double P4 = -1.65339022054652515390e-06;
    const double P5 = 4.13813679705723846039e-08;
    double kf = 0.0;
    double r = 0.0;
    double r2 = 0.0;
    double c = 0.0;
    double expr = 0.0;
    int k = 0;
    if (x < -745.13321910194111211lf) { return 0.0lf; }

    if (x > 709.78271289338399678lf) { return 1.0e308lf * 1.0e308lf; }

    kf = floor(x * INV_LN2 + 0.5lf);
    k = int(kf);
    r = (x - kf * LN2_HI) - kf * LN2_LO;
    r2 = r * r;
    c = r - r2 * (P1 + r2 * (P2 + r2 * (P3 + r2 * (P4 + r2 * P5))));
    expr = 1.0lf - ((r * c) / (c - 2.0lf) - r);
    return ldexp(expr, k);
}

double ldg_log(double x)
{
    const double LN2_HI = 6.93147180369123816490e-01;
    const double LN2_LO = 1.90821492927058770002e-10;
    const double SQRT2_HALF = 7.07106781186547524401e-01;
    const double Lg1 = 6.666666666666735130e-01;
    const double Lg2 = 3.999999999940941908e-01;
    const double Lg3 = 2.857142874366239149e-01;
    const double Lg4 = 2.222219843214978396e-01;
    const double Lg5 = 1.818357216161805012e-01;
    const double Lg6 = 1.531383769920937332e-01;
    const double Lg7 = 1.479819860511658591e-01;
    double f = 0.0;
    double ff = 0.0;
    double s = 0.0;
    double z = 0.0;
    double w = 0.0;
    double t1 = 0.0;
    double t2 = 0.0;
    double region = 0.0;
    int k = 0;
    if (x <= 0.0lf) { return -1.0e308lf; }

    f = frexp(x, k);
    if (f < SQRT2_HALF)
    {
        f = f * 2.0lf;
        k = k - 1;
    }
    ff = f - 1.0lf;
    s = ff / (2.0lf + ff);
    z = s * s;
    w = z * z;
    t1 = w * (Lg2 + w * (Lg4 + w * Lg6));
    t2 = z * (Lg1 + w * (Lg3 + w * (Lg5 + w * Lg7)));
    region = t2 + t1;
    return double(k) * LN2_HI - ((s * (ff - region) - double(k) * LN2_LO) - ff);
}

#endif
