# MXU

This is a project for fun where I'm going to try and build a semi tape out ready MXU (matrix multiplication unit). This will be based off of what nearly all AI accelerators use as their bread and butter, composed of a systolic array, skew and deskew buffers, and control logic to get the whole thing to work with as little CPU involvement as possible.

## Systolic Array Computation

I want to determine what it looks like for data flowing through a weight stationary systolic array, so I can really determine what the flow looks like. I'm going to start by trying to simulate 2x2 matrix multiplication with two matrices $A$ and $B$ defined as

$$
A = \begin{bmatrix}
a_{11} & a_{12} \\
a_{21} & a_{22}
\end{bmatrix}, \space
B = \begin{bmatrix}
b_{11} & b_{12} \\
b_{21} & b_{22}
\end{bmatrix}
$$

We'll call the result $C$. To do this, let's go cycle by cycle. I'll assume that $B$ is already loaded into the grid. For each PE, we're going to store the weight value plus the input value to forward in the next cycle. The first value in the bracket will be the weight value, the second will be the value to compute on, and the third will be the result. I'll assume we write all registers on rising edge, and compute the partial sum / value to compute on combinationally.

Cycle 0:
$$
\begin{array}{cc|cc}
& & 0 & & 0 \\
& & \downarrow & & \downarrow
& & \\
\hline
a_{11} & \rightarrow & [b_{11}, a_{11}, 0] & a_{11} \rightarrow & [b_{12}, -, 0] \\
& & a_{11} b_{11} & & - \\
& & \downarrow & & \downarrow \\
- & \rightarrow & [b_{21}, -, -] & - \rightarrow & [b_{22}, -, -] \\
& & - & & -\\
& & \downarrow & & \downarrow
\end{array}
$$

Cycle 1:
$$
\begin{array}{cc|cc}
& & 0 & & 0 \\
& & \downarrow & & \downarrow
& & \\
\hline
a_{21} & \rightarrow & [b_{11}, a_{21}, 0] & a_{21} \rightarrow & [b_{12}, a_{11}, 0] \\
& & a_{21} b_{11} & & a_{12} b_{12} \\
& & \downarrow & & \downarrow \\
a_{12} & \rightarrow & [b_{21}, a_{12}, a_{11} b_{11}] & a_{12} \rightarrow & [b_{22}, -,  -] \\
& & (a_{12} b_{21} + a_{11} b_{11}) & & -\\
& & \downarrow & & \downarrow
\end{array}
$$

Cycle 2:
$$
\begin{array}{cc|cc}
& & 0 & & 0 \\
& & \downarrow & & \downarrow
& & \\
\hline
- & \rightarrow & [b_{11}, -, 0] & - \rightarrow & [b_{12}, a_{21}, 0] \\
& & - & & a_{21} b_{12} \\
& & \downarrow & & \downarrow \\
a_{22} & \rightarrow & [b_{21}, a_{22}, a_{21} b_{11}] & a_{22} \rightarrow & [b_{22}, a_{12},  a_{12} b_{12}] \\
& & (a_{22} b_{21} + a_{21} b_{11}) & & (a_{12} b_{22} + a_{12} b_{12}) \\
& & \downarrow & & \downarrow
\end{array}
$$

Cycle 3:
$$
\begin{array}{cc|cc}
& & 0 & & 0 \\
& & \downarrow & & \downarrow
& & \\
\hline
- & \rightarrow & [b_{11}, -, 0] & - \rightarrow & [b_{12}, -, 0] \\
& & - & & - \\
& & \downarrow & & \downarrow \\
- & \rightarrow & [b_{21}, -, -] & - \rightarrow & [b_{22}, a_{22},  a_{21} b_{12}] \\
& & (a_{22} b_{21} + a_{21} b_{11}) & & (a_{22} b_{22} + a_{21} b_{12}) \\
& & \downarrow & & \downarrow
\end{array}
$$

From this, we can see that the output $C_{11}$ is available in cycle 2, $C_{21}$ and $C_{12}$ are available in cycle 3, and $C_{22}$ is available in cycle 4. Collecting our values along the way, we find that

$$
C = \begin{bmatrix}
a_{12} b_{21} + a_{11} b_{11} & a_{12} b_{22} + a_{12} b_{12} \\
a_{22} b_{21} + a_{21} b_{11} & a_{22} b_{22} + a_{21} b_{12}
\end{bmatrix}
$$

This is the correct product, meaning that we have proved (at least in the 2x2 case) that the systolic array does what we expect it to. Generalizing, we'll need to (1) feed the rows of the transpose of $A$ to the rows of our array (2) start feeding values from row $i$ into our array at cycle $i$ (assuming we 0-index), and (3) delay collecting the results of column $i$ until cycle $n + i - 1$. 

## Systolic Array Loading

In the above example we assumed that the weights were already pre-loaded into our array. However, we also need to consider how to get them there in the first place. One initial idea I had was having a special load enable signal come in from the top, be broadcasted down a column, and then have weights be passed down and stored at each clock cycle. The problem with this, however, is it makes it much harder to simultaneously compute and load weights. When running a transformer, we'll need to switch out the weights in our systolic array tons of times per layer, so we want to be able to get two things done at once. To do this, we'll introduce a shadow register, which will hold the weight to be switched to based on a new signal. The load enable is again broadcasted down a column, and weights are still daisy-chained down, but now we only write to the shadow register, not the one holding the current weight. This way, when we want to switch, we daisy-chain a switch signal through the systolic array (BY ROW!) so it's like a wavefront switching signals before the values to compute on stream through. It creates a 1 cycle bubble but this should be much more efficient than trying to mux our way out of this.