% Relative frequency response for speaker
clc
clear
close all

%Include all csv files.
Table = ["20Procent-Båda.csv", "50Procent-Bas.csv", "50Procent-Diskant.csv","100Procent-Båda.csv", "100Procent-Bas.csv", "100Procent-Diskant.csv"];


for i = 1: length(Table)
    T = readtable(Table(i)); 
    Y = T{1:end,2}+T{1:end,3};
    X = T{1:end,1};

    B = smoothdata(Y);

    Max_Val = max(B);
    B = B./Max_Val;

    subplot(2,3,i);
   
    semilogx(X, 20*log10(B))
    title(Table(i));
       
    xlim([20 20000])
    yl = ylim;
    ylim([yl(1), yl(2)+5]);
    
    %legend('Magnitude response')
    grid on
    xlabel('Hz')
    ylabel('dB')
   
end


for i = 1: length(Table)
    T = readtable(Table(i)); 
    Y = T{1:end,2}+T{1:end,3};
    X = T{1:end,1};

    B = smoothdata(Y);

    Max_Val = max(B);
    B = B./Max_Val;

    figure();
   
    semilogx(X, 20*log10(B))
    title(Table(i));
    set(gcf,'name',Table(i));
       
    xlim([20 20000])
    yl = ylim;
    ylim([yl(1), yl(2)+5]);
    
    %legend('Magnitude response')
    grid on
    xlabel('Hz')
    ylabel('dB')
   
end

