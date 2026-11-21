// Tobo Exciter (TE-02): the model, without any JUCE in it. Copyright (C) 2026 Tobokegao. Licensed under the GNU AGPL v3 or later.
//
//   u   = direct * x + filt * HP(x)              HP: state-variable filter whose high-pass node clips
//   out = u + multi * sum_i a_i * BP_i(u)        BP_i: six resonators whose band-pass node saturates
// Levels: 1.0 is the 0 dBFS of the measurement of the hardware.
// Run it at 4 x 44.1 kHz or more.
#pragma once
#include <cmath>

namespace te02
{

// the six fixed peaks of MULTIPEAK, fitted to the measured hardware
struct Peak { double f0, q, a; };
static const Peak kPeaks[6] = {
    {  986.0, 10.8, 2.75 },
    { 1372.0, 10.5, 2.47 },
    { 1890.0,  9.7, 2.23 },
    { 2380.0,  9.2, 1.97 },
    { 3461.0,  9.0, 2.16 },
    { 4966.0,  8.8, 1.95 },
};

// Non-linear parts, fitted to sine sweeps and to a re-amped guitar.
// The filter's op-amp output (the high-pass node) clips hard and lopsided: at the resonance the unit puts out
// a nearly square wave. MULTIPEAK comes after the filter and the balance mix; its resonators saturate too.
static const double kThrMP = 0.65;
static const double kHpPos = 0.38;
static const double kHpNeg = 0.294;
// the output stage clips as well, lower on the positive side
static const double kOutPos = 0.8;
static const double kOutNeg = 1.2;

inline double sat6 (double u, double thr)       // u / (1 + |u/thr|^6)^(1/6)
{
    double a = u / thr; a *= a; a = a * a * a;
    return u / std::cbrt (std::sqrt (1.0 + a));
}

inline double sat4 (double u, double thr)       // u / (1 + |u/thr|^4)^(1/4)
{
    double a = u / thr; a *= a; a *= a;
    return u / std::sqrt (std::sqrt (1.0 + a));
}

inline double sat8 (double u, double thr)       // u / (1 + |u/thr|^8)^(1/8)
{
    double a = u / thr; a *= a; a *= a; a *= a;
    return u / std::sqrt (std::sqrt (std::sqrt (1.0 + a)));
}

struct Svf
{
    double g = 0.1, r2 = 1.0, den = 1.0, s1 = 0.0, s2 = 0.0, hp = 0.0, bp = 0.0;

    void set (double fs, double f0, double q)
    {
        const double pi = 3.14159265358979323846;
        if (f0 > 0.45 * fs) f0 = 0.45 * fs;
        g = std::tan (pi * f0 / fs);
        r2 = 1.0 / q;
        den = 1.0 / (1.0 + r2 * g + g * g);
    }
    void reset() { s1 = s2 = hp = bp = 0.0; }

    // the filter: its high-pass node clips. hp = C(a - b*hp), solved by a few rounds
    inline void tickClipHP (double x)
    {
        const double a = x - (r2 + g) * s1 - s2;
        const double b = r2 * g + g * g;
        double h = a / (1.0 + b);
        for (int it = 0; it < 4; ++it)
        {
            const double v = a - b * h;
            h = 0.5 * h + 0.5 * sat4 (v, v < 0.0 ? kHpNeg : kHpPos);
        }
        hp = h;
        const double bb = g * h + s1;
        s1 = g * h + bb;
        bp = bb;
        const double lp = g * bb + s2;
        s2 = g * bb + lp;
    }

    template <typename Sat> inline void tick (double x, double thr, Sat sat)
    {
        hp = (x - (r2 + g) * s1 - s2) * den;
        double b = sat (g * hp + s1, thr);
        s1 = g * hp + b;
        bp = b;
        const double lp = g * b + s2;
        s2 = g * b + lp;
    }
};

// The four knobs, 0..1 (0 = fully left, 0.5 = 12 o'clock, 1 = fully right), through the tapers
// measured at five positions.
inline double taper (const double* y, double t, bool logScale)
{
    if (std::isnan (t)) t = 0.5;       // not a number (a damaged session can hold one): the middle. It would index outside the table
    if (t <= 0.0) return y[0];
    if (t >= 1.0) return y[4];
    const double p = t * 4.0;
    const int i = (int) p;
    const double f = p - i;
    if (logScale) return std::exp (std::log (y[i]) * (1.0 - f) + std::log (y[i + 1]) * f);
    return y[i] * (1.0 - f) + y[i + 1] * f;
}

struct Knobs { double frequency = 0.5, color = 0.5, balance = 0.5, multipeak = 0.5; };

struct Coeffs
{
    double f0, q, direct, filt, multi;

    static Coeffs from (const Knobs& k)
    {
        static const double F0[5]     = { 687.0, 1111.0, 3074.0, 5852.0, 6800.0 };
        static const double Q[5]      = { 0.40, 0.48, 0.79, 2.3, 17.7 };
        static const double DIRECT[5] = { 1.0, 0.805, 0.45, 0.143, 0.0 };
        static const double FILT[5]   = { 0.0, 0.43, 1.17, 2.06, 2.55 };
        static const double MULTI[5]  = { 0.01, 0.18, 0.48, 0.81, 1.0 };
        Coeffs c;
        c.f0     = taper (F0, k.frequency, true);
        c.q      = taper (Q, k.color, true);
        c.direct = taper (DIRECT, k.balance, false);
        c.filt   = -taper (FILT, k.balance, false);      // the filtered sound is added upside down
        c.multi  = taper (MULTI, k.multipeak, false);
        return c;
    }
};

class Engine
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        for (int i = 0; i < 6; ++i) { res[i].set (fs, kPeaks[i].f0, kPeaks[i].q); res[i].reset(); }
        hp.reset();
        smooth = 1.0 - std::exp (-1.0 / (0.01 * fs));     // the knobs move over about 10 ms
        cur = target;
        hp.set (fs, cur.f0, cur.q);
        sinceSet = 0;
    }

    void setKnobs (const Knobs& k) { target = Coeffs::from (k); }
    void setCoeffs (const Coeffs& c) { target = c; }         // for tests: the fitted values of one take
    void snap() { cur = target; hp.set (fs, cur.f0, cur.q); }

    inline double tick (double x)
    {
        // glide to the knob positions; the filter's coefficients are worked out every 16 samples
        cur.direct += smooth * (target.direct - cur.direct);
        cur.filt   += smooth * (target.filt - cur.filt);
        cur.multi  += smooth * (target.multi - cur.multi);
        if (++sinceSet >= 16)
        {
            sinceSet = 0;
            const double s16 = 1.0 - std::pow (1.0 - smooth, 16.0);
            cur.f0 *= std::pow (target.f0 / cur.f0, s16);
            cur.q  *= std::pow (target.q / cur.q, s16);
            hp.set (fs, cur.f0, cur.q);
        }

        hp.tickClipHP (x);
        const double u = cur.direct * x + cur.filt * hp.hp;
        double y = u;
        for (int i = 0; i < 6; ++i)
        {
            res[i].tick (u, kThrMP, sat8);
            y += cur.multi * kPeaks[i].a * res[i].r2 * res[i].bp;
        }
        return sat8 (y, y < 0.0 ? kOutNeg : kOutPos);
    }

private:
    double fs = 192000.0, smooth = 0.001;
    int sinceSet = 0;
    Svf res[6], hp;
    Coeffs target = Coeffs::from (Knobs()), cur = target;
};

} // namespace te02
