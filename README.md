# Room/Chamber Reverberation Measurement

## Project Overview

This project focuses on measuring the reverberation time of a room or chamber using custom-designed microphone and speaker boards. Our approach is based on the method described in the paper, **"New Method of Measuring Reverberation Time"**, published on December 14, 1964 by Manfred R. Schroeder. You can find the paper [here](https://www.ee.columbia.edu/~dpwe/papers/Schro65-reverb.pdf).

### Key Concept

Instead of using traditional curve-fitting methods on complex non-linear decay functions, we implement the **Schroeder method**. This method simplifies the process by transforming the non-linear decay into a linear form, making it much easier to fit. Ultimately, this technique has proven to be highly effective in our project for measuring reverberation time accurately.

### Features

- Custom-built **microphone** and **speaker boards** for data collection
- Offline data analysis based on **Schroeder’s method** for reverberation measurement

### Methodology

1. **Data Collection**: We collect sound decay data using our custom microphone and speaker setup.
2. **Data Transformation**: Using the method from Schroeder’s paper, we transform the non-linear decay into a linear function.
3. **Fitting**: The transformed data is then easily fitted to extract the reverberation time.

### Reference Paper

- **Title**: New Method of Measuring Reverberation Time
- **Author**: Manfred R. Schroeder
- **Publication Date**: December 14, 1964
- **Link**: [New Method of Measuring Reverberation Time](https://www.ee.columbia.edu/~dpwe/papers/Schro65-reverb.pdf)
