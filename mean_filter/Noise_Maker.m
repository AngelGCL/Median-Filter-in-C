a = imread('basketball_200x200.png');
h = imread('firebird-logo-400x400.png');
g = imread('Julia_set_camp4_1000x1000.png');
f = imread('logo800x800.png');
e = imread('photon_mapping5000.png');

o = imnoise(a, 'salt & pepper', 0.02);
l = imresize(o, [200 NaN]);
imwrite(l, 'bad_ball200.png');
imshow(l);
%%
p = imnoise(e, 'salt & pepper', 0.02);
x = imresize(p, [5000 NaN]);
imwrite(x, 'bad_photon5000.png');

q = imnoise(f, 'salt & pepper', 0.02);
z = imresize(q, [800 NaN]);
imwrite(z, 'bad_logo800.png');

t = imnoise(g, 'salt & pepper', 0.02);
y = imresize(t, [1000 NaN]);
imwrite(y, 'bad_julia1000.png');

s = imnoise(h, 'salt & pepper', 0.02);
k = imresize(s, [400 NaN]);
imwrite(k, 'bad_firebird400.png');


%%