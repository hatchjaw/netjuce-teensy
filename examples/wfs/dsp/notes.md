# `fSlowN` variables
On the topic of control-rate (`fSlowN`) values, the following dsp:
```faust
import("stdfaust.lib");
process = hslider("f", 0, 0, 1, .01) : si.smooth(.999);
```
compiles to:
```c++
virtual void compute(int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
    FAUSTFLOAT* output0 = outputs[0];
    float fSlow0 = 0.001f * float(fHslider0);
    for (int i0 = 0; i0 < count; i0 = i0 + 1) {
        fRec0[0] = fSlow0 + 0.999f * fRec0[1];
        output0[i0] = FAUSTFLOAT(fRec0[0]);
        fRec0[1] = fRec0[0];
    }
}
```
`fSlow0` is the target value, i.e. the "current" value of 
the slider as far as the user is concerned. Then, according to the equation
for the default `si.smooth`, `fRec0` ($y$) is updated as per:

$$ y[n] = (1 - s)x[n] + sy[n - 1] $$

where $s = .999$. $x[n]$ is calculated outside the loop, so it stays constant 
for the entirety of the block. (Thus what looks at a glance like a broken 
implementation, is in fact valid.)

Removing `smoo` results in:
```c++
virtual void compute(int count, FAUSTFLOAT** RESTRICT inputs, FAUSTFLOAT** RESTRICT outputs) {
    FAUSTFLOAT* output0 = outputs[0];
    float fSlow0 = float(fHslider0);
    for (int i0 = 0; i0 < count; i0 = i0 + 1) {
        output0[i0] = FAUSTFLOAT(fSlow0);
    }
}
```
Clearly the slider value is only updated once per block.

For per-sample updates, one would need to move `float fSlow = float(fHslider0);`
into the loop, or have something like this:
```c++
for (int i0 = 0; i0 < count; i0 = i0 + 1) {
    // Expose slider value via a lambda.
    if (onSample != nullptr) {
        onSample(this, &fHslider0);
    }
    // re-update fSlow0
    fSlow0 = float(fHslider0);
    output0[i0] = FAUSTFLOAT(fSlow0);
}
```

# Bar graphs
Want to see smoothing in action in the web IDE?
```faust
import("stdfaust.lib");
f = hslider("f", 0, 0, 1, .01) : si.smooth(.999);
process = f <: attach(_, hbargraph("smoothed", 0, 1));
```
