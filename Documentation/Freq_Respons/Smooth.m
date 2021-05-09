% Relative frequency response for speaker
clc
clear
close all

T = readtable('50Procent-Diskant.csv');
Y = T{1:end,2}+T{1:end,3};
X = T{1:end,1};

B = smoothdata(Y);

loglog(X,Y, X,B);
xlim([20 20000])
legend('Freqency respons', 'Smoothed freqency respons')
grid on
xlabel('Hz')
ylabel('dB')