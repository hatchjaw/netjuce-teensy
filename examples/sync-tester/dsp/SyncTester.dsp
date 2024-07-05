import("stdfaust.lib");

// Expect an incoming 16-bit unipolar sawtooth wave x, where
//    period = 1 << shift
//      x[0] = 0
//      x[n] = (x[n-1] + 1) % period
//        f0 = [sampling rate] / period
//
//        0 <= x[n] <= period - 1

// Faust will convert this to floating point.
// A scaling factor is necessary.
//        0 <= x[n] <= (1 / (1 << (15 - shift))) - (1 / 1 << 15)

// E.g. shift = 10
//        0 <= x[n] <= (1 / 1 << 5) - (1 / 1 << 15)
//                  <= 1/32 - 1/32768

// Calculate the offset between the incoming signal and the generated signal.
// To do so, generate a unipolar sawtooth wave y and subtract it from x.
// The offset, d, is
//       / x - y             , y < x
//   d = |
//       \ (x + 1/factor) - y, otherwise
process = _ <: _,-(saw),+((1/factor)-saw) : select2(saw >= _)
with {
    // Number of bits by which 1 can be shifted without exceeding signed 16-bit.
    MAXSHIFT = 15;

    // Desired samples per period.
    SHIFT = 15;
    PERIOD = 1<<SHIFT;

    //
    factor = 1<<(MAXSHIFT-SHIFT);

    // Frequency of the saw.
    f0 = ma.SR / PERIOD;
    //
    saw = os.lf_sawpos(f0) / factor;
};
