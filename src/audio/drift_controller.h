#pragma once
#include <algorithm>
#include <cmath>

namespace NitLink {
// All inputs are seconds. Integrate by elapsed time, never by wake count.
// A one-second low-pass rejects USB packet jitter; anti-windup and slew
// limits keep the correction bounded during scheduler stalls.
class DriftController {
public:
    void Reset() { filtered_ = integral_ = ppm_ = 0; }
    double Update(double errorSeconds, double dt) {
        if (!std::isfinite(errorSeconds) || !std::isfinite(dt) || dt <= 0) return ppm_;
        dt = std::min(dt, 0.1);
        filtered_ += (1 - std::exp(-dt)) * (errorSeconds - filtered_);
        const double next = std::clamp(integral_ + filtered_ * dt * 20000, -1500.0, 1500.0);
        const double raw = filtered_ * 250000 + next;
        if (std::abs(raw) < 2000 || raw * filtered_ < 0) integral_ = next;
        const double target = std::clamp(filtered_ * 250000 + integral_, -2000.0, 2000.0);
        ppm_ += std::clamp(target - ppm_, -400 * dt, 400 * dt);
        return ppm_;
    }
private:
    double filtered_ = 0, integral_ = 0, ppm_ = 0;
};
// Four-point cubic interpolation. For drift correction only (near unity),
// not a general sample-rate converter. Windows handles nominal rate changes.
inline double DriftCubic(double a, double b, double c, double d, double t) {
    return b + 0.5 * t * (c - a + t * (2*a - 5*b + 4*c - d + t * (3*(b-c) + d-a)));
}
}
