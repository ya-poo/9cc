#!/bin/bash
assert() {
    expected="$1"
    input="$2"

    ./9cc "$input" >tmp.s
    cc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

assert 0 '0;'
assert 42 '42;'
assert 21 '5+20-4;'
assert 41 ' 12 + 34 - 5 ;'
assert 47 '5+6*7;'
assert 15 '5*(9-6);'
assert 4 '(3+5)/2;'
assert 10 '-10+20;'
assert 10 '- -10;'
assert 10 '- - +10;'

assert 0 '0==1;'
assert 1 '42==42;'
assert 1 '0!=1;'
assert 0 '42!=42;'

assert 1 '0<1;'
assert 0 '1<1;'
assert 0 '2<1;'
assert 1 '0<=1;'
assert 1 '1<=1;'
assert 0 '2<=1;'

assert 1 '1>0;'
assert 0 '1>1;'
assert 0 '1>2;'
assert 1 '1>=0;'
assert 1 '1>=1;'
assert 0 '1>=2;'

assert 3 '1; 2; 3;'
assert 3 'a=3; a;'
assert 3 'a=27; b=3; c=10; (a + b)/c;'
assert 5 'a=3; 5;'
assert 26 'a=1; b=a+1; c=b+1; d=c+1; e=d+1; f=e+1; g=f+1; h=g+1; i=h+1; j=i+1; k=j+1; l=k+1; m=l+1; n=m+1; o=n+1; p=o+1; q=p+1; r=q+1; s=r+1; t=s+1; u=t+1; v=u+1; w=v+1; x=w+1; y=x+1; z=y+1;'

assert 8 'foo123ABC=3; bar=5; foo123ABC+bar;'
assert 3 '_foo123=3; _foo123;'

assert 1 'return 1; 2; 3;'
assert 2 '1; return 2; 3;'
assert 1 'return 1; return 2;'
assert 4 'a=1; b=3; return a+b;'

assert 3 'if (0) return 2; return 3;'
assert 3 'if (1-1) return 2; return 3;'
assert 2 'if (1) return 2; return 3;'
assert 2 'if (2-1) return 2; return 3;'

assert 2 'if (1) return 2; else return 3;'
assert 3 'if (0) return 2; else return 3;'

assert 10 'i=0; while(i<10) i=i+1; return i;'

assert 55 "sum = 0;
for(i=1; i<=10; i=i+1)
    sum = sum + i;
return sum;"

echo OK
