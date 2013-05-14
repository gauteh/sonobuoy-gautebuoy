function makespectrogram (varargin)
n = size(varargin,2);

%% Find min and max t for reference
gmint = NaN;
gmaxt = NaN;

for k=1:n
  m = varargin{k};
 
  t = cat(1, m.t);
  gmint = min([gmint t(1)]);
  gmaxt = max([gmaxt t(end)]);
end


%% Compute spectrogram
for k=1:n
  pm = n;
  pn = 2;
  
  m = varargin{k};
  fprintf ('stream: %s\n', m(1).ChannelFullName);
  
  if (strcmp(strtrim(m(1).StationIdentifierCode), 'GAKS'))
    seismo = true;
  else
    seismo = false;
  end
  
  t = cat(1, m.t);
  d = cat(1, m.d);
  d = double(d);
  
  if ~seismo
    d = c2p(d); % convert counts to pressure [mPa]
  end

  Fs = 250; % Hz
  F  = 0:1:125;
  win = 256; % samples
  overlap = 150; % samples overlap

  % filter
  fprintf ('filtering..\n');
  fcut = 8;
  [b, a] = butter (6, fcut / (Fs/2), 'high');
  %d = real(filter (b, a, d));

  fprintf ('creating and plotting spectrogram..\n');
  subplot (pm, pn, (k-1)*2 + 1);
  [y,f,T,p] = spectrogram(d, win,overlap,F,Fs,'yaxis');
  %surf(T,f,10*log10(abs(p)),'EdgeColor','none');
  
  % Realign T to ref time frame
  mint = t(1); maxt = t(end);
  Tr = T + (mint - gmint)*24*60*60;
  
  surf(Tr,f,20.*log10(abs(p)),'EdgeColor','none');
  axis xy; axis tight; view (0,90);
 
  colormap (jet);
  %xlabel('Time [s]');
  ylabel('Frequency (Hz)');
  xlim ([0 ((gmaxt-gmint)*24*60*60)]);
  title (sprintf('Station %s (filtered above %g Hz) (dB)', m(1).ChannelFullName, fcut));
  a = xlim;
  if (k == n)
    xlabel ('Time [s]');
  end

  fprintf ('plotting trace..\n');
  subplot (pm, pn, (k-1)*2 + 2);
  tt = linspace(Tr(1), Tr(end), length(d));
  %tt = tt./Fs;
  plot (tt, d);
  xlim (a);
  if (k == n)
    xlabel ('Time [s]');
  end
  
  if ~seismo
    ylabel ('Amplitude [mPa]');
  else
    ylabel ('Amplitude [counts]');
  end
  
  
end

end