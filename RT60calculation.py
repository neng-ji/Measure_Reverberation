import numpy as np
import matplotlib.pyplot as plt
from scipy.integrate import cumtrapz
from scipy.stats import linregress
import pandas as pd

def fourier_denoising(sig, min_freq, max_freq, dt=1.0):
    trans = np.fft.fft(sig)
    freqs = np.fft.fftfreq(len(sig), d=dt)
    trans[np.where(np.logical_or(np.abs(freqs) < min_freq, np.abs(freqs) > max_freq))] = 0
    res = np.fft.ifft(trans)
    return res.real

def calculate_rt60_and_plot(signal, sample_rate, min_freq=600, max_freq=700):
   
    r_t = signal - np.mean(signal)

    r_t = fourier_denoising(r_t, min_freq, max_freq, dt=1/sample_rate)
    r_squared = np.square(r_t)
   
    plt.figure(1)
    #plt.plot(f_t)
    plt.plot(r_t)
    
    #integral
    f_t = cumtrapz(r_squared[::-1], dx=1/sample_rate)[::-1]
    plt.figure(2)
    #plt.plot(f_t)
    plt.plot(r_squared)
    
    plt.figure(3)
    plt.plot(f_t)
    
    f_t_db = 10 * np.log10(f_t / np.max(f_t)) 
    
    t_valid = np.linspace(0, len(f_t_db) / sample_rate, num=len(f_t_db))
    
    idx_start = np.where(f_t_db <= -5)[0][0]
    idx_end = np.where(f_t_db <= -60)[0][0]
    
    print(idx_end/sample_rate)
    
    t_fit = t_valid[idx_start:idx_end]
    f_fit = f_t_db[idx_start:idx_end]
    
    # Linear regression
    slope, intercept, _, _, _ = linregress(t_fit, f_fit)
    
    # Calculate RT60 
    rt60 = -60 / slope
    # Plotting
    plt.figure(figsize=(10, 6))
    plt.plot(t_valid, f_t_db, label='Decay Curve')
    plt.plot(t_fit, slope * t_fit + intercept, 'r-', label='Linear Fit')
    plt.axhline(-60, color='green', linestyle=':', label='-60 dB')
    
    plt.title('Decay Curve with Linear Fit')
    plt.xlabel('Time (s)')
    plt.ylabel('Level (dB)')
    plt.legend()
    plt.grid(True)
    plt.show()
    
    return rt60

df = pd.read_csv('0504c016.csv')
sampling_rate = int(df.iloc[-1]['raw data'])
df.drop(df.tail(1).index, inplace=True)
data = df['raw data'].values

# Calculate RT60 and plot
rt60 = calculate_rt60_and_plot(data, sampling_rate)
print(f"Calculated RT60: {rt60:.2f} seconds")
