# -*- coding: utf-8 -*-
"""
Created on Thu Apr 11 14:27:13 2024

@author: dudle
"""

import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
from scipy.fft import fft, fftfreq
from scipy.optimize import curve_fit
from scipy import signal
from scipy.stats import sem

plt.close('all')
np.seterr(divide = 'ignore')

def line(x,m,b):
    return m*x + b

def runnamefinder(run):
    runname = ['0','0','0']
    if run<10:
        runname[2] = str(run)
    else:
        runname[1] = '1'
        runname[2] = str(run-10)
    return ''.join(runname)




results = np.array([])
errors = np.array([])
# for data in ['studio1/3_26_24_KCPA_Obs_Deck/2603e015.csv']:
for data in ['4_19_24_KCPA/1904e006.csv']:

    df = pd.read_csv(data);
    N=len(df['raw data'])
    rate = df.iloc[-1]['raw data']
    df.drop(df.tail(1).index, inplace = True)
    
    #the data
    h = np.array(df['raw data']) - np.mean(df['raw data'])
    
    #the time scale
    t= np.linspace(0,(len(df['raw data'])-1)/rate, num=len(df['raw data']))
    
    # plt.plot(t,h, color = '#152a4c')
    # plt.xlabel('Time (s)')
    # plt.ylabel('ADC Counts')
    # plt.show()
    
    #filter data
    low =  440
    high = low*2
    sos = signal.butter(3, (low,high) , 'bandpass', output = 'sos', fs = rate)
    hf = signal.sosfilt(sos, h)
    hA = np.abs(signal.hilbert(hf))
    # plt.plot(t, hf, color = '#152a4c')
    # plt.xlabel('Time (s)')
    # plt.ylabel('ADC Counts')
    # plt.show()
    
    #hilbert transform for analytic signal
    hA = np.abs(signal.hilbert(hf))
    # plt.plot(t, hA, color = '#152a4c')
    # plt.xlabel('Time (s)')
    # plt.ylabel('ADC Counts')
    
    
    
    #moving average filter
    hM = np.array(pd.DataFrame(hA).rolling(int(rate/100)).mean().fillna(0)[0])
    # plt.plot(t, hM, color = '#152a4c')
    # plt.xlabel('Time (s)')
    # plt.ylabel('ADC Counts')
    
    
    
    #convert to dB
    E = 20 * np.log10(hM/max(hM))
    # plt.plot(t, E, color = '#152a4c')
    # plt.xlabel('Time (s)')
    # plt.ylabel('dB')
    
    #schroeder integration method
    L =  10 * np.log10(np.cumsum(hM[::-1]**2)[::-1] / (sum(hM**2)))
    # plt.plot(t, L, color = '#152a4c')
    
    
    #fittin
    #EDT
    # start = np.where(L <= -1)[0][0]
    # end = np.where(L <= -10)[0][0]
    # EDTpopt, EDTcov = curve_fit(line, t[start:end], L[start:end], p0=[-1, 1.5])
    # plt.plot(t[start:end], line(t[start:end], *EDTpopt))
    # print('EDT = ' + str(-10/EDTpopt[0]) + ' s' )
    
    #T10
    # start = np.where(L <= -5)[0][0]
    # end = np.where(L <= -15)[0][0]
    # T10popt, T10cov = curve_fit(line, t[start:end], L[start:end], p0=[-1, 1.5])
    # results = np.append(results,-60/T10popt[0])
    # print('RT60 from T10 = ' + str(-60/T10popt[0]) + ' s' )
    # plt.plot(t, L, label = 'RT60 = ' + str(round(-60/T10popt[0],2)) + ' s')
    # plt.plot(t[start:end], line(t[start:end], *T10popt), label = 'Linear Fit', linewidth=2.0)
    # plt.plot([0,2],[-5,-5], '--', color='red')
    # plt.plot([0,2],[-15,-15], '--', color='red')
    
    #T20
    # start = np.where(L <= -5)[0][0]
    # end = np.where(L <= -25)[0][0]
    # T20popt, T20cov = curve_fit(line, t[start:end], L[start:end], p0=[-1, 1.5])
    # results = np.append(results,-60/T20popt[0])
    # plt.plot(t[start:end], line(t[start:end], *T20popt))
    # print('RT60 from T20 = ' + str(-60/T20popt[0]) + ' s' )
    # plt.plot(t, L, label = 'RT60 = ' + str(round(-60/T20popt[0],2)) + ' s')
    # plt.plot([0,2],[-5,-5], color='red')
    # plt.plot([0,2],[-25,-25], color='red')
    
    #T30
    start = np.where(L <= -5)[0][0]
    end = np.where(L <= -35)[0][0]
    T30popt, T30cov = curve_fit(line, t[start:end], L[start:end], p0=[-1, 1.5])
    results = np.append(results,-60/T30popt[0])
    print('RT60 from T30 = ' + str(-60/T30popt[0]) + ' s' )
    plt.plot(t, L, label = 'RT60 = ' + str(round(-60/T30popt[0],2)) + ' s')
    plt.plot(t[start:end], line(t[start:end], *T30popt), label = 'Linear Fit', linewidth=2.0)
    plt.plot([0,2],[-5,-5], '--', color='red')
    plt.plot([0,2],[-35,-35], '--',  color='red')
    
    

    n = len(t[start:end])
    xbar = np.mean(t[start:end])
    ybar = np.mean(L[start:end])
    sx = np.sqrt(sum((t[start:end]-xbar)**2)/(n-1))
    sy = np.sqrt(sum((L[start:end]-ybar)**2)/(n-1))
    r = sum((t[start:end]-xbar)*(L[start:end]-ybar))/(n-1)/(sx*sy)
    slope = r*sy/sx
    SE = np.sqrt((1-r*r)/(n-2))*sy/sx
    # print(SE)
    # print('error = ' + str(SE*60/slope**2))
    errors = np.append(errors,SE*60/slope**2)
    
print(max(results)-min(results))
print(np.mean(results))
plt.xlabel('Time (s)')
plt.ylabel('Volume (dB)')
plt.legend()
plt.title('Shroeder Integration Curve: 440 Hz')
plt.show()
    
    

#ISO 3382-2 values
